#!/bin/bash
# RetroAchievements Offline Scraper for macOS / Linux
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

echo "========================================="
echo " RetroAchievements Offline PC Scraper    "
echo "========================================="
echo ""
echo "This script downloads achievement data to your SD card so you can"
echo "play offline on your R36S without a Wi-Fi adapter."
echo ""

read -p "Enter your RetroAchievements Username: " RA_USER
read -p "Enter your Web API Key (from retroachievements.org/settings): " RA_API_KEY
read -p "Enter the path to your SD card's ROM partition (e.g. /Volumes/EASYROMS): " SD_PATH

if [ ! -d "$SD_PATH" ]; then
    echo "Error: SD card path not found at $SD_PATH"
    exit 1
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/progress"

echo "Setup complete. Ready to scrape games on $SD_PATH"
echo "Note: Full ROM hashing is complex. This script is a stub for Phase 0."
echo "In a complete implementation, this would scan $SD_PATH/roms,"
echo "hash each file, and call the RA API."

# Example hardcoded fetch for game ID 1234
GAME_ID=1234
echo "Fetching sample game $GAME_ID..."
JSON_OUT=$(curl -s "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=${RA_USER}&y=${RA_API_KEY}&g=${GAME_ID}")

if [ -n "$JSON_OUT" ] && [[ "$JSON_OUT" == *"{"* ]]; then
    echo "$JSON_OUT" > "$DEST_DIR/games/${GAME_ID}.json"
    echo "Saved $GAME_ID.json"
else
    echo "Failed to fetch data. Check your API key."
fi

echo "Scrape complete."
