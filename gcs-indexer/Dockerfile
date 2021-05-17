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
FROM ubuntu:20.04 AS base

# Create separate targets for each phase, this makes the final deployment stage
# small as it contains only what is needed.
FROM base AS devtools

# Install the minimal tools necessary to download vcpkg, and then download it.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y curl gzip tar unzip
WORKDIR /var/tmp/build/vcpkg
RUN curl -s -L https://github.com/Microsoft/vcpkg/archive/2021.05.12.tar.gz | \
    tar -xzf - --strip-components=1

# Install the typical development tools, zip + unzip are used by vcpkg to
# maintain its cache.
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ ninja-build unzip zip
RUN apt install -y pkg-config
RUN ./bootstrap-vcpkg.sh

# Create a separate stage where we build all the dependencies.
FROM devtools AS dependencies

RUN ./vcpkg install --clean-after-build openssl
RUN ./vcpkg install --clean-after-build curl
RUN ./vcpkg install --clean-after-build crc32c
RUN ./vcpkg install --clean-after-build protobuf
RUN ./vcpkg install --clean-after-build abseil
RUN ./vcpkg install --clean-after-build grpc
RUN ./vcpkg install --clean-after-build google-cloud-cpp
RUN ./vcpkg install --clean-after-build boost-program-options
RUN ./vcpkg install --clean-after-build boost-asio
RUN ./vcpkg install --clean-after-build boost-beast
RUN ./vcpkg install --clean-after-build boost-property-tree
RUN ./vcpkg install --clean-after-build boost-serialization

FROM base AS with-certs
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt-get --no-install-recommends install -y libstdc++6 ca-certificates && \
    apt-get upgrade -y && \
    rm -rf /var/cache/apt/*
RUN update-ca-certificates
WORKDIR /r

# Copy the source code to /v/source and compile it.
FROM dependencies AS build
COPY . /v/source
WORKDIR /v/source

# Run the CMake configuration step, setting the options to use vcpkg.
RUN cmake -S/v/source -B/v/binary -GNinja \
    -DCMAKE_TOOLCHAIN_FILE=/var/tmp/build/vcpkg/scripts/buildsystems/vcpkg.cmake

RUN cmake --build /v/binary
RUN strip /v/binary/pubsub_handler

FROM with-certs AS pubsub-handler
WORKDIR /r
COPY --from=build /v/binary/pubsub_handler /r

CMD "/r/pubsub_handler" \
    "--project=${SPANNER_PROJECT}" \
    "--instance=${SPANNER_INSTANCE}" \
    "--database=${SPANNER_DATABASE}"

FROM with-certs AS tools
WORKDIR /r
COPY --from=build /v/binary/create_database /r
COPY --from=build /v/binary/delete_index_for_bucket /r
COPY --from=build /v/binary/refresh_index_for_bucket /r

CMD "/bin/false"
