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
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/LAHEE"
cd $GAMEDIR

# Use LOVE engine for UI
LOVE_EXE="love"
if [ -f "/usr/bin/love" ]; then
    LOVE_EXE="/usr/bin/love"
elif [ -f "$controlfolder/runtimes/love/love" ]; then
    LOVE_EXE="$controlfolder/runtimes/love/love"
fi

# Launch with gptokeyb for controller mapping
$GPTOKEYB "$LOVE_EXE" -c "lahee.gptk" &
$ESUDO "$LOVE_EXE" . > lahee_ui.log 2>&1

$ESUDO systemctl restart oga_events &
printf "\033c" >> /dev/tty1
