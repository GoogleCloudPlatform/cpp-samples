#!/bin/sh

set -e

if [ "x${DOCKER_BUILD}" != "xyes" ]; then
  echo "Not a Docker-based build, exit successfully."
  exit 0
fi

IMAGE="cached-${DISTRO?}-${DISTRO_VERSION?}"
latest_id=$(sudo docker inspect -f '{{ .Id }}' ${IMAGE?}:latest >/dev/null || echo "")

echo IMAGE = $IMAGE
echo IMAGE LATEST ID = $latest_id

# TODO() - on cron buiids, we would want to disable the cache
# altogether, to make sure we can still build against recent versions
# of grpc, protobug, the compilers, etc.
cacheargs=""
if [ "x${latest_id}" != "x" ]; then
  cacheargs="--cache-from ${IMAGE?}:latest"
fi

echo cache args = $cacheargs

sudo docker build -t ${IMAGE?}:tip ${cacheargs?} \
     --build-arg DISTRO_VERSION=${DISTRO_VERSION?} \
     --build-arg CXX=${CXX?} \
     --build-arg CC=${CC?} \
     --build-arg TRAVIS_JOB_NUMBER=${TRAVIS_JOB_NUMBER} \
     -f bigtable/api/docker/Dockerfile.${DISTRO?} bigtable/

exit 0
