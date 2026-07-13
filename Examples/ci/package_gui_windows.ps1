#!/usr/bin/env pwsh
# Package iGameVis.exe and runtime dependencies into a portable zip.
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [string]$QtHome,

    [Parameter(Mandatory = $true)]
    [string]$OutputZip
)

$ErrorActionPreference = "Stop"

$exe = Get-ChildItem -Path $BuildDir -Recurse -Filter "iGameVis.exe" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\Release\\" } |
    Select-Object -First 1

if (-not $exe) {
    $exe = Get-ChildItem -Path $BuildDir -Recurse -Filter "iGameVis.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

if (-not $exe) {
    throw "iGameVis.exe not found under $BuildDir"
}

$stageDir = Join-Path $env:RUNNER_TEMP "iGameVis-portable"
if (Test-Path $stageDir) {
    Remove-Item -Recurse -Force $stageDir
}
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null

Write-Host "Staging from $($exe.Directory.FullName)"
Copy-Item -Path "$($exe.Directory.FullName)\*" -Destination $stageDir -Recurse -Force

$windeployqt = Join-Path $QtHome "5.15.2\msvc2019_64\bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt"
}

Write-Host "Running windeployqt..."
& $windeployqt (Join-Path $stageDir "iGameVis.exe") --no-translations --no-opengl-sw
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$ffmpegBin = Join-Path (Split-Path $BuildDir -Parent) "ThirdParty\FFMPEG\bin"
if (Test-Path $ffmpegBin) {
    Write-Host "Copying FFMPEG DLLs from $ffmpegBin"
    Copy-Item -Path "$ffmpegBin\*.dll" -Destination $stageDir -Force -ErrorAction SilentlyContinue
}

$manifest = @"
iGameVis Windows portable build
Commit: $env:GITHUB_SHA
Ref:    $env:GITHUB_REF
Built:  $(Get-Date -Format "yyyy-MM-dd HH:mm:ss UTC" -AsUTC)
"@
Set-Content -Path (Join-Path $stageDir "BUILD_INFO.txt") -Value $manifest -Encoding UTF8

if (Test-Path $OutputZip) {
    Remove-Item -Force $OutputZip
}

Write-Host "Creating $OutputZip"
Compress-Archive -Path "$stageDir\*" -DestinationPath $OutputZip -Force

Write-Host "Package ready: $OutputZip ($(("{0:N2}" -f ((Get-Item $OutputZip).Length / 1MB)) MB)"
