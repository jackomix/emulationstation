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
    elif [ "$OS_NAME" = "Darwin" ]; then
        echo "macOS detected. Downloading and compiling RAHasher from source..."
        git clone --recursive --depth 1 https://github.com/LeXofLeviafan/RAHasher.git rahasher_src
        cd rahasher_src
        make -f Makefile.RAHasher
        cp bin/RAHasher ../
        cd ..
        rm -rf rahasher_src
        echo "RAHasher compiled successfully!"
    else
        echo "Error: Pre-compiled RAHasher is only available for Linux and Windows."
        echo "Please compile RAHasher for your OS manually and place the 'RAHasher' executable in this folder."
        exit 1
    fi
fi

read -p "Enter your RetroAchievements Username: " RA_USER
read -p "Enter your Web API Key (from retroachievements.org/settings): " RA_API_KEY

# Assume the script is inside EASYROMS/tools/
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SD_PATH="$SCRIPT_DIR/.."

if [ ! -d "$SD_PATH/gba" ] && [ ! -d "$SD_PATH/snes" ] && [ ! -d "$SD_PATH/achievements" ]; then
    echo "Warning: It looks like this script isn't located on your SD card."
    echo "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/progress"

echo "Setup complete. Scanning ROMs..."

# 2. Iterate over systems and ROMs
find "$SD_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) | while read ROM_FILE; do
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
