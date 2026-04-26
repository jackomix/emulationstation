# Emulated RetroAchievements Protocol

LAHEE emulates the RetroAchievements server API to allow emulators to function offline or with local data.

## Base Endpoint
All RA-related requests are routed through:
`http://<host>:<port>/laheer/dorequest.php`

The `/laheer/` prefix is critical for binary patching character-length matching.

## Core Routes (via `?r=`)

### `login` / `login2`
**Purpose**: Authenticates the user.
**Parameters**: `u` (username), `p` (password/hash - ignored by LAHEE), `t` (token).
**LAHEE Behavior**: 
- If the user doesn't exist, it is automatically created.
- Returns a session token.
- If `AutoSessionOnSingleUser` is enabled and only one user exists, it bypasses strict token checks.

### `gameid`
**Purpose**: Resolves a ROM hash to a RetroAchievements Game ID.
**Parameters**: `m` (MD5 hash).
**LAHEE Behavior**: Scans loaded `.hash.txt` files for a match.

### `patch` / `achievementsets`
**Purpose**: Fetches achievement definitions, leaderboards, and rich presence scripts.
**Parameters**: `g` (Game ID) or `m` (MD5 hash).
**LAHEE Behavior**: 
- Combines all achievement sets found for that Game ID.
- Automatically "localifies" badge URLs to point to LAHEE.
- Supports `LoadUnofficialAsOfficial` to force unofficial achievements to be recognized by emulators that normally ignore them.

### `startsession`
**Purpose**: Marks the beginning of a gameplay session.
**Parameters**: `g` (Game ID), `t` (token), `h` (hardcore flag).
**LAHEE Behavior**: 
- Initializes playtime tracking.
- Returns previous unlocks for synchronization.

### `awardachievement`
**Purpose**: Unlocks an achievement.
**Parameters**: `a` (Achievement ID), `t` (token), `h` (hardcore flag), `m` (ROM hash).
**LAHEE Behavior**: 
- Updates local user profile.
- Triggers `LiveTicker` notifications and `CaptureManager` events.

### `ping`
**Purpose**: Periodic heartbeat and presence update.
**Parameters**: `t` (token), `x` (ROM hash), `m` (rich presence string).
**LAHEE Behavior**: 
- Updates "approximate" playtime.
- Records presence history (status messages).

### `submitlbentry`
**Purpose**: Submits a score to a leaderboard.
**Parameters**: `i` (Leaderboard ID), `s` (score), `t` (token), `m` (ROM hash).
**LAHEE Behavior**: Records the entry and returns the user's best score.

## Supplemental Routes

### `latestintegration`
**Purpose**: Checks for RAIntegration updates.
**LAHEE Behavior**: Returns a dummy version (0.0) to prevent unwanted updates.

### `codenotes2`
**Purpose**: Fetches developer notes for memory addresses.
**LAHEE Behavior**: Returns notes from local storage or optionally fetches them from the real RA server if `AutoUpdateCodeNotes` is enabled.

### `hashlibrary`
**Purpose**: Fetches all registered hashes for a console.
**Parameters**: `c` (Console ID).
