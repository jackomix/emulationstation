# LAHEE Achievement Scraper
# Run this on your PC to fetch achievement data for your ROMs folder

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ServerDir = Join-Path $PSScriptRoot "Server"
$ServerPath = Join-Path $ServerDir "LAHEE.exe"
$ConfigFile = Join-Path $ServerDir "LAHEE.json"
# Assume ROMs are in the folder containing the RetroAchievements hub
$RomsPath = (Get-Item $PSScriptRoot).Parent.FullName

if (-not (Test-Path $ServerPath)) {
    Write-Error "Could not find LAHEE server binary at $ServerPath"
    Write-Host "Ensure you have extracted the 'Server' folder correctly."
    pause
    exit
}

# --- CREDENTIAL CHECK ---
$needsConfig = $true
if (Test-Path $ConfigFile) {
    $json = Get-Content $ConfigFile | ConvertFrom-Json
    if ($json.LAHEE.RAFetch.WebApiKey -and $json.LAHEE.RAFetch.Username) {
        $needsConfig = $false
    }
}

if ($needsConfig) {
    Write-Host "==============================================="
    Write-Host "      RetroAchievements API Key Required       "
    Write-Host "==============================================="
    Write-Host "LAHEE needs your RA API key to fetch data."
    Write-Host "Get it here: https://retroachievements.org/settings"
    Write-Host ""
    $raUser = Read-Host "RetroAchievements Username"
    $raKey = Read-Host "RetroAchievements Web API Key"
    $raPass = Read-Host "RetroAchievements Password (for image downloads)"
    
    $configTemplate = @{
        LAHEE = @{
            RAFetch = @{
                Url = "https://retroachievements.org"
                Username = $raUser
                WebApiKey = $raKey
                Password = $raPass
            }
        }
    }
    
    $configTemplate | ConvertTo-Json -Depth 10 | Out-File $ConfigFile -Encoding UTF8
    Write-Host "Config saved to $ConfigFile"
    Write-Host "-----------------------------------------------"
}

Write-Host "==============================================="
Write-Host "          LAHEE Achievement Scraper            "
Write-Host "==============================================="
Write-Host "Server: $ServerPath"
Write-Host "ROMs:   $RomsPath"
Write-Host "-----------------------------------------------"

# Run LAHEE in scrape mode
& $ServerPath --hub "$PSScriptRoot" scrape "$RomsPath"

Write-Host "-----------------------------------------------"
Write-Host "Scrape complete!"
pause
