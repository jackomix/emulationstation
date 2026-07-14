#!/bin/bash
# RetroAchievements Offline Scraper for Linux / macOS
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

# Set our working directory to the location of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Assume the script is inside EASYROMS/tools/
SD_PATH="$SCRIPT_DIR/.."

# ANSI colors for styling
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

show_header() {
    clear
    echo -e "${CYAN}====================================================${NC}"
    echo -e "${BOLD}${GREEN}        RetroAchievements Offline Scraper           ${NC}"
    echo -e "${CYAN}====================================================${NC}"
    echo ""
}

# 1. Download/Compile RAHasher if it doesn't exist
if [ ! -f "RAHasher" ]; then
    show_header
    echo -e "${YELLOW}RAHasher not found. Setting up...${NC}"
    
    OS_NAME=$(uname -s)
    if [ "$OS_NAME" = "Linux" ]; then
        echo -e "${BLUE}Downloading pre-compiled RAHasher for Linux...${NC}"
        curl -sL "https://github.com/LeXofLeviafan/RAHasher/releases/download/1.8.3/RAHasher-x64-Linux-1.8.3.zip" -o rahasher.zip
        unzip -q -o rahasher.zip RAHasher
        chmod +x RAHasher
        rm rahasher.zip
        echo -e "${GREEN}RAHasher downloaded successfully.${NC}"
        sleep 1
    elif [ "$OS_NAME" = "Darwin" ]; then
        echo -e "${BLUE}macOS detected. Downloading and compiling RAHasher from source...${NC}"
        COMPILE_LOG="/tmp/rahasher_compile.log"
        echo -e "Compilation logs will be saved to: ${MAGENTA}$COMPILE_LOG${NC}"
        rm -rf /tmp/rahasher_src
        
        echo -ne "Cloning repository... "
        if git clone --recursive --depth 1 https://github.com/LeXofLeviafan/RAHasher.git /tmp/rahasher_src > "$COMPILE_LOG" 2>&1; then
            echo -e "${GREEN}Done.${NC}"
        else
            echo -e "${RED}Failed. See $COMPILE_LOG${NC}"
            exit 1
        fi
        
        cd /tmp/rahasher_src || exit 1
        
        MAC_ARCH=$(uname -m)
        if [ "$MAC_ARCH" = "arm64" ]; then
            MAKE_ARCH="arm64"
        else
            MAKE_ARCH="x64"
        fi
        
        sed -i '' 's/-static-libgcc//g' Makefile.RAHasher >> "$COMPILE_LOG" 2>&1
        sed -i '' 's/-static-libstdc++//g' Makefile.RAHasher >> "$COMPILE_LOG" 2>&1
        
        echo -ne "Compiling RAHasher (this may take a minute)... "
        if make -f Makefile.RAHasher ARCH=$MAKE_ARCH LDFLAGS="" >> "$COMPILE_LOG" 2>&1; then
            echo -e "${GREEN}Done.${NC}"
        else
            echo -e "${RED}Failed. See $COMPILE_LOG${NC}"
            exit 1
        fi
        
        if [ -f "bin64/RAHasher" ]; then
            cp bin64/RAHasher "$SCRIPT_DIR/"
            rm -rf /tmp/rahasher_src
            rm -f "$COMPILE_LOG"
            echo -e "${GREEN}RAHasher compiled successfully!${NC}"
            sleep 1
        else
            echo -e "${RED}Error: Compiled binary not found. See $COMPILE_LOG${NC}"
            exit 1
        fi
        cd "$SCRIPT_DIR" || exit 1
    else
        echo -e "${RED}Error: Pre-compiled RAHasher is only available for Linux/macOS.${NC}"
        echo "Please compile RAHasher for your OS manually and place the 'RAHasher' executable in this folder."
        exit 1
    fi
fi

# Load credentials if they exist
LOADED_SAVED=0
CRED_FILE="$SCRIPT_DIR/credentials.txt"
if [ -f "$CRED_FILE" ]; then
    source "$CRED_FILE" 2>/dev/null
    if [ -n "$RA_USER" ] && [ -n "$RA_API_KEY" ]; then
        LOADED_SAVED=1
    fi
