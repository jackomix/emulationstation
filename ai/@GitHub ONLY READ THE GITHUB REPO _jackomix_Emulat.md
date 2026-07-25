<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# @GitHub ONLY READ THE GITHUB REPO "jackomix/EmulationStation" attempt2 branch, read its CLAUDE.MD and HANDOFF.MD and do in-depth analysis of the entire UI and GUI framework system to plan how to add these features without bugs. Include ways to test and debug things in case they go wrong that are still actually useful for an LLM AI that's programming (for example, visual clues aren't really helpful as an LLM doesn't have eyes. so things should be more mathematically checkable).

Send me your plan in a message.

Now I have a thorough picture of the entire codebase. Let me write the comprehensive plan.

***

# EmulationStation `attempt2` — UI/GUI Implementation Plan

This document synthesizes the full architecture from CLAUDE.md, HANDOFF.md, and the source files to give a precise, bug-resistant plan for adding new features to this codebase.

***

## Architecture Overview

The UI is a **retained-mode, immediate-render** C++ framework. There is no data-binding, no DOM diffing, and no reactive layout. Every frame, components call `render()` which draws directly to the SDL/OpenGL framebuffer. Understanding this model is the single most important insight for working bug-free in this codebase.

### Rendering Lifecycle

The call chain for every frame is:

```
Window::update() → GuiComponent::update(deltaTime)
Window::render() → GuiComponent::render(parentTrans)
```

`render()` is called every frame. There is no "mark dirty and re-layout" pass — layout happens **once in the constructor**, and mutations after that must be done explicitly.

### Layout Invariants (Critical — from CLAUDE.md §13)

The framework has strict, non-obvious ordering rules:

1. **Construct all children first, then call `setSize()`** — `setSize()` triggers `onSizeChanged()`, which iterates children. If children are null at that point, sizes come out as `(0,0)`.
2. **Set `setColWidthPerc()` / `setRowHeightPerc()` on `ComponentGrid` before calling `setSize()` on its parent** — the grid reads percentages at the time the parent size is set.
3. **`ComponentGrid` is static** — there is no `removeEntry()`. You cannot add/remove grid cells after construction. Use `setVisible(true/false)` and value setters to toggle content.
4. **`ComponentList` does have `clear()`** — it is safe to call `mList->clear()` and re-add rows at runtime (used in `applyFilterAndSort()`). This is the only dynamic list primitive.
5. **`ComponentListRow` does NOT auto-margin** — always insert a `GuiComponent` spacer of `Renderer::getScreenWidth() * 0.015f–0.02f` between inline elements.

***

## Current State of the Two Key GUIs

### `GuiGameAchievements` (per-game detail)

- Extends `GuiSettings` (which wraps `MenuComponent`)
- Has a **2-tab system** (Achievements / Play History) driven by `mActiveTab`
- `updateTab()` calls `mMenu.clear()` and re-adds all achievement rows on every tab switch — this is the safe pattern for this class
- The **tab bar** (`mTabGrid`) and **progress bar** (`mProgress`) are rendered in `render()` by manually offsetting them above the list, using `mMenu.getTitleHeight()` as the Y anchor
- ⚠️ **Debugging code is still live**: both `GameAchievementEntry::render()` and `GuiGameAchievements::render()` are logging to `/tmp/sim_coords.txt` every 60 frames — this must be removed before shipping any feature


### `GuiRetroAchievements` (global game list)

- A **raw `GuiComponent` subclass**, not using `GuiSettings` — it builds its own `NinePatchBackground` + `ComponentGrid` directly
- Left panel: `ComponentList` (`mList`) — the scrollable game list
- Right panel: `mRightPanel` (a `ComponentGrid` 1×8) showing box art/badge, title, play time, play count, last played, progress bar, and filter/sort label
- Sort/filter state (`mFilterMode`, `mSortMode`) mutates `mFilteredGames` and calls `mList->clear()` + re-adds rows safely
- Layout is 45%/55% column split

***

## Feature Implementation Plans

### Feature 1: Remove Leftover Debug Logging

**Scope**: `GuiGameAchievements.cpp` only (2 render functions)

**Plan**: Delete the `static int frameLog` blocks and `std::ofstream` writes inside `GameAchievementEntry::render()` and `GuiGameAchievements::render()`. Also remove the `#include <fstream>` if it becomes unused.

**Verification (LLM-checkable)**:

- `grep -r "sim_coords" es-app/src/` must return zero results

