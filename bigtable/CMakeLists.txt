# Cloud Bigtable C++ API examples cmake file
#
# Copyright 2017, Google Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

cmake_minimum_required(VERSION 2.8)

set(PACKAGE_NAME      "cpp-docs-samples-bigtable")
set(PACKAGE_VERSION   "0.1")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME   "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/GoogleCloudPlatform/cpp-docs-samples")
project(${PACKAGE_NAME} CXX)
set(CXX_STANDARD_REQUIRED 14)

include(FindPkgConfig)
pkg_check_modules(GRPCPP REQUIRED grpc++)
pkg_check_modules(GRPC REQUIRED grpc)

add_executable(create_cluster create_cluster.cc)
install (TARGETS create_cluster DESTINATION bin)
