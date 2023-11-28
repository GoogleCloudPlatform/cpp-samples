// Copyright 2023 Google Inc.
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

#include "parse_args.h"
#include "google/cloud/opentelemetry/configure_basic_tracing.h"
#include "google/cloud/opentelemetry_options.h"
#include "google/cloud/pubsub/options.h"
#include <boost/program_options.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace po = ::boost::program_options;

ParseResult ParseArguments(int argc, char* argv[]) {
  po::positional_options_description positional;
  positional.add("project-id", 1);
  positional.add("topic-id", 2);
  po::options_description desc(
      "A simple publisher application with Open Telemetery enabled");
  // The following empty line comments are for readability.
  desc.add_options()
      //
      ("help,h", "produce help message")
      //
      ("project-id", po::value<std::string>()->required(),
       "the name of the Google Cloud project")
      //
      ("topic-id", po::value<std::string>()->required(),
       "the name of the Google Cloud topic")
      // Tracing options
      ("tracing-rate", po::value<double>()->default_value(1.0),
       "otel::BasicTracingRateOption value")
      // Processor options
      ("max-queue-size", po::value<int>()->default_value(0),
       "If set to 0, uses the default tracing configuration.")
      // Message options
      ("message-count,n", po::value<int>()->default_value(1),
       "the number of messages to publish")
      //
      ("message-size", po::value<int>()->default_value(1),
       "the desired message payload size")
      // Flow control options
      ("max-pending-messages", po::value<std::size_t>(),
       "pubsub::MaxPendingMessagesOption value")
      //
      ("max-pending-bytes", po::value<std::size_t>(),
       "pubsub::MaxPendingBytesOption value")
      //
      ("publisher-action", po::value<std::string>(),
       "pubsub::FullPublisherAction value "
       "(block|ignore|reject)")
      // Batching options
      ("max-hold-time", po::value<int>(),
       "pubsub::MaxHoldTimeOption value in us")
      //
      ("max-batch-bytes", po::value<std::size_t>(),
       "pubsub::MaxBatchBytesOption value")
      //
      ("max-batch-messages", po::value<std::size_t>(),
       "pubsub::MaxBatchMessagesOption value");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
                .options(desc)
                .positional(positional)
                .run(),
            vm);

  ParseResult result;
  if (vm.count("help") || argc == 1) {
    std::cerr << "Usage: " << argv[0] << " <project-id> <topic-id>\n";
    std::cerr << desc;
    return result;
  }

  // This must come before po::notify which raises any errors when parsing the
  // arguments. This ensures if --help is passed, the program does not raise any
  // issues about missing required arguments.
  po::notify(vm);
  // Get arguments that are required or optional and have defaults set
  auto const project_id = vm["project-id"].as<std::string>();
  auto const topic_id = vm["topic-id"].as<std::string>();
  auto const tracing_rate = vm["tracing-rate"].as<double>();
  auto const message_count = vm["message-count"].as<int>();
  auto const message_size = vm["message-size"].as<int>();
  auto const max_queue_size = vm["max-queue-size"].as<int>();

  // Validate the command-line options.
  if (project_id.empty()) {
    throw std::runtime_error("The project-id cannot be empty");
  }
  if (topic_id.empty()) {
    throw std::runtime_error("The topic-id cannot be empty");
  }
  if (tracing_rate == 0) {
    throw std::runtime_error(
        "Setting the tracing rate to 0 will produce zero traces.");
  }
  if (message_count == 0) {
    throw std::runtime_error(
        "Setting the message count to 0 will produce zero traces.");
  }
  result.project_id = project_id;
  result.topic_id = topic_id;
  result.message_count = message_count;
  result.message_size = message_size;
  result.max_queue_size = max_queue_size;
  result.otel_options =
      gc::Options{}.set<gc::otel::BasicTracingRateOption>(tracing_rate);
  result.publisher_options =
      gc::Options{}.set<gc::OpenTelemetryTracingOption>(true);
  std::cout << static_cast<bool>(result.publisher_options.get<gc::OpenTelemetryTracingOption>()) << "\n";
  if (vm.count("max-pending-messages")) {
    auto const max_pending_messages =
        vm["max-pending-messages"].as<std::size_t>();
    result.publisher_options.set<gc::pubsub::MaxPendingMessagesOption>(
        max_pending_messages);
  }
  if (vm.count("max-pending-bytes")) {
    auto const max_pending_bytes = vm["max-pending-bytes"].as<std::size_t>();
    result.publisher_options.set<gc::pubsub::MaxPendingBytesOption>(
        max_pending_bytes);
  }
  if (vm.count("publisher-action")) {
    auto const publisher_action = vm["publisher-action"].as<std::string>();
    gc::pubsub::FullPublisherAction action;
    if (publisher_action == "reject") {
      action = gc::pubsub::FullPublisherAction::kRejects;
    } else if (publisher_action == "block") {
      action = gc::pubsub::FullPublisherAction::kBlocks;
    } else if (publisher_action == "ignore") {
      action = gc::pubsub::FullPublisherAction::kIgnored;
    } else {
      throw std::runtime_error(
          "publisher-action is invalid. it must be one of the three values: "
          "block|ignore|reject");
    }
    result.publisher_options.set<gc::pubsub::FullPublisherActionOption>(action);
  }
  if (vm.count("max-hold-time")) {
    auto const max_hold_time = vm["max-hold-time"].as<int>();
    result.publisher_options.set<gc::pubsub::MaxHoldTimeOption>(
        std::chrono::microseconds(max_hold_time));
  }
  if (vm.count("max-batch-bytes")) {
    auto const max_batch_bytes = vm["max-batch-bytes"].as<std::size_t>();
    result.publisher_options.set<gc::pubsub::MaxBatchBytesOption>(
        max_batch_bytes);
  }
  if (vm.count("max-batch-messages")) {
    auto const max_batch_messages = vm["max-batch-messages"].as<std::size_t>();
    result.publisher_options.set<gc::pubsub::MaxBatchMessagesOption>(
        max_batch_messages);
  }
  return result;
}
