# Copyright 2024 Google LLC
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
FROM ubuntu:22.04 as base

FROM base AS build
# Install the minimal tools necessary to download vcpkg, and then download it.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y curl gzip tar unzip
WORKDIR /var/tmp/build/vcpkg
RUN curl -s -L https://github.com/Microsoft/vcpkg/archive/2024.03.25.tar.gz | \
    tar -xzf - --strip-components=1

# Install the typical development tools, zip + unzip are used by vcpkg to
# maintain its cache.
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ python3 ninja-build unzip zip
RUN apt install -y pkg-config
RUN ./bootstrap-vcpkg.sh

RUN ./vcpkg install google-cloud-cpp

# Copy the source code to /src and compile it.
WORKDIR /src
COPY simple.cc /src
COPY CMakeLists.txt /src
COPY vcpkg.json /src

# Run the CMake configuration step, setting the options to use vcpkg.
RUN cmake -S . -B /build -GNinja \
    -DCMAKE_TOOLCHAIN_FILE=/var/tmp/build/vcpkg/scripts/buildsystems/vcpkg.cmake

# Compile the binary and strip it to reduce its size.
RUN cmake --build /build
RUN strip /build/simple

# Create the final deployment image, we need the CA certificates, as the
# application needs to verify the Google SSL certificates.
FROM base AS batch-simple
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt-get --no-install-recommends install -y libstdc++6 ca-certificates && \
    rm -rf /var/cache/apt/*

WORKDIR /root
# Copy the program from the previously created stage and input file.
COPY --from=build /build/simple /root
COPY hello-world-container.json /root

# Define some env variables
ENV PROJECT_ID=cloud-cpp-testing-resources
ENV REGION_ID=us-central1
ENV JOB_ID=test-container-run
ENV JOB_FILE=hello-world-container.json

# Create an entrypoint for the program.
ENTRYPOINT ./simple "$PROJECT_ID" "$REGION_ID" "$JOB_ID" "$JOB_FILE"
