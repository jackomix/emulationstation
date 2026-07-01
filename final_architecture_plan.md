# Final Architecture Plan: Native C++ Profiles, iiSU Home View, and LAHEE Achievements

This document represents the final, fully-researched blueprint for implementing the **User Profile System**, **iiSU Home View**, and **LAHEE Achievements** natively and seamlessly in C++ inside EmulationStation.

---

## 1. User Profiles & Sandboxing (`ProfileManager`)

### The exFAT/FAT32 Symlink Problem & Solution
The ROMs partition `/storage/roms/` on the R36S is typically formatted as exFAT or FAT32 (so users can transfer files on Windows/Mac). Since exFAT/FAT32 does not support filesystem-level symbolic links, any attempt to run `symlink` directly inside the ROMs folder will fail.

**Our VFS Bind-Mount Solution**:
We will sandbox saves, states, and screenshots using Linux virtual filesystem (VFS) **bind mounts**, which operate at the kernel layer and bypass filesystem restrictions:
1. Profiles will be stored in `/storage/roms/profiles/<profile_id>/` (e.g. `/storage/roms/profiles/1/savestates` and `/storage/roms/profiles/1/screenshots`).
2. When a profile is activated (or on boot), we dynamically run:
   - `umount /storage/roms/savestates`
   - `umount /storage/roms/screenshots`
   - `mount --bind /storage/roms/profiles/<active_id>/savestates /storage/roms/savestates`
   - `mount --bind /storage/roms/profiles/<active_id>/screenshots /storage/roms/screenshots`
3. This redirection is 100% transparent. RetroArch and all other emulators continue to read/write to `/storage/roms/savestates` and `/storage/roms/screenshots` normally, but their writes automatically route to the active profile's folder.
4. **Data Preservation**: On first boot, if `/storage/roms/savestates` is a normal directory containing files, we migrate those files to `/storage/roms/profiles/1/savestates` before establishing the bind mounts so the user loses zero data.

### Metadata Interception via Back-Pointer
To sandbox favorites and play stats without writing profile-specific data to the shared `gamelist.xml` files:
1. We add a back-pointer `FileData* mOwner` to the `MetaDataList` class (`es-app/src/MetaData.h`).
2. In the `FileData` constructor, we link the back-pointer: `mMetadata.mOwner = this;`.
3. In `MetaDataList::get` and `MetaDataList::set`, we intercept reads/writes for profile-specific fields (`MetaDataId::Favorite`, `MetaDataId::PlayCount`, `MetaDataId::LastPlayed`, `MetaDataId::GameTime`):
   - **`get`**: If the key is profile-specific, we check `ProfileManager`. If `ProfileManager` has an entry for the ROM, we return it. Otherwise, we default to the underlying scraped value in `mMap`.
   - **`set`**: We write the value directly to the active profile's database in `ProfileManager` (which writes to `/storage/roms/profiles/<active_id>/metadata.json`) and return immediately, preventing the shared `mMap` from being flagged as changed (`mWasChanged = true`).

---

## 2. iiSU-style Home View (`HomeView`)

We will stack views vertically using `ViewController`'s camera:
* **Y = 0**: `HomeView` (Dashboard)
* **Y = ScreenHeight**: `SystemView` (System Carousel)
* **Y = ScreenHeight * 2**: `GameListView`s

### Layout Design (`ComponentGrid`)
We will instantiate a `ComponentGrid` of size `(4, 3)` (4 columns by 3 rows) inside `HomeView`:
* **Profile Widget** (Col 0, Row 0, Span 2x1):
  - Displays the active profile avatar (circular format) and profile name (e.g. "Player").
* **Clock & Status Widget** (Col 2, Row 0, Span 2x1):
  - We reuse the native `ClockComponent`, `NetworkIconComponent`, `BatteryIconComponent`, and `BatteryTextComponent` inside a horizontal layout container.
* **Continue Playing Card** (Col 0, Row 1, Span 2x2):
  - Displays the last played game with cover art taking up the full 2x2 tile space. Shows a small overlay with the game title and play progress.
  - Pressing "A" on this tile launches the game immediately.
* **Browse Systems Tile** (Col 2, Row 1, Span 1x1):
  - Clean icon/text tile that scrolls down to the `SystemView`.
* **Achievements Tile** (Col 3, Row 1, Span 1x1):
  - Action icon to open `GuiRetroAchievements`.
* **User Profiles Tile** (Col 2, Row 2, Span 1x1):
  - Action icon to open `GuiProfileManager`.
* **Settings Tile** (Col 3, Row 2, Span 1x1):
  - Action icon to open the main menu.

### Screen Tabs (`[ HOME ]  [ SYSTEMS ]`)
1. We will render a persistent header tab bar at the top of the screen.
2. In `ViewController::render`, if `mState.viewing` is `HOME_VIEW` or `SYSTEM_SELECT`, we render the tab bar using `parentTrans` (screen space matrix) instead of the camera's `trans` matrix so it remains fixed at the top of the screen during vertical camera scrolls.
3. In `ViewController::input`, if `leftshoulder` (L1) or `rightshoulder` (R1) are pressed at the top level, we transition:
   - `leftshoulder` on SystemView -> `goToHomeView(false)`
   - `rightshoulder` on HomeView -> `goToSystemView(activeSystem, false)`

---

## 3. Modular LAHEE Achievements Integration

We will redirect RetroAchievements API endpoints and bypass online connectivity/login requirements when a local server is specified.

### Endpoint Resolution (`resolveUrl`)
In `RetroAchievements.cpp`, we introduce `resolveUrl(const std::string& url)`:
* Check setting `global.retroachievements.server`. If it points to a custom server (e.g., `http://127.0.0.1:8000/lahee/`):
  - Replace `https://retroachievements.org` and `http://i.retroachievements.org` with the custom local server address.
  - API method files like `https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php` are translated to `http://127.0.0.1:8000/laheer/dorequest.php?r=API_GetGameInfoAndUserProgress&...`.

### Bypass Logic
1. **Login Test Bypass**:
   - In `RetroAchievements::testAccount`, if `global.retroachievements.server` is custom, skip the network request entirely and return `true` with a mock token `"local_token"`.
2. **Network Warning Bypass**:
   - In `SystemView::input` (line 1641) and other achievement triggers, check `isNetworkAvailableForCheevos()`. If a custom server URL is set, return `true` immediately to skip the "YOU ARE NOT CONNECTED TO A NETWORK" warning dialog.

### Daemon Lifecycle Management
To autostart the offline achievements server daemon `LAHEE Server.sh`:
1. In `main.cpp` startup (near thread creation), we call `RetroAchievements::startLocalServer()`. This checks if autostart is enabled, forks the process, calls `setsid()` to detach the child from the parent terminal, and executes `/bin/sh -c "./LAHEE Server.sh start"`.
2. In `main.cpp` shutdown (near threads cleanup), we call `RetroAchievements::stopLocalServer()`, which sends a `SIGTERM` to the daemon PID (and `SIGKILL` if it doesn't shut down within 5 seconds), ensuring no orphaned processes are left running on the R36S.
