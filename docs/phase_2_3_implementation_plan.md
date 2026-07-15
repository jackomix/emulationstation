# Implementation Plan: Profile & Game Stats Polish (Phases 2 & 3)

This plan details the implementation of Phase 2 (Core UX Architecture) and Phase 3 (Achievements & Stats Redesign) to finalize the unified profile and game stats features.

---

## Phase 2: Core UX Architecture

### 1. Consolidated Profile Selector Grid (`GuiProfileSelect`)
Replace the vertical profile list with a clean, horizontal selector grid.
- **Layout**: Inherit directly from `GuiComponent`. Use a 1-row `ComponentGrid` containing custom profile cards.
- **Boot Flow**:
  - If "Auto-login to Last Profile" is enabled in system settings, bypass this selector and load the last active profile.
  - If only one profile exists, bypass this selector entirely.
  - On load/switch: Verify and restore missing profile directories (`saves`, `savestates`, `screenshots`). Reset view state to the main System Select screen with filters/searches cleared.
- **Switch Action**: Selecting a profile card sets it active, clears the GUI stack, and returns to the reloaded System Select view.
- **Profile Management**: 
  - The final tile in the horizontal grid is a "Create New Profile" card with a "+" sign.
  - Pressing the **X button** on a profile card opens a popup menu with **Rename** and **Delete** actions.
  - Deleting the active profile automatically falls back to the first available profile and triggers a full UI reload. Deleting prompts a confirmation dialog.

### 2. Decluttered Main Menu (`GuiMenu`)
Integrate active profile info directly into the Start Menu.
- **Interactive Profile Header**: Inserted as the first row in the menu list. Shows the active profile's avatar (placeholder `:/cartridge.svg`), username, and total points. Pressing **A** on this header opens the Game Stats screen (`GuiRetroAchievements`).
- **"SWITCH PROFILE" Entry**: Positioned directly above "QUIT" at the bottom of the list. Opens `GuiProfileSelect`.
- **Cleanup**: Delete obsolete `GuiProfileSettings` files.

---

## Phase 3: Achievements & Stats Redesign

### 3. Unified "Game Stats" Screen (`GuiRetroAchievements`)
A centralized dashboard representing all games played by the active profile.
- **Split-Pane Layout**: A 45% left (game list) / 55% right (details panel) split.
- **Left Pane**: List of games played by the active profile (`PlayCount > 0` or has achievements).
- **Right Pane**: Details card showing selected game's box art, total playtime, play count, and achievement completion progress.
- **Sorting & Filtering**:
  - Sort by "Most Played" (playtime, default), "Last Played", or "Title".
  - Filter by "All Played Games", "Only Games with Achievements", or "Completed (100%)".

### 4. Unified Game Details Screen (`GuiGameAchievements`)
Clicking a game in the Stats list opens its detailed stats and achievements page.
- **Stats Block (Top)**: Displays Playtime, Play Count, and Last Played.
- **Achievements List (Bottom)**: Displays achievements if supported.
  - Unlocked: Vibrant color badge, unlock date, and hardcore status.
  - Locked: Grayscale badge (asset suffix `_lock.png` loaded from scraper) with a lock icon overlay.
- **Shoulder Shortcuts**:
  - **L1/R1** (`pageup`/`pagedown` inputs): Cycle achievement filters ("All" / "Unlocked" / "Locked").
  - **L2/R2**: Cycle sorting options ("Default Order" / "Points" / "Unlock Date").
  - Filter/sort choices display dynamically in the screen subtitle.

---

## 4. Technical Implementation Details

Based on codebase verification, here are the technical paths for these features:

### 4.1 Main Menu Header Row
- `MenuComponent` wraps a `ComponentList`.
- Insert the interactive Profile Header directly as the first `ComponentListRow`.
- Use a `WebImageComponent` (avatar) and two `TextComponent`s (username and points) inside the row.
- The row height automatically scales to fit the avatar size, inheriting D-pad scrolling, highlight bars, and selection callbacks natively.

### 4.2 Cycle Shortcuts in Game Achievements Screen
- `"pageup"` (L1), `"pagedown"` (R1), `"l2"`, and `"r2"` keys are natively mapped in `InputConfig`.
- In `GuiGameAchievements::input`, directly intercept these shoulder and trigger inputs to cycle filters and sorting in real-time.
- No separate menu row is needed as a fallback.

### 4.3 Profile Grid Switcher (`GuiProfileSelect`)
- Change the base class of `GuiProfileSelect` to inherit from `GuiComponent` directly.
- Use a 1-row `ComponentGrid` populated with custom `ProfileCard` components.
- `ComponentGrid::input` natively maps D-pad left/right inputs to `moveCursor(Vector2i(-1/1, 0))` and manages focus loss/gain, audio menu move sounds, and help prompts.
- Pressing the X button (mapped to `"x"`) pushes a standard `GuiSettings` popup menu for "Rename Profile" and "Delete Profile".
