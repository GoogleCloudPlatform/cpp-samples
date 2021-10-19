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

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y build-essential git gcc g++ clang llvm cmake ninja-build pkg-config python3 tar zip unzip curl

RUN echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] http://packages.cloud.google.com/apt cloud-sdk main" | \
    tee -a /etc/apt/sources.list.d/google-cloud-sdk.list \
    && curl -sSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | \
    apt-key --keyring /usr/share/keyrings/cloud.google.gpg  add - \
    && apt-get update -y \
    && apt-get install google-cloud-sdk -y

WORKDIR /usr/local/vcpkg
# Pin vcpkg to a version including `google-cloud-cpp-v1.32.1`
RUN curl -sSL "https://github.com/microsoft/vcpkg/archive/b8f91d13f3494cd673776aabdc504f5c736fdba2.tar.gz" | \
    tar --strip-components=1 -zxf - \
    && ./bootstrap-vcpkg.sh
