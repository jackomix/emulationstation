# Master Plan: Amber-ES & LAHEE Integration

## Objective
Seamlessly integrate the LAHEE offline achievement server into the Amber-ES (EmulationStation) frontend to provide a unified, offline-capable achievement experience on handheld devices like the R36S.

---

## 1. Achievement Menu & UI Integration

### 1.1 "View Achievements" Menu Item
- **Location**: `es-app/src/guis/GuiGameOptions.cpp`
- **Changes**: 
    - Add a new entry to the game menu list: `_("VIEW ACHIEVEMENTS")`.
    - Map this entry to a callback that creates and pushes `GuiGameAchievements`.
    - Ensure it only shows for systems that support RetroAchievements (check `system->getHasRetroAchievements()`).

### 1.2 "Achievement Options" Main Menu
- **Location**: `es-app/src/guis/GuiMenu.cpp`
- **Changes**:
    - Add `_("ACHIEVEMENT OPTIONS")` to the main menu (likely under "RETROACHIEVEMENTS SETTINGS" or as a new top-level item).
    - Create a new `GuiSettings` screen containing:
        - **LAHEE Status**: Read-only indicator (ping `localhost:8000`).
        - **Patch RetroArch**: Calls `lahee_patch_ra.py`.
        - **Unpatch RetroArch**: Calls `lahee_unpatch_ra.py`.
        - **Fetch Missing Data**: Triggers a global fetch for the current gamelist (bulk download for all games).

---

## 2. Server Management (Auto-Start)

### 2.1 LAHEE Lifecycle
- **Location**: `es-app/src/main.cpp`
- **Changes**:
    - Add a function `startLAHEEServer()` that runs `LAHEE Server.sh`.
    - **Session Logic**: Ensure the server is only initialized the **first time** EmulationStation is launched during a console session.
    - **Persistence**: Once started, the server should remain active throughout the entire session (even when ES restarts after returning from a game) until the hardware is shut down or rebooted.
    - **Shutdown**: Move the `stopLAHEEServer()` call to the OS shutdown/reboot scripts rather than the standard ES exit routine to maintain persistence.

### 2.2 Configuration
- **Location**: `es-core/src/Settings.cpp`
- **Changes**:
    - Add `AutoStartLAHEE` (default: true).
    - Add `RetroAchievementsServerURL` (default: `http://127.0.0.1:8000/laheer/`).

---

## 3. Scraper & Metadata Extensions

### 3.1 Metadata "icon" Field
- **Location**: `es-core/src/MetaData.h` / `MetaData.cpp`
- **Changes**:
    - Add `METADATA_ICON` to the enumeration.
    - Add "icon" to the default metadata map with type `MD_PATH`.
- **Theming**: Ensure themes can access `<icon>` for `<game>` and `<system>`.

### 3.2 Scraper Logic & Media Management
- **Location**: `es-app/src/scrapers/ScreenScraper.cpp`
- **Hashing**: Integrate [RAHasher](https://github.com/LeXofLeviafan/RAHasher) for accurate ROM hashing.
- **Game Icons**: 
    - Treat game icons as a standard media type (like marquees or screenshots).
    - Save to the system's local `images/` folder (e.g., `nes/images/<game>-icon.png`).
    - Update the `FileData` metadata (`METADATA_ICON`) with the local path.

### 3.3 UI Scrape Trigger (Single Game)
- **Task**: Allow ES to trigger a LAHEE "fetch" for a specific game.
- **Implementation**: ES makes a POST request to `http://127.0.0.1:8000/laheer/dorequest.php?r=laheetriggerfetch&gameid=<id>`.
- **Clarification**: This offers a way for users to manually scrape achievement data (JSON/Badges) for the currently selected game if it was missed during a bulk scrape or if they are focused on a specific title.

---

## 4. Achievement Data Storage & External Tooling

### 4.1 Centralized Data Folder (`RetroAchievements`)
- **Location**: A dedicated `RetroAchievements` folder at the root of the games partition. ES will resolve this path dynamically based on its configured ROM directory.
- **Internal Organization**:
    - **Console Subfolders**: Data organized by console (e.g., `nes/`, `snes/`, `gbc/`).
    - **File Naming**: Descriptive filenames using `<GameID>-<Game Name>.set.json` (e.g., `1446-Super Mario Bros.set.json`).
- **Contents**:
    - Console folders contain the `.set.json` achievement data and associated badge icons.
    - The root of `RetroAchievements/` contains player profile data and avatars.

### 4.2 PowerShell Scraper (`LAHEE_EasyRoms_Scraper.ps1`)
- **Hashing**: Uses [RAHasher](https://github.com/LeXofLeviafan/RAHasher) CLI for compatibility with RetroAchievements' specialized hashing methods.
- **Functionality**:
    1. Resolve the `RetroAchievements` root folder on the games partition.
    2. Scan for systems and read Game IDs.
    3. Download achievement JSON and Badges from RA Web API (using the user's API Key).
    4. Store them using the new consolidated structure (e.g., `RetroAchievements/nes/1446-Super Mario Bros.set.json`).

---

## 5. Userpics Support

### 5.1 Profile Avatar
- **Location**: `es-app/src/guis/GuiMenu.cpp`
- **Changes**:
    - Use `WebImageComponent` to display the user's avatar.
    - Source URL: `http://127.0.0.1:8000/laheer/UserPic/<username>.png`.
    - Re-evaluate the `lahee_diag.py` and `FastFailUserPic` to ensure zero delay when avatars are missing.

## 6. Implementation Strategy: "Script-First" Validation

To ensure the integration is robust, the project will follow a **"Script-First"** approach:

### 6.1 Why start with the PowerShell Scraper (Phase 4)?
- **Logic Validation**: We can perfect the RAHasher integration and LAHEE API communication logic in a high-level language (PowerShell) before porting it to C++.
- **Zero-Risk Testing**: The script can be tested against the `EASYROMS` partition without needing to recompile EmulationStation or modify the handheld's filesystem directly.
- **Immediate Utility**: Users without Wi-Fi dongles get a working solution immediately, while the C++ integration is being developed.
- **API Sandbox**: It serves as a live "sandbox" to verify that LAHEE's `r=laheetriggerfetch` and `r=laheeinfo` endpoints return the exact data formats ES expects.

### 6.2 Porting to C++
Once the PowerShell logic is verified to correctly download `.set.json` files and badges that LAHEE can read, the logic will be ported into `es-app/src/scrapers/ScreenScraper.cpp` and a new `AchievementService.cpp` within Amber-ES.

---

## Verification Plan

### Test Case 1: Auto-Start
1. Boot ES.
2. Check `ps aux` to see if `LAHEE.dll` (via dotnet) is running.
3. Verify ES can ping `localhost:8000`.

### Test Case 2: Menu Integration
1. Open Game Options for a game with achievements.
2. Select "View Achievements".
3. Verify local data from LAHEE is displayed.

### Test Case 3: Patching
1. Click "Patch RetroArch" in ES menu.
2. Launch RetroArch.
3. Verify it logs into LAHEE automatically.
