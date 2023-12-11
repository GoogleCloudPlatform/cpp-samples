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

#include "google/cloud/opentelemetry/trace_exporter.h"
#include "google/cloud/pubsub/blocking_publisher.h"
#include "google/cloud/pubsub/message.h"
#include "google/cloud/pubsub/topic.h"
#include "parse_args.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include <iostream>

// Create a few namespace aliases to make the code easier to read.
namespace gc = ::google::cloud;
namespace pubsub = gc::pubsub;
namespace otel = gc::otel;
namespace trace_sdk = ::opentelemetry::sdk::trace;
namespace trace = ::opentelemetry::trace;

namespace {

void ConfigureCloudTraceTracer(ParseResult const& args) {
  auto exporter = otel::MakeTraceExporter(gc::Project(args.project_id));
  trace_sdk::BatchSpanProcessorOptions span_options;
  span_options.max_queue_size = args.max_queue_size;
  auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
      std::move(exporter), span_options);
  auto provider =
      trace_sdk::TracerProviderFactory::Create(std::move(processor));
  trace::Provider::SetTracerProvider(std::move(provider));
}

void Cleanup() {
  auto provider = trace::Provider::GetTracerProvider();
  if (provider) {
    static_cast<trace_sdk::TracerProvider*>(provider.get())->ForceFlush();
  }

  std::shared_ptr<trace::TracerProvider> none;
  trace::Provider::SetTracerProvider(none);
}

} // namespace

int main(int argc, char* argv[]) try {
  auto args = ParseArguments(argc, argv);
  if (args.project_id.empty() && args.topic_id.empty()) {
    return 1;
  }
  std::cout << "Using project `" << args.project_id << "` and topic `"
            << args.topic_id << "`\n";

  // Automatically call `Cleanup()` before returning from `main()`.
  std::shared_ptr<void> cleanup(nullptr, [](void*) { Cleanup(); });

  ConfigureCloudTraceTracer(args);

  auto blocking_publisher = pubsub::BlockingPublisher(
      pubsub::MakeBlockingPublisherConnection(args.publisher_options));

  auto id = blocking_publisher.Publish(
      pubsub::Topic(args.project_id, args.topic_id),
      pubsub::MessageBuilder().SetData("Hello!").Build());
  if (id) {
    std::cout << "Sent message with id: " << *id << "\n";
  }
  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}
