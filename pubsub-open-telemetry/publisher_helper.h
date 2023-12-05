// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PUBLISHER_H
#define CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PUBLISHER_H

#include "google/cloud/pubsub/publisher.h"
#include "parse_args.h"

// Create a publisher using the configuration set in `args`.
::google::cloud::pubsub::Publisher CreatePublisher(ParseResult const& args);

// Publish message(s) using the `publisher` set in `args`.
void Publish(::google::cloud::pubsub::Publisher& publisher,
             ParseResult const& args);

// Wait for the traces to be exported before exiting the program.
void Cleanup();

#endif  // CPP_SAMPLES_PUBSUB_OPEN_TELEMETRY_PUBLISHER_H