fi

show_header

# If saved credentials found, ask to use them
if [ $LOADED_SAVED -eq 1 ]; then
    echo -e "${GREEN}Found saved credentials for RetroAchievements user: ${BOLD}$RA_USER${NC}"
    read -p "Use these credentials? [Y/n]: " use_saved
    if [[ "$use_saved" =~ ^[nN] ]]; then
        LOADED_SAVED=0
    fi
fi

if [ $LOADED_SAVED -eq 0 ]; then
    echo -e "${YELLOW}Please enter your RetroAchievements credentials:${NC}"
    read -p "Username: " RA_USER
    read -p "Web API Key (from retroachievements.org/settings): " RA_API_KEY
    read -s -p "Password (used once to fetch auth token): " RA_PASSWORD
    echo ""
fi

echo -e "\n${BLUE}Logging in to obtain API Token...${NC}"
LOGIN_RES=$(curl -s -G --data-urlencode "r=login" --data-urlencode "u=${RA_USER}" --data-urlencode "p=${RA_PASSWORD}" "https://retroachievements.org/dorequest.php")
RA_TOKEN=$(echo "$LOGIN_RES" | grep -o '"Token":"[^"]*"' | cut -d'"' -f4)

if [ -z "$RA_TOKEN" ]; then
    echo -e "${RED}Error: Failed to obtain Token. Please check your username/password and API key.${NC}"
    exit 1
fi
echo -e "${GREEN}Token obtained successfully!${NC}"

# Ask to save credentials if not using saved ones
if [ $LOADED_SAVED -eq 0 ]; then
    echo ""
    echo -e "${YELLOW}Would you like to save these credentials to credentials.txt for future convenience?${NC}"
    echo -e "${RED}WARNING: The credentials (including password) will be stored in plain text.${NC}"
    read -p "Save credentials? [y/N]: " save_choice
    if [[ "$save_choice" =~ ^[yY] ]]; then
        printf 'RA_USER=%q\n' "$RA_USER" > "$CRED_FILE"
        printf 'RA_API_KEY=%q\n' "$RA_API_KEY" >> "$CRED_FILE"
        printf 'RA_PASSWORD=%q\n' "$RA_PASSWORD" >> "$CRED_FILE"
        chmod 600 "$CRED_FILE"
        echo -e "${GREEN}Credentials saved to $CRED_FILE (permissions restricted to owner).${NC}"
    fi
fi

sleep 1

# Setup paths and menu
if [ ! -d "$SD_PATH/gba" ] && [ ! -d "$SD_PATH/snes" ] && [ ! -d "$SD_PATH/achievements" ]; then
    show_header
    echo -e "${YELLOW}Warning: It looks like this script isn't located on your SD card.${NC}"
    echo "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
    read -p "Press Enter to continue anyway..."
fi

DEST_DIR="$SD_PATH/achievements"
mkdir -p "$DEST_DIR/games"
mkdir -p "$DEST_DIR/images"
mkdir -p "$DEST_DIR/badges"
mkdir -p "$DEST_DIR/patchdata"

# Download user summary & avatar
show_header
echo -e "${BLUE}Downloading user summary...${NC}"
curl -s "https://retroachievements.org/API/API_GetUserSummary.php?z=${RA_USER}&y=${RA_API_KEY}&u=${RA_USER}&g=100&a=100" > "$DEST_DIR/user.json"

AVATAR_URL=$(grep -o '"UserPic":"[^"]*"' "$DEST_DIR/user.json" | cut -d'"' -f4 | head -n 1)
if [ -n "$AVATAR_URL" ]; then
    echo -e "${BLUE}Downloading user avatar...${NC}"
    curl -sL "https://retroachievements.org${AVATAR_URL}" -o "$DEST_DIR/avatar.png"
fi

