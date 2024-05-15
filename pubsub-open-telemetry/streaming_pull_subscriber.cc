// Copyright 2024 Google LLC
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

//! [START pubsub_subscribe_otel_tracing]
#include "google/cloud/opentelemetry/configure_basic_tracing.h"
#include "google/cloud/opentelemetry_options.h"
#include "google/cloud/pubsub/message.h"
#include "google/cloud/pubsub/publisher.h"
#include "google/cloud/pubsub/subscriber.h"
#include "google/cloud/pubsub/subscription.h"
#include <iostream>

int main(int argc, char* argv[]) try {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <project-id> <topic-id> <subscription-id>\n";
    return 1;
  }

  std::string const project_id = argv[1];
  std::string const topic_id = argv[2];
  std::string const subscription_id = argv[3];

  // Create a few namespace aliases to make the code easier to read.
  namespace gc = ::google::cloud;
  namespace otel = gc::otel;
  namespace pubsub = gc::pubsub;

  auto constexpr kWaitTimeout = std::chrono::seconds(30);

  auto project = gc::Project(project_id);
  auto configuration = otel::ConfigureBasicTracing(project);

  // Publish a message with tracing enabled.
  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(project_id, topic_id),
      gc::Options{}.set<gc::OpenTelemetryTracingOption>(true)));
  // Block until the message is actually sent and throw on error.
  auto id = publisher.Publish(pubsub::MessageBuilder().SetData("Hi!").Build())
                .get()
                .value();
  std::cout << "Sent message with id: (" << id << ")\n";

  // Receive a message using streaming pull with tracing enabled.
  auto subscriber = pubsub::Subscriber(pubsub::MakeSubscriberConnection(
      pubsub::Subscription(project_id, subscription_id),
      gc::Options{}.set<gc::OpenTelemetryTracingOption>(true)));

  auto session =
      subscriber.Subscribe([&](pubsub::Message const& m, pubsub::AckHandler h) {
        std::cout << "Received message " << m << "\n";
        std::move(h).ack();
      });

  std::cout << "Waiting for messages on " + subscription_id + "...\n";

  // Blocks until the timeout is reached.
  auto result = session.wait_for(kWaitTimeout);
  if (result == std::future_status::timeout) {
    std::cout << "timeout reached, ending session\n";
    session.cancel();
  }

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}
//! [END pubsub_subscribe_otel_tracing]
