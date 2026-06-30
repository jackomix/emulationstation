#!/bin/bash
# RetroAchievements Offline Scraper for Linux / macOS
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

echo "========================================="
echo " RetroAchievements Offline PC Scraper    "
echo "========================================="
echo ""

# 1. Download RAHasher if it doesn't exist
if [ ! -f "RAHasher" ]; then
    echo "RAHasher not found. Downloading..."
    
    OS_NAME=$(uname -s)
    if [ "$OS_NAME" = "Linux" ]; then
        curl -sL "https://github.com/LeXofLeviafan/RAHasher/releases/download/1.8.3/RAHasher-x64-Linux-1.8.3.zip" -o rahasher.zip
        unzip -q -o rahasher.zip RAHasher
        chmod +x RAHasher
        rm rahasher.zip
        echo "RAHasher downloaded successfully."
    else
        echo "Error: Pre-compiled RAHasher is only available for Linux and Windows."
        echo "Please compile RAHasher for macOS manually and place the 'RAHasher' executable in this folder."
        exit 1
    fi
fi

read -p "Enter your RetroAchievements Username: " RA_USER
read -p "Enter your Web API Key (from retroachievements.org/settings): " RA_API_KEY
read -p "Enter the path to your SD card's ROM partition (e.g. /media/user/EASYROMS): " SD_PATH

if [ ! -d "$SD_PATH/roms" ]; then
    echo "Error: ROM directory not found at $SD_PATH/roms"
    exit 1
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/progress"

echo "Setup complete. Scanning ROMs..."

# 2. Iterate over systems and ROMs
find "$SD_PATH/roms" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) | while read ROM_FILE; do
    echo "Hashing: $(basename "$ROM_FILE")"
    
    # Run RAHasher to get the hash
    HASH_OUTPUT=$(./RAHasher "$ROM_FILE")
    # RAHasher output looks like:
    # File: /path/to/game.gba
    # Hash: 1234567890abcdef1234567890abcdef
    
    HASH=$(echo "$HASH_OUTPUT" | grep "^Hash:" | awk '{print $2}')
    
    if [ -n "$HASH" ]; then
        # Use RA API to get Game ID from hash
        # Wait, the official API endpoint to get Game ID from hash requires developer permissions if using Web API,
        # but the Web API v1 supports fetching game info directly by hash!
        
        # We can try fetching the game data by hash via the Web API directly.
        # But wait, the standard API requires game ID. 
        # Actually, let's use the API_GetGameInfoAndUserProgress endpoint which might accept hash.
        # Ah, no, the Web API to get GameID from Hash is API_GetGameID.php?i=<hash>
        GAME_ID_JSON=$(curl -s "https://retroachievements.org/API/API_GetGameID.php?z=${RA_USER}&y=${RA_API_KEY}&i=${HASH}")
        
        # Check if the API returned a valid Game ID
        if [[ "$GAME_ID_JSON" =~ \"GameID\":([0-9]+) ]]; then
            GAME_ID="${BASH_REMATCH[1]}"
            
            if [ "$GAME_ID" -ne 0 ]; then
                echo "Found Game ID: $GAME_ID. Fetching achievements..."
                
                # Fetch full game data
                JSON_OUT=$(curl -s "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=${RA_USER}&y=${RA_API_KEY}&g=${GAME_ID}")
                
                if [ -n "$JSON_OUT" ] && [[ "$JSON_OUT" == *"{"* ]]; then
                    echo "$JSON_OUT" > "$DEST_DIR/games/${GAME_ID}.json"
                    echo "Saved data for Game $GAME_ID."
                else
                    echo "Failed to fetch data for Game ID $GAME_ID."
                fi
            else
                echo "No RetroAchievements match for this ROM hash."
            fi
        else
            echo "Failed to lookup hash on RA API."
        fi
    else
        echo "Failed to hash file."
    fi
    echo "---------------------------------"
done

echo "Scraping complete!"
