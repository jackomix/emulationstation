# Agent Handoff

Use this document to pass state to the next agent session. Keeps context small, resets token accumulation.

## Current Status
- **Goal**: Phase 2 Core UX Architecture - Session 2 (Profiles & Menu Integration)
- **Completed**:
  - Remerged `GuiProfileSelect` references into `GuiMenu.cpp` and `es-app/CMakeLists.txt` after deleting `GuiProfileSettings.h`/`.cpp`.
  - Built successfully on GitHub Actions (GHA Run 29441533729).
  - Deployed to R36S device and restarted EmulationStation.
  - Reordered Constructor Lifecycle in `GuiProfileSelect.cpp`.
  - Standardized Theme and Aesthetics.
  - Fixed Dangerous Bypass Logic.
  - Implemented Dynamic Resolution Scaling.
  - Pushed to `attempt2` branch.

## Current Bug (Overlapping Text in UI)
Fixed overlapping text bug by initializing grid early, moving sizing to end of constructor, and removing bypass from `update()`. Screenshot verified: text no longer overlaps, grid and cards scale dynamically based on screen resolution.

## Action Plan for Next Session

### 1. Wait for GHA Build
- [x] Monitor GitHub Actions for `attempt2` branch build.
- [x] Average build duration is 211 seconds. Timer scheduled.

### 2. Deploy and Verify
- [x] Download artifact via `gh run download`.
- [x] SCP to device `/tmp`.
- [x] Move to `/roms/EmulationStation/emulationstation` and restart.
- [x] Take screenshot via SSH to verify UI layout and scaling.
      (Result: UI renders correctly. Grid cards are spaced and scaled, overlap bug is fixed).

### 3. Visual Adjustments (Fixed)
- **Zero Border Visibility**: Fixed. Changed `mBackground->setEdgeColor` to `0x888888FF` for unselected and `0xFFFFFFFF` for selected states instead of using `theme->Background.color` which was full black.
- **Card Backgrounds**: Fixed. Set `mBackground->setCenterColor` to a dark gray (`0x222222FF`) instead of full black.
- **Screen Background**: Kept as is. Note: Background is a black overlay that is half opacity on a black background, making it look like there is no transparency at all. Also, the screen background has weird rounded corners because it uses `NinePatchComponent` with `:/frame.png` (which is a dialog box frame with rounded corners) instead of a simple full-screen flat rectangle. Needs adjustment in future session.
- **Avatar missing**: Fixed. Swapped `WebImageComponent` with `ImageComponent` and changed missing paths to existing icons (`:/cartridge.svg`, `:/fav_add.svg`).
- **"CREATE NE..." truncated**: Fixed. Changed text font from `theme->Text.font` to `Font::get(FONT_SIZE_SMALL)` so it fits.
- **Cards not vertically centered**: Fixed. Dynamically calculated `totalHeight` of card content and centered vertically by calculating `startY`.
- **No title/header**: Fixed. Added a new `TextComponent` `mTitle` to `GuiProfileSelect` that renders "_("SELECT PROFILE")" directly above the grid.

## Relevant Files
- [GuiProfileSelect.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiProfileSelect.h)
- [GuiProfileSelect.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiProfileSelect.cpp)
- [GuiMenu.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiMenu.cpp)
