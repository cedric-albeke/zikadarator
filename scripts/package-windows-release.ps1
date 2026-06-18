param(
    [string] $BuildDir = "build/aaa-release",
    [string] $Configuration = "Release",
    [string] $PackageName = "ZIKADARATOR-v1-AAA-Release",
    [string] $ModuleInfoFallback = "packaging/windows/moduleinfo.json"
)

$ErrorActionPreference = "Stop"

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $Path))
}

function Assert-InRepo {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )

    $resolved = [System.IO.Path]::GetFullPath($Path)
    if (-not $resolved.StartsWith($script:RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to operate outside repo: $resolved"
    }

    return $resolved
}

function Assert-FileExists {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,
        [Parameter(Mandatory = $true)]
        [string] $Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label was not found: $Path"
    }
}

function Write-PackageChecksums {
    param(
        [Parameter(Mandatory = $true)]
        [string] $PackagePath
    )

    $entries = @(
        @{ Label = "ZIKADARATOR.exe"; Path = Join-Path $PackagePath "ZIKADARATOR.exe" },
        @{ Label = "ZIKADARATOR.vst3/Contents/x86_64-win/ZIKADARATOR.vst3"; Path = Join-Path $PackagePath "ZIKADARATOR.vst3/Contents/x86_64-win/ZIKADARATOR.vst3" },
        @{ Label = "ZIKADARATOR.vst3/Contents/Resources/moduleinfo.json"; Path = Join-Path $PackagePath "ZIKADARATOR.vst3/Contents/Resources/moduleinfo.json" },
        @{ Label = "install.bat"; Path = Join-Path $PackagePath "install.bat" },
        @{ Label = "README.txt"; Path = Join-Path $PackagePath "README.txt" }
    )

    $lines = @(
        "ZIKADARATOR V1 Windows Test Package SHA-256",
        "Generated: $(Get-Date -Format o)",
        ""
    )

    foreach ($entry in $entries) {
        Assert-FileExists -Path $entry.Path -Label $entry.Label
        $hash = Get-FileHash -LiteralPath $entry.Path -Algorithm SHA256
        $lines += "$($hash.Hash.ToLowerInvariant())  $($entry.Label)"
    }

    $lines | Set-Content -LiteralPath (Join-Path $PackagePath "SHA256SUMS.txt") -Encoding ASCII
}

function Test-PackageChecksums {
    param(
        [Parameter(Mandatory = $true)]
        [string] $PackagePath
    )

    $checksumPath = Join-Path $PackagePath "SHA256SUMS.txt"
    Assert-FileExists -Path $checksumPath -Label "Package checksum manifest"

    $entries = Get-Content -LiteralPath $checksumPath | Where-Object {
        $_ -match '^[0-9a-f]{64}\s{2}.+'
    }

    if ($entries.Count -eq 0) {
        throw "No checksum entries found in $checksumPath"
    }

    foreach ($entry in $entries) {
        $parts = $entry -split '\s{2}', 2
        if ($parts.Count -ne 2) {
            throw "Invalid checksum line: $entry"
        }

        $expectedHash = $parts[0]
        $relativePath = $parts[1] -replace '/', [System.IO.Path]::DirectorySeparatorChar
        $artifactPath = Join-Path $PackagePath $relativePath

        Assert-FileExists -Path $artifactPath -Label "Checksummed package artifact"

        $actualHash = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actualHash -ne $expectedHash) {
            throw "Checksum mismatch for $($parts[1]): expected $expectedHash, got $actualHash"
        }
    }
}

