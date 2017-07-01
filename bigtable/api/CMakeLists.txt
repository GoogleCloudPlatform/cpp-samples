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
cmake_minimum_required(VERSION 3.5)

# ... define the project name, version, and main programming language ...
set(PACKAGE_NAME      "cpp-docs-samples-bigtable-api")
set(PACKAGE_VERSION   "0.1")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME   "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/GoogleCloudPlatform/cpp-docs-samples/issues")
project(${PACKAGE_NAME} CXX C)

# ... configure the Compiler options, we will be using C++11 features,
# so enable that, both for newer cmake versions ...
set(CXX_STANDARD_REQUIRED 11)
# ... and for older ones ...
set(CMAKE_CXX_STANDARD ${CXX_STANDARD_REQUIRED})

# ... the author is paranoid.  Turn on all available warnings
# and turn warnings into errors to stop the build if any warning is
# emitted ...
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Werror COMPILER_SUPPORTS_WERROR)
if(COMPILER_SUPPORTS_WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()
CHECK_CXX_COMPILER_FLAG(-Wall COMPILER_SUPPORTS_WALL)
if(COMPILER_SUPPORTS_WALL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
CHECK_CXX_COMPILER_FLAG(/WX COMPILER_SUPPORTS_WX)
if(COMPILER_SUPPORTS_WX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
endif()
CHECK_CXX_COMPILER_FLAG(/W4 COMPILER_SUPPORTS_SWALL)
if(COMPILER_SUPPORTS_SWALL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
endif()

# ... include the functions to compile proto files ...
include(FindProtobuf)

# ... find the grpc and grpc++ libraries using pkg-config ...
include(FindPkgConfig)
pkg_check_modules(GRPCPP REQUIRED grpc++)
pkg_check_modules(GRPC REQUIRED grpc)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

# ... define where the googleapis protos can be found, and add that
# directory to the search path for header files ...
# Typically you can generate these using:
#  $ make -C googleapis OUTPUT=$PWD/googleapis-gens
set(GOOGLEAPIS_PATH "${CMAKE_SOURCE_DIR}/googleapis")
set(GOOGLEAPIS_GENS_PATH "${GOOGLEAPIS_PATH}-gens")
include_directories("${GOOGLEAPIS_GENS_PATH}")

################################################################
# Create targets here ...

# ... discover all the generated Google API files and turn them into a
# static library ...
file(GLOB_RECURSE GOOGLEAPIS_SRCS ${GOOGLEAPIS_GENS_PATH}/*.pb.cc)
file(GLOB_RECURSE GOOGLEAPIS_HDRS ${GOOGLEAPIS_GENS_PATH}/*.pb.h)
add_library(googleapis ${GOOGLEAPIS_SRCS} ${GOOGLEAPIS_HDRS})

# A very simple example, list the instances for a give project
add_executable(list_instances list_instances.cc)
target_link_libraries(list_instances googleapis grpc++ grpc protobuf)
