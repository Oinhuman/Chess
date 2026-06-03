$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildScript = Join-Path $Root "build.ps1"
$ExePath = Join-Path $Root "1.exe"

& $BuildScript
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $ExePath
