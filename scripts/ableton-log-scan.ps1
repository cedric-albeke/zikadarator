$ErrorActionPreference = "Stop"

function Write-Section {
    param([string] $Title)
    Write-Host ""
    Write-Host "== $Title =="
}

function Get-LatestAbletonLog {
    $abletonRoot = Join-Path $env:APPDATA "Ableton"
    if (-not (Test-Path -LiteralPath $abletonRoot)) {
        return $null
    }

    $liveDir = Get-ChildItem -LiteralPath $abletonRoot -Directory -Filter "Live 12*" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($null -eq $liveDir) {
        return $null
    }

    $candidate = Join-Path $liveDir.FullName "Preferences\Log.txt"
    if (Test-Path -LiteralPath $candidate) {
        return $candidate
    }

    return $null
}

function Show-Matches {
    param(
        [string] $Path,
        [string[]] $Patterns,
        [int] $Last = 80
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        Write-Host "Missing: $Path"
        return
    }

    $item = Get-Item -LiteralPath $Path
    Write-Host "Path: $Path"
    Write-Host ("Size bytes: {0}" -f $item.Length)
    Write-Host ("Last write: {0}" -f $item.LastWriteTime)

    $matches = Select-String -LiteralPath $Path -Pattern $Patterns -ErrorAction SilentlyContinue |
        Select-Object -Last $Last

    if ($matches) {
        $matches | ForEach-Object { $_.Line }
    } else {
        Write-Host "No matching lines."
    }
}

$abletonLog = Get-LatestAbletonLog
$zikLog = Join-Path $env:APPDATA "ZIKADARATOR\UI-Debug.log"
$usageDir = Join-Path $env:APPDATA "Ableton\Live Reports\Usage"

Write-Section "Ableton Live log"
if ($abletonLog) {
    Show-Matches -Path $abletonLog -Patterns @(
        "ZIKADARATOR",
        "Restore .*failed",
        "parameter count",
        "crash",
        "fatal",
        "dropout",
        "drop out",
        "exception"
    )
} else {
    Write-Host "No Ableton Live 12 log found under $env:APPDATA\Ableton."
}

Write-Section "ZIKADARATOR UI log"
Show-Matches -Path $zikLog -Patterns @(
    "error",
    "fail",
    "exception",
    "processBlock",
    "setSelectedSlot",
    "createEditor",
    "destructed",
    "constructed"
) -Last 120

if (Test-Path -LiteralPath $zikLog) {
    $tail = Get-Content -LiteralPath $zikLog -Tail 2000 -ErrorAction SilentlyContinue
    $hotCount = ($tail | Select-String -Pattern @(
        "processBlock",
        "setSelectedSlot"
    ) -ErrorAction SilentlyContinue | Measure-Object).Count
    Write-Host ("Hot debug line count in last 2000 lines: {0}" -f $hotCount)
}

Write-Section "Latest Ableton usage logs"
if (Test-Path -LiteralPath $usageDir) {
    Get-ChildItem -LiteralPath $usageDir -Filter *.log |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 3 FullName, Length, LastWriteTime |
        Format-Table -AutoSize
} else {
    Write-Host "No usage log directory found: $usageDir"
}
