#!/bin/bash

# Standard PortMaster path discovery
XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source $controlfolder/control.txt

GAMEDIR="/$directory/ports/LAHEE"
cd "$GAMEDIR"

# Use the terminal for output
printf "\033c" >> /dev/tty1
echo "--- LAHEE OFFICIAL FETCHER ---" >> /dev/tty1

# Check if credentials are set
API_KEY=$(grep "WebApiKey" LAHEE.json | cut -d '"' -f 4)
if [ -z "$API_KEY" ]; then
    echo "ERROR: WebApiKey is missing in LAHEE.json!" >> /dev/tty1
    echo "Please add your RetroAchievements API Key to LAHEE.json first." >> /dev/tty1
    sleep 10
    exit 1
fi

echo "Credentials found. Fetching NES classics..." >> /dev/tty1

# List of NES Classic IDs
# 1446: Super Mario Bros
# 1448: Super Mario Bros 3
# 1449: The Legend of Zelda
# 1453: Mega Man 2
# 1450: Castlevania
# 1451: Metroid
# 1452: Contra
GAMES=(1446 1448 1449 1453 1450 1451 1452)

for ID in "${GAMES[@]}"; do
    echo "Fetching Game ID: $ID..." >> /dev/tty1
    # Run LAHEE with the fetch command
    $ESUDO ./LAHEE "fetch $ID" >> /dev/tty1 2>&1
done

echo "" >> /dev/tty1
echo "Fetch Complete!" >> /dev/tty1
echo "Achievements and Hashes are now in your Data folder." >> /dev/tty1
echo "Returning to menu in 10 seconds..." >> /dev/tty1
sleep 10
printf "\033c" >> /dev/tty1
