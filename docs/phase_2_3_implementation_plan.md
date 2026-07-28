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

### 3. Per-Game Page (`GuiGameAchievements`) - Build First
Small edit to existing achievement screen. Screen is now accessible even if the game has no achievements.
- **Header**: Keep existing header as-is (game icon, name — no changes).
- **Tab System**: Switched via L1/R1.
  - **Achievements Tab**: Existing icon/name/description list, unchanged. If the game has no achievements (unsupported by RA, or just none defined), the list is empty with faded centered text like "No achievements".
  - **Play History Tab**: Plain list showing date/time played + duration.

### 4. Profile Details Screen (`GuiRetroAchievements`) - Build Second
Completely replace/rewrite current profile details screen design. Copy per-game page's tab structure, then adapt.
- **Tab System**: Switched via L1/R1.
  - **Played Games Tab**: Scrollable list of games. Game icon/box art on the left, game title with "14h 32m · 23/47 achievements" underneath in smaller text. For games with no RA support/no achievements, just show playtime, omit achievement mention entirely. Pressing A on a game row goes to that game's per-game page.
  - **Profile Stats Tab**: Aggregate info — total playtime, total points, games played, etc.
- **Filtering/Sorting**: Handled in a separate submenu/window, not inline. Includes a minimum playtime filter with discrete steps (default 15 minutes, settable to 0 for all games).

### Phase 3 Milestones

#### Milestone 3.1: Per-Game Page Infrastructure & Achievements Tab
- Allow opening `GuiGameAchievements` even if the game has no achievements.
- Implement the tab layout container with both tabs present (L1/R1 toggles active view).
- Implement the Achievements Tab: default behavior when achievements exist, display "No achievements" centered faded text when empty.
- Play History Tab is created but remains empty/placeholder.

#### Milestone 3.2: Per-Game Page Play History Tab
- Fetch play history (start times and durations) for the selected game.
- Populate the Play History Tab with a clean vertical list of past sessions.

#### Milestone 3.3: Profile Details Screen & Played Games Tab
- Set up the tab layout container in `GuiRetroAchievements`.
- Implement the Played Games Tab: list of played games matching the profile, displaying icon, title, playtime, and achievement count.
- Handle fallback layout (playtime only) for games without achievement support.
- Map the "A" button action on a game row to push `GuiGameAchievements` for that game.

#### Milestone 3.4: Profile Details Screen Stats Tab & Filters Submenu
- Implement the Profile Stats Tab showing aggregates: total playtime, total points, and games played.
- Create the sorting/filtering submenu overlay (triggered by option buttons).
- Implement the minimum playtime filter with discrete steps (default 15 mins, min 0 mins).

### Emerging Conventions
- **L1/R1**: Tab switching wherever a screen has tabs.
- **Filters/Sort**: Always pushed to a dedicated submenu.

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


### 5. Future Ideas / Miscellaneous
- **Duplicate RetroAchievements**: Implement deduplication logic for multiple ROM copies of the same game (RA identifies by hash).
- **HowLongToBeat Support**: Integrate HLTB data to show expected completion times for games.
- **Mark as Beaten**: Add functionality to manually mark games as 'Beaten'. This could be synced or managed alongside RetroAchievements data when they confirm the game is beaten.
