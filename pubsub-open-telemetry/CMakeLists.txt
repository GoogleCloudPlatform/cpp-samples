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
project(pubsub-open-telemetry CXX)

find_package(google_cloud_cpp_pubsub CONFIG REQUIRED)
find_package(google_cloud_cpp_opentelemetry CONFIG REQUIRED)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(opentelemetry-cpp CONFIG REQUIRED)

add_library(parse_args parse_args.cc parse_args.h)
target_compile_features(parse_args PUBLIC cxx_std_14)
target_link_libraries(
    parse_args PUBLIC Boost::program_options google-cloud-cpp::pubsub
                      google-cloud-cpp::opentelemetry)

add_library(publisher_helper publisher_helper.cc publisher_helper.h)
target_compile_features(publisher_helper PUBLIC cxx_std_14)
target_link_libraries(publisher_helper PUBLIC parse_args opentelemetry-cpp::api
                                              opentelemetry-cpp::sdk)

add_executable(blocking_publisher blocking_publisher.cc)
target_compile_features(blocking_publisher PRIVATE cxx_std_14)
target_link_libraries(blocking_publisher PRIVATE parse_args)

add_executable(publisher_jaeger publisher_jaeger.cc)
target_compile_features(publisher_jaeger PRIVATE cxx_std_14)
target_link_libraries(
    publisher_jaeger
    PRIVATE publisher_helper parse_args opentelemetry-cpp::otlp_grpc_client
            opentelemetry-cpp::otlp_grpc_exporter)

add_executable(publisher publisher.cc)
target_compile_features(publisher PRIVATE cxx_std_14)
target_link_libraries(publisher PRIVATE publisher_helper parse_args)

add_executable(publisher_zipkin publisher_zipkin.cc)
target_compile_features(publisher_zipkin PRIVATE cxx_std_14)
target_link_libraries(
    publisher_zipkin
    PRIVATE publisher_helper parse_args
            opentelemetry-cpp::opentelemetry_exporter_zipkin_trace)

add_executable(quickstart quickstart.cc)
target_compile_features(quickstart PRIVATE cxx_std_14)
target_link_libraries(quickstart PRIVATE google-cloud-cpp::pubsub
                                         google-cloud-cpp::opentelemetry)

add_executable(unary_pull_subscriber unary_pull_subscriber.cc)
target_compile_features(unary_pull_subscriber PRIVATE cxx_std_14)
target_link_libraries(
    unary_pull_subscriber PRIVATE google-cloud-cpp::pubsub
                                  google-cloud-cpp::opentelemetry)

add_executable(streaming_pull_subscriber streaming_pull_subscriber.cc)
target_compile_features(streaming_pull_subscriber PRIVATE cxx_std_14)
target_link_libraries(
    streaming_pull_subscriber PRIVATE google-cloud-cpp::pubsub
                                      google-cloud-cpp::opentelemetry)
