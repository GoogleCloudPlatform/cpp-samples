// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PARSE_ARGUMENTS_H
#define CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PARSE_ARGUMENTS_H

#include "google/cloud/options.h"
#include <optional>
#include <string>

namespace gc = ::google::cloud;

// Parse the command line arguments.
struct ParseResult {
  // Required.
  std::string project_id;
  std::string topic_id;

  // Optional with defaults set.
  int message_count;
  int message_size;
  int max_queue_size;

  gc::Options otel_options;
  gc::Options publisher_options;
};

ParseResult ParseArguments(int argc, char* argv[]);

#endif  // CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PARSE_ARGUMENTS_H
