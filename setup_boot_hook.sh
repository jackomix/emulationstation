#!/bin/bash
SYS_WRAPPER="/usr/bin/emulationstation/emulationstation"
BACKUP_WRAPPER="/usr/bin/emulationstation/emulationstation.bak_wrapper"
LOG_FILE="/roms/ports/boot_hook_log.txt"

# Redirect all stdout and stderr to the log file so you can see any OS errors
exec > >(tee -a "$LOG_FILE") 2>&1

echo "=== Installing Bootloader Hook ==="
echo "Target: $SYS_WRAPPER"
echo "Backup: $BACKUP_WRAPPER"

# Remount root filesystem as read-write
echo "Remounting root filesystem as read-write..."
ROOT_DEV=$(findmnt -n -o SOURCE / 2>/dev/null || echo "/dev/root")
echo "Root device detected: $ROOT_DEV"
sudo mount -o remount,rw "$ROOT_DEV" / || sudo mount -o remount,rw /

# 1. Backup the original script/binary if we haven't already
if [ ! -f "$BACKUP_WRAPPER" ]; then
    echo "Creating backup..."
    sudo cp "$SYS_WRAPPER" "$BACKUP_WRAPPER"
else
    echo "Backup already exists."
fi

# 2. Remove the target file link to prevent "Text file busy"
echo "Removing old target to free the path..."
sudo rm -f "$SYS_WRAPPER"

# 3. Write the new hook wrapper script using sudo tee
echo "Writing new wrapper script..."
sudo tee "$SYS_WRAPPER" > /dev/null << 'EOF'
#!/bin/bash

# Hook: If test binary exists inside 'EmulationStation' folder on ROMs partition, run it.
if [ -f /roms/EmulationStation/emulationstation ]; then
    chmod +x /roms/EmulationStation/emulationstation
    /roms/EmulationStation/emulationstation "$@"
    RET_VAL=$?
    
    # Copy any generated log files to the ROMs partition for easy access on macOS
    cp -f /home/ark/.emulationstation/logs/es_log.txt /roms/EmulationStation/es_log.txt 2>/dev/null
    cp -f /home/ark/.emulationstation/es_log.txt /roms/EmulationStation/es_log.txt 2>/dev/null
    cp -f /storage/.emulationstation/logs/es_log.txt /roms/EmulationStation/es_log.txt 2>/dev/null
    
    exit $RET_VAL
fi

# Fallback: Run original system launcher if no custom binary exists
exec /usr/bin/emulationstation/emulationstation.bak_wrapper "$@"
EOF

# 4. Set executable permission on the new wrapper
echo "Setting permissions..."
sudo chmod +x "$SYS_WRAPPER"

# 5. Patch /opt/runemu.sh for retroarch profile support
if [ -f /opt/runemu.sh ]; then
    if ! grep -q "es_profile.cfg" /opt/runemu.sh; then
        echo "Patching /opt/runemu.sh for multi-user profile support..."
        sudo cp /opt/runemu.sh /opt/runemu.sh.bak_profile
        
        # Inject the definition of APPENDCONFIG at the top (after shebang)
        sudo sed -i '2i APPENDCONFIG=""\nif [ -f /tmp/es_profile.cfg ]; then\n    APPENDCONFIG="--appendconfig /tmp/es_profile.cfg"\nfi\n' /opt/runemu.sh
        
        # Insert $APPENDCONFIG into retroarch and retroarch32 calls
        sudo sed -i 's/retroarch /retroarch \$APPENDCONFIG /g' /opt/runemu.sh
        sudo sed -i 's/retroarch32 /retroarch32 \$APPENDCONFIG /g' /opt/runemu.sh
        
        echo "/opt/runemu.sh successfully patched!"
    else
        echo "/opt/runemu.sh already patched."
    fi
else
    echo "/opt/runemu.sh not found (this is normal if not running on the ArkOS console)."
fi

# 6. Flush writes to the disk
sync

echo "Hook successfully installed! Rebooting system in 3 seconds..."
sync

# Reboot the system to apply the changes immediately
sleep 3
sudo reboot
