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

cmake_minimum_required(VERSION 3.20)
project(cpp-samples-getting-started CXX)

find_package(functions_framework_cpp REQUIRED)
find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(google_cloud_cpp_spanner REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)

add_library(gcs_indexing # cmake-format: sort
            gcs_indexing.cc gcs_indexing.h)
target_link_libraries(gcs_indexing PUBLIC google-cloud-cpp::spanner
                                          google-cloud-cpp::storage)
target_include_directories(gcs_indexing PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(gcs_indexing PUBLIC cxx_std_17)

add_library(functions_framework_cpp_function # cmake-format: sort
            index_gcs_prefix.cc)
target_link_libraries(
    functions_framework_cpp_function
    PRIVATE gcs_indexing
    PUBLIC functions-framework-cpp::framework google-cloud-cpp::pubsub
           google-cloud-cpp::spanner google-cloud-cpp::storage)
target_compile_features(functions_framework_cpp_function PUBLIC cxx_std_17)

add_executable(gke_index_gcs gke/index_gcs.cc)
target_link_libraries(
    gke_index_gcs PRIVATE gcs_indexing google-cloud-cpp::pubsub
                          google-cloud-cpp::spanner google-cloud-cpp::storage)

add_library(update_gcs_index # cmake-format: sort
            update/update_gcs_index.cc)
target_link_libraries(update_gcs_index PUBLIC functions-framework-cpp::framework
                                              google-cloud-cpp::spanner)
target_compile_features(update_gcs_index PUBLIC cxx_std_17)
