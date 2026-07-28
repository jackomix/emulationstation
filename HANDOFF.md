# Handoff for Next Agent

## Current State
The project is on branch `attempt2`. The Activity Log UI has been partially refactored based on user feedback (crash fixed, date row selectable, timeline duration removed, hyphen spacing fixed). However, EmulationStation's native playtime tracking is insufficient and cannot handle game session timing natively.

---

## Next Priority Task: Native Play Session Tracking

We are completely overhauling how EmulationStation tracks game sessions.

Refer to the detailed specification plan: [play_session_tracking_plan.md](./play_session_tracking_plan.md)

### 1. Data Structure and Persistence
- Implement a sidecar tracking system using **rapidjson**.
- Store `playhistory/` (e.g., `nes_history.json`) inside the active user's profile directory (`ProfileManager::getInstance()->getProfileDataPath()`).

### 2. Core Integration (`PlayHistoryManager`)
- Create a `PlayHistoryManager` singleton to manage sessions.
- **Game Launch Hook**: In `FileData::launchGame`, generate a unique session ID. Write an atomic "incomplete" session to `.tmp` and rename to disk.
- **Heartbeat Thread**: Spawn a background `std::thread` before `process.run()`. It must tick every 30 seconds, increment duration, and atomically write to disk.
- **Game End Hook**: When `process.run()` finishes, stop the thread and finalize the duration and `completed` status to disk.

### 3. RetroAchievements Correlation
- When the game cleanly exits, fetch unlocked achievements via RA API.
- Map the achievements to the session strictly by checking if their UTC `DateEarned` falls inside the session's duration window (`startTime` to `startTime + durationSeconds`).

### 4. UI Overhaul
- Rename the Activity tab to **PLAY HISTORY** in the UI.
- Rewrite `GuiGameAchievements` to iterate over real `PlaySession` objects natively retrieved from `PlayHistoryManager` rather than artificial day-groups from RA data.
- Ensure orphaned achievements (earned outside known ES sessions) are grouped under generic headers (e.g. "Played on Another Device").
- Handle interrupted sessions gracefully: if a battery dies, the next boot sees an incomplete session, finalizes it with the last known heartbeat duration, and maps RA data accordingly.
