#!/bin/bash

# LAHEE Native Launcher for EmulationStation
# This script handles environment setup required for the .NET runtime

# 1. Path Setup
SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
ROM_ROOT="$(cd "$SERVER_DIR/../.." && pwd)"
HUB_DIR="$(cd "$SERVER_DIR/.." && pwd)"

export LD_LIBRARY_PATH="$SERVER_DIR:$LD_LIBRARY_PATH"
cd "$SERVER_DIR"

# 3. Launch with passed arguments
# We use tail -f /dev/null to keep stdin open so LAHEE doesn't exit immediately
chmod +x ./LAHEE
tail -f /dev/null | nohup ./LAHEE "$@" > "$HUB_DIR/lahee.log" 2>&1 &

# 3. PID for ES
echo $! > /tmp/lahee.pid
