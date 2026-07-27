# Implementation Plan: Adding Dual-Tab Navigation to `GuiGameAchievements`

> **Target File**: `es-app/src/guis/GuiGameAchievements.h` and `es-app/src/guis/GuiGameAchievements.cpp`  
> **Status**: Verified against current codebase (Commit `7ef249921` single-tab baseline)  
> **Goal**: Add a tabbed interface (`ACHIEVEMENTS` and `PLAY HISTORY`) with dual-navigation support (shoulder bumpers + D-pad accessibility).

---

## 1. Codebase Verification & Fact-Check Summary

Before creating this plan, 5 specialized subagents conducted a comprehensive investigation of the codebase. Here are the empirical findings that correct or refine previous AI theories (`ai/sonnet.md`):

| Aspect | `ai/sonnet.md` Claim | Empirical Codebase Reality |
| :--- | :--- | :--- |
| **`ComponentTab` Role** | Assumed `ComponentTabItem::addElement` holds full tab pages/containers. | **`ComponentTab` is strictly a tab selector bar.** `ComponentTabItem::addElement` only holds tab bar labels/icons. The parent GUI owns content views and swaps them via `setCursorChangedCallback()` (matches `GuiBios.cpp` & `GuiBatoceraStore.cpp`). |
| **Tab 1 Title & Data** | Called Tab 1 `"INFO"`. | **Should be `"PLAY HISTORY"`** per `HANDOFF.md`. `FileData` already tracks `GameTime` (seconds), `PlayCount`, `LastPlayed`, `Developer`, `Publisher`, `Genre`, `ReleaseDate`, and `Desc`. |
| **Input Collision** | Recommended delegating all non-X inputs directly to `GuiSettings::input()`. | **Shoulder bumpers (`leftshoulder`/`pageup`) MUST be intercepted FIRST.** Otherwise, `ComponentList::input()` will catch shoulder button inputs and perform a 6-item page scroll instead of switching tabs. |
| **Partial Tab State** | Claimed partial tab code was live in `.cpp`. | **False.** `GuiGameAchievements` was reverted to a clean single-tab baseline (commit `7ef249921`). Zero `mTabGrid` or sim logging exists. |
| **480p Math** | Claimed `WINDOW_WIDTH` = 540px. | **Correct.** On 640×480 screen, `min(480 * 1.125, 640 * 0.90) = 540px`. |

---

## 2. Architecture & Design Specification

### A. Dual Navigation Model
1. **Bumper Mode**: Pressing `L1`/`R1` (mapped to `leftshoulder`/`rightshoulder` or `pageup`/`pagedown`) directly shifts active tab selection (`mTabs->moveCursor(-1)` / `mTabs->moveCursor(1)`) regardless of UI focus.
2. **D-pad Accessibility Mode**:
   - Pressing **UP** when the cursor is on row 0 of the list causes `ComponentList::input()` to return `false`.
   - `MenuComponent`'s grid passes focus UP into the `ComponentTab` header.
   - When `ComponentTab` has focus, D-pad **LEFT/RIGHT** navigates tabs (`mTabs->input()` returns `true`).
   - Pressing D-pad **DOWN** while on `ComponentTab` returns focus to item 0 of the list.

### B. Tab Structure & Contents

#### Tab 0: `ACHIEVEMENTS`
- **Header Subtitle**: Softcore/hardcore points and completion totals: `Achievements (softcore): X/N`, `Achievements (hardcore): Y/N`, `Points: A/B`.
- **Content View**: `mMenu` list populated with `GameAchievementEntry` rows (badge image, title, description, points, unlock date, hardcore star).
- **Progress Bar**: `RetroAchievementProgress` (`mProgress`) visible in header area.

