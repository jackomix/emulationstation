# Agent Handoff

Use this document to pass state to the next agent session. Keeps context small, resets token accumulation.

## Current Status
- **Goal**: Phase 3 Achievements & Stats Redesign
- **Status**: Milestone 3.2 UI layout is broken (overlap issues). Needs fixing.
- **Next Task**: Fix layout overlap in `GuiGameAchievements` and build a proper tab bar component.

## Next Step Details (Layout Fix Plan)
1. **Header Space (Fix Overlap):**
   - Stop using the `\r\n\r\n` hack (it fails because `setSubTitle` strips it).
   - Feed the real stats text (`mAchievementSubtitle`) to `mMenu.setSubTitle()`. This forces the engine to reserve real layout space for the header.
   - The Stats will display in the header automatically. Draw the progress bar below the stats inside `render()` manually. The space is now safely reserved so the list won't overlap.
2. **Tab Bar UI (Like Mockup):**
   - Build a `ComponentGrid(1, 2)` (one row, two columns). Left: "ACHIEVEMENTS", Right: "PLAY HISTORY".
   - Active tab: Fill background with theme text color, invert font color to theme background.
   - Inactive tab: Transparent background, normal font color.
   - Put the grid in a `ComponentListRow`, set `selectable = false`.
   - `addRow()` this Tab Bar row first, before adding the achievement rows. It will sit right on top of the list like the mockup.
## Relevant Files
- [phase_2_3_implementation_plan.md](file:///Users/jacko/Documents/MyEmulationStation/docs/phase_2_3_implementation_plan.md)
- [GuiGameAchievements.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiGameAchievements.h)
- [GuiGameAchievements.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiGameAchievements.cpp)
- [GuiRetroAchievements.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiRetroAchievements.h)
- [GuiRetroAchievements.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiRetroAchievements.cpp)
