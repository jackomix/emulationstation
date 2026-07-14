#!/bin/bash
# RetroAchievements Offline Scraper for Linux / macOS
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

# Set our working directory to the location of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Assume the script is inside EASYROMS/tools/
SD_PATH="$SCRIPT_DIR/.."

echo "========================================="
echo " RetroAchievements Offline PC Scraper    "
echo "========================================="
echo ""

# 1. Download/Compile RAHasher if it doesn't exist
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
        echo "macOS detected. Downloading and compiling RAHasher from source in /tmp..."
        rm -rf /tmp/rahasher_src
        git clone --recursive --depth 1 https://github.com/LeXofLeviafan/RAHasher.git /tmp/rahasher_src
        cd /tmp/rahasher_src || exit 1
        
        MAC_ARCH=$(uname -m)
        if [ "$MAC_ARCH" = "arm64" ]; then
            MAKE_ARCH="arm64"
        else
            MAKE_ARCH="x64"
        fi
        
        sed -i '' 's/-static-libgcc//g' Makefile.RAHasher
        sed -i '' 's/-static-libstdc++//g' Makefile.RAHasher
        
        make -f Makefile.RAHasher ARCH=$MAKE_ARCH LDFLAGS=""
        
        if [ -f "bin64/RAHasher" ]; then
            cp bin64/RAHasher "$SCRIPT_DIR/"
            echo "RAHasher compiled successfully!"
        else
            echo "Error: Compilation failed. RAHasher binary not found."
            exit 1
        fi
        rm -rf /tmp/rahasher_src
        cd "$SCRIPT_DIR" || exit 1
    else
        echo "Error: Pre-compiled RAHasher is only available for Linux and Windows."
        echo "Please compile RAHasher for your OS manually and place the 'RAHasher' executable in this folder."
        exit 1
    fi
fi

read -p "Enter your RetroAchievements Username: " RA_USER
read -p "Enter your Web API Key (from retroachievements.org/settings): " RA_API_KEY
read -s -p "Enter your Password (used once to fetch the auth token for patchdata): " RA_PASSWORD
echo ""

echo "Logging in to obtain Token..."
LOGIN_RES=$(curl -s -G --data-urlencode "r=login" --data-urlencode "u=${RA_USER}" --data-urlencode "p=${RA_PASSWORD}" "https://retroachievements.org/dorequest.php")
RA_TOKEN=$(echo "$LOGIN_RES" | grep -o '"Token":"[^"]*"' | cut -d'"' -f4)
if [ -z "$RA_TOKEN" ]; then
    echo "Error: Failed to obtain Token. Please check your username/password."
    exit 1
fi
echo "Token obtained successfully!"

if [ ! -d "$SD_PATH/gba" ] && [ ! -d "$SD_PATH/snes" ] && [ ! -d "$SD_PATH/achievements" ]; then
    echo "Warning: It looks like this script isn't located on your SD card."
    echo "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/images"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/patchdata"

echo "Downloading user summary..."
curl -s "https://retroachievements.org/API/API_GetUserSummary.php?z=${RA_USER}&y=${RA_API_KEY}&u=${RA_USER}&g=100&a=100" > "$DEST_DIR/user.json"

AVATAR_URL=$(grep -o '"UserPic":"[^"]*"' "$DEST_DIR/user.json" | cut -d'"' -f4 | head -n 1)
if [ -n "$AVATAR_URL" ]; then
    echo "Downloading user avatar..."
    curl -sL "https://retroachievements.org${AVATAR_URL}" -o "$DEST_DIR/avatar.png"
fi

echo "{" > "$DEST_DIR/hashes.json"
FIRST_HASH=1

# Graceful exit on Ctrl+C so hashes.json doesn't get corrupted
cleanup_and_exit() {
    echo ""
    echo "Process interrupted! Saving progress and closing JSON safely..."
    echo "}" >> "$DEST_DIR/hashes.json"
    exit 1
}
trap cleanup_and_exit SIGINT