$script:RepoRoot = [System.IO.Path]::GetFullPath((Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
Set-Location $script:RepoRoot

$buildPath = Resolve-RepoPath $BuildDir
$fallbackPath = Resolve-RepoPath $ModuleInfoFallback
$packagePath = Assert-InRepo (Resolve-RepoPath $PackageName)
$zipPath = Assert-InRepo (Resolve-RepoPath "$PackageName.zip")

$standaloneExe = Join-Path $buildPath "ZikadaFX_artefacts/$Configuration/Standalone/ZIKADARATOR.exe"
$vst3Bundle = Join-Path $buildPath "ZikadaFX_artefacts/$Configuration/VST3/ZIKADARATOR.vst3"
$vst3Binary = Join-Path $vst3Bundle "Contents/x86_64-win/ZIKADARATOR.vst3"
$moduleInfo = Join-Path $vst3Bundle "Contents/Resources/moduleinfo.json"

Assert-FileExists -Path $standaloneExe -Label "Standalone artifact"
Assert-FileExists -Path $vst3Binary -Label "VST3 binary"
Assert-FileExists -Path $fallbackPath -Label "Tracked moduleinfo fallback"

$moduleInfoNeedsRepair = $true
if (Test-Path -LiteralPath $moduleInfo -PathType Leaf) {
    $moduleInfoNeedsRepair = ((Get-Item -LiteralPath $moduleInfo).Length -eq 0)
}

if ($moduleInfoNeedsRepair) {
    New-Item -ItemType Directory -Force -Path (Split-Path $moduleInfo) | Out-Null
    Copy-Item -LiteralPath $fallbackPath -Destination $moduleInfo -Force
}

if ((Get-Item -LiteralPath $moduleInfo).Length -eq 0) {
    throw "moduleinfo.json is still empty after repair: $moduleInfo"
}

if (Test-Path -LiteralPath $packagePath) {
    $resolvedPackagePath = Assert-InRepo $packagePath
    Remove-Item -LiteralPath $resolvedPackagePath -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $packagePath | Out-Null
Copy-Item -LiteralPath $standaloneExe -Destination (Join-Path $packagePath "ZIKADARATOR.exe") -Force
Copy-Item -LiteralPath $vst3Bundle -Destination (Join-Path $packagePath "ZIKADARATOR.vst3") -Recurse -Force

$installerDir = Join-Path $packagePath "installer"
New-Item -ItemType Directory -Force -Path $installerDir | Out-Null
Copy-Item -LiteralPath (Join-Path $script:RepoRoot "packaging/windows/ZIKADARATOR.iss") `
          -Destination (Join-Path $installerDir "ZIKADARATOR-Setup.iss") -Force

@"
@echo off
setlocal
set VST3_DIR=%ProgramFiles%\Common Files\VST3
echo Installing ZIKADARATOR.vst3 to "%VST3_DIR%"
if not exist "%VST3_DIR%" mkdir "%VST3_DIR%"
xcopy /E /I /Y "%~dp0ZIKADARATOR.vst3" "%VST3_DIR%\ZIKADARATOR.vst3"
echo.
echo Optional standalone app is included as ZIKADARATOR.exe next to this installer.
echo Done.
pause
"@ | Set-Content -LiteralPath (Join-Path $packagePath "install.bat") -Encoding ASCII

@"
ZIKADARATOR V1 Windows Test Package

Contents:
- ZIKADARATOR.exe: standalone test app
- ZIKADARATOR.vst3: VST3 plugin bundle
- install.bat: copies the VST3 bundle into the common VST3 folder
- installer/ZIKADARATOR-Setup.iss: Inno Setup script for building an installer

If Windows Application Control blocks JUCE's VST3 helper, this package script restores
a tracked moduleinfo.json fallback before packaging so the bundle is not left with a
zero-byte manifest.

SHA256SUMS.txt lists checksums for the standalone, VST3 binary, moduleinfo, installer
helper, and this README so tester downloads can be verified after transfer.
"@ | Set-Content -LiteralPath (Join-Path $packagePath "README.txt") -Encoding ASCII

Write-PackageChecksums -PackagePath $packagePath
Test-PackageChecksums -PackagePath $packagePath

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

Compress-Archive -Path (Join-Path $packagePath "*") -DestinationPath $zipPath -Force

Write-Host "Windows release package created:"
Write-Host "Package: $packagePath"
Write-Host "ZIP:     $zipPath"
Write-Host "VST3:    $(Join-Path $packagePath "ZIKADARATOR.vst3")"
Write-Host "EXE:     $(Join-Path $packagePath "ZIKADARATOR.exe")"
