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

cmake_minimum_required(VERSION 3.20)

# Define the project name and where to report bugs.
set(PACKAGE_BUGREPORT
    "https://github.com/GoogleCloudPlatform/cpp-samples/issues")
project(pubsub-avro CXX)

find_package(google_cloud_cpp_pubsub CONFIG REQUIRED)
find_package(unofficial-avro-cpp CONFIG REQUIRED)

# Generate the avro C++ files using the avro compiler.
find_program(AVROGENCPP NAMES avrogencpp)
macro (run_avro_compiler file namespace)
    add_custom_command(
        OUTPUT ${file}.hh
        COMMAND ${AVROGENCPP} --input ${CMAKE_CURRENT_SOURCE_DIR}/${file}.avro
                --output ${file}.hh --namespace ${namespace}
        DEPENDS ${AVROGENCPP} ${CMAKE_CURRENT_SOURCE_DIR}/${file}.avro
        COMMENT "Executing Avro compiler")
    set_source_files_properties(${file}.hh PROPERTIES GENERATED TRUE)
endmacro (run_avro_compiler)

run_avro_compiler(schema1 v1)
run_avro_compiler(schema2 v2)

add_executable(quickstart quickstart.cc schema1.hh schema2.hh)
target_compile_features(quickstart PRIVATE cxx_std_14)
target_include_directories(quickstart PRIVATE SYSTEM
                                              "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(quickstart PRIVATE google-cloud-cpp::pubsub
                                         unofficial::avro-cpp::avrocpp)
