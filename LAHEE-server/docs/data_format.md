# LAHEE Data Formats & Storage

LAHEE uses a simple file-based storage system for game data, user profiles, and assets.

## Directory Structure

### `Data/` (Achievement & Game Data)
Contains the core definitions of games and achievements.
- **`<id>-<name>.set.json`**: Main achievement set file. Contains game metadata, achievement code, descriptions, and point values.
- **`<id>-<name>.hash.txt`**: A simple text file where each line is a valid ROM MD5 hash for that Game ID.
- **`<id>-comments.json`**: Stores user comments fetched from the RA server or created locally.
- **`<id>-z<aid>-<name>.ach.json`**: Individual achievement files (used when adding/modifying single achievements).

### `User/` (User Profiles)
Stores progression and settings for each user.
- **`<username>.json`**: The primary data file for a user.
- **`<username>.bak`**: Automatic backup created before saving.
- **Content**: Tracks achievement status (Locked/Softcore/Hardcore), unlock dates, approximate playtime, and presence history.

### `Badge/` (Assets)
Stores achievement icons.
- **`<badge_id>.png`**: The "unlocked" version of the icon (64x64).
- **`<badge_id>_lock.png`**: The "locked" (grayscale) version of the icon.

## Data Schemas

### Game Data (`.set.json`)
LAHEE supports several versions of the RA JSON format. It internally upgrades legacy V1 data to the modern `GameData` structure.
Key fields:
- `ID`: RetroAchievements Game ID.
- `Title`: Name of the game.
- `ConsoleID`: Platform ID (e.g., 1 for NES, 2 for SNES).
- `ROMHashes`: List of associated MD5 hashes.
- `AchievementSets`: Lists of achievement objects grouped by set type (Core, Subset, Unofficial).

### ROM Hashes (`.hash.txt`)
Files are parsed line-by-line. Empty lines and comments are ignored. Multiple `.hash.txt` files for the same Game ID are merged during initialization.

## Filename Conventions
LAHEE relies on filenames to determine the Game ID:
- Format: `[GAMEID]-[ANY_LABEL].[EXTENSION]`
- Example: `3885-FFCC.set.json` -> Parsed as Game ID 3885.
- This allows users to easily organize files and merge sets by giving them the same ID prefix.
