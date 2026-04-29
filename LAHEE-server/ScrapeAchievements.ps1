# LAHEE Professional Achievement Scraper
# Refactored for clean UX, progress bars, and credential management

$PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ServerDir = Join-Path $PSScriptRoot "Server"
$ServerPath = Join-Path $ServerDir "LAHEE.exe"
$ConfigFile = Join-Path $ServerDir "LAHEE.json"
$RomsPath = (Get-Item $PSScriptRoot).Parent.FullName

# --- HELPER FUNCTIONS ---

function Show-Header {
    Clear-Host
    Write-Host "===============================================" -ForegroundColor Cyan
    Write-Host "       LAHEE NATIVE ACHIEVEMENT SCRAPER        " -ForegroundColor White -BackgroundColor Blue
    Write-Host "===============================================" -ForegroundColor Cyan
}

function Get-Credentials {
    Show-Header
    Write-Host "[ CONFIGURATION ]" -ForegroundColor Yellow
    
    $currentConfig = @{ LAHEE = @{ RAFetch = @{ Username = ""; WebApiKey = ""; Password = "" } } }
    if (Test-Path $ConfigFile) {
        $currentConfig = Get-Content $ConfigFile | ConvertFrom-Json
    }

    $raUser = Read-Host "RetroAchievements Username [$($currentConfig.LAHEE.RAFetch.Username)]"
    if ([string]::IsNullOrWhiteSpace($raUser)) { $raUser = $currentConfig.LAHEE.RAFetch.Username }

    $raKey = Read-Host "RetroAchievements Web API Key (hidden)" -AsSecureString
    $raKey = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto([System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($raKey))
    if ([string]::IsNullOrWhiteSpace($raKey)) { $raKey = $currentConfig.LAHEE.RAFetch.WebApiKey }

    $raPass = Read-Host "RetroAchievements Password/Token (hidden)" -AsSecureString
    $raPass = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto([System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($raPass))
    if ([string]::IsNullOrWhiteSpace($raPass)) { $raPass = $currentConfig.LAHEE.RAFetch.Password }

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
    Write-Host "`n[+] Configuration saved to Server/LAHEE.json" -ForegroundColor Green
    Start-Sleep -Seconds 2
}

function Start-Scrape {
    if (-not (Test-Path $ConfigFile)) {
        Write-Host "[!] Credentials not found. Please configure them first." -ForegroundColor Red
        Start-Sleep -Seconds 2
        return
    }

    Show-Header
    Write-Host "Scanning ROMs in: $RomsPath" -ForegroundColor Gray
    Write-Host "Launching LAHEE Engine..." -ForegroundColor Gray

    # CLEAN PATH: Remove trailing \ so it doesn't escape the CLI quotes
    $cleanRomsPath = $RomsPath.TrimEnd('\')

    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = $ServerPath
    $processInfo.Arguments = "--hub `"$PSScriptRoot`" --machine scrape `"$cleanRomsPath`""
    $processInfo.RedirectStandardOutput = $true
    $processInfo.UseShellExecute = $false
    $processInfo.CreateNoWindow = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $processInfo

    try {
        $process.Start() | Out-Null
        $stdout = $process.StandardOutput

        while (-not $stdout.EndOfStream) {
            $line = $stdout.ReadLine()
            if ($line -match "PROGRESS:(\d+):(\d+)") {
                $curr = [int]$matches[1]
                $total = [int]$matches[2]
                $percent = ($curr / $total) * 100
                Write-Progress -Activity "Scraping Achievements" -Status "Processing game $curr of $total" -PercentComplete $percent
            }
            elseif ($line -match "STATUS:(.*)") {
                Write-Host "[+] $($matches[1])" -ForegroundColor Green
            }
            elseif ($line -match "ERROR:(.*)") {
                Write-Host "[!] $($matches[1])" -ForegroundColor Red
            }
        }
        $process.WaitForExit()
    }
    finally {
        if (-not $process.HasExited) {
            $process.Kill()
        }
        Write-Progress -Activity "Scraping Achievements" -Completed
    }

    Write-Host "`n[ FINISHED ] All identified sets have been downloaded." -ForegroundColor Cyan
    Write-Host "Press any key to return to menu..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}

# --- MAIN MENU LOOP ---

if (-not (Test-Path $ServerPath)) {
    Write-Error "Could not find LAHEE server binary at $ServerPath"
    pause
    exit
}

while ($true) {
    Show-Header
    Write-Host "[1] Start Bulk Scrape" -ForegroundColor White
    Write-Host "[2] Configure Credentials (User/API Key)" -ForegroundColor White
    Write-Host "[3] Exit" -ForegroundColor White
    Write-Host ""
    $choice = Read-Host "Select an option"

    switch ($choice) {
        "1" { Start-Scrape }
        "2" { Get-Credentials }
        "3" { exit }
        default { Write-Host "Invalid selection." -ForegroundColor Red; Start-Sleep -Seconds 1 }
    }
}
