#!/usr/bin/env powershell

$dir = Split-Path (Get-Item -Path ".\" -Verbose).FullName
if (Test-Path variable:env:APPVEYOR_BUILD_FOLDER) {
  $dir = Split-Path $env:APPVEYOR_BUILD_FOLDER
}

$integrate = "$dir\vcpkg\vcpkg.exe integrate install" 
Invoke-Expression $integrate

cd bigtable\api
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="$dir\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows-static ..
cmake --build .
