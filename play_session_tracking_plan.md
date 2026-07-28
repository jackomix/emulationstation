# Play Session Tracking Architecture Plan

## Objective
Implement native, accurate play session tracking in EmulationStation. This will replace the artificial "grouping by day" logic and accurately track exactly when a user boots a game, how long they play, and which achievements pop during that exact window.

## 1. Data Structure and Persistence
We will introduce a sidecar tracking system using **rapidjson** (already a dependency in EmulationStation).

- **Location**: Inside the user's profile directory (e.g., `ProfileManager::getInstance()->getProfileDataPath() + "/playhistory/"`). This keeps session data isolated per user profile.
- **File Granularity**: One file per system (e.g., `nes_history.json`). This avoids file spam while keeping data organized.
- **Structure**:
  ```json
  {
    "gamePath": "/roms/nes/mario.nes",
    "sessions": [
      {
        "id": "1627471200",
        "startTime": "2026-07-28T14:00:00Z",
        "durationSeconds": 1800,
        "completed": true
      }
    ]
  }
  ```

## 2. Core Integration: `PlayHistoryManager`
Create a new singleton `PlayHistoryManager` in `es-core` to manage session lifecycles safely and elegantly.

### Hooking into Game Launch (`FileData::launchGame`)
1. **Pre-Launch (Atomic Write)**: Generate a unique session ID (UNIX timestamp). Create the session with `completed: false` and write to a `.tmp` file, then rename to replace the real JSON. This atomic write guarantees 0% corruption risk.
2. **Heartbeat Thread**: Spawn a detached `std::thread` right before launching the emulator. 
   - The thread sleeps in 1-second ticks.
   - Every 30 seconds, it wakes up, increments `durationSeconds` by 30, and saves to disk (using atomic writes). 
3. **Post-Launch**: When the game process exits (via Start+Select or normal exit), stop the heartbeat thread, calculate the exact final duration, flag the session as `completed: true`, and do a final atomic save.

## 3. Handling Battery Death & Crashes
If the device battery dies or the OS hard crashes during a game:
- **Session Duration**: Because the heartbeat saves to disk every 30 seconds, the maximum playtime lost is 29 seconds. The JSON file will show `completed: false` and the duration will be whatever the heartbeat saved last.
- **RetroAchievements Correlation**: The RA API fetch that normally happens on game exit won't run. However, when the user reboots and opens the **PLAY HISTORY** tab, EmulationStation fetches the RA API natively. The code will see the `completed: false` session, finalize it, and map the fetched achievement `DateEarned` timestamps to the session window (`startTime` to `startTime + heartbeat_duration`). Any achievements earned before the battery died will cleanly snap into the interrupted session.

## 4. RetroAchievements Correlation (Normal Exit)
RetroAchievements (RA) does not know about "game sessions"; it only knows when an achievement was unlocked.
To bridge this gap elegantly:
1. When the game cleanly exits, immediately trigger an RA API fetch for unlocked achievements.
2. For each unlocked achievement, read its `DateEarned` (UTC).
3. Cross-reference the timestamp against the user's just-completed `PlaySession` window (`startTime` to `startTime + durationSeconds`).
4. Strict UTC matching is used to permanently associate those achievements with the session in the JSON file. No more guessing UI overlaps.

## 5. UI Overhaul (`GuiGameAchievements`)
The Activity tab will be renamed to **PLAY HISTORY** and become a true session log:
- **Timeline Generation**: Iterate over `PlaySession` objects natively retrieved from `PlayHistoryManager`.
- **Session Header**: 
  - Displays local Date and Time (e.g., `July 28, 2026 at 2:00 PM`).
  - Displays accurate, tracked duration (e.g., `30 mins`).
- **Nesting**: Achievements that occurred during that session are nested below the session header.
- **External Unlocks**: Achievements earned on a different device (or before tracking was added) will be grouped into generic date headers (e.g., "Played on Another Device") since no local ES session exists for them.
