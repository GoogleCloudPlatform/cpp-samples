#!/bin/sh

set -e

if [ "x${DOCKER_BUILD}" != "xyes" ]; then
  echo "Not a Docker-based build, exit successfully."
  exit 0
fi

IMAGE=cached-${DISTRO?}-${DISTRO_VERSION?}
TARBALL=docker-images/${DISTRO?}/${DISTRO_VERSION?}/saved.tar.gz

# The build creates a new "*:tip" image, we want to save it as
# "*:latest" so it can be used as a cache source in the next build.
sudo docker image tag ${IMAGE?}:tip ${IMAGE?}:latest

sudo docker save ${IMAGE?}:latest | gzip - > ${TARBALL?}

exit 0
