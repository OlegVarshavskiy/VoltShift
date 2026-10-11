param(
    [string]$Compiler = 'gcc',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$compilerCommand = Get-Command $Compiler -ErrorAction SilentlyContinue
if (-not $compilerCommand -and $Compiler -eq 'gcc') {
    $bundledCandidate = 'C:\msys64\ucrt64\bin\gcc.exe'
    if (Test-Path -LiteralPath $bundledCandidate) {
        $compilerCommand = Get-Command $bundledCandidate
    }
}
if (-not $compilerCommand) { throw 'MinGW-w64 GCC not found. Pass -Compiler with the full path to gcc.exe.' }
$compilerPath = $compilerCommand.Source
$savedPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compilerPath) + ';' + $env:PATH
    $buildDir = Join-Path $PSScriptRoot 'build'
    New-Item -ItemType Directory -Force $buildDir | Out-Null
    $flags = @('-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-I', (Join-Path $PSScriptRoot 'include'))
    $sources = @('hardware.c', 'gpu.c', 'battery.c', 'power_plan.c', 'capabilities.c') | ForEach-Object { Join-Path $PSScriptRoot "src/$_" }
    $libraries = @('-ladvapi32', '-lsetupapi', '-lpowrprof')
    & $compilerPath @flags (Join-Path $PSScriptRoot 'src/main.c') @sources '-o' (Join-Path $buildDir 'voltshift.exe') @libraries
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
    Write-Host 'Built build/voltshift.exe'
    if ($Test) {
        & $compilerPath @flags (Join-Path $PSScriptRoot 'tests/test_diagnostics.c') @sources '-o' (Join-Path $buildDir 'test_diagnostics.exe') @libraries
        if ($LASTEXITCODE -ne 0) { throw "Test build failed: $LASTEXITCODE" }
        & (Join-Path $buildDir 'test_diagnostics.exe')
        if ($LASTEXITCODE -ne 0) { throw "Tests failed: $LASTEXITCODE" }
    }
} finally { $env:PATH = $savedPath }
