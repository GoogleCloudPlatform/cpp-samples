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

# Define the project name and where to report bugs.
set(PACKAGE_BUGREPORT
    "https://github.com/GoogleCloudPlatform/cpp-samples/issues")
project(cpp-samples-speech-api CXX)

find_package(google_cloud_cpp_speech REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(Threads)

add_library(parse_arguments STATIC parse_arguments.cc parse_arguments.h)
target_compile_features(parse_arguments PUBLIC cxx_std_14)
target_link_libraries(
    parse_arguments PUBLIC Boost::program_options
                           google-cloud-cpp::cloud_speech_protos)
if (MSVC)
    # The protobuf-generated files have warnings under the default MSVC
    # settings. We are not interested in these warnings, because we cannot fix
    # them.
    target_compile_options(parse_arguments PUBLIC "/wd4244" "/wd4251")
endif ()

foreach (target async_transcribe streaming_transcribe
                streaming_transcribe_singlethread transcribe)
    add_executable("${target}" "${target}.cc")
    target_link_libraries(
        "${target}" PRIVATE parse_arguments google-cloud-cpp::speech
                            Boost::program_options Threads::Threads)
endforeach ()

if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    AND ("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL 11))
    add_executable(streaming_transcribe_coroutines
                   streaming_transcribe_coroutines.cc)
    target_compile_features(streaming_transcribe_coroutines PUBLIC cxx_std_20)
    target_compile_options(streaming_transcribe_coroutines
                           PUBLIC "-fcoroutines")
    target_link_libraries(
        streaming_transcribe_coroutines
        PRIVATE parse_arguments google-cloud-cpp::speech Boost::program_options
                Threads::Threads)
endif ()