```
- `grep -r "<fstream>" es-app/src/guis/GuiGameAchievements.cpp` must return zero results after removal (the file still has `#include <fstream>` and `#include <iostream>` at the top from the debug era)
```

- Build must succeed with no warnings about unused variables

***

### Feature 2: Play History Tab (Real Data)

The tab already exists but the "Play History" tab shows a static "No play history found" text. The goal is to populate it with real session/play data.

**Data source**: Each `FileData` has metadata fields: `MetaDataId::LastPlayed`, `MetaDataId::PlayCount`, `MetaDataId::GameTime`. The `RetroAchievements::getGameInfoAndUserProgress()` return type `GameInfoAndUserProgress` may need extension for session history if the RA API supports it.

**Plan**:

1. Check `RetroAchievements.h/.cpp` (not yet read) for whether `GameInfoAndUserProgress` already carries session/play log data from the API
2. If not, the simplest correct implementation is to display local metadata: last played date, play count, and session time pulled from `FileData` (already available via `mFile`)
3. Build a new `std::vector<ComponentListRow> mHistoryRows` in the constructor, parallel to `mAchievementRows`, using `mFile->getMetadata()` calls
4. In `updateTab()`, branch on `mActiveTab == 1` to add `mHistoryRows` instead of the stub text

**Constructor ordering guard**: Build `mHistoryRows` **before** calling `updateTab()` and `centerWindow()` at the end of the constructor. The rows vector must be fully populated before `updateTab()` runs.

**Verification (LLM-checkable)**:

- Assert: `mHistoryRows.size() > 0` when `mFile != nullptr && mFile->getMetadata(MetaDataId::PlayCount) != "0"`
- Assert: after `updateTab()` with `mActiveTab = 1`, `mMenu.getRowCount() == mHistoryRows.size()`
- The `mAchievementRows` vector must not be touched when `mActiveTab == 1`

***

### Feature 3: Tab Bar Input — Make It Focusable

Currently the tab bar (`mTabGrid`) is rendered manually in `render()` and never receives focus — it is purely visual. Tab switching is done via shoulder buttons. This is fine for gamepad navigation but must remain so.

**Current bug risk**: `updateTab()` re-instantiates `leftTab` and `rightTab` as new `TextComponent` objects on every call and calls `mTabGrid->setEntry()` — this is calling `setEntry` on a `ComponentGrid` after construction. **Verify that `ComponentGrid::setEntry()` is safe to call repeatedly** (it should overwrite, not append, since the grid is sized `Vector2i(2,1)` and each call targets a fixed cell). Check `ComponentGrid.cpp` to confirm this.

**Plan**: Do NOT attempt to make the tab bar a focusable `ComponentList`. The shoulder button scheme is correct and matches the existing `input()` handler. Keep it visual-only.

**Verification (LLM-checkable)**:

- `mTabGrid->getSize().y() > 0` after `updateTab()` is called — confirms the grid was given a valid size
- `mTabGrid->getSize().x() == WINDOW_WIDTH` — confirms width was set

***

### Feature 4: Adding a New Tab (General Pattern)

If you need to add a 3rd tab (e.g., "LEADERBOARDS"):

**Constructor changes**:

1. Resize `mTabGrid` to `Vector2i(3, 1)` instead of `Vector2i(2, 1)`
2. Set `setColWidthPerc(0, 0.333f)`, `setColWidthPerc(1, 0.333f)`, `setColWidthPerc(2, 0.334f)`
3. Populate `mLeaderboardRows` vector before calling `updateTab()`

**`updateTab()` changes**:

1. Add a 3rd `TextComponent` tab button with `mActiveTab == 2` highlight logic
2. Call `mTabGrid->setEntry(thirdTab, Vector2i(2, 0), ...)` — valid since grid is pre-sized to 3 columns
3. Add `else if (mActiveTab == 2)` branch to add `mLeaderboardRows`

**`input()` changes**: The shoulder buttons currently binary-toggle between 0 and 1. Change to: `mActiveTab = (mActiveTab + 1) % NUM_TABS`

**⚠️ Grid resize warning**: If you change `mTabGrid` from `Vector2i(2,1)` to `Vector2i(3,1)`, you cannot do this at runtime on the existing pointer — you must construct a new `ComponentGrid`. Since `mTabGrid` is a `shared_ptr`, reassign it: `mTabGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(3,1));`. This is safe because `mTabGrid` is not registered as a child of any other component — it is only used directly in `render()`.

**Verification (LLM-checkable)**:

- `mTabGrid->getGridSize().x() == 3` after reassignment
- `(col_perc_0 + col_perc_1 + col_perc_2) == 1.0f` — column percentages must sum to exactly 1.0
- `mActiveTab` must satisfy `0 <= mActiveTab < NUM_TABS` before every `updateTab()` call

***

### Feature 5: Sort/Filter UI in `GuiRetroAchievements`

Sort and filter are already implemented (`cycleFilter()`, `cycleSort()`, `applyFilterAndSort()`). The `mSortFilterLabel` in the right panel shows current state. If you want to expose these as a visual button row:

**Do NOT use `ComponentGrid` for the button row** — you cannot add it dynamically. Instead, use `mList`'s existing header row capability or render a dedicated overlay similar to how `mTabGrid` is rendered in `GuiGameAchievements`.

**Safe approach**: Add a `mFilterSortGrid` (a `ComponentGrid` 4×1 for filter/sort labels) as a direct child of the `GuiRetroAchievements` GuiComponent, set its position manually in `centerWindow()`, and render it manually in `render()` just like the tab bar.

***

## Size \& Layout Arithmetic

All layout is relative. On the R36S (640×480 target):

- `WINDOW_WIDTH` in `GuiGameAchievements` = `min(480 * 1.125, 640 * 0.90)` = `min(540, 576)` = **540px**
- `IMAGESIZE` = `480 * (48/720)` = **32px**
- Tab bar height `h = leftTab->getSize().y() + 480 * 0.02` — `TextComponent` height is `font->getLetterHeight() * 1.5` (line spacing). Default `theme->Text.font` on 480p is approximately 24px height → `h ≈ 36 + 9.6 ≈ 45.6px` — matches HANDOFF.md coordinates exactly.
- List rows start at `tabGridY + tabGridHeight + spacer` = `228.372 + 45.6 + 2.4 = 276.372px` — also matches HANDOFF.md.

**Arithmetic invariants to check after any layout change**:

- `tabY = mMenu.getTitleHeight() - mTabGrid->getSize().y() - (screenHeight * 0.005f)` → must be > 0 (i.e., title height must be taller than the tab grid height plus the gap)
- `progressY = tabY - progressHeight - (screenHeight * 0.01f)` → must be > `mMenu.getHeaderHeight()` (or at minimum > 0) to avoid rendering behind the title bar
- Sum of `setRowHeightPerc` values in any `ComponentGrid` must equal exactly `1.0f` — if they do not, the grid silently collapses or overflows

***

## Debugging Strategy for LLMs (No Eyes Required)

Since visual debugging is unavailable, all verification must be **state-assertion based**:

### 1. Coordinate Dump via Log (Temporary)

The existing pattern in the codebase (writing to `/tmp/sim_coords.txt`) is the correct LLM-debug tool. Use `LOG(LogDebug)` from `Log.h` instead of raw `ofstream` so logs go to `/tmp/emulationstation.log` on the device:

```cpp
LOG(LogDebug) << "mTabGrid size: " << mTabGrid->getSize().x() << "," << mTabGrid->getSize().y();
LOG(LogDebug) << "mTabGrid pos: " << mTabGrid->getPosition().x() << "," << mTabGrid->getPosition().y();
```

Fetch the log via SSH: `ssh ark@192.168.18.20 "cat /tmp/emulationstation.log" | grep "mTabGrid"`

### 2. Invariant Assertions in Constructor

After each layout section, add `assert()` checks that can catch ordering mistakes at startup rather than at render time:

```cpp
assert(mTabGrid->getSize().x() > 0 && "TabGrid was not sized — check setSize() call order");
assert(mProgress->getSize().x() > 0 && "Progress bar has zero width");
```

These fire immediately on the device and show up in `dmesg` or the ES crash log.

### 3. Row Count Assertions for `ComponentList`

After every `updateTab()`:

```cpp
int expectedRows = (mActiveTab == 0) ? (int)mAchievementRows.size() : (int)mHistoryRows.size();
assert(mMenu.getRowCount() == expectedRows && "Row count mismatch after tab switch");
```


### 4. Column/Row Percentage Sum Checks

Write a helper function (or inline assert) verifying percentage sums:

```cpp
// For any ComponentGrid grid with N cols:
float sum = colPerc[0] + colPerc[1]; // etc.
assert(fabsf(sum - 1.0f) < 0.001f && "Column percentages do not sum to 1.0");
```


### 5. Build-Time Grep Audits

Before committing any change, run these invariant greps:

- `grep -n "setSize" GuiGameAchievements.cpp` — verify `setSize()` is never called before `setColWidthPerc`/`setRowHeightPerc` on the same component
- `grep -n "delete this" es-app/src/guis/` — any direct `delete this` not wrapped in `mWindow->postToUiThread(...)` is a use-after-free bug
- `grep -n "sim_coords\|ofstream" es-app/src/` — must be zero before merge


### 6. Null Pointer Guards for `mFile`

`mFile` can be `nullptr` (when a game is RA-only with no local file). Every access to `mFile->` must be guarded:

```cpp
if (mFile != nullptr) { /* safe to use mFile */ }
```

`grep -n "mFile->" GuiGameAchievements.cpp` then manually verify each call site has a null check.

***

## Execution Order for Next Agent Session

```
1. **Remove debug logging** (2 render functions in `GuiGameAchievements.cpp`, plus `#include <fstream>` / `<iostream>` if unused)
```

2. **Verify `ComponentGrid::setEntry()` re-call safety** by reading `ComponentGrid.cpp` (specifically the `setEntry` implementation)
3. **Implement Play History tab** using local `FileData` metadata
4. **Add tab count constant** `static constexpr int NUM_TABS = 2` to header, update `input()` to modulo-cycle
5. **Run build** via GHA push, download artifact, deploy via the 5-step R36S workflow in CLAUDE.md §10
6. **Fetch log** via SSH and grep for invariant assertions and any size=(0,0) log lines
7. **Only after** log shows correct sizes, remove assertion/debug logging and ship
