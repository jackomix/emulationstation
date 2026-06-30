# RetroAchievements Offline Scraper for Windows
# Fetches game achievement data so EmulationStation can use it offline on the R36S.

Write-Host "========================================="
Write-Host " RetroAchievements Offline PC Scraper    "
Write-Host "========================================="
Write-Host ""

$SCRIPT_DIR = Split-Path -Path $PSScriptRoot -Parent
$SD_PATH = $SCRIPT_DIR

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

if (-not (Test-Path -Path (Join-Path $SD_PATH "gba")) -and -not (Test-Path -Path (Join-Path $SD_PATH "snes")) -and -not (Test-Path -Path (Join-Path $SD_PATH "achievements"))) {
    Write-Host "Warning: It looks like this script isn't located on your SD card."
    Write-Host "Please copy the 'tools' folder to the root of your EASYROMS partition and run it from there."
}

$DEST_DIR = Join-Path -Path $SD_PATH -ChildPath "achievements"
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "games") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "badges") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path -Path $DEST_DIR -ChildPath "progress") | Out-Null

Write-Host "Setup complete. Scanning ROMs..."

function Get-SystemKey {
    param([string]$FolderName)
    switch -Regex ($FolderName.ToLower()) {
        "^(nes|famicom)$" { return "NES" }
        "^(snes|sfc)$" { return "SNES" }
        "^gba$" { return "GBA" }
        "^gb$" { return "GB" }
        "^gbc$" { return "GBC" }
        "^n64$" { return "N64" }
        "^nds$" { return "DS" }
        "^(genesis|megadrive)$" { return "MD" }
        "^mastersystem$" { return "SMS" }
        "^gamegear$" { return "GG" }
        "^atari2600$" { return "2600" }
        "^atari7800$" { return "7800" }
        "^atarilynx$" { return "Lynx" }
        "^psx$" { return "PS1" }
        "^psp$" { return "PSP" }
        "^dreamcast$" { return "DC" }
        "^saturn$" { return "SAT" }
        "^segacd$" { return "SCD" }
        "^sega32x$" { return "32X" }
        "^pcengine$" { return "PCE" }
        "^pcenginecd$" { return "PCCD" }
        "^(neogeo|arcade|mame|mame2003)$" { return "ARC" }
        "^neogeocd$" { return "NGCD" }
        "^(ngp|ngpc)$" { return "NGP" }
        "^fds$" { return "FDS" }
        "^virtualboy$" { return "VB" }
        "^(wonderswan|wonderswancolor)$" { return "WSWAN" }
        "^coleco$" { return "CV" }
        "^pokemonmini$" { return "MINI" }
        "^sg-1000$" { return "SG1K" }
        "^3do$" { return "3DO" }
        "^gameandwatch$" { return "G&W" }
        "^pico$" { return "Pico" }
        "^supergrafx$" { return "SGFX" }
        "^vectrex$" { return "VEC" }
        "^intellivision$" { return "INTV" }
        default { return "UNKNOWN" }
    }
}

# 2. Iterate over systems and ROMs
Write-Host "Counting files..."
$Extensions = @("*.gba", "*.zip", "*.sfc", "*.nes", "*.md", "*.z64", "*.cue", "*.chd")
$RomFiles = Get-ChildItem -Path $SD_PATH -Include $Extensions -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -notmatch "^._" -and $_.Name -notmatch "^readme\.md$" }

$TotalRoms = $RomFiles.Count
Write-Host "Found $TotalRoms ROMs to process."

$CountProcessed = 0
$CountMatched = 0
$CountSaved = 0
$CountNoMatch = 0
$CountFailed = 0
$CountSkipped = 0

foreach ($File in $RomFiles) {
    $CountProcessed++
    $ParentDir = $File.Directory.Name
    $SysKey = Get-SystemKey -FolderName $ParentDir
    
    Write-Host "[$CountProcessed/$TotalRoms] Hashing: $($File.Name)"
    
    if ($SysKey -eq "UNKNOWN") {
        Write-Host "  -> Skipping: Unknown system folder '$ParentDir'"
        $CountSkipped++
        continue
    }
    
    # Run RAHasher
    $HashOutput = & $HasherPath $SysKey $File.FullName 2>$null
    
    if ($HashOutput -is [array]) {
        $Hash = $HashOutput[-1].Trim()
    } else {
        $Hash = $HashOutput.Trim()
    }
    
    if ($Hash -and $Hash.Length -eq 32) {
        $IdUrl = "https://retroachievements.org/API/API_GetGameID.php?z=$RA_USER&y=$RA_API_KEY&i=$Hash"
        
        try {
            $IdJson = Invoke-RestMethod -Uri $IdUrl
            $GameId = $IdJson.GameID
            
            if ($GameId -and $GameId -ne 0) {
                $CountMatched++
                
                $OutFile = Join-Path -Path $DEST_DIR -ChildPath "games\$GameId.json"
                if (Test-Path -Path $OutFile) {
                    Write-Host "  -> Found Game ID: $GameId (Already cached, skipping)"
                    $CountSaved++
                } else {
                    Write-Host "  -> Found Game ID: $GameId. Fetching achievements..."
                    
                    $DataUrl = "https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=$RA_USER&y=$RA_API_KEY&g=$GameId"
                    $DataJson = Invoke-RestMethod -Uri $DataUrl
                    
                    $JsonString = $DataJson | ConvertTo-Json -Depth 10
                    Set-Content -Path $OutFile -Value $JsonString
                    Write-Host "  -> Saved data for Game $GameId."
                    $CountSaved++
                    
                    Start-Sleep -Milliseconds 200
                }
            } else {
                Write-Host "  -> No RetroAchievements match for this ROM."
                $CountNoMatch++
            }
        } catch {
            Write-Host "  -> Error: Failed to lookup hash or fetch data from RA API."
            $CountNoMatch++
        }
    } else {
        Write-Host "  -> Error: Failed to hash file or unsupported format."
        $CountFailed++
    }
}

Write-Host ""
Write-Host "=========================================="
Write-Host " Scraping Complete!"
Write-Host " ROMs scanned:  $CountProcessed"
Write-Host " Matched on RA: $CountMatched"
Write-Host " Saved to SD:   $CountSaved"
Write-Host " No match:      $CountNoMatch"
Write-Host " Hash failed:   $CountFailed"
Write-Host " Skipped (dir): $CountSkipped"
Write-Host "=========================================="
