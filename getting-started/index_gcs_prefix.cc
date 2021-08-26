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

#include "gcs_indexing.h"
#include <absl/time/time.h>
#include <google/cloud/functions/cloud_event.h>
#include <google/cloud/pubsub/publisher.h>
#include <google/cloud/spanner/client.h>
#include <google/cloud/storage/client.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

namespace gcf = ::google::cloud::functions;
namespace gcs = ::google::cloud::storage;
namespace pubsub = ::google::cloud::pubsub;
namespace spanner = ::google::cloud::spanner;
using google::cloud::cpp_samples::LogError;
using google::cloud::cpp_samples::UpdateObjectMetadata;

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
        pubsub::Topic(getenv("GOOGLE_CLOUD_PROJECT"), getenv("TOPIC_ID"));
    return pubsub::Publisher(
        pubsub::MakePublisherConnection(std::move(topic), {}));
  }();
  return publisher;
}

spanner::Client GetSpannerClient() {
  auto getenv = [](char const* var) {
    auto const* value = std::getenv(var);
    if (value == nullptr) {
      throw std::runtime_error("Environment variable " + std::string(var) +
                               " is not set");
    }
    return std::string(value);
  };

  static auto const client = [&] {
    auto database = spanner::Database(getenv("GOOGLE_CLOUD_PROJECT"),
                                      getenv("SPANNER_INSTANCE"),
                                      getenv("SPANNER_DATABASE"));
    return spanner::Client(spanner::MakeConnection(std::move(database)));
  }();
  return client;
}

void ThrowIfNotOkay(std::string const& context,
                    google::cloud::Status const& status) {
  if (status.ok()) return;
  std::ostringstream os;
  os << "error while " << context << " status=" << status;
  throw std::runtime_error(std::move(os).str());
}

}  // namespace

void IndexGcsPrefix(gcf::CloudEvent event) {  // NOLINT
  // We only have 10 seconds to handle this event. If there is a lot of data in
  // a prefix we will need to stop and issue a new event to continue.
  auto const deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(7);

  if (event.data_content_type().value_or("") != "application/json") {
    return LogError("expected application/json data");
  }
  auto const payload = nlohmann::json::parse(event.data().value_or("{}"));
  if (!payload.contains("message")) {
    return LogError("missing embedded Pub/Sub message");
  }
  auto const& message = payload["message"];
  if (!message.contains("attributes")) {
    return LogError("missing Pub/Sub attributes");
  }
  auto const& attributes = message["attributes"];
  if (!attributes.contains("bucket")) {
    return LogError("missing 'bucket' attribute in Pub/Sub message");
  }
  auto const bucket = attributes.value("bucket", "");
  auto const prefix = [&attributes]() {
    if (!attributes.contains("prefix")) return gcs::Prefix();
    return gcs::Prefix(attributes.value("prefix", ""));
  }();
  auto const start = [&attributes]() {
    if (!attributes.contains("start")) return gcs::StartOffset();
    return gcs::StartOffset(attributes.value("start", ""));
  }();

  auto client = gcs::Client::CreateDefaultClient().value();
  auto publisher = GetPublisher();

  int mutation_count = 0;
  std::vector<google::cloud::future<google::cloud::Status>> pending;
  for (auto const& entry : client.ListObjectsAndPrefixes(bucket, prefix, start,
                                                         gcs::Delimiter("/"))) {
    ThrowIfNotOkay("listing bucket " + bucket, entry.status());
    auto const now = std::chrono::steady_clock::now();
    if (now > deadline) {
      struct SplitPoint {
        std::string operator()(std::string const& s) { return s; }
        std::string operator()(gcs::ObjectMetadata const& o) {
          return o.name();
        }
      };
      std::string start = absl::visit(SplitPoint{}, *entry);
      auto builder = pubsub::MessageBuilder{}
                         .InsertAttribute("bucket", bucket)
                         .InsertAttribute("start", std::move(start));
      if (prefix.has_value()) {
        builder.InsertAttribute("prefix", prefix.value());
      }
      pending.push_back(
          publisher.Publish(std::move(builder).Build()).then([](auto f) {
            return f.get().status();
          }));
      break;
    }

    if (absl::holds_alternative<std::string>(*entry)) {
      pending.push_back(
          publisher
              .Publish(
                  pubsub::MessageBuilder{}
                      .InsertAttribute("bucket", bucket)
                      .InsertAttribute("prefix", absl::get<std::string>(*entry))
                      .Build())
              .then([](auto f) { return f.get().status(); }));
    } else {
      auto const& object = absl::get<gcs::ObjectMetadata>(*entry);
      auto update = UpdateObjectMetadata(object);
      GetSpannerClient()
          .Commit(
              [m = std::move(update)](auto) { return spanner::Mutations{m}; })
          .value();
      ++mutation_count;
    }
  }
  publisher.Flush();
  google::cloud::Status status;
  for (auto& p : pending) {
    auto publish_status = p.get();
    if (publish_status.ok()) continue;
    status = std::move(publish_status);
  }
  ThrowIfNotOkay("publishing one or more messages", status);
  std::cout << "DEBUG inserted " << mutation_count << " rows and sent "
            << pending.size() << " messages\n";
}
