#!/usr/bin/env powershell

# Stop on errors, similar (but not quite as useful as) "set -e" on Unix shells ...
$ErrorActionPreference = "Stop"

$dir = Split-Path (Get-Item -Path ".\" -Verbose).FullName
if (Test-Path variable:env:APPVEYOR_BUILD_FOLDER) {
  $dir = Split-Path $env:APPVEYOR_BUILD_FOLDER
}

$integrate = "$dir\vcpkg\vcpkg.exe integrate install" 
Invoke-Expression $integrate
if ($LASTEXITCODE) {
  throw "vcpkg integrate failed with exit code $LASTEXITCODE"
}

cd bigtable\api
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="$dir\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows-static ..
if ($LASTEXITCODE) {
  throw "cmake failed with exit code $LASTEXITCODE"
}

cmake --build .
if ($LASTEXITCODE) {
  throw "cmake build failed with exit code $LASTEXITCODE"
}
