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

cmake_minimum_required(VERSION 3.20...3.23)
project(cpp-samples CXX C)

include(ExternalProject)

set(samples
    # cmake-format: sort
    bigquery/write
    cloud-run-hello-world
    gcs-fast-transfers
    getting-started
    getting-started/update
    iot/mqtt-ciotc
    populate-bucket
    setup
    speech/api)

add_custom_target(verify-samples ALL)

function (add_sample_directory dir)
    string(REPLACE "/" "-" name "${dir}")

    externalproject_add(
        verify-sample-${name}
        EXCLUDE_FROM_ALL ON
        PREFIX "${PROJECT_BINARY_DIR}/cmake-prefix-${name}"
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/${dir}"
        BINARY_DIR "${PROJECT_BINARY_DIR}/${dir}"
        CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "<BINARY_DIR>"
        BUILD_ALWAYS ON
        INSTALL_COMMAND ""
        # Use LOG_* ON to keep builds quiet on success
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_OUTPUT_ON_FAILURE ON)
    add_dependencies(verify-samples verify-sample-${name})
endfunction ()

foreach (sample IN LISTS samples)
    add_sample_directory("${sample}")
endforeach ()
