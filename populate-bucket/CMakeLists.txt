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

cmake_minimum_required(VERSION 3.20)

# Define the project name and where to report bugs.
set(PACKAGE_BUGREPORT
    "https://github.com/GoogleCloudPlatform/cpp-samples/issues")
project(populate-bucket CXX)

find_package(google_cloud_cpp_pubsub REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(Threads)

add_executable(populate_bucket populate_bucket.cc)
target_compile_features(populate_bucket PRIVATE cxx_std_17)
target_link_libraries(
    populate_bucket PRIVATE google-cloud-cpp::pubsub google-cloud-cpp::storage
                            Boost::program_options Threads::Threads)

include(GNUInstallDirs)
install(TARGETS populate_bucket RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
