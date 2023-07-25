# Copyright 2021 Google LLC
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

FROM ubuntu:22.04 AS base

# Create separate targets for each phase, this allows us to cache intermediate
# stages when using Google Cloud Build, and makes the final deployment stage
# small as it contains only what is needed.
FROM base AS devtools

# Install the typical development tools and some additions:
#   - ninja-build is a backend for CMake that often compiles faster than
#     CMake with GNU Make.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt install -y \
        build-essential \
        cmake \
        curl \
        gcc \
        g++ \
        git \
        ninja-build \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip

# Copy the source code to /v/source and compile it.
FROM devtools AS build

WORKDIR /v/vcpkg
RUN curl -sSL "https://github.com/Microsoft/vcpkg/archive/2023.07.21.tar.gz" | \
    tar --strip-components=1 -zxf - \
    && ./bootstrap-vcpkg.sh -disableMetrics

COPY vcpkg.json /v/deps/
RUN /v/vcpkg/vcpkg install --x-manifest-root=/v/deps  --clean-after-build

# Run the CMake configuration step.
COPY . /v/source
RUN cmake -S/v/source -B/v/binary -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/v/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && cmake --build /v/binary --target gke_index_gcs \
    && strip /v/binary/gke_index_gcs

# Create the final deployment image, install the minimal dependencies for the
# program.
FROM base AS getting-started-gke

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt-get --no-install-recommends install -y libstdc++6 ca-certificates && \
    rm -rf /var/cache/apt/*

WORKDIR /r

# Copy the program from the previously created stage and make it the entry point.
COPY --from=build /v/binary/gke_index_gcs /r

ENTRYPOINT [ "/r/gke_index_gcs" ]
