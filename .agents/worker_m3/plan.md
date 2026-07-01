# Implementation Plan - Milestone 3: Input & Navigation (R2)

## 1. Code Changes
Modify `es-app/src/views/ViewController.cpp`:
- **Start Button check**:
  - Location: Right at the start of `ViewController::input` (before `if (mLockInput)`).
  - Logic: If the input is mapped to "start" and is a press (`input.value != 0`), and if `mState.viewing` is `HOME_VIEW` or `SYSTEM_SELECT`, open the main menu via `mWindow->pushGui(new GuiMenu(mWindow))` and return `true` (consumed).
- **Shoulder Buttons (L/R Bumpers)**:
  - Location: Under Phase 3 (`#ifdef ES_HOME_VIEW` block).
  - Logic:
    - If `leftshoulder`/`pageup` is pressed:
      - If `mState.viewing` is `SYSTEM_SELECT`, transition to `HOME_VIEW` via `goToHomeView(false)` if `input.value != 0`. In either `HOME_VIEW` or `SYSTEM_SELECT`, return `true` (fully consuming the input).
    - If `rightshoulder`/`pagedown` is pressed:
      - If `mState.viewing` is `HOME_VIEW`, transition to `SYSTEM_SELECT` via `goToSystemView(SystemData::sSystemVector.front(), false)` if `input.value != 0` (and `SystemData::sSystemVector` is not empty). In either `HOME_VIEW` or `SYSTEM_SELECT`, return `true` (fully consuming the input).
      
## 2. Compilation and Verification
Run the build command from `/Users/jacko/Documents/myEmulationStation`:
```bash
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu || nproc)
```
Ensure build compiles cleanly.

## 3. Post-Implementation Documentation
Update `BRIEFING.md`, `progress.md`, and write `handoff.md`.
