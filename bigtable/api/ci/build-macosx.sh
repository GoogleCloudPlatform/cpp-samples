#!/bin/sh

set -e

if [ "x${MACOSX_BUILD}" != "xyes" ]; then
  echo "Not a Mac OS X build, exit successfully"
  exit 0
fi

cd bigtable/api
make -C googleapis OUTPUT=$PWD/googleapis-gens

# On my local workstation I prefer to keep all the build artifacts in
# a sub-directory, not so important for Travis builds, but that makes
# this script easier to test.
test -d build || mkdir build

cd build
cmake ..

make -j 2

exit 0
