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

cmake_minimum_required(VERSION 3.5)
project(cpp-samples-getting-started CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(functions_framework_cpp REQUIRED)
find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(google_cloud_cpp_spanner REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)

add_library(
  functions_framework_cpp_function # cmake-format: sortable
  gcs_indexing.cc gcs_indexing.h index_gcs_prefix.cc)
target_link_libraries(
  functions_framework_cpp_function functions-framework-cpp::framework
  google-cloud-cpp::pubsub google-cloud-cpp::spanner google-cloud-cpp::storage)