#### Tab 1: `PLAY HISTORY`
- **Header Subtitle**: Game Metadata Overview (`ConsoleName`, `Developer`, `Genre`).
- **Content View**: `mMenu` list populated with structured information rows:
  1. **Game Media / Box Art**: Row displaying local box art (`mFile->getImagePath()`) or web badge (`ra.getImageUrl()`).
  2. **Play Time**: `Utils::Time::secondsToString(mFile->getMetadata(MetaDataId::GameTime))` (e.g. `12h 45m`).
  3. **Play Count**: `mFile->getMetadata(MetaDataId::PlayCount)` (e.g. `24 times`).
  4. **Last Played**: `mFile->getMetadata(MetaDataId::LastPlayed)`.
  5. **Overall Achievement Completion**: Integrated `RetroAchievementProgress` bar widget.
  6. **Game Description**: Wrapped text of `mFile->getMetadata(MetaDataId::Desc)`.

### C. Layout & Sizing Invariants (Rule 13 Compliance for 480p)
- **Target Resolution**: 640 × 480 px (R36S Handheld).
- **`WINDOW_WIDTH`**: 540.0 px.
- **Header Height**: ~105.0 px.
- **Tab Selector Bar (`ComponentTab`)**: Positioned directly below header grid. Height = $480 \times 0.06 = \mathbf{28.8\text{ px}}$. Width = 540.0 px.
- **List Panel Height**: Dynamically calculated by `MenuComponent::updateSize()` after `mMenu.setMaxHeight(...)`.
- **Button Grid Height**: 35.2 px at bottom of menu.

---

## 3. Step-by-Step Implementation Guide

### Step 1: Update Header (`es-app/src/guis/GuiGameAchievements.h`)

1. Include `ComponentTab.h`:
   ```cpp
   #include "components/ComponentTab.h"
   ```
2. Add private members to `GuiGameAchievements`:
   ```cpp
   std::shared_ptr<ComponentTab> mTabs;
   int mActiveTab; // 0 = ACHIEVEMENTS, 1 = PLAY HISTORY
   RetroAchievementInfo mRaInfo; // Store achievement data for tab rebuilds
   
   void populateTabContent();
   void populateAchievementsTab();
   void populatePlayHistoryTab();
   ```

### Step 2: Initialize Tab Bar in Constructor (`GuiGameAchievements.cpp`)

1. Inside constructor `GuiGameAchievements::GuiGameAchievements`:
   ```cpp
   mActiveTab = 0;
   mRaInfo = ra;
   
   // Instantiate Tab Bar
   mTabs = std::make_shared<ComponentTab>(mWindow);
   mTabs->addTab(_("ACHIEVEMENTS"));
   mTabs->addTab(_("PLAY HISTORY"));
   
   // Wire selection callback
   mTabs->setCursorChangedCallback([this](const CursorState& state) {
       if (mActiveTab != mTabs->getCursorIndex()) {
           mActiveTab = mTabs->getCursorIndex();
           populateTabContent();
       }
   });
   ```

2. Add `mTabs` to `mMenu` or register as child component so it receives sizing and rendering updates.

### Step 3: Implement Content Swapping (`populateTabContent`)

```cpp
void GuiGameAchievements::populateTabContent()
{
    mMenu.clear(); // Clear existing rows
    
    if (mActiveTab == 0)
    {
        populateAchievementsTab();
    }
    else
    {
        populatePlayHistoryTab();
    }
    
    mMenu.updateSize();
    centerWindow();
}
```

#### `populateAchievementsTab()`:
- Format subtitle with softcore/hardcore points/counts.
- Re-add all `GameAchievementEntry` rows to `mMenu`.
- Ensure `mProgress->setVisible(true)`.

#### `populatePlayHistoryTab()`:
- Format subtitle with Console & Developer info.
- Add rows for Box Art / Badge, Play Time, Play Count, Last Played, and Game Description.
- Ensure `mProgress->setVisible(true)`.

### Step 4: Implement Input Handling (`GuiGameAchievements::input`)

