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
project(cpp-samples-speech-api CXX)

# Configure the Compiler options, we will be using C++11 features.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(google_cloud_cpp_googleapis REQUIRED)
find_package(Threads)

add_library(parse_arguments STATIC parse_arguments.cc parse_arguments.h)
target_link_libraries(parse_arguments
                      PUBLIC google-cloud-cpp::cloud_speech_protos)

foreach(target async_transcribe streaming_transcribe
               streaming_transcribe_singlethread transcribe)
  add_executable("${target}" "${target}.cc")
  target_link_libraries(
    "${target}" PRIVATE parse_arguments google-cloud-cpp::cloud_speech_protos
                        Threads::Threads)
endforeach()
