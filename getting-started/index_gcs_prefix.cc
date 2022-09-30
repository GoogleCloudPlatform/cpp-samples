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
#include <google/cloud/functions/http_request.h>
#include <google/cloud/functions/http_response.h>
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
using google::cloud::cpp_samples::GetEnv;
using google::cloud::cpp_samples::UpdateObjectMetadata;

pubsub::Publisher GetPublisher() {
  static auto const publisher = [&] {
    auto topic =
        pubsub::Topic(GetEnv("GOOGLE_CLOUD_PROJECT"), GetEnv("TOPIC_ID"));
    return pubsub::Publisher(
        pubsub::MakePublisherConnection(std::move(topic), {}));
  }();
  return publisher;
}

spanner::Client GetSpannerClient() {
  static auto const client = [&] {
    auto database = spanner::Database(GetEnv("GOOGLE_CLOUD_PROJECT"),
                                      GetEnv("SPANNER_INSTANCE"),
                                      GetEnv("SPANNER_DATABASE"));
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

nlohmann::json LogFormat(std::string const& sev, std::string const& msg) {
  return nlohmann::json{{"severity", sev}, {"message", msg}}.dump();
}

gcf::HttpResponse LogError(std::string const& msg) {
  std::cerr << LogFormat("error", msg) << "\n";
  return gcf::HttpResponse{}
      .set_result(gcf::HttpResponse::kBadRequest)
      .set_payload(msg);
}

}  // namespace

gcf::HttpResponse IndexGcsPrefix(gcf::HttpRequest request) {  // NOLINT
  // This example assumes the push subscription is set for 10 minute deadline.
  // We allow ourselves up to 5 minutes processing this request.
  auto const deadline =
      std::chrono::steady_clock::now() + std::chrono::minutes(5);

  auto const ct = request.headers().find("content-type");
  if (ct == request.headers().end() || ct->second != "application/json") {
    return LogError("expected application/json data");
  }
  auto const payload = nlohmann::json::parse(request.payload());
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

  auto client = gcs::Client();
  auto publisher = GetPublisher();

  int mutation_count = 0;
  std::vector<google::cloud::future<google::cloud::Status>> pending;
  for (auto const& entry : client.ListObjectsAndPrefixes(bucket, prefix, start,
                                                         gcs::Delimiter("/"))) {
    ThrowIfNotOkay("listing bucket " + bucket, entry.status());
    if (std::chrono::steady_clock::now() >= deadline) {
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
      auto const& p = absl::get<std::string>(*entry);
      // Do not reschedule the same prefix we are processing.
      if (prefix.has_value() && prefix.value() == p) continue;
      pending.push_back(publisher
                            .Publish(pubsub::MessageBuilder{}
                                         .InsertAttribute("bucket", bucket)
                                         .InsertAttribute("prefix", p)
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
  return gcf::HttpResponse{};
}
