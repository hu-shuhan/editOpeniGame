param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [int]$TimeoutSeconds = 30,
    [string]$WorkingDirectory = "",
    [string[]]$Arguments = @(),
    [switch]$RequireExit
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    Write-Error "Executable not found: $Executable"
    exit 1
}

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$executableDirectory = Split-Path -Parent $executablePath
$examplesDirectory = Split-Path -Parent $executableDirectory

if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
    $WorkingDirectory = (Get-Location).Path
}

# Runtime DLLs are deployed to the Examples build directory while Visual
# Studio places executables in its Release/Debug configuration directory.
$env:PATH = "$executableDirectory;$examplesDirectory;$env:PATH"

$startParameters = @{
    FilePath = $executablePath
    WorkingDirectory = $WorkingDirectory
    PassThru = $true
    WindowStyle = "Hidden"
}
if ($Arguments.Count -gt 0) {
    $startParameters.ArgumentList = $Arguments
}

$process = Start-Process @startParameters
$finished = $process.WaitForExit($TimeoutSeconds * 1000)

if (-not $finished) {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    $process.WaitForExit()
    if ($RequireExit) {
        Write-Error "Timed out after $TimeoutSeconds seconds: $executablePath"
        exit 124
    }
    exit 0
}

exit $process.ExitCode
