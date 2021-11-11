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
project(cpp-samples-getting-started-update CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(functions_framework_cpp REQUIRED)
find_package(google_cloud_cpp_spanner REQUIRED)

add_library(
  functions_framework_cpp_function EXCLUDE_FROM_ALL # cmake-format: sortable
                                                    update_gcs_index.cc)
target_link_libraries(
  functions_framework_cpp_function PUBLIC functions-framework-cpp::framework
                                          google-cloud-cpp::spanner)
target_compile_features(functions_framework_cpp_function PUBLIC cxx_std_17)
