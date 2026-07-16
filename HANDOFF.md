# Agent Handoff: Resetting Achievements Layout Simulation

The current simulation attempts have failed to match the target screenshots because the automatic bounding-box algorithms and coordinates were flawed. We are resetting state so a new agent can approach the problem cleanly.

## What is Wrong
- The previous agent attempted to automate layout measurement using OpenCV thresholding, but the target bounding boxes did not align correctly with the actual text. They ended up capturing parts of progress bars, background shapes, or icons, leading to wrong coordinates and incorrect PIL font size calculations.
- An attempt to use C++ baseline math (e.g. `0.085 * H` for title height) failed because the active `es-theme-switch` XML overrides the default EmulationStation engine margins, positions, and font sizes.
- The layout is currently mismatched and needs a clean visual-first layout matching strategy.

## Active Scripts in `simulations/game_achievements/`
1. `sim.py`: Restored to its original layout simulator structure from the beginning of this session. It uses component-by-component drawing. The colors (like the blue progress bar) and original class interface `App` with `.draw()` are restored.
2. `diff.py`: Computes visual difference overlays between the simulation (`state1.png`) and the real screenshot (`achievements.png`), outputting `comparison.png`.
3. `visualize_bounds.py`: Diagnostic script that draws the OpenCV-detected bounding boxes over `achievements.png` and saves it to `bounds_check.png` to visually audit what the code thinks it is measuring. Opening this file shows that the text coordinates are inaccurate.

## Next Steps for the Next Agent
- **Stop guessing simulation coordinates.** The primary goal is to figure out how text rendering works internally once and for all.
- Modify the C++ source code (`GuiGameAchievements.cpp`, etc.) to inject standard `printf()` or `std::cout` statements that output the `.getSize()` and `.getPosition()` of every UI element directly to the terminal (bypassing the internal ES logger).
- Cross-compile and deploy EmulationStation to the R36S console (see `CLAUDE.md` for deployment instructions).
- Instruct the user to navigate to the achievements screen to trigger the C++ logs, capture the log file from the console, and take a new screenshot.
- Use a script to parse the exact true C++ coordinates from the logs and draw bounding boxes on the new screenshot to visually prove the engine's internal math exactly matches the screen.
- Finally, trace the source code to figure out exactly *how* the engine arrives at those numbers (analyzing theme overrides and FreeType font calculations) so we can perfectly replicate it in future simulations.
