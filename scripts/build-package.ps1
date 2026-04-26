param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ProjectName = "BaseLib"
$Version = "1.0.0"
$BuildDate = Get-Date -Format "yyyyMMdd"

$ScriptDir = Split-Path $PSScriptRoot -Parent
$BuildDir = Join-Path $ScriptDir "buildpkg-$Platform"
$PackageDir = Join-Path $ScriptDir "package"

function Write-Step($Message) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
}

function WriteOK($Message) {
    Write-Host "[OK] $Message" -ForegroundColor Green
}

function WriteErr($Message) {
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

Write-Host ""
Write-Host "########################################" -ForegroundColor Magenta
Write-Host "#  BaseLib Build & Package Script      #" -ForegroundColor Magenta
Write-Host "#  v$Version ($BuildDate)                    #" -ForegroundColor Magenta
Write-Host "########################################" -ForegroundColor Magenta

Write-Step "Checking System"
$os = Get-CimInstance Win32_OperatingSystem
$version = [System.Version]$os.Version
if ($version.Major -lt 10) {
    WriteErr "Windows 10 or higher required"
    exit 1
}
WriteOK "Windows version OK"

Write-Step "Checking Build Tools"
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$hasVS = $false
if (Test-Path $vsWhere) {
    $vsInfo = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion -format value
    if ($vsInfo -match "^17\.") {
        $hasVS = $true
        WriteOK "Visual Studio 2022 detected"
    }
}
if (-not $hasVS) {
    WriteErr "Visual Studio 2022 not found"
    exit 1
}

Write-Step "Building $ProjectName v$Version ($Configuration)"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -NoNewline
    Remove-Item -Recurse -Force $BuildDir
    WriteOK "Cleaned"
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Write-Host "Configuring CMake..." -NoNewline
$cmakeOut = & cmake -S $ScriptDir -B $BuildDir -G "Visual Studio 17 2022" -A $Platform 2>&1
if ($LASTEXITCODE -ne 0) {
    WriteErr "CMake configuration failed"
    exit 1
}
WriteOK "Configured"

Write-Host "Building..." -NoNewline
$buildOut = & cmake --build $BuildDir --config $Configuration --parallel 2>&1
if ($LASTEXITCODE -ne 0) {
    WriteErr "Build failed"
    exit 1
}
WriteOK "Built"

Write-Step "Creating Package"

$packageName = "$ProjectName-$Version-$Platform-$Configuration"
$pkgFolder = Join-Path $PackageDir $packageName
$zipPath = Join-Path $PackageDir "$packageName.zip"

if (Test-Path $pkgFolder) {
    Remove-Item -Recurse -Force $pkgFolder
}
New-Item -ItemType Directory -Path (Join-Path $pkgFolder "lib") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $pkgFolder "include") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $pkgFolder "docs") -Force | Out-Null

Write-Host "Copying libraries..." -NoNewline
$srcLib = Join-Path $BuildDir (Join-Path $Configuration "BaseLib.lib")
if (Test-Path $srcLib) {
    Copy-Item $srcLib (Join-Path $pkgFolder "lib") -Force
    WriteOK "Library copied"
} else {
    WriteErr "Library not found at $srcLib"
    exit 1
}

$srcDll = Join-Path $ScriptDir "build\Release\BaseLibShared.dll"
if (Test-Path $srcDll) {
    Write-Host "Copying DLLs..." -NoNewline
    New-Item -ItemType Directory -Path (Join-Path $pkgFolder "bin") -Force | Out-Null
    Copy-Item $srcDll (Join-Path $pkgFolder "bin") -Force
    WriteOK "DLL copied"
}

Write-Host "Copying headers..." -NoNewline
$includeSrc = Join-Path $ScriptDir "include"
$includeDest = Join-Path $pkgFolder "include"
Get-ChildItem -Path $includeSrc -Filter "*.h" -Recurse | ForEach-Object {
    $relPath = $_.FullName.Substring($includeSrc.Length + 1)
    $destPath = Join-Path $includeDest $relPath
    $destFolder = Split-Path $destPath -Parent
    if (-not (Test-Path $destFolder)) {
        New-Item -ItemType Directory -Path $destFolder -Force | Out-Null
    }
    Copy-Item $_.FullName $destPath -Force
}
WriteOK "Headers copied"

Write-Host "Copying documentation..." -NoNewline
$apiDocSrc = Join-Path $ScriptDir "archive\api-documentation\BaseLib-API-Documentation.md"
$apiDocDest = Join-Path $pkgFolder "docs\BaseLib-API-Documentation.md"
if (Test-Path $apiDocSrc) {
    Copy-Item $apiDocSrc $apiDocDest -Force
    WriteOK "Documentation copied"
} else {
    Write-Host "Documentation not found, skipping" -ForegroundColor Yellow
}

$readme = @"
# $ProjectName v$Version

## Package Contents
- `lib/` - Static library files (.lib)
- `bin/` - Dynamic library files (.dll)
- `include/` - Header files
- `docs/` - Documentation

## Quick Start
1. Add `include` to your include paths
2. Link against `lib/BaseLib.lib` (static) or `bin/BaseLibShared.dll` (dynamic)

## Example (Static Library)
```cpp
#include `"base.h`"
#include <iostream>

int main() {
    base::Logger::init();
    BASE_LOG_INFO(`"Hello from BaseLib!`");
    return 0;
}
```

## Requirements
- C++11 or higher
- Windows 10+ (for Windows builds)

---
Generated by BaseLib Build Script v$Version ($BuildDate)
"@
$readme | Set-Content -Path (Join-Path $pkgFolder "README.md") -Encoding UTF8

Write-Host "Creating ZIP archive..." -NoNewline
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path $pkgFolder -DestinationPath $zipPath -CompressionLevel Optimal
WriteOK "Archive created: $zipPath"

Write-Step "Validating Package"
$zipSize = [math]::Round((Get-Item $zipPath).Length / 1KB, 2)
Write-Host "ZIP size: $zipSize KB" -ForegroundColor Cyan

$tempDir = Join-Path $env:TEMP "baselib_test_$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
Expand-Archive -Path $zipPath -DestinationPath $tempDir -Force
$extractDir = Get-ChildItem -Path $tempDir -Directory | Select-Object -First 1

$headerCount = (Get-ChildItem -Path (Join-Path $extractDir.FullName "include") -Filter "*.h" -Recurse).Count
$libCount = (Get-ChildItem -Path (Join-Path $extractDir.FullName "lib") -Filter "*.lib").Count
$dllCount = (Get-ChildItem -Path (Join-Path $extractDir.FullName "bin") -Filter "*.dll" -ErrorAction SilentlyContinue).Count

Write-Host "Headers found: $headerCount" -ForegroundColor Cyan
Write-Host "Libraries found: $libCount" -ForegroundColor Cyan
Write-Host "DLLs found: $dllCount" -ForegroundColor Cyan

if ($headerCount -eq 0 -or $libCount -eq 0) {
    WriteErr "Package validation failed"
    Remove-Item -Recurse -Force $tempDir
    exit 1
}
WriteOK "Package validated"
Remove-Item -Recurse -Force $tempDir

$endTime = Get-Date
Write-Host ""
Write-Host "########################################" -ForegroundColor Green
Write-Host "#  Build Completed Successfully!      #" -ForegroundColor Green
Write-Host "########################################" -ForegroundColor Green
Write-Host ""
Write-Host "Package: $zipPath" -ForegroundColor Cyan
Write-Host "Size: $zipSize KB" -ForegroundColor Cyan
Write-Host ""
