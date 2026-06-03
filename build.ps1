$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$ExePath = Join-Path $Root "1.exe"

$CompileArgs = @(
    (Join-Path $Root "1.cpp"),
    "-o", $ExePath,
    "-leasyx",
    "-lgdi32",
    "-limm32",
    "-lmsimg32",
    "-lole32",
    "-loleaut32",
    "-lwinhttp",
    "-lcrypt32",
    "-finput-charset=UTF-8",
    "-fexec-charset=UTF-8"
)

& g++ @CompileArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Built $ExePath"
