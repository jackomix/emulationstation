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

## Next Steps for the Next Agent
- **Missing Header Coordinates**: Note that the game icon, game title, and subtitle text (softcore/hardcore points) are managed by `MenuComponent` (`mMenu`) and were not logged in the first pass. You will need to inject logging into `es-app/src/components/MenuComponent.cpp` to get their exact bounds.
- Analyze the font engine (`TextComponent.cpp` and `Font.cpp`) and theme overrides (`es-theme-switch`) to understand exactly why Title height is `36` and Substring height is `30`.
- Update `simulations/game_achievements/sim.py` to use these precise coordinates instead of guesses.
- Remove the temporary logging code from `GuiGameAchievements.cpp` once the math is fully understood and simulated.