# Detect available system directories
SYSTEM_LIST=("nes" "famicom" "snes" "sfc" "gba" "gb" "gbc" "n64" "nds" "genesis" "megadrive" "mastersystem" "gamegear" "atari2600" "atari7800" "atarilynx" "psx" "psp" "dreamcast" "saturn" "segacd" "sega32x" "pcengine" "pcenginecd" "neogeo" "arcade" "mame" "mame2003" "neogeocd" "ngp" "ngpc" "fds" "virtualboy" "wonderswan" "wonderswancolor" "coleco" "pokemonmini" "sg-1000" "3do" "gameandwatch" "pico" "supergrafx" "vectrex" "intellivision")

AVAILABLE_SYSTEMS=()
for sys in "${SYSTEM_LIST[@]}"; do
    if [ -d "$SD_PATH/$sys" ]; then
        AVAILABLE_SYSTEMS+=("$sys")
    fi
done

# Main menu
SELECTED_SYSTEM=""
while true; do
    show_header
    echo -e "${BOLD}Select scrape scope:${NC}"
    echo "  1) Scrape ALL systems"
    echo "  2) Scrape a SPECIFIC system"
    if [ $LOADED_SAVED -eq 1 ]; then
        echo "  3) Clear saved credentials"
        echo "  4) Exit"
    else
        echo "  3) Exit"
    fi
    echo ""
    read -p "Enter choice [1]: " main_choice
    main_choice=${main_choice:-1}
    
    if [ "$main_choice" = "1" ]; then
        break
    elif [ "$main_choice" = "2" ]; then
        if [ ${#AVAILABLE_SYSTEMS[@]} -eq 0 ]; then
            echo -e "${RED}No matching system directories found in $SD_PATH!${NC}"
            read -p "Press Enter to go back..."
            continue
        fi
        
        while true; do
            show_header
            echo -e "${BOLD}Select a system:${NC}"
            for i in "${!AVAILABLE_SYSTEMS[@]}"; do
                echo "  $((i+1))) ${AVAILABLE_SYSTEMS[$i]}"
            done
            echo "  $(( ${#AVAILABLE_SYSTEMS[@]} + 1 ))) Back to main menu"
            echo ""
            read -p "Enter choice: " sys_choice
            
            if [[ "$sys_choice" =~ ^[0-9]+$ ]]; then
                if [ "$sys_choice" -eq "$(( ${#AVAILABLE_SYSTEMS[@]} + 1 ))" ]; then
                    break
                elif [ "$sys_choice" -gt 0 ] && [ "$sys_choice" -le "${#AVAILABLE_SYSTEMS[@]}" ]; then
                    SELECTED_SYSTEM="${AVAILABLE_SYSTEMS[$((sys_choice-1))]}"
                    break 2
                fi
            fi
        done
    elif [ "$main_choice" = "3" ]; then
        if [ $LOADED_SAVED -eq 1 ]; then
            rm -f "$CRED_FILE"
            LOADED_SAVED=0
            echo -e "${GREEN}Credentials cleared.${NC}"
            sleep 1
        else
            exit 0
        fi
    elif [ "$main_choice" = "4" ] && [ $LOADED_SAVED -eq 1 ]; then
        exit 0
    fi
done

# Scan path based on selection
if [ -n "$SELECTED_SYSTEM" ]; then
    SCAN_PATH="$SD_PATH/$SELECTED_SYSTEM"
    echo -e "${BLUE}Scanning only: $SELECTED_SYSTEM ($SCAN_PATH)...${NC}"
else
    SCAN_PATH="$SD_PATH"
    echo -e "${BLUE}Scanning all systems...${NC}"
fi

# Count files to process
TOTAL_ROMS=$(find "$SCAN_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) \
    ! -name "._*" ! -iname "readme.md" 2>/dev/null | wc -l | tr -d ' ')

if [ "$TOTAL_ROMS" -eq 0 ]; then
    echo -e "${RED}No supported ROMs found to process!${NC}"
    exit 0
fi

# Write hashes.json header safely
echo "{" > "$DEST_DIR/hashes.json"
FIRST_HASH=1
HASHES_CLOSED=0

# Counters
COUNT_PROCESSED=0
COUNT_MATCHED=0
COUNT_SAVED=0
COUNT_NOMATCH=0
COUNT_FAILED=0
COUNT_SKIPPED=0

# Progress drawing helpers
draw_bar() {
    local current="$1"
    local total="$2"
    local width=35
    if [ "$total" -eq 0 ]; then
        percent=0
        filled=0
    else
        percent=$((current * 100 / total))
        filled=$((current * width / total))
    fi
    local empty=$((width - filled))
    
    local bar=""
    for ((k=0; k<filled; k++)); do bar="${bar}="; done
    if [ $filled -lt $width ] && [ $current -gt 0 ]; then
        bar="${bar:0:-1}>"
    fi
    for ((k=0; k<empty; k++)); do bar="${bar} "; done
    
    echo -n "[$bar] $percent% ($current/$total)"
}

FIRST_DRAW=1
CURRENT_GAME_NAME=""
TOTAL_ASSETS=0
CURR_ASSET=0

draw_status() {
    # Move cursor up 3 lines to overwrite previous status
    if [ "$FIRST_DRAW" -eq 0 ]; then
        echo -ne "\033[3A\033[K"
    else
        FIRST_DRAW=0
    fi
    
    # Line 1: ROMs overall progress bar
    echo -ne "${BOLD}ROMs Progress:${NC}   "
    draw_bar "$COUNT_PROCESSED" "$TOTAL_ROMS"
    echo -ne "\n\033[K"
    
    # Line 2: Current game details
    if [ -n "$CURRENT_GAME_NAME" ]; then
        local display_name="${CURRENT_GAME_NAME:0:45}"
        echo -ne "${BLUE}Current Game:${NC}    $display_name\n\033[K"
    else
        echo -ne "${BLUE}Current Game:${NC}    Scanning...\n\033[K"
    fi
    
    # Line 3: Current game achievements/assets progress bar
    echo -ne "${YELLOW}Achievements:${NC}    "
    if [ "$TOTAL_ASSETS" -gt 0 ]; then
        draw_bar "$CURR_ASSET" "$TOTAL_ASSETS"
    else
        echo -ne "[-----------------------------------] 0% (0/0)"
    fi
    echo -ne "\n"
}

save_and_exit() {
    if [ "$HASHES_CLOSED" -eq 0 ]; then
        echo "}" >> "$DEST_DIR/hashes.json"
        HASHES_CLOSED=1
    fi
    
    # Clear progress bar footprint
    if [ "$FIRST_DRAW" -eq 0 ]; then
        echo -ne "\033[3A\033[K\033[B\033[K\033[B\033[K\033[2A"
    fi
    
    echo ""
    echo -e "${YELLOW}====================================================${NC}"
    echo -e "${BOLD}${RED}             Scraping Stopped Safely                ${NC}"
    echo -e "${YELLOW}====================================================${NC}"
    echo -e "  ROMs scanned:      $COUNT_PROCESSED"
    echo -e "  Matched on RA:     $COUNT_MATCHED"
    echo -e "  Saved / Updated:   $COUNT_SAVED"
    echo -e "  No match:          $COUNT_NOMATCH"
    echo -e "  Hash failed:       $COUNT_FAILED"
    echo -e "  Skipped:           $COUNT_SKIPPED"
    echo -e "${YELLOW}====================================================${NC}"
    exit 0
}

SKIP_CURRENT=0

trap_ctrl_c() {
    # Reset trap momentarily
    trap - SIGINT
    
    # Move down to print menu below the progress bars
    echo -e "\n"
    echo -e "${YELLOW}====================================================${NC}"
    echo -e "${BOLD}${RED}                  SCRAPER PAUSED                    ${NC}"
    echo -e "${YELLOW}====================================================${NC}"
    echo -e "  [c] Continue scraping"
    echo -e "  [s] Skip current game / ROM"
    echo -e "  [q] Save current progress and Quit"
    echo -e "${YELLOW}====================================================${NC}"
    read -r -p "Select option (c/s/q) [c]: " choice
    
    # Clear the menu we just printed (8 lines)
    echo -ne "\033[8A\033[K\033[B\033[K\033[B\033[K\033[B\033[K\033[B\033[K\033[B\033[K\033[B\033[K\033[B\033[K\033[8A"
    
    case "$choice" in
        [sS])
            SKIP_CURRENT=1
            trap trap_ctrl_c SIGINT
            ;;
        [qQ])
            save_and_exit
            ;;
        *)
            # Reset first draw so it draws cleanly
            FIRST_DRAW=1
            trap trap_ctrl_c SIGINT
            ;;
    esac
}

