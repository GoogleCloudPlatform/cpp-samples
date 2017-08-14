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

# ... configure the Compiler options, we will be using C++14 features,
# so enable that ...
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ... the author is paranoid.  Turn on all available warnings
# and turn warnings into errors to stop the build if any warning is
# emitted ...
include(CheckCXXCompilerFlag)

if(NOT MSVC)
  CHECK_CXX_COMPILER_FLAG(-Werror COMPILER_SUPPORTS_WERROR)
  if(COMPILER_SUPPORTS_WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif()
  CHECK_CXX_COMPILER_FLAG(-Wall COMPILER_SUPPORTS_WALL)
  if(COMPILER_SUPPORTS_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif()
endif()

# ... include the functions to compile proto files ...
include(FindProtobuf)

# ... find the grpc and grpc++ libraries ...
if(WIN32)
  # ... use find_package and vcpkg on Windows ...
  find_package(GRPC REQUIRED grpc>=1.4)
  find_package(PROTOBUF REQUIRED protobuf>=3.0)
  link_directories(${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})
  include_directories(${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})
  set(GRPC_LIBRARIES gRPC::grpc++ gRPC::grpc)
  set(PROTOBUF_LIBRARIES protobuf::libprotobuf)
  # Use the same settings that gRPC uses...
  add_definitions(-D_WIN32_WINNT=0x600 -D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
  if(MSVC)
    add_definitions(/wd4065 /wd4506 /wd4267 /wd4800 /wd4291 /wd4838)
    if(VCPKG_TARGET_TRIPLET MATCHES "-static$")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif()
  endif()
else()
  # ... use pkg-config on Linux and Mac OSX ...
  include(FindPkgConfig)
  pkg_check_modules(GRPCPP REQUIRED grpc++>=1.4.1)
  pkg_check_modules(GRPC REQUIRED grpc>=4.0)
  pkg_check_modules(PROTOBUF REQUIRED protobuf>=3.0)
  link_directories(${GRPCPP_LIBRARY_DIRS} ${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})
  include_directories(${GRPCPP_INCLUDE_DIRS} ${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})
endif()

include(cmake/CompileProtos.cmake)

set(PROTOBUF_IMPORT_DIRS "${PROJECT_SOURCE_DIR}/googleapis" "${PROJECT_SOURCE_DIR}")
PROTOBUF_GENERATE_CPP(PROTO_SOURCES PROTO_HDRS
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/bigtable_instance_admin.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/bigtable_table_admin.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/common.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/instance.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/table.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/v2/bigtable.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/v2/data.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/longrunning/operations.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/rpc/status.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/api/annotations.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/api/auth.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/api/http.proto)
GRPC_GENERATE_CPP(GRPCPP_SOURCES GRPCPP_HDRS
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/bigtable_instance_admin.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/admin/v2/bigtable_table_admin.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/bigtable/v2/bigtable.proto
        ${PROJECT_SOURCE_DIR}/googleapis/google/longrunning/operations.proto)
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

################################################################
# Create targets here ...

# ... discover all the generated Google API files and turn them into a
# static library ...
add_library(googleapis ${PROTO_SOURCES} ${PROTO_HDRS} ${GRPCPP_SOURCES} ${GRPCPP_HDRS})

add_executable(list_instances list_instances.cc)
target_link_libraries(list_instances googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(create_instance create_instance.cc)
target_link_libraries(create_instance googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(delete_instance delete_instance.cc)
target_link_libraries(delete_instance googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(create_table create_table.cc)
target_link_libraries(create_table googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(delete_table delete_table.cc)
target_link_libraries(delete_table googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(list_tables list_tables.cc)
target_link_libraries(list_tables googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

PROTOBUF_GENERATE_CPP(TAQ_PROTO_SRCS TAQ_PROTO_HDRS ${PROJECT_SOURCE_DIR}/taq.proto)

add_executable(upload_taq upload_taq.cc parse_taq_line.cc ${TAQ_PROTO_SRCS} ${TAQ_PROTO_HDRS})
target_link_libraries(upload_taq googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(upload_taq_batch upload_taq_batch.cc parse_taq_line.cc ${TAQ_PROTO_SRCS} ${TAQ_PROTO_HDRS})
target_link_libraries(upload_taq_batch googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

add_executable(read_rows read_rows.cc ${TAQ_PROTO_SRCS} ${TAQ_PROTO_HDRS})
target_link_libraries(read_rows googleapis ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
