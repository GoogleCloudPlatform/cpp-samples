#!/bin/sh

set -e

if [ "x${MACOSX_BUILD}" != "xyes" ]; then
  echo "Not a Mac OS X build, exit successfully."
  exit 0
fi

brew install grpc protobuf

exit 0
