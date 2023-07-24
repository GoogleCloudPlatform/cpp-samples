# Copyright 2020 Google LLC
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

# We chose Ubuntu to build the image because we are familiar with it.
FROM ubuntu:22.04 AS base

# Create separate targets for each phase, this makes the final deployment stage
# small as it contains only what is needed.
FROM base AS devtools

# Install the minimal tools necessary to download vcpkg, and then download it.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y curl gzip tar unzip
WORKDIR /var/tmp/build/vcpkg
RUN curl -s -L https://github.com/Microsoft/vcpkg/archive/2023.07.21.tar.gz | \
    tar -xzf - --strip-components=1

# Install the typical development tools, zip + unzip are used by vcpkg to
# maintain its cache.
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ python3 ninja-build unzip zip
RUN apt install -y pkg-config
RUN ./bootstrap-vcpkg.sh

# Create a separate stage where we build all the dependencies.
FROM devtools AS dependencies

RUN ./vcpkg install openssl
RUN ./vcpkg install curl
RUN ./vcpkg install crc32c
RUN ./vcpkg install protobuf
RUN ./vcpkg install abseil
RUN ./vcpkg install grpc
RUN ./vcpkg install google-cloud-cpp
RUN ./vcpkg install boost-program-options

# Copy the source code to /v/source and compile it.
FROM dependencies AS build
COPY . /v/source
WORKDIR /v/source

# Run the CMake configuration step, setting the options to use vcpkg.
RUN cmake -S/v/source -B/v/binary -GNinja \
    -DCMAKE_TOOLCHAIN_FILE=/var/tmp/build/vcpkg/scripts/buildsystems/vcpkg.cmake

# Compile the binary and strip it to reduce its size.
RUN cmake --build /v/binary
RUN strip /v/binary/populate_bucket

# Create the final deployment image, we need the CA certificates, as the
# application needs to verify the Google SSL certificates.
FROM base AS populate-bucket
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt-get --no-install-recommends install -y libstdc++6 ca-certificates && \
    rm -rf /var/cache/apt/*
WORKDIR /r

# Copy the program from the previously created stage and make it the entry point.
COPY --from=build /v/binary/populate_bucket /r

# Make the image fail if invoked with an explicit command line.
CMD "/bin/false"
