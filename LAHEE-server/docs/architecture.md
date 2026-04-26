# LAHEE Architecture Overview

L.A.H.E.E. (Local Achievements Home Edition Enhanced) is a RetroAchievements (RA) service emulator designed for offline progression, custom achievement sets, and modded gaming.

## Core Components

### 1. Network & Protocol Emulation (`Src/Network.cs`)
The heart of LAHEE is its HTTP server (powered by WatsonWebserver). It intercepts and responds to requests that would normally go to `retroachievements.org`.
- **Base Route**: `/laheer/` (used for character-length matching during binary patching).
- **RA Route**: `dorequest.php` handles almost all emulator interactions.
- **Content Routes**: Serves badges, user pictures, and the Web UI.

### 2. Static Data Manager (`Src/StaticDataManager.cs`)
Manages game metadata, achievement definitions, and ROM hashes.
- **Loading Strategy**: Scans the `Data/` directory for `.json`, `.set.json`, `.hash.txt`, and `.txt` files.
- **Game ID Resolution**: Parses Game IDs from filenames (e.g., `1234-GameTitle.set.json`).
- **Merging**: Automatically merges multiple achievement sets (Core, Subsets, Custom) for the same Game ID into a single unified set for the emulator.

### 3. User Manager (`Src/UserManager.cs`)
Handles local user profiles and session management.
- **Profiles**: Stored as JSON in the `User/` directory.
- **Progression**: Tracks unlocks (Softcore/Hardcore), playtime, and leaderboard entries.
- **Session Tokens**: Manages temporary tokens used by emulators to authenticate requests.

### 4. Live Ticker (`Src/LiveTicker.cs`)
A real-time broadcast system using WebSockets to update the Web UI.
- **Events**: Pushes notifications for unlocks, pings, and status changes.

### 5. Capture Manager (`Src/CaptureManager.cs`)
Automates media capture when achievements are earned.
- **Triggers**: Configurable in `appsettings.json`.
- **Actions**: Can take screenshots (Desktop/Window) or send commands to OBS via WebSockets (e.g., "SaveReplayBuffer").

## Data Flow

1.  **Initialization**:
    - `AppConfig` loads settings.
    - `UserManager` loads local user profiles.
    - `StaticDataManager` scans `Data/` and builds the game/achievement index.
    - `Network` starts the HTTP server.
2.  **Emulator Interaction**:
    - Emulator makes a request to `dorequest.php?r=login`.
    - `Network` routes to `RALogin`, which validates against `UserManager`.
    - Emulator requests Game ID via ROM Hash (`r=gameid`).
    - `StaticDataManager` finds the game; `Network` returns the ID.
    - Emulator requests achievement data (`r=patch` or `r=achievementsets`).
3.  **Progression**:
    - When an achievement triggers, the emulator calls `r=awardachievement`.
    - LAHEE updates the user's local profile and triggers `CaptureManager` and `LiveTicker`.
