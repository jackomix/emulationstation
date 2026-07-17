# Agent Handoff: Resetting Achievements Layout Simulation

The C++ coordinates have been successfully extracted from the engine during runtime.

## What Was Done
1. Injected C++ logging (`std::ofstream` to `/tmp/sim_coords.txt`) into `GuiGameAchievements.cpp` to dump exact positions and sizes of UI components.
2. Compiled and deployed to R36S console.
3. Captured screenshot and coordinates log on device.
4. Created `simulations/game_achievements/visualize_cxx_bounds.py` which visually mapped these coordinates onto the screenshot, proving they match perfectly.

## True C++ Engine Coordinates (Absolute)
- **Menu Background**: pos 0,0, size 640,480
- **TabGrid**: pos 0, 228.372, size 540, 45.6
- **Progress Bar**: pos 25.6, 201.572, size 122.177, 22
- **List Rows (GameAchievementEntry)**: Height 66, start Y at 276.372 (228.372 + 45.6 + 2.4 spacer)
  - **Text Title**: size 544.222 x 36, offset X 75.7778, offset Y 0
  - **Substring Desc**: size 544.222 x 30, offset X 75.7778, offset Y 36

## EmulationStation Achievement UI Layout - Status

## Current State
1. We successfully identified how the font sizing works for EmulationStation. `fontSize` is specified in `es-theme-switch` as a proportion of `Math::min(getScreenHeight(), getScreenWidth())`.
2. EmulationStation uses `1.5f` as the default `mLineSpacing` for `TextComponent`. This maps perfectly to the 36 and 30 pixel heights we observed for font heights of 24 and 20 respectively.
3. We updated `simulations/game_achievements/sim.py` to use these precise coordinates based on the theme definitions and padding variables in `MenuComponent.cpp`.
4. We successfully removed the temporary debugging code (the `sim_coords` dumps) from `GuiGameAchievements.cpp` and `MenuComponent.cpp`.

## Next Steps
- Implement the tab bar in C++ that was mocked in the UI sim.
- Review and refine the list component visual layout in EmulationStation.