echo "Setup complete. Scanning ROMs..."

# Function to map folder name to RAHasher system key
get_system_key() {
    local folder="$1"
    case "$folder" in
        nes|famicom) echo "NES" ;;
        snes|sfc) echo "SNES" ;;
        gba) echo "GBA" ;;
        gb) echo "GB" ;;
        gbc) echo "GBC" ;;
        n64) echo "N64" ;;
        nds) echo "DS" ;;
        genesis|megadrive) echo "MD" ;;
        mastersystem) echo "SMS" ;;
        gamegear) echo "GG" ;;
        atari2600) echo "2600" ;;
        atari7800) echo "7800" ;;
        atarilynx) echo "Lynx" ;;
        psx) echo "PS1" ;;
        psp) echo "PSP" ;;
        dreamcast) echo "DC" ;;
        saturn) echo "SAT" ;;
        segacd) echo "SCD" ;;
        sega32x) echo "32X" ;;
        pcengine) echo "PCE" ;;
        pcenginecd) echo "PCCD" ;;
        neogeo|arcade|mame|mame2003) echo "ARC" ;;
        neogeocd) echo "NGCD" ;;
        ngp|ngpc) echo "NGP" ;;
        fds) echo "FDS" ;;
        virtualboy) echo "VB" ;;
        wonderswan|wonderswancolor) echo "WSWAN" ;;
        coleco) echo "CV" ;;
        pokemonmini) echo "MINI" ;;
        sg-1000) echo "SG1K" ;;
        3do) echo "3DO" ;;
        gameandwatch) echo "G&W" ;;
        pico) echo "Pico" ;;
        supergrafx) echo "SGFX" ;;
        vectrex) echo "VEC" ;;
        intellivision) echo "INTV" ;;
        *) echo "UNKNOWN" ;;
    esac
}

