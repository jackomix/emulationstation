# RetroAchievements Offline Scraper for Windows
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

Write-Host "========================================="
Write-Host " RetroAchievements Offline PC Scraper    "
Write-Host "========================================="
Write-Host ""

# 1. Download RAHasher if it doesn't exist
$HasherPath = Join-Path -Path $PSScriptRoot -ChildPath "RAHasher.exe"

if (-not (Test-Path -Path $HasherPath)) {
    Write-Host "RAHasher.exe not found. Downloading..."
    $ZipPath = Join-Path -Path $PSScriptRoot -ChildPath "rahasher.zip"
    $Url = "https://github.com/LeXofLeviafan/RAHasher/releases/download/1.8.3/RAHasher-x64-Windows-1.8.3.zip"
    
    try {
        Invoke-WebRequest -Uri $Url -OutFile $ZipPath
        Expand-Archive -Path $ZipPath -DestinationPath $PSScriptRoot -Force
        Remove-Item -Path $ZipPath
        Write-Host "RAHasher downloaded successfully."
    } catch {
        Write-Error "Failed to download RAHasher automatically. Please download it from https://github.com/LeXofLeviafan/RAHasher/releases and place RAHasher.exe in this folder."
        exit
    }
}

$RA_USER = Read-Host "Enter your RetroAchievements Username"
$RA_API_KEY = Read-Host "Enter your Web API Key (from retroachievements.org/settings)"

# Assume the script is inside EASYROMS/tools/
$SD_PATH = Split-Path -Path $PSScriptRoot -Parent

if (-not (Test-Path -Path (Join-Path $SD_PATH "gba")) -and -not (Test-Path -Path (Join-Path $SD_PATH "snes")) -and -not (Test-Path -Path (Join-Path $SD_PATH "achievements"))) {
    Write-Host "Warning: It looks like this script isn't located on your SD card."
    Write-Host "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
}

$DEST_DIR = Join-Path -Path $SD_PATH -ChildPath "achievements"
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "games") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "badges") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "progress") | Out-Null

Write-Host "Setup complete. Scanning ROMs..."

# 2. Iterate over systems and ROMs
$Extensions = @("*.gba", "*.zip", "*.sfc", "*.nes", "*.md", "*.z64", "*.cue", "*.chd")
$RomFiles = Get-ChildItem -Path $SD_PATH -Include $Extensions -Recurse -File | Where-Object { $_.Name -notmatch "^._" -and $_.Name -notmatch "^readme\.md$" }

foreach ($File in $RomFiles) {
    Write-Host "Hashing: $($File.Name)"
    
    # Run RAHasher to get the hash
    $HashOutput = & $HasherPath $File.FullName
    $HashLine = $HashOutput | Where-Object { $_ -match "^Hash:\s+(.+)" }
    
    if ($HashLine) {
        $Hash = $matches[1]
        
        # Call API to get Game ID from hash
        $IdUrl = "https://retroachievements.org/API/API_GetGameID.php?z=$RA_USER&y=$RA_API_KEY&i=$Hash"
        
        try {
            $IdJson = Invoke-RestMethod -Uri $IdUrl
            $GameId = $IdJson.GameID
            
            if ($GameId -and $GameId -ne 0) {
                Write-Host "Found Game ID: $GameId. Fetching achievements..."
                
                $DataUrl = "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=$RA_USER&y=$RA_API_KEY&g=$GameId"
                $DataJson = Invoke-RestMethod -Uri $DataUrl
                
                $JsonString = $DataJson | ConvertTo-Json -Depth 10
                $OutFile = Join-Path -Path $DEST_DIR -ChildPath "games\$GameId.json"
                Set-Content -Path $OutFile -Value $JsonString
                Write-Host "Saved data for Game $GameId."
            } else {
                Write-Host "No RetroAchievements match for this ROM hash."
            }
        } catch {
            Write-Host "Failed to lookup hash or fetch data from RA API."
        }
    } else {
        Write-Host "Failed to hash file."
    }
    Write-Host "---------------------------------"
}

Write-Host "Scraping complete!"
