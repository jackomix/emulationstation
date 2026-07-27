# Handoff for Next Agent

## Current State
The project is on branch `attempt2`. The most recent successful feature was refactoring `GuiGameAchievements` to load asynchronously and adding rarity percentages/points to the achievement list. READ CLAUDE.MD FIRST TO UNDERSTAND CERTAIN THINGS.

The user requested 3 UI improvements. The previous agent attempted to spawn subagents to fix them, but the user requested a full halt to hand off the session. **All previous subagents have been killed, and a faulty change to `ComponentTab.cpp` was successfully reverted via `git restore`. You are starting with a completely clean slate.**

## Pending Tasks (Launch Subagents for Each)

The user wants you to launch individual subagents for these tasks, have them commit the changes WITHOUT building, and then push and build on the main thread when they are all done.

### Task 1: Achievement Badge Scaling & Marquee Hover (File: `GuiGameAchievements.cpp`)
- **Bug:** Unlocked achievements have a much larger badge image than locked achievements.
- **Root Cause:** Unlocked achievements render an extra "Unlocked on: [date]" line below the description. This increases the total height of the row. Because the badge image scales dynamically to fit the row height, it stretches and becomes much larger.
- **Fix:** Remove the dedicated "Unlocked on" row entirely to keep row heights uniform. Instead, elegantly append the unlock timestamp to the *end* of the achievement description text (e.g., separated by a slash, maybe italicized).
- **Feature Addition (Marquee):** Long descriptions get cut off with `...`. Add a marquee scroll effect so that when a user hovers over an achievement, the description text automatically scrolls (bouncing left/right) after ~1.5 seconds, allowing them to read the full text without needing a taller row.

### Task 2: Info Tab Description Highlighting (File: `GuiGameAchievements.cpp`)
- **Bug:** The Info tab uses a `MultiLineMenuEntry` for the game description. It wraps perfectly, but when the user scrolls down and selects it, a massive bright highlight box covers the entire block of text, taking over the whole screen and looking very clunky.
- **Fix:** Redesign this interaction to look elegant. The user suggested putting real thought into this. Maybe when you select it then moving up and down actually scrolls the "camera" so to speak? So to reach options below it you just scroll the camera downwards? This needs planning and a user review as well.

### Task 3: Tab Bar Symmetrical Scrolling (File: `es-core/src/components/ComponentTab.cpp`)
- **Bug:** The tab bar (Achievements, Activity, Info, Options) overflows the screen slightly, but the horizontal scrolling feels awkward. Moving right to the 4th tab scrolls correctly, but moving back left to the 3rd tab instantly scrolls back, cutting off the right side prematurely.
- **Fix:** Implement true symmetrical edge-scrolling. The tab bar should *only* scroll when the user's cursor actively pushes against the edge of the screen (i.e., only scroll left when moving to a tab that is currently off-screen to the left, and vice versa).
- **Note:** The previous agent's changes to this file were already completely reverted to prevent refactor rot. You are starting fresh.
