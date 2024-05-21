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

# We chose Alpine to build the image because it has good support for creating
# statically-linked, small programs.
FROM alpine:3.19 AS build

# Install the typical development tools for C++, and
# the base OS headers and libraries.
RUN apk update && \
    apk add \
        build-base \
        cmake \
        curl \
        git \
        gcc \
        g++ \
        libc-dev \
        linux-headers \
        ninja \
        pkgconfig \
        tar \
        unzip \
        zip

# Copy the source code to /src and compile it.
COPY . /src
WORKDIR /src

# Run the CMake configuration step, setting the options to create
# a statically linked C++ program
RUN cmake -S /src -B /build -GNinja \
    -DCMAKE_BUILD_TYPE=Release

# Compile the binary and strip it to reduce its size.
RUN cmake --build /build
RUN strip /build/finsim

# Make the program the entry point.
ENTRYPOINT [ "/build/finsim" ]
CMD [ "input.txt"]
