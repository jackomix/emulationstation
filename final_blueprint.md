# Final Production Blueprint: Native C++ Profiles, iiSU Home View, and LAHEE Achievements

This document represents the final, fully-refined blueprint for implementing the **User Profile System**, **iiSU Home View**, and **LAHEE Achievements** natively in C++ inside EmulationStation.

---

## Feature 1: User Profiles (`ProfileManager`)

### 1. Ext4 Symlink Redirection (Fallback) & CLI Overrides (Primary)
To support exFAT TF2 micro SD cards safely without root privileges or mount-pollution:
1. **RetroArch Primary Path Override**:
   - For RetroArch (and other libretro emulators), we bypass symlinks entirely.
   - **Detection Logic**: 
     - The `%CORE%` check is performed on the **raw command template** from `SystemData::getLaunchCommand()` *before* token substitution.
     - The `"retroarch"` substring check is performed on the **first whitespace-delimited token of the resolved command** *after* substitution.
     - If either check passes, we treat it as a RetroArch/libretro launch.
   - If detected, we append `--savefile-directory /storage/roms/profiles/<active_id>/saves --savestate-directory /storage/roms/profiles/<active_id>/savestates` directly to the command-line arguments.
2. **Standalone Emulator Redirection (Fallback)**:
   - For standalone emulators that do not support command-line overrides (e.g. PPSSPP, DraStic), we use ext4-based symlink routing.
   - We create `/storage/profiles/active` on the ext4 system partition (TF1) pointing to `/storage/roms/profiles/<active_id>` (on the exFAT TF2 partition).
   - The emulators' standard config-defined save paths (e.g. `/home/ark/.config/ppsspp/PSP/SAVEDATA`) are replaced with symbolic links pointing to `/storage/profiles/active/saves`.

### 2. Atomic First-Launch Migration (Idempotent)
To prevent any data loss if ES is killed or power-cycles mid-migration:
1. **Idempotency Check**: On start, we call `lstat(original_path, &st); if (S_ISLNK(st.st_mode)) return;` to verify if the redirection is already in place.
2. Rename the original saves folder atomically on the ext4 partition:
   `rename(original_path, original_path + ".pre_profiles_backup")`.
3. Create the symlink: `symlink(profile_target_path, original_path)`.
4. Verify the symlink resolves to a valid, readable directory:
   Check `access(original_path, R_OK | W_OK) == 0`.
5. Only delete the `.pre_profiles_backup` directory once verification passes (or leave it as a fallback). If verification fails, restore the backup.

