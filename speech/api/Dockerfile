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

FROM debian:buster AS devtools

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
          curl \
          gcc \
          git-core \
          g++ \
          libtool \
          make \
          pkg-config \
          vim

# Build source dependencies
RUN \
  cd /home/ && \
  git clone https://github.com/grpc/grpc.git && \
  cd grpc/ && \
  git submodule update --init && \
  make -j ${NCPU} && \
  make install && \
  cd third_party/protobuf/ && \
  make -j ${NCPU} install && \
  cd /home/ && \
  git clone https://github.com/googleapis/googleapis.git && \
  cd googleapis/ && \
  LANGUAGE=cpp make -j ${NCPU} all

FROM devtools AS cpp-speech
COPY . /home/speech/api
RUN cd /home/speech/api && make
