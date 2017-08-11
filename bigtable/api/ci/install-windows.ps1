#!/usr/bin/env powershell

# Using relative paths works both on appveyor and in development workstations
cd ..

# ... update or clone the vcpkg package manager ...
if (Test-Path vcpkg\.git) {
  cd vcpkg
  git pull
} elseif (Test-Path vcpkg\installed) {
  move vcpkg vcpkg-tmp
  git clone https://github.com/Microsoft/vcpkg
  move vcpkg-tmp\installed vcpkg
  cd vcpkg
} else {
  git clone https://github.com/Microsoft/vcpkg
  cd vcpkg
}

# ... install cmake because the version in appveyor is too old for some of
# the packages ...
choco install -y cmake cmake.portable

# ... build the tool each time, it is fast to do so ...
powershell -exec bypass scripts\bootstrap.ps1

# ... integrate installed packages into the build environment ...
.\vcpkg integrate install

# ... if necessary, install grpc again.  Normally the packages are
# cached by the CI system (appveyor) so this is not too painful ...
.\vcpkg install zlib:x86-windows-static
#.\vcpkg install openssl:x86-windows-static
#.\vcpkg install protobuf:x86-windows-static
#.\vcpkg install grpc:x86-windows-static

cd ..\cpp-docs-samples
