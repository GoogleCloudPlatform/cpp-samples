#!/bin/sh

set -e

if [ "x${MACOSX_BUILD}" != "xyes" ]; then
  echo "Not a Mac OS X build, exit successfully."
  exit 0
fi

brew install grpc protobuf

echo "DEBUG output for install script"
brew info grpc | grep '^/usr/local/Cellar' | awk '{print $1}'
brew info protobuf | grep '^/usr/local/Cellar' | awk '{print $1}'

pkg-config --cflags protobuf
pkg-config --cflags grpc

exit 0