# Find ROMs and count them, suppressing permission errors
echo "Counting files..."
TOTAL_ROMS=$(find "$SD_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) \
    ! -name "._*" ! -iname "readme.md" 2>/dev/null | wc -l | tr -d ' ')

echo "Found $TOTAL_ROMS ROMs to process."

# Counters
COUNT_PROCESSED=0
COUNT_MATCHED=0
COUNT_SAVED=0
COUNT_NOMATCH=0
COUNT_FAILED=0
COUNT_SKIPPED=0

# Iterate over systems and ROMs
while IFS= read -r ROM_FILE; do
    ((COUNT_PROCESSED++))
    BASENAME=$(basename "$ROM_FILE")
    # Get the parent folder name to determine the system
    PARENT_DIR=$(basename "$(dirname "$ROM_FILE")")
    SYS_KEY=$(get_system_key "$PARENT_DIR")
    
    echo "[$COUNT_PROCESSED/$TOTAL_ROMS] Hashing: $BASENAME"
    
    if [ "$SYS_KEY" = "UNKNOWN" ]; then
        echo "  -> Skipping: Unknown system folder '$PARENT_DIR'"
        ((COUNT_SKIPPED++))
        continue
    fi
    
    # Run RAHasher with the system key
    HASH_OUTPUT=$(./RAHasher "$SYS_KEY" "$ROM_FILE" 2>/dev/null)
    HASH=$(echo "$HASH_OUTPUT" | tr -d '[:space:]')
    
    if [ -n "$HASH" ] && [ ${#HASH} -eq 32 ]; then # Valid MD5 length
        # Use RA API to get Game ID from hash
        GAME_ID_JSON=$(curl -s -A "RetroArch" "https://retroachievements.org/dorequest.php?r=gameid&m=${HASH}")
        
        # Check if the API returned a valid Game ID
        if [[ "$GAME_ID_JSON" =~ \"GameID\":([0-9]+) ]]; then
            GAME_ID="${BASH_REMATCH[1]}"
            
            if [ "$GAME_ID" -ne 0 ]; then
                ((COUNT_MATCHED++))
                
                if [ "$FIRST_HASH" -eq 1 ]; then
                    echo "  \"$HASH\": $GAME_ID" >> "$DEST_DIR/hashes.json"
                    FIRST_HASH=0
                else
                    echo ", \"$HASH\": $GAME_ID" >> "$DEST_DIR/hashes.json"
                fi
                
                # Check if we already have it
                if [ -f "$DEST_DIR/games/${GAME_ID}.json" ]; then
                    echo "  -> Found Game ID: $GAME_ID (Already cached, skipping)"
                    ((COUNT_SAVED++))
                else
                    echo "  -> Found Game ID: $GAME_ID. Fetching achievements..."
                    
                    # Fetch full game data
                    JSON_OUT=$(curl -s "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=${RA_USER}&y=${RA_API_KEY}&g=${GAME_ID}&u=${RA_USER}")
                    
                    if [ -n "$JSON_OUT" ] && [[ "$JSON_OUT" == *"{"* ]]; then
                        echo "$JSON_OUT" > "$DEST_DIR/games/${GAME_ID}.json"
                        echo "  -> Saved data for Game $GAME_ID."
                        ((COUNT_SAVED++))
                        
                        # Parse badges and download them
                        BADGES=$(echo "$JSON_OUT" | grep -o '"BadgeName":"[^"]*"' | cut -d'"' -f4 | sort -u)
                        for badge in $BADGES; do
                            if [ ! -f "$DEST_DIR/badges/${badge}.png" ]; then
                                curl -sL "https://media.retroachievements.org/Badge/${badge}.png" -o "$DEST_DIR/badges/${badge}.png"
                            fi
                            if [ ! -f "$DEST_DIR/badges/${badge}_lock.png" ]; then
                                curl -sL "https://media.retroachievements.org/Badge/${badge}_lock.png" -o "$DEST_DIR/badges/${badge}_lock.png"
                            fi
                        done
                        
                        # Parse game images and download them (BoxArt, Icon, Title)
                        IMAGES=$(echo "$JSON_OUT" | grep -o '"Image[a-zA-Z]*":"[^"]*"' | cut -d'"' -f4 | sort -u | grep "^/Images/")
                        for img in $IMAGES; do
                            IMG_FILENAME=$(basename "$img")
                            if [ ! -f "$DEST_DIR/images/${IMG_FILENAME}" ]; then
                                curl -sL "https://media.retroachievements.org${img}" -o "$DEST_DIR/images/${IMG_FILENAME}"
                            fi
                        done
                        
                        # Download patchdata for rcheevos
                        if [ ! -f "$DEST_DIR/patchdata/${GAME_ID}.json" ]; then
                            curl -s "https://retroachievements.org/dorequest.php?r=patch&u=${RA_USER}&t=${RA_TOKEN}&g=${GAME_ID}" > "$DEST_DIR/patchdata/${GAME_ID}.json"
                        fi
                    else
                        echo "  -> Error: Failed to fetch data for Game ID $GAME_ID."
                    fi
                    
                    # Small sleep to avoid rate limiting
                    sleep 0.2
                fi
            else
                echo "  -> No RetroAchievements match for this ROM."
                ((COUNT_NOMATCH++))
            fi
        else
            echo "  -> Error: Failed to lookup hash on RA API."
            ((COUNT_NOMATCH++))
        fi
    else
        echo "  -> Error: Failed to hash file or unsupported format."
        ((COUNT_FAILED++))
    fi
    
done < <(find "$SD_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) \
    ! -name "._*" ! -iname "readme.md" 2>/dev/null)

echo "}" >> "$DEST_DIR/hashes.json"

echo ""
echo "=========================================="
echo " Scraping Complete!"
echo " ROMs scanned:  $COUNT_PROCESSED"
echo " Matched on RA: $COUNT_MATCHED"
echo " Saved to SD:   $COUNT_SAVED"
echo " No match:      $COUNT_NOMATCH"
echo " Hash failed:   $COUNT_FAILED"
echo " Skipped (dir): $COUNT_SKIPPED"
echo "=========================================="
