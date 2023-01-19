# ~~~
# Copyright 2023 Google LLC
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

cmake_minimum_required(VERSION 3.10...3.24)
project(setup-cpp-cmake CXX)

# [START cpp_setup_cmake_require_package]
find_package(google_cloud_cpp_storage REQUIRED)
# [END cpp_setup_cmake_require_package]
find_package(Threads)

add_executable(hello_world hello_world.cc)
target_compile_features(hello_world PUBLIC cxx_std_14)

# [START cpp_setup_cmake_use_package]
target_link_libraries(hello_world PUBLIC google-cloud-cpp::storage)
# [END cpp_setup_cmake_use_package]