### 3. Metadata Interception & Wear Protection
1. **Copy Guard constructors**: Define copy constructors and assignment operators for `MetaDataList` in [MetaData.h](file:///Users/jacko/Documents/myEmulationStation/es-app/src/MetaData.h) to set `mOwner = nullptr` on copies, preventing dangling pointers.
2. **JSON Schema Versioning**: Write profile metadata to `/storage/roms/profiles/<id>/metadata.json` with a root `"schema_version": 1` field to support future upgrades.
3. **In-Memory Write Caching**: All stats modifications are written to memory and only flushed to the SD card at:
   - Game exit (hooked in `FileData::launchGame` after emulator process ends).
   - Profile switch.
   - Clean EmulationStation shutdown.
4. **Dirty Flag Suppression**: Intercepting `MetaDataList::set` for profile fields returns immediately, bypassing `mWasChanged = true` so the shared `gamelist.xml` is never flagged for rewrite.

---

## Feature 2: HomeView Dashboard (`HomeView`)

Implemented as a flat `GuiComponent` using coordinate-based layout inside `onSizeChanged()` to match native ES layout code. We ensure `HomeView` is instantiated after `ProfileManager::init()` so the "Continue Playing" widget fetches the correct active profile's `lastplayed` statistics.

### 1. View Culling
To prevent Mali-400 GPU frame drops during vertical pan transitions between HomeView (Y=0) and SystemView (Y=ScreenHeight):
```cpp
float viewScreenY = mHomeView->getPosition().y() + mCamera.translation().y();
if (viewScreenY > -Renderer::getScreenHeight() && viewScreenY < Renderer::getScreenHeight())
{
    mHomeView->render(trans);
}
```

### 2. Matrix State Preservation
In `ViewController::render()`, wrap the screen-space tab header drawing with save/restore matrix blocks:
```cpp
Renderer::setMatrix(parentTrans); // Fixed screen-space
mTabHeader->render(parentTrans);
Renderer::setMatrix(trans);       // Restore camera-offset
```

### 3. Transition Gating (Boolean Flag Model)
To prevent diagonal camera paths if a user navigates systems horizontally while a vertical view transition is active:
- Guard all input routing and transition checks using a simple boolean flag `mYTransitioning` on `ViewController`:
  ```cpp
  if (mYTransitioning)
      return false;
  ```
- `mYTransitioning` is set to `true` when a shoulder button press triggers a vertical transition.
- In `ViewController::update()`, once `std::abs(mCamera.translation().y() - mCameraTargetY) < 1.0f` (resting position), we set `mYTransitioning = false`.

---

## Feature 3: LAHEE Integration

### 1. Anchored URL Parsing & Network Warning Bypasses
- All substitutions in `resolveUrl()` use `rfind(prefix, 0) == 0` for anchored prefix matches.
- Connectivity checks are bypassed only when `global.retroachievements.server` is a custom/local IP or domain.

### 2. Clean posix_spawn Process Manager
- **File Descriptor Culling**: Scan `/proc/self/fd` to close only active descriptors rather than looping over `sysconf(_SC_OPEN_MAX)`.
- **Console Log Redirection**: Redirect the child's stdout/stderr to `/tmp/lahee.log` using `posix_spawn_file_actions_addopen()` and `adddup2()`.
- **Double-Spawn Guard**: Prior to spawning, verify if the LAHEE daemon is already active by testing if socket port `8000` is open, or checking if the PID inside `/tmp/lahee.pid` is alive using `kill(pid, 0) == 0`.

### 3. Decoupled Non-Blocking Zombie Reaping
To prevent zombie processes without modifying global signal handlers, we encapsulate the logic cleanly inside `RetroAchievements::update(int deltaTime)`:
- `RetroAchievements::update(int deltaTime)` is called periodically from `ViewController::update()`.
- Inside `RetroAchievements::update`:
  ```cpp
  if (sLocalServerPid > 0)
  {
      int status;
      pid_t res = waitpid(sLocalServerPid, &status, WNOHANG);
      if (res == sLocalServerPid)
      {
          LOG(LogInfo) << "LAHEE daemon process has exited (PID: " << sLocalServerPid << ")";
          sLocalServerPid = 0; // Reset PID
      }
  }
  ```

---

## Feature Flags & Project Structure
All three features will be wrapped inside clean `#ifdef` flags:
* `#ifdef ES_PROFILES`
* `#ifdef ES_HOME_VIEW`
* `#ifdef ES_LAHEE`

In `CMakeLists.txt`, compile definitions will use the modern target scope pattern:
```cmake
option(ES_PROFILES "Enable Sandboxed User Profiles" ON)
option(ES_HOME_VIEW "Enable iiSU-style Home Dashboard View" ON)
option(ES_LAHEE "Enable LAHEE local achievements integration" ON)

if(ES_PROFILES)
    target_compile_definitions(emulationstation PRIVATE ES_PROFILES)
endif()
if(ES_HOME_VIEW)
    target_compile_definitions(emulationstation PRIVATE ES_HOME_VIEW)
endif()
if(ES_LAHEE)
    target_compile_definitions(emulationstation PRIVATE ES_LAHEE)
endif()
```
These compile-time flags allow features to be compiled and debugged independently, ensuring bisectability of the codebase.
