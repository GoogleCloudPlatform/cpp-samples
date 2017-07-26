#!/bin/sh

set -e

if [ "x${DOCKER_BUILD}" != "xyes" ]; then
  echo "Not a Docker-based build, exit successfully."
  exit 0
fi

sudo apt-get update
sudo apt-get install -y docker-ce
sudo docker --version

TARBALL=docker-images/${DISTRO?}/${DISTRO_VERSION?}/saved.tar.gz
if [ -f ${TARBALL?} ]; then
  gunzip <${TARBALL?} | sudo docker load || echo "Could not load saved image, continue without cache"
fi

echo "DEBUG output for install script"
ls -l docker-images/${DISTRO}/${DISTRO_VERSION} || /bin/true
sudo docker image ls

exit 0
