# ~~~
# Copyright 2022 Google LLC
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

cmake_minimum_required(VERSION 3.20)

# Define the project name and where to report bugs.
set(PACKAGE_BUGREPORT
    "https://github.com/GoogleCloudPlatform/cpp-samples/issues")
project(cpp-samples-bigquery-write CXX)

find_package(google_cloud_cpp_bigquery REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(Threads)

protobuf_generate_cpp(SCHEMA_SRCS SCHEMA_HDRS schema.proto)

add_executable("single_threaded_write" ${SCHEMA_SRCS} single_threaded_write.cc)
target_include_directories(single_threaded_write SYSTEM
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries("single_threaded_write" PRIVATE google-cloud-cpp::bigquery
                                                      Threads::Threads)
