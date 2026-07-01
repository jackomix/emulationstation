# Implementation Plan: Multi-User Profiles (Netflix-Style)

> **Goal**: Let multiple people share a single device and ROM library, each with their own isolated saves, save states, screenshots, and last-played history — while sharing ROMs, themes, and BIOS files.

---

## How It Works (The Concept)

Think of it like Netflix profiles. On boot, ES shows a **"Who's Playing?"** screen with a list of profile avatars. Whoever selects their name gets their own private data folder. The massive shared library of ROMs is completely unaffected — only lightweight personal data (saves, states, screenshots) is scoped to the active profile.

---

## Codebase Analysis

Before diving into the plan, here is where each key concern lives in the existing code:

| Concern | File(s) |
| :--- | :--- |
| All path resolution (saves, screenshots, logs) | [`es-core/src/Paths.h`](file:///Users/jacko/Documents/myEmulationStation/es-core/src/Paths.h) / [`Paths.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-core/src/Paths.cpp) |
| Save state directory lookup | [`es-app/src/SaveStateConfigFile.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/SaveStateConfigFile.cpp) — `SaveStateConfig::getDirectory()` |
| Save state file enumeration | [`es-app/src/SaveStateRepository.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/SaveStateRepository.cpp) |
| Gamelist metadata (last played, play count, favorites) | [`es-app/src/Gamelist.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/Gamelist.cpp) |
| Application startup / argument parsing | [`es-app/src/main.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/main.cpp) |
| Settings persistence | [`es-core/src/Settings.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-core/src/Settings.cpp) |
| Main menu | [`es-app/src/guis/GuiMenu.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/guis/GuiMenu.cpp) |
| All views / navigation | [`es-app/src/views/ViewController.cpp`](file:///Users/jacko/Documents/myEmulationStation/es-app/src/views/ViewController.cpp) |

---

## Directory Layout

### Shared (unchanged, no profile prefix)
```
/roms/
  nes/        ← ROMs for NES
  snes/       ← ROMs for SNES
  ...

/home/ark/.emulationstation/
  themes/     ← shared
  music/      ← shared
  es_systems.cfg
```

### Per-Profile (new, scoped to active profile)
```
/roms/profiles/
  jacko/
    saves/          ← RetroArch & other emulator save files (.srm, etc.)
    savestates/     ← Save state slots per system
    screenshots/    ← In-game screenshots
    gamelists/      ← Per-system gamelist.xml (stores last played, play count, favorites per user)
    es_settings.cfg ← UI preferences (sort order, filters, last system)

  player2/
    saves/
    savestates/
    screenshots/
    gamelists/
    es_settings.cfg
```

> **Why `/roms/profiles/`?** The ROMs partition (`/dev/mmcblk0p3`) is the only storage that's immediately accessible on the SD card without remounting and is the natural home for user data. It's also what gets backed up easily.

---

## Implementation Steps

### Step 1 — `ProfileManager` (New Singleton Class)

**New file**: `es-core/src/ProfileManager.h` / `ProfileManager.cpp`

This is the heart of the feature. It is a singleton (matching the pattern of `Settings`, `Paths`) that:

1. Defines a `Profile` struct:
   ```cpp
   struct Profile {
       std::string name;       // "jacko"
       std::string avatarPath; // path to a .png avatar image
   };
   ```

2. Reads/writes a profile list from `/roms/profiles/profiles.json` (or a simple plain-text/XML file to avoid a JSON dependency, e.g. `profiles.cfg`).

3. Holds the **currently active profile** (`mActiveProfile`).

4. Exposes a key helper used everywhere:
   ```cpp
   static std::string getProfileDataPath();  // returns "/roms/profiles/jacko"
   ```

5. Methods:
   - `loadProfiles()` — reads the profile list from disk.
   - `saveProfiles()` — persists changes.
   - `setActiveProfile(const std::string& name)` — switches active profile, triggers path recalculation.
   - `createProfile(const std::string& name)` — creates folder structure on disk.
   - `deleteProfile(const std::string& name)` — removes the folder (with confirmation).
   - `getProfiles()` — returns the list.
   - `getActiveProfile()` — returns current.
   - `isProfilesEnabled()` — returns false if no profiles exist yet (single-user fallback).

---

### Step 2 — Wire `ProfileManager` into `Paths`

**Modified file**: `es-core/src/Paths.cpp` — constructor and `loadCustomConfiguration()`

Currently `Paths.cpp` hardcodes paths like:
```cpp
mSaveStatesPath = "/userdata/saves"; // on Batocera
// or falls through to defaults
```

The `getDirectory()` call in `SaveStateConfigFile.cpp` ultimately resolves through `Paths::getSavesPath()`.

**The change**: After `ProfileManager` loads, inject the profile's base path into `Paths` as an override for three specific paths:

```cpp
// In Paths constructor (near the end, after loadCustomConfiguration):
if (ProfileManager::getInstance()->isProfilesEnabled()) {
    std::string profileBase = ProfileManager::getInstance()->getProfileDataPath();
    mSaveStatesPath    = profileBase + "/savestates";
    mScreenShotsPath   = profileBase + "/screenshots";
    // mUserEmulationStationPath stays global (themes, input stay shared)
}
```

Add a new method `Paths::recalculateProfilePaths()` so the profile switch can call it without restarting ES.

**Gamelist path**: The gamelist path is resolved inside `SystemData.cpp` independently. Add a new `Paths::getGamelistOverridePath()` that returns empty string when no profile is active, or `profileBase + "/gamelists"` when one is. `SystemData` will check this when loading `gamelist.xml`.

---

### Step 3 — Wire `ProfileManager` into `SystemData` / `Gamelist`

**Modified file**: `es-app/src/SystemData.cpp` and `Gamelist.cpp`

`gamelist.xml` stores per-game metadata: last played, play count, favorite flag, and scraped art paths.

Only **three fields** need to be profile-scoped:
- `lastplayed`
- `playcount`
- `favorite`

Everything else (name, description, publisher, scraped art) should stay in the shared global gamelist.

**Strategy — Overlay Gamelists**:
- ES always loads the global `gamelist.xml` from the ROM folder (shared artwork and metadata).
- After loading, if a profile is active, it loads a second **overlay** gamelist from `profileBase/gamelists/<system>/gamelist.xml` that contains only per-user fields.
- On save, user-specific fields write to the overlay file only; global fields write to the main gamelist.

This avoids duplicating game descriptions and artwork paths per user.

**Affected functions in `Gamelist.cpp`**:
- `parseGamelist()` — add a second load pass from the profile gamelist path.
- `updateGamelist()` — split writes between shared and profile gamelist.

---

### Step 4 — `GuiProfileSelect` (New Startup Screen)

**New file**: `es-app/src/guis/GuiProfileSelect.h` / `GuiProfileSelect.cpp`

This is the "Who's Playing?" screen shown at launch. It follows the same `GuiComponent` pattern as `GuiMsgBox`, etc.

**Layout**:
- Full-screen, shown before `ViewController` initializes.
- Displays a horizontal row of profile tiles (name + avatar image).
- Each tile is selectable with D-pad / joystick.
- A "+" tile at the end allows creating a new profile.
- Pressing A on a tile sets the active profile and proceeds.
- Pressing B (if a profile is already set) goes straight in (skip screen option for single-user use).

**Trigger point in `main.cpp`**:
```cpp
// After window init, before SystemData::loadConfig():
if (ProfileManager::getInstance()->isProfilesEnabled()) {
    window.pushGui(new GuiProfileSelect(&window, [&]() {
        // On profile selected: proceed with normal ES startup
        loadSystems(window);
    }));
}
```

By pushing the profile screen before systems load, we guarantee `Paths` uses the correct profile root when `SystemData` and `SaveStateRepository` initialize.

---

### Step 5 — Profile Management in `GuiMenu`

**Modified file**: `es-app/src/guis/GuiMenu.cpp`

Add a **"Profiles"** entry to the main menu (similar to the existing "Quit" entry). This opens `GuiProfileSettings` with options:

- **Switch Profile** — re-shows `GuiProfileSelect`, then reloads gamelists.
- **Create Profile** — prompts for a name (using the existing `GuiTextEditPopup`), creates the folder structure.
- **Delete Profile** — shows a list with confirmation dialog (`GuiMsgBox`).
- **Rename Profile** — prompts for a new name.
- **Disable Profiles** — removes the feature entirely (falls back to single-user behavior).

---

### Step 6 — `es_settings.cfg` Scoping

**Modified file**: `es-core/src/Settings.cpp` — `loadFile()` / `saveFile()`

Currently settings load from `~/.emulationstation/es_settings.cfg`. 

When a profile is active, a **profile-local** settings file at `profileBase/es_settings.cfg` should override a small subset of user-specific settings:

- Last selected system
- Sort order
- Filter state

Shared settings (display mode, audio volume, controller mappings) should remain global. The simplest approach: load the global file first, then load the profile file and let it override matching keys.

---

### Step 7 — Backup/Export Script

**New file**: `ports/backup_profile.sh`

A simple shell script the user can run from the Ports menu that:
1. Reads the active profile name from a lockfile or asks via `whiptail`.
2. `tar`s up `/roms/profiles/<name>/` into `/roms/profiles/<name>_backup_$(date).tar.gz`.
3. Prints completion to the terminal/log.

This is the "two-second backup" UX promise — just zip the tiny profile folder.

---

## File Change Summary

| File | Action | Purpose |
| :--- | :--- | :--- |
| `es-core/src/ProfileManager.h` | **Create** | Profile data struct, list management, active profile singleton |
| `es-core/src/ProfileManager.cpp` | **Create** | Load/save profiles from disk, path helpers |
| `es-core/src/Paths.h` | **Modify** | Add `recalculateProfilePaths()`, `getGamelistOverridePath()` |
| `es-core/src/Paths.cpp` | **Modify** | Inject profile-scoped `mSaveStatesPath`, `mScreenShotsPath` at end of constructor |
| `es-app/src/main.cpp` | **Modify** | Show `GuiProfileSelect` before system load if profiles exist |
| `es-app/src/Gamelist.cpp` | **Modify** | Load overlay profile gamelist; split writes on save |
| `es-app/src/guis/GuiProfileSelect.h/.cpp` | **Create** | Animated "Who's Playing?" picker screen |
| `es-app/src/guis/GuiProfileSettings.h/.cpp` | **Create** | Profile management menu (create/delete/rename/switch) |
| `es-app/src/guis/GuiMenu.cpp` | **Modify** | Add "Profiles" entry to main menu |
| `es-core/src/Settings.cpp` | **Modify** | Load profile `es_settings.cfg` as an overlay after global |
| `es-core/CMakeLists.txt` | **Modify** | Add `ProfileManager.cpp` to build |
| `es-app/CMakeLists.txt` | **Modify** | Add `GuiProfileSelect.cpp`, `GuiProfileSettings.cpp` to build |
| `ports/backup_profile.sh` | **Create** | One-command profile backup script |

---

## What Is NOT Changed

- ROMs folders — completely untouched.
- Themes and artwork — completely shared.
- BIOS files — shared.
- Controller mappings (`es_input.cfg`) — shared (each person uses the same controller).
- RetroArch global config (`retroarch.cfg`) — shared. Per-profile RetroArch saves are handled purely through the redirected save state and save file paths.
- The GitHub Actions build — no changes needed; this is purely runtime behavior.

---

## Risks & Mitigations

| Risk | Mitigation |
| :--- | :--- |
| Gamelist overlay adds complexity to save logic | Only three fields are profile-scoped. A flag `mProfileOverride = true` on `FileData` marks which fields route to the overlay file. |
| `Paths` is a singleton initialized once | Add `recalculateProfilePaths()` so a mid-session profile switch can update paths without restarting. Gamelists will need to reload after a switch. |
| User picks a profile name with special characters | Sanitize profile names (alphanumeric + dash/underscore only) in `ProfileManager::createProfile()`. |
| exFAT on `/roms` doesn't support symlinks | No symlinks needed — everything is directory-based plain files. |
| Profile folder doesn't exist on first boot | `ProfileManager::createProfile()` uses `Utils::FileSystem::createDirectory()` recursively. |

---

---

## ⚠️ Critical: RetroArch Does Not Automatically Respect ES Paths

ES and RetroArch are two completely separate processes. RetroArch reads its save directories from **its own config file** (`/home/ark/.config/retroarch/retroarch.cfg`), not from anything ES sets internally.

### What ES *does* control directly
| Type | Mechanism | Result |
| :--- | :--- | :--- |
| **Save states** (`.state`, `.state.auto`) | ES passes `-state_file "/full/path"` as a command-line arg (see `SaveState.cpp:setupSaveState()`) | ✅ Fully redirectable just by changing `Paths::getSavesPath()` |
| **Screenshots** | ES's own screenshot writer uses `Paths::getScreenShotPath()` | ✅ Fully redirectable |
| **Gamelist metadata** | ES writes its own XML files | ✅ Fully redirectable |

### What ES does NOT control
| Type | Who controls it | Problem |
| :--- | :--- | :--- |
| **Battery saves** (`.srm`, `.sav`, `.eep`) | RetroArch reads `savefile_directory` from `retroarch.cfg` | ❌ NOT redirected by changing `Paths` |

Battery saves are the game's actual internal saves — the RPG progress file your character sleeps into on a real cartridge. RetroArch writes these silently on its own regardless of what ES's `Paths` singleton says.

### The Fix: `--appendconfig` + A Temp Session File

RetroArch has a built-in CLI argument for exactly this situation:

```bash
retroarch --appendconfig "/tmp/es_profile.cfg" -L core.so rom.rom
```

Values in the appended file override the main `retroarch.cfg` **for that session only**. Nothing is written back to disk. No restore needed.

#### ES side — write the session file before launch

**Modified file**: `es-app/src/FileData.cpp` — `launchGame()`

```cpp
// Before process.run():
if (ProfileManager::getInstance()->isProfilesEnabled()) {
    std::string profileBase = ProfileManager::getInstance()->getProfileDataPath();
    std::ofstream f("/tmp/es_profile.cfg");
    f << "savefile_directory = \"" << profileBase << "/saves\"\n";
    f << "savestate_directory = \"" << profileBase << "/savestates\"\n";
    f << "screenshot_directory = \"" << profileBase << "/screenshots\"\n";
}

int exitCode = process.run();

// After game exits — clean up:
Utils::FileSystem::removeFile("/tmp/es_profile.cfg");
```

#### ArkOS wrapper side — pass the flag to RetroArch

ArkOS doesn't launch `retroarch` directly — it calls a shell wrapper (`/opt/runemu.sh`). That script internally calls RetroArch, so ES can't pass `--appendconfig` through its own command line.

The wrapper needs a small addition (a one-time system-side patch):

```bash
# In /opt/runemu.sh, where retroarch is called:
APPENDCONFIG=""
if [ -f /tmp/es_profile.cfg ]; then
    APPENDCONFIG="--appendconfig /tmp/es_profile.cfg"
fi

retroarch $APPENDCONFIG -L "$CORE" "$ROM" ...
```

If `/tmp/es_profile.cfg` doesn't exist (single-user mode, no profile selected), the script behaves identically to before.

#### Why this is better than patching `retroarch.cfg`

| | `--appendconfig` approach | Direct `retroarch.cfg` patching |
| :--- | :--- | :--- |
| **Crash safety** | ✅ Session-only, nothing to restore | ❌ Must restore after exit or config stays broken |
| **File integrity** | ✅ Main config never touched | ❌ Risk of corruption mid-write |
| **Simplicity** | ✅ Write file, delete file | ❌ Read → parse → replace → write → restore |
| **Auto cleanup** | ✅ `/tmp` is wiped on reboot as a bonus failsafe | ❌ Requires explicit restore logic |

> [!NOTE]
> The patch to `/opt/runemu.sh` is a one-time modification to the ArkOS system scripts on the device. Since `/` is `btrfs` and read-write, this is straightforward. The `setup_boot_hook.sh` script can be extended to apply this patch automatically as part of the initial setup.

---

## Suggested Build Order


**Modified file**: `es-app/src/FileData.cpp` — `launchGame()`

Add a `patchRetroarchConfig()` helper that:
1. Reads `/home/ark/.config/retroarch/retroarch.cfg` line by line.
2. Replaces (or appends) `savefile_directory = "..."` with the profile-scoped path.
3. Writes the file back before `process.run()` is called.
4. Saves the **original value** so it can be restored afterward.

```cpp
// Pseudocode — before process.run() in FileData::launchGame():
std::string originalSaveDir;
if (ProfileManager::getInstance()->isProfilesEnabled()) {
    std::string profileSaves = ProfileManager::getInstance()->getProfileDataPath() + "/saves";
    Utils::FileSystem::createDirectory(profileSaves);
    originalSaveDir = patchRetroarchConfig("savefile_directory", profileSaves);
}

int exitCode = process.run();

// After game exits — restore retroarch.cfg:
if (!originalSaveDir.empty())
    patchRetroarchConfig("savefile_directory", originalSaveDir);
```

The `patchRetroarchConfig()` function is ~20 lines using `std::ifstream`/`std::ofstream`, the same pattern already used throughout `Paths.cpp` and `Settings.cpp`. It returns the old value so it can be restored cleanly.

> [!IMPORTANT]
> Restoration is essential. If ES crashes mid-game and can't restore the cfg, the next boot will have RetroArch pointing at a profile path instead of its real save dir. A safe fallback: write the original value to a temp file (`/tmp/ra_savedir_backup.txt`) before patching, and check for it on startup.

---

## Suggested Build Order

1. `ProfileManager` (no dependencies on other new code — can be built and tested in isolation).
2. `Paths` modifications (depends on `ProfileManager`).
3. `Gamelist` overlay logic (depends on `Paths`).
4. `GuiProfileSelect` (depends on `ProfileManager`, can be stubbed to test UI).
5. `main.cpp` hookup (integrates everything).
6. `GuiProfileSettings` + `GuiMenu` entry (polish / management).
7. `Settings` scoping (lowest priority — nice-to-have).
8. `backup_profile.sh` (fully independent, can be done anytime).