Intercept shoulder bumpers **BEFORE** delegating to `GuiSettings::input()`:

```cpp
bool GuiGameAchievements::input(InputConfig* config, Input input)
{
    // 1. Intercept shoulder bumpers for direct tab switching (L1 / R1)
    if (input.value != 0)
    {
        if (config->isMappedTo("leftshoulder", input) || config->isMappedTo("pageup", input))
        {
            int currentTab = mTabs->getCursorIndex();
            if (currentTab > 0)
                mTabs->setCursorIndex(currentTab - 1);
            return true; // CONSUME INPUT to prevent ComponentList 6-item page scroll!
        }
        else if (config->isMappedTo("rightshoulder", input) || config->isMappedTo("pagedown", input))
        {
            int currentTab = mTabs->getCursorIndex();
            if (currentTab < mTabs->size() - 1)
                mTabs->setCursorIndex(currentTab + 1);
            return true; // CONSUME INPUT to prevent ComponentList 6-item page scroll!
        }
    }

    // 2. Action button (X button to Launch Game)
    if (config->isMappedTo("x", input) && input.value != 0)
    {
        if (mFile != nullptr)
        {
            Window* window = mWindow;
            while (window->peekGui() && window->peekGui() != ViewController::get())
                delete window->peekGui();
            ViewController::get()->launch(mFile);
        }
        return true;
    }

    // 3. Delegate up/down and D-pad navigation to GuiSettings
    return GuiSettings::input(config, input);
}
```

### Step 5: Update Help Prompts (`getHelpPrompts`)

Add tab switching help prompt:
```cpp
std::vector<HelpPrompt> GuiGameAchievements::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = GuiSettings::getHelpPrompts();
    prompts.push_back(HelpPrompt("l/r", _("TAB")));
    if (mFile != nullptr)
        prompts.push_back(HelpPrompt("x", _("LAUNCH")));
    return prompts;
}
```

---

## 4. Verification Protocol

Follow `CLAUDE.md` Rule 10 & 12 to deploy and verify changes on the R36S device:

1. **Compile Check**: Ensure local build compiles with zero errors/warnings.
2. **Deploy to Device**:
   ```bash
   git commit -am "Implement dual tabs in GuiGameAchievements"
   git push origin attempt2
   # Wait for GitHub Actions build (~211s)
   gh run download <run_id> --name emulationstation-r36s --dir /tmp/es-artifact
   scp -i ~/.ssh/id_ed25519_antigravity /tmp/es-artifact/EmulationStation/emulationstation ark@192.168.18.20:/tmp/emulationstation
   ssh -i ~/.ssh/id_ed25519_antigravity ark@192.168.18.20 "mv /tmp/emulationstation /roms/EmulationStation/emulationstation && chmod +x /roms/EmulationStation/emulationstation && killall emulationstation"
   ```
3. **Visual Framebuffer Capture**:
   ```bash
   ssh -i ~/.ssh/id_ed25519_antigravity ark@192.168.18.20 "sudo ffmpeg -y -f fbdev -i /dev/fb0 -vframes 1 /tmp/screenshot.png" && scp -i ~/.ssh/id_ed25519_antigravity ark@192.168.18.20:/tmp/screenshot.png /tmp/screen.png
   ```
4. **Verification Checklist**:
   - [ ] `ACHIEVEMENTS` tab renders list cleanly at 480p without overlap.
   - [ ] Pressing `L1`/`R1` (or `[`/`]`) instantly switches between `ACHIEVEMENTS` and `PLAY HISTORY`.
   - [ ] `PLAY HISTORY` tab displays Play Time (`Utils::Time::secondsToString`), Play Count, Last Played, and Game Description.
   - [ ] D-pad UP from top of list shifts focus into `ComponentTab`. D-pad LEFT/RIGHT switches tabs. D-pad DOWN returns focus to list.
   - [ ] Pressing `X` button launches game from either tab.
