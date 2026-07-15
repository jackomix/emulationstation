# Agent Handoff

Use this document to pass state to the next agent session. Keeps context small, resets token accumulation.

## Current Status
- **Goal**: Phase 2 Core UX Architecture - Session 2 (Profiles & Menu Integration)
- **Completed**:
  - Remerged `GuiProfileSelect` references into `GuiMenu.cpp` and `es-app/CMakeLists.txt` after deleting `GuiProfileSettings.h`/`.cpp`.
  - Built successfully on GitHub Actions (GHA Run 29441533729).
  - Deployed to R36S device and restarted EmulationStation.

## Current Bug (Overlapping Text in UI)
The screenshot (`/tmp/screen.png`) shows a light grey background with "CREATE NEW" and focused profile names overlapping on the left-middle side of the screen.

### Root Cause Analysis
1. **Grid Not Sized**: In `GuiProfileSelect.cpp`, `setSize()` is called at the very top of the constructor. This triggers `onSizeChanged()`, which attempts to size `mGrid`. However, `mGrid` is not created until later in the constructor, so `mGrid` is null during this first size pass. The grid remains at `(0, 0)` size, causing all columns to collapse to width 0 and overlap at `(0, 0)`.
2. **Poor Codebase Patterns**: The component was built using ad-hoc sizing, positioning, and hardcoded colors rather than following the native EmulationStation GUI conventions.

## Action Plan for Next Session

### 1. Reorder Constructor Lifecycle
Structure the constructor to follow `GuiMsgBox.cpp` and `GuiSettings.cpp` patterns:
1. Initialize variables and create theme reference first.
2. Initialize children on heap/stack (e.g., `mGrid` and `mBackground`).
3. Call `populateProfiles()` to populate grid entries.
4. Calculate size from contents or use screen-relative bounds.
5. Call `setSize()` at the *end* of the constructor (or after children are instantiated) so `onSizeChanged()` can correctly size `mGrid`.
6. Call `setPosition()` to center the GUI on screen (currently it defaults to 0,0).
7. Call `addChild()` for elements last.

### 2. Standardize Theme and Aesthetics
- Do not hardcode colors like `0x333333FF`, `0x555555FF` in `ProfileCard`.
- Fetch `auto theme = ThemeData::getMenuTheme();` and use `theme->Background.color`, `theme->Text.color`, and the correct fonts to ensure it matches the rest of the UI.
- Use the theme's background shader/colors for `mBackground`.

### 3. Fix Dangerous Bypass Logic
- Do not call `delete this` inside `update()`. This is called every frame when the bypass is active.
- Instead, handle the bypass inside the constructor. If bypass is detected, immediately register a UI thread task to select the profile, trigger the callback, and schedule self-deletion, then return:
  ```cpp
  if (mProfiles.size() <= 1 || autoLogin)
  {
      mWindow->postToUiThread([this]() {
          std::string active = ProfileManager::getInstance()->getActiveProfileName();
          if (active.empty() && !mProfiles.empty()) active = mProfiles.front().name;
          if (!active.empty()) selectProfile(Profile{active});
          if (mDoneCallback) mDoneCallback();
          delete this;
      });
      return;
  }
  ```

### 4. Dynamic Resolution Scaling
- The `ProfileCard` uses hardcoded dimensions (`140`x`180`, avatar `100`x`100`).
- Scale card size and spacing dynamically using screen width/height fractions (e.g., `Renderer::getScreenWidth() * 0.2f`).

## Relevant Files
- [GuiProfileSelect.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiProfileSelect.h)
- [GuiProfileSelect.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiProfileSelect.cpp)
- [GuiMenu.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiMenu.cpp)
