# Plan: LAHEE Technical Documentation

## Objective
Create a comprehensive set of technical documents to allow future developers and AIs to understand, maintain, and extend LAHEE.

## Proposed Documents

### 1. `docs/architecture.md` (High-Level Design)
- **Overview**: Purpose and goals of LAHEE.
- **Component Breakdown**:
    - `Network`: HTTP server and routing.
    - `StaticDataManager`: Achievement and Game ID indexing.
    - `UserManager`: Local profile and session management.
    - `LiveTicker`: WebSocket event broadcasting.
    - `CaptureManager`: Screenshot and OBS integration.
- **Data Flow**: Lifecycle of an achievement session.

### 2. `docs/api_protocol.md` (Emulated RA Protocol)
- **Endpoint**: `dorequest.php`
- **Mapped Routes**:
    - `login`/`login2`: Authentication.
    - `gameid`: ROM hash to Game ID resolution.
    - `patch`/`achievementsets`: Fetching achievement definitions.
    - `startsession`: Beginning a gameplay session.
    - `awardachievement`: Unlocking achievements.
    - `ping`: Presence updates and playtime tracking.
    - `submitlbentry`: Leaderboard submissions.
- **Response Formats**: Brief overview of the JSON structures used.

### 3. `docs/data_format.md` (File System & Schema)
- **Directory Structure**:
    - `Data/`: Achievement sets (`.set.json`), hashes (`.hash.txt`), and comments.
    - `User/`: Local user profiles.
    - `Badge/`: Achievement icons.
- **Naming Conventions**: How filenames impact Game ID and set merging.
- **Surgical Folder Method**: Explanation of why `/laheer/` is used and how the server handles path padding (slashes/dots).

### 4. `docs/patching_mechanism.md` (Emulator Integration)
- **Binary Patching**: Detailed explanation of why hex editing is required.
- **Surgical Folder Strategy**:
    - Length matching requirements for string replacement.
    - Patterns used for domain replacement (e.g., `retroachievements.org` -> `127.0.0.1:8000/laheer`).
    - Protocol correction (`https` -> `http`).

## Implementation Steps
1.  **Draft Architecture**: Detailed breakdown of components.
2.  **Draft Protocol**: Map all internal routes to RA equivalents.
3.  **Draft Data/Patching**: Document the file system and binary patching logic.
4.  **Review**: Ensure all documentation is accurate to the current codebase.

## Verification
- Cross-reference documentation with `Src/Network.cs` and `lahee_patch_ra.py`.
- Verify that file paths mentioned in `docs/data_format.md` match `StaticDataManager.cs`.
