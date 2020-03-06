# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu:18.04 AS devtools

ENV GOOGLE_APPLICATION_CREDENTIALS /home/service-account.json
ENV GOOGLEAPIS_GENS_PATH /home/googleapis/gens
ARG NCPU=4

# Dependencies
RUN \
  apt-get update && \
  apt-get -y install \
          autoconf \
          build-essential \
          clang-tidy \
          cmake \
          curl \
          gcc \
          git-core \
          golang \
          g++ \
          libtool \
          libssl-dev \
          make \
          pkg-config \
          wget \
          vim \
          zlib1g-dev

# Install c-ares
RUN \
  mkdir -p ${HOME}/Downloads && \
  cd ${HOME}/Downloads && \
  wget -q https://github.com/c-ares/c-ares/archive/cares-1_15_0.tar.gz && \
  tar -xf cares-1_15_0.tar.gz && \
  cd c-ares-cares-1_15_0 && \
  mkdir build && cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
  make install && \
  ldconfig


RUN \
  cd ${HOME}/Downloads && \
  wget -q https://github.com/google/protobuf/archive/v3.11.2.tar.gz && \
  tar -xf v3.11.2.tar.gz && \
  cd protobuf-3.11.2/cmake && \
  cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=yes \
        -Dprotobuf_BUILD_TESTS=OFF \
        -H. -Bcmake-out && \
    cmake --build cmake-out -- -j ${NCPU:-4} && \
    cmake --build cmake-out --target install -- -j ${NCPU:-4} && \
    ldconfig

# Build source dependencies
RUN \
  cd /home/ && \
  git clone https://github.com/grpc/grpc.git && \
  cd grpc/ && \
  git checkout v1.26.0 && \
  git submodule update --init && \
  mkdir -p cmake/build && \
  cd cmake/build && \
  cmake ../.. \
    -DgRPC_CARES_PROVIDER=package \
    -DgRPC_PROTOBUF_PROVIDER=package \
    -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package \
    -DgRPC_INSTALL=on && \
  make -j ${NCPU} && \
  make install && \
  cd /home/ && \
  git clone https://github.com/googleapis/googleapis.git && \
  cd googleapis/ && \
  LANGUAGE=cpp make -j ${NCPU} all

FROM devtools AS cpp-speech
COPY . /home/speech/api
RUN cd /home/speech/api && make
