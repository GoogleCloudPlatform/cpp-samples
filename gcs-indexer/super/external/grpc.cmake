# ~~~
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
# ~~~

include(external/external-project-helpers)
include(external/protobuf)
include(external/c-ares)
find_package(OpenSSL REQUIRED)

if (NOT TARGET grpc-project)
    # Give application developers a hook to configure the version and hash
    # downloaded from GitHub.
    set(GOOGLE_CLOUD_CPP_GRPC_URL
        "https://github.com/grpc/grpc/archive/v1.23.0.tar.gz")
    set(GOOGLE_CLOUD_CPP_GRPC_SHA256
        "f56ced18740895b943418fa29575a65cc2396ccfa3159fa40d318ef5f59471f9")

    google_cloud_cpp_set_prefix_vars()
    externalproject_add(
        grpc-project
        DEPENDS protobuf-project c-ares-project
        EXCLUDE_FROM_ALL ON
        PREFIX "${CMAKE_BINARY_DIR}/external/grpc"
        INSTALL_DIR "${GOOGLE_CLOUD_CPP_EXTERNAL_PREFIX}"
        URL ${GOOGLE_CLOUD_CPP_GRPC_URL}
        URL_HASH SHA256=${GOOGLE_CLOUD_CPP_GRPC_SHA256} LIST_SEPARATOR |
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                   -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
                   -DCMAKE_PREFIX_PATH=${GOOGLE_CLOUD_CPP_PREFIX_PATH}
                   -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                   -DCMAKE_INSTALL_RPATH=${GOOGLE_CLOUD_CPP_INSTALL_RPATH}
                   -DgRPC_BUILD_TESTS=OFF
                   -DgRPC_ZLIB_PROVIDER=package
                   -DgRPC_SSL_PROVIDER=package
                   -DgRPC_CARES_PROVIDER=package
                   -DgRPC_PROTOBUF_PROVIDER=package
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> ${PARALLEL}
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON)
endif ()
