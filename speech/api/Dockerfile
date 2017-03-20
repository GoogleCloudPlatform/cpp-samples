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

FROM debian:jessie

# Copy in service-account.json into /home/
COPY service-account.json /home/service-account.json
ENV GOOGLE_APPLICATION_CREDENTIALS /home/service-account.json
ENV GOOGLEAPIS_GENS_PATH /home/googleapis/gens

# Dependencies
RUN \
  apt-get update && \
  apt-get -y install \
          vim \
          curl \
          git-core \
          make \
          gcc \
          g++ && \
  apt-get -y update && \
  apt-get -y install \
          build-essential \
          autoconf \
          libtool \
          pkg-config

# Build source dependencies
RUN \
  cd /home/ && \
  git clone https://github.com/grpc/grpc.git && \
  cd grpc/ && \
  git submodule update --init && \
  make && \
  make install && \
  cd third_party/protobuf/ && \
  make install && \
  cd /home/ && \
  git clone https://github.com/googleapis/googleapis.git && \
  cd googleapis/ && \
  LANGUAGE=cpp make all

# Build Speech API
Run \
  cd /home/ && \
  git clone https://github.com/GoogleCloudPlatform/cpp-docs-samples.git && \
  cd cpp-docs-samples/speech/api && \
  make

