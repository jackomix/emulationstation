#!/bin/bash

# LAHEE Native Launcher for EmulationStation
# This script handles environment setup required for the .NET runtime

# 1. Path Setup
SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
ROM_ROOT="$(cd "$SERVER_DIR/../.." && pwd)"
HUB_DIR="$(cd "$SERVER_DIR/.." && pwd)"

export LD_LIBRARY_PATH="$SERVER_DIR:$LD_LIBRARY_PATH"
cd "$SERVER_DIR"

# 2. Host Safety
if ! grep -q "127.0.0.1.nip.io" /etc/hosts; then
    echo "127.0.0.1 127.0.0.1.nip.io" >> /etc/hosts 2>/dev/null
fi

# 3. Launch with passed arguments
# We use nohup and redirect to the Hub for user visibility
chmod +x ./LAHEE
nohup ./LAHEE "$@" > "$HUB_DIR/lahee.log" 2>&1 &

# 4. PID for ES
echo $! > /tmp/lahee.pid
