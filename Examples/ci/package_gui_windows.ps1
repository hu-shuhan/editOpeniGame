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

if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
    $root = if ($env:GITHUB_WORKSPACE) { $env:GITHUB_WORKSPACE } else { (Get-Location).Path }
    $BuildDir = Join-Path $root $BuildDir
}
$BuildDir = (Resolve-Path -Path $BuildDir).Path

$repoRoot = if ($env:GITHUB_WORKSPACE) { $env:GITHUB_WORKSPACE } else { Split-Path $BuildDir -Parent }

if (-not [System.IO.Path]::IsPathRooted($OutputZip)) {
    $OutputZip = Join-Path $repoRoot $OutputZip
}

if (-not [System.IO.Path]::IsPathRooted($QtHome)) {
    $qtRoot = if ($env:GITHUB_WORKSPACE) { $env:GITHUB_WORKSPACE } else { (Get-Location).Path }
    $QtHome = Join-Path $qtRoot $QtHome
}

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

$resourcesDir = Join-Path $BuildDir "Resources"
if (-not (Test-Path $resourcesDir)) {
    throw "Resources directory not found: $resourcesDir (required for shaders and fonts)"
}

Write-Host "Copying Resources from $resourcesDir"
Copy-Item -Path $resourcesDir -Destination (Join-Path $stageDir "Resources") -Recurse -Force

$windeployqt = Join-Path $QtHome "5.15.2\msvc2019_64\bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    throw "windeployqt not found: $windeployqt"
}

Write-Host "Running windeployqt..."
& $windeployqt (Join-Path $stageDir "iGameVis.exe") --no-translations --no-opengl-sw
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$ffmpegBin = Join-Path $repoRoot "ThirdParty\FFMPEG\bin"
if (Test-Path $ffmpegBin) {
    $ffmpegDlls = Get-ChildItem -Path $ffmpegBin -Filter "*.dll" -ErrorAction SilentlyContinue
    if ($ffmpegDlls) {
        Write-Host "Copying FFMPEG DLLs from $ffmpegBin"
        Copy-Item -Path $ffmpegDlls.FullName -Destination $stageDir -Force
    } else {
        Write-Warning "FFMPEG bin directory exists but contains no DLLs: $ffmpegBin"
    }
} else {
    Write-Warning "FFMPEG bin directory not found; animation export may be unavailable"
}

$requiredFiles = @(
    "iGameVis.exe",
    "Resources\Shaders\Vertex.vert",
    "Resources\Shaders\BlinnPhong.frag",
    "Resources\Assests\Fonts\SourceHanSansCN-Normal.otf"
)
foreach ($relPath in $requiredFiles) {
    $fullPath = Join-Path $stageDir $relPath
    if (-not (Test-Path $fullPath)) {
        throw "Required package file missing: $relPath"
    }
}
Write-Host "Validated required runtime files"

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

$sizeMb = "{0:N2}" -f ((Get-Item $OutputZip).Length / 1MB)
Write-Host "Package ready: $OutputZip ($sizeMb MB)"
