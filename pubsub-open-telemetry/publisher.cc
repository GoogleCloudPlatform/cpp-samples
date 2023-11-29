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

#include "google/cloud/pubsub/publisher.h"
#include "google/cloud/opentelemetry/configure_basic_tracing.h"
#include "google/cloud/opentelemetry/trace_exporter.h"
#include "google/cloud/opentelemetry_options.h"
#include "parse_args.h"
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>
#include <iostream>
#include <string_view>

// Create a few namespace aliases to make the code easier to read.
namespace gc = ::google::cloud;
namespace pubsub = gc::pubsub;
namespace otel = gc::otel;

std::string GeneratePayload(int payload_size) {
  auto gen = google::cloud::internal::DefaultPRNG(std::random_device{}());
  const std::string charset = "abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<std::size_t> rd(0, charset.size() - 1);

  std::string result(payload_size, '0');
  std::generate(result.begin(), result.end(),
                [&rd, &gen, &charset]() { return charset[rd(gen)]; });
  return result;
}

int main(int argc, char* argv[]) try {
  auto args = ParseArguments(argc, argv);
  if (args.project_id.empty() && args.topic_id.empty()) {
    return 1;
  }

  std::cout << "Using project `" << args.project_id << "` and topic `"
            << args.topic_id << "`\n";
  std::unique_ptr<otel::BasicTracingConfiguration>  configuration; 
  if (args.max_queue_size == 0) {
     configuration = std::move(otel::ConfigureBasicTracing(
        gc::Project(args.project_id), args.otel_options));
  } else {
    auto exporter = otel::MakeTraceExporter(gc::Project(args.project_id));
    opentelemetry::sdk::trace::BatchSpanProcessorOptions span_options;
    span_options.max_queue_size = args.max_queue_size;
    auto processor =
        opentelemetry::sdk::trace::BatchSpanProcessorFactory::Create(
            std::move(exporter), span_options);
    auto provider = opentelemetry::sdk::trace::TracerProviderFactory::Create(
        std::move(processor));
    opentelemetry::trace::Provider::SetTracerProvider(std::move(provider));
  }

  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(args.project_id, args.topic_id), args.publisher_options));

  std::cout << "Publishing " << std::to_string(args.message_count)
            << " message(s) with payload size "
            << std::to_string(args.message_size) << "...\n";
  std::vector<gc::future<void>> ids;
  for (int i = 0; i < args.message_count; i++) {
    auto id = publisher
                  .Publish(pubsub::MessageBuilder()
                               .SetData(GeneratePayload(args.message_size))
                               .Build())
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
  for (auto& id : ids) id.get();

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
}