# Copyright 2017, Google Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# Create a docker image with the C++ Google API examples built into it.
# This Dockerfile requires docker 17.05 or higher, it uses an argument to set
# the base image version, which was not supported in early versions of docker.
ARG DISTRO_VERSION=24
FROM fedora:$DISTRO_VERSION
MAINTAINER "Carlos O'Ryan <coryan@google.com>"

# Install the pre-requisites, the long command line is to create as few
# layers as possible in the image ...
RUN dnf makecache && \
  dnf install -y \
    autoconf \
    autoconf-archive \
    automake \
    clang \
    cmake \
    curl \
    gcc-c++ \
    git \
    libtool \
    make \
    pkgconfig \
    shtool \
    wget && \
  dnf clean all

WORKDIR /var/tmp/build-grpc
RUN (git clone https://github.com/grpc/grpc.git && \
    cd grpc && git pull && git submodule update --init && \
    make -j 2 LDXX="g++ -std=c++11" LD=gcc CXX="g++ -std=c++11" CC=gcc && \
    make install && \
    cd third_party/protobuf && make install && \
    echo /usr/local/lib >/etc/ld.so.conf.d/local.conf && ldconfig)

# ... by default, Fedora does not look for packages in
# /usr/local/lib/pkgconfig ...
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

# ... capture the arguments that control the build ...
ARG CXX=clang++
ARG CC=clang

# ... capture the Travis job number, effectively this busts the cache
# in each build, which we want anyway ...
ARG TRAVIS_JOB_NUMBER=
RUN echo Running build=${TRAVIS_JOB_NUMBER}

# ... copy the contents of the source code directory to the container ...
WORKDIR /var/tmp/build-samples
COPY api /var/tmp/build-samples

# ... use separate RUN commands to speed up debugging in local workstations ...
RUN cmake -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_C_COMPILER=${CC} . && make -j 2
