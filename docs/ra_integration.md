# RetroAchievements Integration in Amber-ES

Amber-ES features a robust integration with RetroAchievements.org, allowing users to view their profile, game achievements, and track progress directly from the EmulationStation UI.

## Code Components

### 1. The Service Logic (es-app/src/RetroAchievements.cpp & .h)
This class is the backend for all RA operations in Amber-ES.
- **Authentication**: RetroAchievements::login() sends credentials to the server and retrieves a session token.
- **Data Retrieval**:
    - getUserSummary(): Fetches the user's overall profile data.
    - getGameAchievements(): Fetches achievement definitions for a specific game.
- **URL Generation**: API URLs are constructed in getApiUrl(), which currently has https://retroachievements.org/API/ hardcoded.

### 2. The User Interface (es-app/src/guis/)
- **GuiRetroAchievements**: Displays the user's profile summary, recent games, and total points.
- **GuiGameAchievements**: Displays a list of achievements for a specific game, indicating which are unlocked (Softcore/Hardcore).
- **GuiRetroAchievementsSettings**: Handles the configuration of the RA username, password, and global toggles (Hardcore mode, etc.).

## Communication Protocol

Amber-ES uses the HttpReq class (found in es-core/src/HttpReq.cpp) to perform standard HTTP GET/POST requests.
- **API Version**: It targets the web API (e.g., GetUserSummary.php).
- **Data Format**: Responses are parsed from JSON using the RapidJSON library.

## Integration Path for LAHEE

To bridge Amber-ES with LAHEE, the following changes are recommended:

### 1. Configurable API Endpoint
Currently, the URL is hardcoded. A new setting RetroAchievementsServer should be added to es_settings.cfg.
- **Default**: https://retroachievements.org
- **LAHEE Mode**: http://127.0.0.1:8000 (or the IP of the device running LAHEE).

### 2. Path Mapping
LAHEE emulates both the dorequest.php endpoint (used by emulators) and the API/ endpoints (used by ES).
- Ensure LAHEE's Network.cs routes match the PHP files ES expects (e.g., GetUserSummary.php).

### 3. Local Badge Serving
Amber-ES fetches badge images from media.retroachievements.org. For offline use, these requests should be redirected to LAHEE's /Badge/ route.
