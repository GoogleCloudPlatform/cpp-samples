// Copyright 2021 Google LLC
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

#include <google/cloud/functions/cloud_event.h>
#include <google/cloud/pubsub/publisher.h>
#include <stdexcept>
#include <string>

namespace gcf = ::google::cloud::functions;
namespace pubsub = ::google::cloud::pubsub;

namespace {
pubsub::Publisher GetPublisher() {
  auto getenv = [](char const* var) {
    auto const* value = std::getenv(var);
    if (value == nullptr) {
      throw std::runtime_error("Environment variable " + std::string(var) +
                               " is not set");
    }
    return std::string(value);
  };

  static auto const publisher = [&] {
    auto topic =
        pubsub::Topic(getenv("GOOGLE_CLOUD_PROJECT"), getenv("WORK_TOPIC_ID"));
    return pubsub::Publisher(
        pubsub::MakePublisherConnection(std::move(topic), {}));
  }();
  return publisher;
}
}  // namespace

void GcsIndexScheduler(gcf::CloudEvent /*event*/) {  // NOLINT
  // TODO(#138) - do interesting stuff....
}
