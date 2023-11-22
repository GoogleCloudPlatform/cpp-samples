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

#include "google/cloud/opentelemetry/configure_basic_tracing.h"
#include "google/cloud/opentelemetry_options.h"
#include "google/cloud/pubsub/publisher.h"
#include <iostream>
#include <string_view>

// Create a few namespace aliases to make the code easier to read.
namespace gc = ::google::cloud;
namespace pubsub = gc::pubsub;
namespace otel = gc::otel;

int main(int argc, char* argv[]) try {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <project-id> <topic-id>\n";
    return 1;
  }
  std::string const project_id = argv[1];
  std::string const topic_id = argv[2];
  auto project = gc::Project(project_id);

  //! [START pubsub_publish_otel_tracing]
  auto configuration = otel::ConfigureBasicTracing(project);
  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(project_id, topic_id),
      gc::Options{}.set<gc::OpenTelemetryTracingOption>(true)));

  std::vector<gc::future<void>> ids;
  for (int i = 0; i < 5; i++) {
    auto id = publisher.Publish(pubsub::MessageBuilder().SetData("Hi!").Build())
                  .then([](gc::future<gc::StatusOr<std::string>> f) {
                    auto status = f.get();
                    if (!status) {
                      std::cout << "Error in publish: " << status.status()
                                << "\n";
                      return;
                    }
                    std::cout << "Sent message with id: (" << *status << ")\n";
                  });
    ids.push_back(std::move(id));
  }
  // Block until they are actually sent.
  for (auto& id : ids) id.get();
  //! [END pubsub_publish_otel_tracing]

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}
