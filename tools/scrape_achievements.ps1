# RetroAchievements Offline Scraper for Windows
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

Write-Host "========================================="
Write-Host " RetroAchievements Offline PC Scraper    "
Write-Host "========================================="
Write-Host ""
Write-Host "This script downloads achievement data to your SD card so you can"
Write-Host "play offline on your R36S without a Wi-Fi adapter."
Write-Host ""

$RA_USER = Read-Host "Enter your RetroAchievements Username"
$RA_API_KEY = Read-Host "Enter your Web API Key (from retroachievements.org/settings)"
$SD_PATH = Read-Host "Enter the path to your SD card's ROM partition (e.g. E:\)"

if (-not (Test-Path -Path $SD_PATH)) {
    Write-Error "SD card path not found at $SD_PATH"
    exit
}

$DEST_DIR = Join-Path -Path $SD_PATH -ChildPath "achievements"
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "games") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "badges") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "progress") | Out-Null

Write-Host "Setup complete. Ready to scrape games on $SD_PATH"
Write-Host "Note: Full ROM hashing is complex. This script is a stub for Phase 0."
Write-Host "In a complete implementation, this would scan $SD_PATH\roms,"
Write-Host "hash each file, and call the RA API."

# Example hardcoded fetch for game ID 1234
$GAME_ID = 1234
Write-Host "Fetching sample game $GAME_ID..."
$URL = "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=$RA_USER&y=$RA_API_KEY&g=$GAME_ID"

try {
    $JSON_OUT = Invoke-RestMethod -Uri $URL
    $JsonString = $JSON_OUT | ConvertTo-Json -Depth 10
    $OutFile = Join-Path -Path $DEST_DIR -ChildPath "games\$GAME_ID.json"
    Set-Content -Path $OutFile -Value $JsonString
    Write-Host "Saved $GAME_ID.json"
} catch {
    Write-Host "Failed to fetch data. Check your API key."
}

Write-Host "Scrape complete."
