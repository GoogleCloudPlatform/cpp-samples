# ~~~
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
# ~~~

cmake_minimum_required(VERSION 3.10)

# Define the project name and where to report bugs.
set(PACKAGE_BUGREPORT
    "https://github.com/GoogleCloudPlatform/cpp-samples/issues")
project(GCS-Fast-Transfers CXX)

# Configure the Compiler options, we will be using C++17 features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(google_cloud_cpp_storage REQUIRED)
find_package(Crc32c REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(Threads)

add_library(gcs_fast_transfers STATIC gcs_fast_transfers.cc
                                      gcs_fast_transfers.h)
target_link_libraries(gcs_fast_transfers PRIVATE Boost::headers Crc32c::crc32c)

add_executable(download download.cc)
target_link_libraries(
  download PRIVATE gcs_fast_transfers google-cloud-cpp::storage
                   Boost::program_options fmt::fmt Threads::Threads)

add_executable(upload upload.cc)
target_link_libraries(
  upload PRIVATE gcs_fast_transfers google-cloud-cpp::storage
                 Boost::program_options fmt::fmt Threads::Threads)

find_package(CURL REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

include(GNUInstallDirs)
install(TARGETS download upload RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
