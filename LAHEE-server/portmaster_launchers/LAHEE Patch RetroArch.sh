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

GAMEDIR="/$directory/ports/LAHEE"
cd $GAMEDIR

printf "\033c" >> /dev/tty1
echo "Patching RetroArch..." >> /dev/tty1
$ESUDO python3 lahee_patch_ra.py >> /dev/tty1 2>&1
sleep 3
printf "\033c" >> /dev/tty1
