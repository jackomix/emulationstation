#!/bin/bash
set -euo pipefail

REPO="jackomix/EmulationStation"
BRANCH="attempt2"
ARTIFACT_NAME="emulationstation-r36s"
LOCAL_ARTIFACT_DIR="/tmp/es-artifact"
DEVICE="ark@192.168.18.20"
SSH_KEY="$HOME/.ssh/id_ed25519_antigravity"
DEVICE_TMP="/tmp/emulationstation"
DEVICE_DEST="/roms/EmulationStation/emulationstation"

SSH_OPTS="-i $SSH_KEY -o StrictHostKeyChecking=no -o ConnectTimeout=5"

echo "=== EmulationStation deploy script ==="

# Get latest commit SHA on the branch
LATEST_SHA=$(git rev-parse HEAD)
SHORT_SHA="${LATEST_SHA:0:7}"
echo "Latest commit on $BRANCH: $SHORT_SHA"

# Check if there's already a build (in-progress, queued, or finished) for this exact commit
echo "Checking for existing build of $SHORT_SHA..."
RUN_ID=$(gh run list \
  --repo "$REPO" \
  --branch "$BRANCH" \
  --commit "$LATEST_SHA" \
  --limit 1 \
  --json databaseId \
  --jq '.[0].databaseId')

if [ -z "$RUN_ID" ] || [ "$RUN_ID" = "null" ]; then
  echo "No build found for $SHORT_SHA. Triggering workflow..."

  gh workflow run build.yml \
    --repo "$REPO" \
    --ref "$BRANCH"

  echo "Waiting 10s for run to appear..."
  sleep 10

  RUN_ID=$(gh run list \
    --repo "$REPO" \
    --branch "$BRANCH" \
    --limit 1 \
    --json databaseId \
    --jq '.[0].databaseId')

  echo "Build triggered. Run ID: $RUN_ID"
else
  echo "Found existing build. Run ID: $RUN_ID"
fi

echo "Waiting for build to complete (typically ~1 min)..."

if ! gh run watch "$RUN_ID" \
  --repo "$REPO" \
  --exit-status \
  --interval 20; then
  echo "❌ Build failed on GitHub Actions! Exiting deploy script."
  exit 1
fi

echo "Build succeeded."

# Download artifact
echo "Downloading artifact..."
rm -rf "$LOCAL_ARTIFACT_DIR"
gh run download "$RUN_ID" \
  --repo "$REPO" \
  --name "$ARTIFACT_NAME" \
  --dir "$LOCAL_ARTIFACT_DIR"

BINARY="$LOCAL_ARTIFACT_DIR/EmulationStation/emulationstation"
if [ ! -f "$BINARY" ]; then
  echo "ERROR: Binary not found at $BINARY"
  echo "Artifact contents:"
  find "$LOCAL_ARTIFACT_DIR"
  exit 1
fi
echo "Binary ready: $BINARY"

# SCP with retry loop
attempt=0
while true; do
  attempt=$((attempt + 1))
  echo "Uploading to device (attempt $attempt)..."

  if scp $SSH_OPTS "$BINARY" "$DEVICE:$DEVICE_TMP"; then
    echo "Upload successful."
    break
  else
    echo "SCP failed. Make sure the device is on and connected. Retrying in 5 seconds..."
    sleep 5
  fi
done

# Deploy on device
echo "Deploying on device..."
ssh $SSH_OPTS "$DEVICE" \
  "mv $DEVICE_TMP $DEVICE_DEST && chmod +x $DEVICE_DEST"

# Restart EmulationStation
echo "Restarting EmulationStation..."
ssh $SSH_OPTS "$DEVICE" "killall emulationstation" || true

echo "=== Done! EmulationStation $SHORT_SHA deployed to R36S ==="