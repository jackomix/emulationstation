#!/bin/bash
# RetroAchievements Offline Scraper for Linux / macOS
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

# Set our working directory to the location of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Assume the script is inside EASYROMS/tools/
SD_PATH="$SCRIPT_DIR/.."

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
        
        # Determine correct architecture (Mac uses 'arm64' instead of 'aarch64' expected by Makefile)
        MAC_ARCH=$(uname -m)
        if [ "$MAC_ARCH" = "arm64" ]; then
            MAKE_ARCH="arm64"
        else
            MAKE_ARCH="x64"
        fi
        
        # Compile without static libgcc (unsupported on macOS clang)
        make -f Makefile.RAHasher ARCH=$MAKE_ARCH LDFLAGS=""
        
        # 64-bit outputs to bin64 instead of bin
        cp bin64/RAHasher ../
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

if [ ! -d "$SD_PATH/gba" ] && [ ! -d "$SD_PATH/snes" ] && [ ! -d "$SD_PATH/achievements" ]; then
    echo "Warning: It looks like this script isn't located on your SD card."
    echo "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/progress"

echo "Setup complete. Scanning ROMs..."

# 2. Iterate over systems and ROMs (ignoring macOS ._ files and README.md files)
find "$SD_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) \
    ! -name "._*" ! -iname "readme.md" | while read ROM_FILE; do
    echo "Hashing: $(basename "$ROM_FILE")"
    
    # Run RAHasher to get the hash
    HASH_OUTPUT=$(./RAHasher "$ROM_FILE" 2>/dev/null)
    
    HASH=$(echo "$HASH_OUTPUT" | grep "^Hash:" | awk '{print $2}')
    
    if [ -n "$HASH" ]; then
        # Use RA API to get Game ID from hash
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
