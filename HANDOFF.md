# Agent Handoff

Use this document to pass state to the next agent session. Keeps context small, resets token accumulation.

## Current Status
- **Goal**: Phase 2 Core UX Architecture - Session 3 (Start Menu Profile Integration)
- **Status**: Milestone 1 (GuiProfileSelect consolidated grid layout, alignment, centering, and active focus) is fully completed, deployed, and verified on device.
- **Next Task**: Implement Milestone 2: Decluttered Main Menu (`GuiMenu`).

## Milestone 2 Tasks
1. **Interactive Profile Header**:
   - Inserted as the first row in `GuiMenu` (Start Menu).
   - Display active profile avatar (using `:/cartridge.svg` path), username, and total points.
   - Pressing **A** on this header must open `GuiRetroAchievements` (Game Stats screen).
2. **"SWITCH PROFILE" Entry**:
   - Positioned directly above "QUIT" at the bottom of the list.
   - Pressing **A** on this entry opens `GuiProfileSelect`.

## Relevant Files
- [GuiMenu.cpp](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiMenu.cpp)
- [GuiMenu.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiMenu.h)
- [GuiRetroAchievements.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiRetroAchievements.h)
- [GuiProfileSelect.h](file:///Users/jacko/Documents/MyEmulationStation/es-app/src/guis/GuiProfileSelect.h)
