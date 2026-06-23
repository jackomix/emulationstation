#!/bin/bash

# Detect profiles directory
if [ -d "/roms/profiles" ]; then
	PROFILES_DIR="/roms/profiles"
else
	PROFILES_DIR="${HOME}/.emulationstation/profiles"
fi

XML_FILE="${PROFILES_DIR}/profiles.xml"
if [ ! -f "$XML_FILE" ]; then
	echo "profiles.xml not found! Make sure profiles are enabled and configured."
	sleep 3
	exit 1
fi

ACTIVE_PROFILE=$(grep -oE 'active="[^"]+"' "$XML_FILE" | head -n 1 | cut -d'"' -f2)

if [ -z "$ACTIVE_PROFILE" ]; then
	echo "No active profile found in profiles.xml."
	sleep 3
	exit 1
fi

echo "Active Profile detected: $ACTIVE_PROFILE"
echo "Creating backup for profile '$ACTIVE_PROFILE'..."

BACKUP_FILE="${PROFILES_DIR}/${ACTIVE_PROFILE}_backup_$(date +%Y%m%d_%H%M%S).tar.gz"

tar -czf "$BACKUP_FILE" -C "$PROFILES_DIR" "$ACTIVE_PROFILE"

if [ $? -eq 0 ]; then
	echo "Backup successfully created at:"
	echo "$BACKUP_FILE"
else
	echo "Failed to create backup!"
fi

sleep 4
