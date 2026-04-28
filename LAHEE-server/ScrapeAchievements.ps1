# LAHEE Achievement Scraper
# Run this on your PC to fetch achievement data for your ROMs folder

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ServerPath = Join-Path $PSScriptRoot "Server\LAHEE.exe"
# Assume ROMs are in the folder containing the RetroAchievements hub
$RomsPath = (Get-Item $PSScriptRoot).Parent.FullName

if (-not (Test-Path $ServerPath)) {
    Write-Error "Could not find LAHEE server binary at $ServerPath"
    Write-Host "Ensure you have extracted the 'Server' folder correctly."
    pause
    exit
}

Write-Host "==============================================="
Write-Host "          LAHEE Achievement Scraper            "
Write-Host "==============================================="
Write-Host "Server: $ServerPath"
Write-Host "ROMs:   $RomsPath"
Write-Host "-----------------------------------------------"

# Run LAHEE in scrape mode
# We pass --hub to ensure it uses the local Data/Badges folders
& $ServerPath --hub "$PSScriptRoot" scrape "$RomsPath"

Write-Host "-----------------------------------------------"
Write-Host "Scrape complete!"
pause
