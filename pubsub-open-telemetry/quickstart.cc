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
#include "google/cloud/status.h"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char* argv[]) try {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <project-id> <topic-id>\n";
    return 1;
  }
  std::string const project_id = argv[1];
  std::string const topic_id = argv[2];

  //! [START pubsub_publish_otel_tracing]
  // Create a few namespace aliases to make the code easier to read.
  namespace gc = ::google::cloud;
  namespace otel = gc::otel;
  namespace pubsub = gc::pubsub;

  // This example uses a simple wrapper to export (upload) OTel tracing data
  // to Google Cloud Trace. More complex applications may use different
  // authentication, or configure their own OTel exporter.
  auto project = gc::Project(project_id);
  auto configuration = otel::ConfigureBasicTracing(project);

  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(project_id, topic_id),
      // Configure this publisher to enable OTel tracing. Some applications may
      // chose to disable tracing in some publishers or to dynamically enable
      // this option based on their own configuration.
      gc::Options{}.set<gc::OpenTelemetryTracingOption>(true)));

  // After this point, use the Cloud Pub/Sub C++ client library as usual.
  // In this example, we will send a few messages and configure a callback
  // action for each one.
  std::vector<gc::future<void>> ids;
  for (int i = 0; i < 5; i++) {
    auto id = publisher.Publish(pubsub::MessageBuilder().SetData("Hi!").Build())
                  .then([](gc::future<gc::StatusOr<std::string>> f) {
                    auto id = f.get();
                    if (!id) {
                      std::cout << "Error in publish: " << id.status() << "\n";
                      return;
                    }
                    std::cout << "Sent message with id: (" << *id << ")\n";
                  });
    ids.push_back(std::move(id));
  }
  // Block until the messages  are actually sent.
  for (auto& id : ids) id.get();
  //! [END pubsub_publish_otel_tracing]

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}
