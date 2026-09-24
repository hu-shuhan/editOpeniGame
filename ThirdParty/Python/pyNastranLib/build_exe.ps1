[CmdletBinding()]
param(
    [string]$PythonExe = "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe",
    [string]$VenvDirectory = ".build-venv",
    [string]$DistDirectory = "dist",
    [string]$WorkDirectory = "build"
)

$ErrorActionPreference = "Stop"
$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $scriptDirectory

if (-not (Test-Path -LiteralPath $PythonExe -PathType Leaf)) {
    throw "CPython 3.12 executable not found: $PythonExe"
}

$pythonInfo = & $PythonExe -c "import platform,sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}|{platform.architecture()[0]}')"
if ($LASTEXITCODE -ne 0 -or $pythonInfo -ne "3.12.10|64bit") {
    throw "This build requires CPython 3.12.10 x64; found $pythonInfo"
}

$venvPath = Join-Path $scriptDirectory $VenvDirectory
$venvPython = Join-Path $venvPath "Scripts\python.exe"
if (-not (Test-Path -LiteralPath $venvPython -PathType Leaf)) {
    & $PythonExe -m venv $venvPath
    if ($LASTEXITCODE -ne 0) { throw "Failed to create build virtual environment" }
}

& $venvPython -m pip install --disable-pip-version-check -r "requirements-build.txt"
if ($LASTEXITCODE -ne 0) { throw "Failed to install locked build dependencies" }

& $venvPython -m PyInstaller --clean --noconfirm --distpath $DistDirectory --workpath $WorkDirectory "nastran_to_vtk_cli.spec"
if ($LASTEXITCODE -ne 0) { throw "PyInstaller build failed" }

$exePath = Join-Path (Join-Path $scriptDirectory $DistDirectory) "nastran_to_vtk_cli.exe"
if (-not (Test-Path -LiteralPath $exePath -PathType Leaf)) {
    throw "PyInstaller reported success but the executable is missing: $exePath"
}

$hash = Get-FileHash -LiteralPath $exePath -Algorithm SHA256
Write-Host "Candidate: $exePath"
Write-Host "SHA-256:  $($hash.Hash)"
