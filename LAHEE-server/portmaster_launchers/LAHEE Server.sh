#!/bin/bash

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

# The program is in the LAHEE subdirectory
GAMEDIR="/$directory/ports/LAHEE"
cd "$GAMEDIR"

# Ensure all files are executable
$ESUDO chmod -R +x .

# Kill existing instance if running
$ESUDO killall -9 LAHEE 2>/dev/null

# Try to run with local library path
export LD_LIBRARY_PATH="$GAMEDIR:$LD_LIBRARY_PATH"

# Ensure custom domain resolves to localhost for surgical patching
if ! grep -q "127.0.0.1.nip.io" /etc/hosts; then
    $ESUDO sh -c "echo '127.0.0.1 127.0.0.1.nip.io' >> /etc/hosts" 2>/dev/null
fi

# Start and keep alive
tail -f /dev/null | $ESUDO ./LAHEE > lahee.log 2> crash.log &

# Brief message on screen
printf "\033c" >> /dev/tty1
echo "LAHEE Server starting..." >> /dev/tty1
sleep 2
if ps aux | grep -v grep | grep -q "./LAHEE"; then
    echo "Server is RUNNING." >> /dev/tty1
else
    echo "ERROR: Server failed to start!" >> /dev/tty1
    echo "Check crash.log for details." >> /dev/tty1
fi
sleep 3
printf "\033c" >> /dev/tty1