# Trap SIGINT (Ctrl+C)
trap trap_ctrl_c SIGINT

# System mapping key function
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

show_header
echo -e "${GREEN}Scanning systems and ROMs...${NC}"
echo ""

# Start scraping loop
while IFS= read -r ROM_FILE; do
    SKIP_CURRENT=0
    ((COUNT_PROCESSED++))
    
    BASENAME=$(basename "$ROM_FILE")
    PARENT_DIR=$(basename "$(dirname "$ROM_FILE")")
    SYS_KEY=$(get_system_key "$PARENT_DIR")
    
    CURRENT_GAME_NAME="$BASENAME"
    TOTAL_ASSETS=0
    CURR_ASSET=0
    draw_status
    
    if [ "$SYS_KEY" = "UNKNOWN" ]; then
        ((COUNT_SKIPPED++))
        continue
    fi
    
    # Hash the file
    HASH_OUTPUT=$(./RAHasher "$SYS_KEY" "$ROM_FILE" 2>/dev/null)
    HASH=$(echo "$HASH_OUTPUT" | tr -d '[:space:]')
    
    if [ -n "$HASH" ] && [ ${#HASH} -eq 32 ]; then
        GAME_ID_JSON=$(curl -s -A "RetroArch" "https://retroachievements.org/dorequest.php?r=gameid&m=${HASH}")
        
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
                
                # Fetch full game data or use cached JSON to identify assets
                if [ -f "$DEST_DIR/games/${GAME_ID}.json" ]; then
                    JSON_OUT=$(cat "$DEST_DIR/games/${GAME_ID}.json")
                    ((COUNT_SAVED++))
                else
                    JSON_OUT=$(curl -s "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=${RA_USER}&y=${RA_API_KEY}&g=${GAME_ID}&u=${RA_USER}")
                fi
                
                if [ -n "$JSON_OUT" ] && [[ "$JSON_OUT" == *"{"* ]]; then
                    if [ ! -f "$DEST_DIR/games/${GAME_ID}.json" ]; then
                        echo "$JSON_OUT" > "$DEST_DIR/games/${GAME_ID}.json"
                        ((COUNT_SAVED++))
                    fi
                    
                    # Extract title/name of the game to show in the UI safely
                    if command -v jq >/dev/null 2>&1; then
                        TITLE_MATCH=$(echo "$JSON_OUT" | jq -r '.Title // empty' 2>/dev/null)
                    elif command -v python3 >/dev/null 2>&1; then
                        TITLE_MATCH=$(echo "$JSON_OUT" | python3 -c 'import sys, json; print(json.load(sys.stdin).get("Title", ""))' 2>/dev/null)
                    else
                        TITLE_MATCH=$(echo "$JSON_OUT" | grep -o '"Title":"[^"]*"' | cut -d'"' -f4 | head -n 1)
                    fi
                    
                    if [ -n "$TITLE_MATCH" ]; then
                        CURRENT_GAME_NAME="$TITLE_MATCH"
                    fi
                    
                    # Parse badges and images
                    BADGES=$(echo "$JSON_OUT" | grep -o '"BadgeName":"[^"]*"' | cut -d'"' -f4 | sort -u)
                    IMAGES=$(echo "$JSON_OUT" | grep -o '"Image[a-zA-Z]*":"[^"]*"' | cut -d'"' -f4 | sort -u | grep "^/Images/")
                    
                    declare -a ASSET_URLS=()
                    declare -a ASSET_PATHS=()
                    TOTAL_ASSETS=0
                    
                    for badge in $BADGES; do
                        if [ ! -f "$DEST_DIR/badges/${badge}.png" ]; then
                            ASSET_URLS+=("https://media.retroachievements.org/Badge/${badge}.png")
                            ASSET_PATHS+=("$DEST_DIR/badges/${badge}.png")
                            ((TOTAL_ASSETS++))
                        fi
                        if [ ! -f "$DEST_DIR/badges/${badge}_lock.png" ]; then
                            ASSET_URLS+=("https://media.retroachievements.org/Badge/${badge}_lock.png")
                            ASSET_PATHS+=("$DEST_DIR/badges/${badge}_lock.png")
                            ((TOTAL_ASSETS++))
                        fi
                    done
                    
                    for img in $IMAGES; do
                        IMG_FILENAME=$(basename "$img")
                        if [ ! -f "$DEST_DIR/images/${IMG_FILENAME}" ]; then
                            ASSET_URLS+=("https://media.retroachievements.org${img}")
                            ASSET_PATHS+=("$DEST_DIR/images/${IMG_FILENAME}")
                            ((TOTAL_ASSETS++))
                        fi
                    done
                    
                    if [ ! -f "$DEST_DIR/patchdata/${GAME_ID}.json" ]; then
                        ASSET_URLS+=("https://retroachievements.org/dorequest.php?r=patch&u=${RA_USER}&t=${RA_TOKEN}&g=${GAME_ID}")
                        ASSET_PATHS+=("$DEST_DIR/patchdata/${GAME_ID}.json")
                        ((TOTAL_ASSETS++))
                    fi
                    
                    # Download files with progress
                    CURR_ASSET=0
                    draw_status
                    for ((i=0; i<TOTAL_ASSETS; i++)); do
                        if [ "$SKIP_CURRENT" -eq 1 ]; then
                            break
                        fi
                        curl -sL "${ASSET_URLS[$i]}" -o "${ASSET_PATHS[$i]}"
                        ((CURR_ASSET++))
                        draw_status
                    done
                    
                    # Small sleep to avoid rate limiting
                    sleep 0.2
                else
                    # Error parsing or fetching
                    TOTAL_ASSETS=0
                    CURR_ASSET=0
                    draw_status
                fi
            else
                ((COUNT_NOMATCH++))
            fi
        else
            ((COUNT_NOMATCH++))
        fi
    else
        ((COUNT_FAILED++))
    fi
    
done < <(find "$SCAN_PATH" -type f \( -iname \*.gba -o -iname \*.zip -o -iname \*.sfc -o -iname \*.nes -o -iname \*.md -o -iname \*.z64 -o -iname \*.cue -o -iname \*.chd \) \
    ! -name "._*" ! -iname "readme.md" 2>/dev/null)

# Close JSON safely
echo "}" >> "$DEST_DIR/hashes.json"
HASHES_CLOSED=1

# Clear progress bars and print final stats
if [ "$FIRST_DRAW" -eq 0 ]; then
    echo -ne "\033[3A\033[K\033[B\033[K\033[B\033[K\033[2A"
fi

echo ""
echo -e "${CYAN}====================================================${NC}"
echo -e "${BOLD}${GREEN}               Scraping Complete!                   ${NC}"
echo -e "${CYAN}====================================================${NC}"
echo -e "  ROMs scanned:      $COUNT_PROCESSED"
echo -e "  Matched on RA:     $COUNT_MATCHED"
echo -e "  Saved to SD:       $COUNT_SAVED"
echo -e "  No match:          $COUNT_NOMATCH"
echo -e "  Hash failed:       $COUNT_FAILED"
echo -e "  Skipped:           $COUNT_SKIPPED"
echo -e "${CYAN}====================================================${NC}"
