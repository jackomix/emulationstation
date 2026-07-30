#!/bin/bash
LOG_FILE="/roms/ports/filesystem_structure.txt"

# Redirect stdout and stderr to the log file
exec > "$LOG_FILE" 2>&1

echo "=== System Info ==="
uname -a
echo ""

echo "=== Mounted Filesystems ==="
df -h
echo ""
mount
echo ""

echo "=== Network Config / Hostname ==="
hostname -I
echo ""

echo "=== Common Executables Search ==="
echo "retroarch location: $(which retroarch 2>/dev/null || find / -name "retroarch" -type f 2>/dev/null)"
echo "emulationstation binaries: $(find /usr/bin/ /usr/local/bin/ -name "*emulationstation*" 2>/dev/null)"
echo ""

echo "=== File System Directory Structure ==="
# Crawl root partition, excluding virtual, temp, and roms partitions
find / -mindepth 1 \
  -path "/proc" -prune -o \
  -path "/sys" -prune -o \
  -path "/dev" -prune -o \
  -path "/run" -prune -o \
  -path "/tmp" -prune -o \
  -path "/roms" -prune -o \
  -path "/roms2" -prune -o \
  -path "/storage" -prune -o \
  -path "/userdata" -prune -o \
  -path "/var/lib/flatpak" -prune -o \
  -path "/var/lib/docker" -prune -o \
  -print 2>/dev/null
