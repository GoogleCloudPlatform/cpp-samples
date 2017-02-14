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

# Dockerfile for gcr.io/google-samples/grpc-cxx
# Official image is here https://hub.docker.com/r/grpc/cxx/
# This image is based on Fedora to allow easy building of static binaries
FROM fedora:25

ENV GRPC_RELEASE_TAG v1.1.0

RUN dnf -y install which make gcc gcc-c++ autoconf automake libtool git && \
    git clone -b ${GRPC_RELEASE_TAG} https://github.com/grpc/grpc /var/local/git/grpc && \
    cd /var/local/git/grpc && \
    git submodule update --init && \
    make && make install && make clean && \
    cd /var/local/git/grpc/third_party/protobuf && \
    make && make install && make clean
