# LAHEE Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the entire integration plan for LAHEE and Amber-ES.

**Architecture:** C++ modifications to EmulationStation frontend to add LAHEE server features, handle `icon` metadata, modify the `ScreenScraper` to handle `icon` fields and RAHasher hashing, add avatar loading, and create the PowerShell external scraper script.

**Tech Stack:** C++, CMake, PowerShell, EmulationStation.

---

### Task 1: Add LAHEE Settings to RetroAchievements Options

**Files:**
- Modify: `es-app/src/guis/GuiRetroAchievementsSettings.cpp`

- [ ] **Step 1: Add LAHEE Menu Items**

Add LAHEE options to the RetroAchievements settings GUI.

```cpp
	addGroup(_("LAHEE OFFLINE SERVER"));
	addEntry(_("PATCH RETROARCH"), true, [window] { Utils::Platform::ProcessStartInfo("python /userdata/roms/ports/LAHEE/lahee_patch_ra.py").run(); });
	addEntry(_("UNPATCH RETROARCH"), true, [window] { Utils::Platform::ProcessStartInfo("python /userdata/roms/ports/LAHEE/lahee_unpatch_ra.py").run(); });
```

### Task 2: Add `METADATA_ICON` Field

**Files:**
- Modify: `es-core/src/MetaData.h`
- Modify: `es-core/src/MetaData.cpp`

- [ ] **Step 1: Add to `MetaDataId` Enum**

In `MetaData.h`, add `CheevosIcon` (or `Icon`) to `MetaDataId`.

```cpp
	Icon,
```

- [ ] **Step 2: Add to `MetaDataList` defaults**

In `MetaData.cpp`, register `Icon` in the appropriate place (in `initMetadata()`).

```cpp
	mGameDefaultMap["icon"] = MetaDataDecl("icon", MD_PATH, "icon", "image", "iconPath");
```

### Task 3: ScreenScraper & Single Game Trigger

**Files:**
- Modify: `es-app/src/scrapers/ScreenScraper.cpp`
- Modify: `es-app/src/guis/GuiGameOptions.cpp`

- [ ] **Step 1: Update ScreenScraper to Parse Icon**

Extract `<image type="icon">` from ScreenScraper API and save it to the local `images/` directory.

- [ ] **Step 2: Add Scrape Trigger**

Add an entry in `GuiGameOptions.cpp` to trigger a manual LAHEE scrape using HTTP POST.

### Task 4: Profile Avatar in GuiMenu

**Files:**
- Modify: `es-app/src/guis/GuiMenu.cpp`

- [ ] **Step 1: Add WebImageComponent to GuiMenu**

Fetch the user avatar from LAHEE's local HTTP server.

### Task 5: Create `LAHEE_EasyRoms_Scraper.ps1`

**Files:**
- Create: `LAHEE_EasyRoms_Scraper.ps1`

- [ ] **Step 1: Write the PowerShell Script**

Create the script in the repository root to scrape games using RAHasher and RA Web API.
