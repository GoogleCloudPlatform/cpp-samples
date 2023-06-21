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
project(cpp-samples-iot-mqtt-ciotc C)

find_package(OpenSSL REQUIRED)
find_package(jansson REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)

# libjwt is not available through vcpkg, we use FetchContent instead.
include(FetchContent)
FetchContent_Declare(
    libjwt
    URL https://github.com/benmcollins/libjwt/archive/b6849cbea22f43bbd2f115c3bab55ad903272c62.tar.gz
)
FetchContent_GetProperties(libjwt)
if (NOT libjwt_POPULATED)
    FetchContent_Populate(libjwt)
    add_subdirectory(${libjwt_SOURCE_DIR} ${libjwt_BINARY_DIR})
endif ()

add_executable(mqtt_ciotc mqtt_ciotc.c)
target_link_libraries(
    mqtt_ciotc
    PRIVATE jwt jansson::jansson eclipse-paho-mqtt-c::paho-mqtt3cs-static
            unofficial::UUID::uuid)
target_include_directories(mqtt_ciotc PRIVATE "${libjwt_SOURCE_DIR}/include")
