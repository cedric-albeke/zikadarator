param(
    [string] $BuildDir = "build/verify",
    [string] $Configuration = "Debug",
    [string] $PluginvalVersion = "v1.0.4",
    [int] $PluginvalStrictness = 5,
    [int] $PluginvalTimeoutMs = 60000,
    [string] $Generator = "",
    [string] $Architecture = "",
    [string] $Vst3ValidatorPath = ""
)

$ErrorActionPreference = "Stop"

function Resolve-Tool {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Name,
        [string[]] $Candidates = @()
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command -and $command.Source) {
        return $command.Source
    }

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }

    throw "Required tool '$Name' was not found on PATH or known fallback locations."
}

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Label,
        [Parameter(Mandatory = $true)]
        [scriptblock] $Command
    )

    Write-Host ""
    Write-Host "==> $Label"
    & $Command
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

$vsCmake = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
$vsCtest = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe"
$cmake = Resolve-Tool -Name "cmake.exe" -Candidates @($vsCmake)
$ctest = Resolve-Tool -Name "ctest.exe" -Candidates @($vsCtest)
$node = Resolve-Tool -Name "node.exe"

$buildPath = Join-Path $repoRoot $BuildDir
$pluginvalDir = Join-Path $repoRoot "build/pluginval-$PluginvalVersion"
$pluginvalExe = Join-Path $pluginvalDir "pluginval.exe"
$pluginvalZip = Join-Path $repoRoot "build/pluginval-$PluginvalVersion.zip"
$pluginvalResults = Join-Path $repoRoot "build/pluginval-results"
$vst3Path = Join-Path $buildPath "ZikadaFX_artefacts/$Configuration/VST3/ZIKADARATOR.vst3"

Invoke-Step "Validate release metadata" {
    $postinstallPath = "packaging/macos/scripts/postinstall"
    $postinstallStage = git ls-files --stage -- $postinstallPath
    if ($LASTEXITCODE -ne 0) { throw "git ls-files failed with exit code $LASTEXITCODE" }

    if ($postinstallStage -notmatch "^100755\s") {
        throw "$postinstallPath must be tracked as executable mode 100755. Found: $postinstallStage"
    }
}

Invoke-Step "Configure CMake" {
    $configureArgs = @("-S", $repoRoot, "-B", $buildPath)

    if ($Generator.Trim().Length -gt 0) {
        $configureArgs += @("-G", $Generator)
    }

    if ($Architecture.Trim().Length -gt 0) {
        $configureArgs += @("-A", $Architecture)
    }

    & $cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed with exit code $LASTEXITCODE" }
}

Invoke-Step "Run source smoke tests" {
    & $node "scripts/source-smoke-tests.mjs"
    if ($LASTEXITCODE -ne 0) { throw "Source smoke tests failed with exit code $LASTEXITCODE" }
}

Invoke-Step "Build regression test targets" {
    & $cmake --build $buildPath --config $Configuration --target ZikadaEngineTests
    if ($LASTEXITCODE -ne 0) { throw "ZikadaEngineTests build failed with exit code $LASTEXITCODE" }

    & $cmake --build $buildPath --config $Configuration --target ZikadaProcessorTests
    if ($LASTEXITCODE -ne 0) { throw "ZikadaProcessorTests build failed with exit code $LASTEXITCODE" }
}

Invoke-Step "Run CTest" {
    & $ctest --test-dir $buildPath -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "CTest failed with exit code $LASTEXITCODE" }
}

Invoke-Step "Build VST3 artifact" {
    & $cmake --build $buildPath --config $Configuration --target ZikadaFX_VST3
    if ($LASTEXITCODE -ne 0) { throw "ZikadaFX_VST3 build failed with exit code $LASTEXITCODE" }

    if (-not (Test-Path -LiteralPath $vst3Path)) {
        throw "Expected VST3 artifact was not found: $vst3Path"
    }
}

Invoke-Step "Prepare pluginval $PluginvalVersion" {
    $pathPluginval = Get-Command "pluginval.exe" -ErrorAction SilentlyContinue
    if ($pathPluginval -and $pathPluginval.Source) {
        $script:pluginvalExe = $pathPluginval.Source
        Write-Host "Using pluginval from PATH: $script:pluginvalExe"
        return
    }

    if (-not (Test-Path -LiteralPath $pluginvalExe)) {
        New-Item -ItemType Directory -Force -Path (Split-Path $pluginvalZip) | Out-Null
        $url = "https://github.com/Tracktion/pluginval/releases/download/$PluginvalVersion/pluginval_Windows.zip"
        Write-Host "Downloading $url"
        Invoke-WebRequest -Uri $url -OutFile $pluginvalZip

        if (Test-Path -LiteralPath $pluginvalDir) {
            Remove-Item -LiteralPath $pluginvalDir -Recurse -Force
        }

        Expand-Archive -LiteralPath $pluginvalZip -DestinationPath $pluginvalDir -Force
    }

    if (-not (Test-Path -LiteralPath $pluginvalExe)) {
        throw "pluginval.exe was not found after setup: $pluginvalExe"
    }
}

Invoke-Step "Run pluginval level $PluginvalStrictness" {
    New-Item -ItemType Directory -Force -Path $pluginvalResults | Out-Null

    $pluginvalArgs = @(
        "--strictness-level", $PluginvalStrictness,
        "--timeout-ms", $PluginvalTimeoutMs,
        "--output-dir", $pluginvalResults,
        "--output-filename", "ZIKADARATOR-pluginval-level$PluginvalStrictness.log"
    )

    if ($Vst3ValidatorPath.Trim().Length -gt 0) {
        if (-not (Test-Path -LiteralPath $Vst3ValidatorPath)) {
            throw "VST3 validator path does not exist: $Vst3ValidatorPath"
        }

        $pluginvalArgs += @("--vst3validator", $Vst3ValidatorPath)
    }
    else {
        Write-Host "No VST3 validator path supplied; pluginval will skip Steinberg's VST3 validator subtest."
    }

    $pluginvalArgs += @("--validate", $vst3Path)

    & $pluginvalExe @pluginvalArgs
    if ($LASTEXITCODE -ne 0) { throw "pluginval failed with exit code $LASTEXITCODE" }
}

Write-Host ""
Write-Host "Windows validation completed successfully."
Write-Host "VST3: $vst3Path"
Write-Host "pluginval log: $(Join-Path $pluginvalResults "ZIKADARATOR-pluginval-level$PluginvalStrictness.log")"
