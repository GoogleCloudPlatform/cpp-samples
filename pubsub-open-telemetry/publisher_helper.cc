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

#include "publisher_helper.h"
#include <iostream>
#include <string>

namespace gc = ::google::cloud;

namespace {

std::string GeneratePayload(int payload_size) {
  auto gen = google::cloud::internal::DefaultPRNG(std::random_device{}());
  const std::string charset = "abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<std::size_t> rd(0, charset.size() - 1);

  std::string result(payload_size, '0');
  std::generate(result.begin(), result.end(),
                [&rd, &gen, &charset]() { return charset[rd(gen)]; });
  return result;
}

}  // namespace

pubsub::Publisher CreatePublisher(ParseResult const& args) {
  return pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(args.project_id, args.topic_id), args.publisher_options));
}

void Publish(pubsub::Publisher& publisher, ParseResult const& args) {
  std::cout << "Publishing " << std::to_string(args.message_count)
            << " message(s) with payload size "
            << std::to_string(args.message_size) << "...\n";
  std::vector<gc::future<std::string>> ids;
  for (int i = 0; i < args.message_count; i++) {
    auto id = publisher
                  .Publish(pubsub::MessageBuilder()
                               .SetData(GeneratePayload(args.message_size))
                               .Build())
                  .then([](gc::future<gc::StatusOr<std::string>> f) {
                    return f.get().value();
                  });
    ids.push_back(std::move(id));
  }
  for (auto& id : ids) try {
      std::cout << "Sent message with id: " << id.get() << "\n";
    } catch (std::exception const& ex) {
      std::cout << "Error in publish: " << ex.what() << "\n";
    }
  std::cout << "Message(s) published\n";
}
