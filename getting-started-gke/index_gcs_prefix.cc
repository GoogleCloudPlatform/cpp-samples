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

#include <google/cloud/pubsub/publisher.h>
#include <google/cloud/pubsub/subscriber.h>
#include <google/cloud/spanner/client.h>
#include <google/cloud/spanner/mutations.h>
#include <google/cloud/storage/client.h>
#include <google/cloud/storage/object_metadata.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

namespace gcs = ::google::cloud::storage;
namespace pubsub = ::google::cloud::pubsub;
namespace spanner = ::google::cloud::spanner;
using google::cloud::future;
using google::cloud::promise;
using google::cloud::Status;

std::string GetEnv(char const* var) {
  auto const* value = std::getenv(var);
  if (value == nullptr) {
    throw std::runtime_error("Environment variable " + std::string(var) +
                             " is not set");
  }
  return value;
}

class MutationBatcher {
 public:
  MutationBatcher(spanner::Client client);

  future<Status> Push(gcs::ObjectMetadata const& o);
  void Flush();

 private:
  void FlushIfNeeded(std::unique_lock<std::mutex> const& lk);
  void Flush(std::unique_lock<std::mutex> const& lk);
  void ReapBackgroundTasks(std::unique_lock<std::mutex> const& lk);

  struct Item {
    spanner::Mutation mutation;
    promise<Status> done;
  };

  spanner::Client client_;
  std::mutex mu_;
  std::vector<Item> items_;
  std::vector<std::future<void>> background_tasks_;
};

void IndexGcsPrefix(pubsub::Message m, pubsub::AckHandler h, gcs::Client client,
                    pubsub::Publisher publisher, MutationBatcher& batcher);

}  // namespace

int main(int argc, char* argv[]) try {
  auto gcs_client = gcs::Client();
  auto spanner_client =
      spanner::Client(spanner::MakeConnection(spanner::Database(
          GetEnv("GOOGLE_CLOUD_PROJECT"), GetEnv("SPANNER_INSTANCE"),
          GetEnv("SPANNER_DATABASE"))));

  auto batcher = std::make_shared<MutationBatcher>(spanner_client);

  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(GetEnv("GOOGLE_CLOUD_PROJECT"), GetEnv("TOPIC_ID"))));

  auto subscriber = pubsub::Subscriber(pubsub::MakeSubscriberConnection(
      pubsub::Subscription(GetEnv("GOOGLE_CLOUD_PROJECT"),
                           GetEnv("SUBSCRIPTION_ID")),
      google::cloud::Options{}
          .set<pubsub::MaxOutstandingMessagesOption>(32)
          .set<google::cloud::GrpcBackgroundThreadPoolSizeOption>(32)));

  auto session =
      subscriber.Subscribe([g = std::move(gcs_client), p = std::move(publisher),
                            b = std::move(batcher)](auto m, auto h) {
        IndexGcsPrefix(std::move(m), std::move(h), g, p, *b);
      });
  auto status = session.get();
  if (status.ok()) return 0;
  std::cerr << "Error in subscription: " << status << "\n";
  return 1;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n";
  return 1;
} catch (...) {
  std::cerr << "Unknown exception thrown\n";
  return 1;
}

namespace {

using GetField = std::function<spanner::Value(gcs::ObjectMetadata const&)>;

void ThrowIfNotOkay(std::string const& context,
                    google::cloud::Status const& status) {
  if (status.ok()) return;
  std::ostringstream os;
  os << "error while " << context << " status=" << status;
  throw std::runtime_error(std::move(os).str());
}

std::string LogFormat(std::string const& sev, std::string const& msg) {
  return nlohmann::json{{"severity", sev}, {"message", msg}}.dump();
}

void LogError(std::string const& msg) {
  std::cerr << LogFormat("error", msg) << "\n";
}

auto const& Columns() {
  static auto const columns = [] {
    auto field = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            return spanner::Value(f(o));
          });
    };

    auto custom = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name), [f = std::move(functor)](
                               gcs::ObjectMetadata const& o) { return f(o); });
    };

    auto optional_string = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            auto s = f(o);
            if (s.empty()) return spanner::Value(absl::optional<std::string>());
            return spanner::Value(std::move(s));
          });
    };

    auto timestamp = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            auto tp = f(o);
            if (tp == std::chrono::system_clock::time_point{}) {
              return spanner::Value(absl::optional<spanner::Timestamp>());
            }
            auto ts = spanner::MakeTimestamp(tp).value();
            return spanner::Value(ts);
          });
    };

    return std::map<std::string, GetField>({
        field("name", [](auto const& o) { return o.name(); }),
        field("bucket", [](auto const& o) { return o.bucket(); }),
        field("generation", [](auto const& o) { return o.generation(); }),
        field("metageneration",
              [](auto const& o) { return o.metageneration(); }),
        timestamp("timeCreated",
                  [](auto const& o) { return o.time_created(); }),
        timestamp("updated", [](auto const& o) { return o.updated(); }),
        timestamp("timeDeleted",
                  [](auto const& o) { return o.time_deleted(); }),
        timestamp("customTime", [](auto const& o) { return o.custom_time(); }),
        field("temporaryHold",
              [](auto const& o) { return o.temporary_hold(); }),
        field("eventBasedHold",
              [](auto const& o) { return o.event_based_hold(); }),
        timestamp("retentionExpirationTime",
                  [](auto const& o) { return o.retention_expiration_time(); }),
        field("storageClass", [](auto const& o) { return o.storage_class(); }),
        timestamp("timeStorageClassUpdated",
                  [](auto const& o) { return o.time_storage_class_updated(); }),
        field(
            "size",
            [](auto const& o) { return static_cast<std::int64_t>(o.size()); }),
        field("crc32c", [](auto const& o) { return o.crc32c(); }),
        optional_string("md5Hash", [](auto const& o) { return o.md5_hash(); }),
        optional_string("contentType",
                        [](auto const& o) { return o.content_type(); }),
        optional_string("contentEncoding",
                        [](auto const& o) { return o.content_encoding(); }),
        optional_string("contentDisposition",
                        [](auto const& o) { return o.content_disposition(); }),
        optional_string("contentLanguage",
                        [](auto const& o) { return o.content_language(); }),
        optional_string("cacheControl",
                        [](auto const& o) { return o.cache_control(); }),

        field("metadata",
              [](auto const& o) {
                nlohmann::json json{};
                for (auto const& [k, v] : o.metadata()) json[k] = v;
                return json.dump();
              }),
        custom("owner",
               [](auto const& o) {
                 if (!o.has_owner()) {
                   return spanner::Value(absl::optional<std::string>());
                 }
                 return spanner::Value(nlohmann::json{
                     {"entity", o.owner().entity},
                     {"entityId",
                      o.owner().entity_id}}.dump());
               }),
        field("componentCount",
              [](auto const& o) { return o.component_count(); }),
        optional_string("etag", [](auto const& o) { return o.etag(); }),
        custom("customerEncryption",
               [](auto const& o) {
                 if (!o.has_customer_encryption()) {
                   return spanner::Value(absl::optional<std::string>());
                 }
                 return spanner::Value(nlohmann::json{
                     {"encryptionAlgorithm",
                      o.customer_encryption().encryption_algorithm},
                     {"keySha256", o.customer_encryption().key_sha256}}
                                           .dump());
               }),
        optional_string("kmsKeyName",
                        [](auto const& o) { return o.kms_key_name(); }),
    });
  }();
  return columns;
}

auto Names() {
  static auto const names = [] {
    auto columns = Columns();
    std::vector<std::string> names(columns.size());
    std::transform(columns.begin(), columns.end(), names.begin(),
                   [](auto p) { return p.first; });
    return names;
  }();
  return names;
}

spanner::Mutation UpdateObjectMetadata(gcs::ObjectMetadata const& object) {
  auto const& columns = Columns();
  std::vector<spanner::Value> values(columns.size());
  std::transform(columns.begin(), columns.end(), values.begin(),
                 [&object](auto const& p) {
                   auto const& [name, to_value] = p;
                   return to_value(object);
                 });
  return spanner::InsertOrUpdateMutationBuilder("gcs_objects", Names())
      .AddRow(std::move(values))
      .Build();
}

MutationBatcher::MutationBatcher(spanner::Client client)
    : client_(std::move(client)) {}

future<Status> MutationBatcher::Push(gcs::ObjectMetadata const& o) {
  std::unique_lock lk(mu_);
  // Make room for the new data.
  FlushIfNeeded(lk);
  items_.push_back(Item{UpdateObjectMetadata(o), promise<Status>{}});
  return items_.back().done.get_future();
}

void MutationBatcher::Flush() { Flush(std::unique_lock(mu_)); }

void MutationBatcher::FlushIfNeeded(std::unique_lock<std::mutex> const& lk) {
  // Spanner limits a commit to 20,000 mutations, where each modified column
  // counts as a separate "mutation".
  auto constexpr kSpannerMutationLimit = 20'000UL;
  // Spanner recommends changing at most "a few hundred rows" at a time:
  //   https://cloud.google.com/spanner/docs/bulk-loading
  auto constexpr kEfficientRowLimit = 256UL;

  if (items_.size() >= kEfficientRowLimit) return Flush(lk);
  if (items_.size() * Columns().size() < kSpannerMutationLimit) return;
  Flush(lk);
}

void MutationBatcher::Flush(std::unique_lock<std::mutex> const& lk) {
  if (items_.empty()) return;
  std::vector<Item> items;
  items.swap(items_);
  background_tasks_.push_back(std::async(
      std::launch::async,
      [](spanner::Client client, std::vector<Item> items) {
        std::vector<spanner::Mutation> mutations(items.size());
        std::transform(items.begin(), items.end(), mutations.begin(),
                       [](auto& i) { return std::move(i.mutation); });
        auto commit_result = client.Commit(std::move(mutations));
        for (auto& i : items) i.done.set_value(commit_result.status());
      },
      client_, std::move(items)));
  ReapBackgroundTasks(lk);
}

void MutationBatcher::ReapBackgroundTasks(
    std::unique_lock<std::mutex> const& lk) {
  auto constexpr kMaxRunningBackgroundTasks = 128;
  while (background_tasks_.size() >= kMaxRunningBackgroundTasks) {
    background_tasks_.erase(
        std::remove_if(background_tasks_.begin(), background_tasks_.end(),
                       [](auto& t) {
                         using namespace std::chrono_literals;
                         return t.wait_for(10ms) == std::future_status::ready;
                       }),
        background_tasks_.end());
  }
}

template <typename T>
future<std::vector<future<T>>> when_all(std::vector<future<T>> w) {
  class Accumulator : public std::enable_shared_from_this<Accumulator> {
   public:
    Accumulator() : unsatisfied_(0) {}

    future<std::vector<future<T>>> Start(std::vector<future<T>> all) {
      auto self = this->shared_from_this();
      unsatisfied_ = all.size();
      accumulator_.resize(all.size());
      std::size_t i = 0;
      for (auto& f : all) {
        cont_.push_back(f.then([index = i++, self](future<T> g) {
          self->OnCompletion(index, std::move(g));
        }));
      }
      return done_.get_future();
    }

   private:
    void OnCompletion(std::size_t index, future<T> g) {
      accumulator_[index] = std::move(g);
      if (--unsatisfied_ == 0) done_.set_value(std::move(accumulator_));
    }
    std::atomic<std::size_t> unsatisfied_;
    std::vector<future<T>> accumulator_;
    std::vector<future<void>> cont_;
    google::cloud::promise<std::vector<future<T>>> done_;
  };

  auto accumulator = std::make_shared<Accumulator>();
  return accumulator->Start(std::move(w));
}

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void IndexGcsPrefix(pubsub::Message m, pubsub::AckHandler h, gcs::Client client,
                    pubsub::Publisher publisher, MutationBatcher& batcher) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
  auto const attributes = m.attributes();
  auto i = attributes.find("bucket");
  if (i == attributes.end()) {
    return LogError("missing 'bucket' attribute in Pub/Sub message");
  }
  auto const bucket = i->second;
  auto const prefix = [&attributes] {
    auto i = attributes.find("prefix");
    if (i == attributes.end()) return gcs::Prefix();
    return gcs::Prefix(i->second);
  }();
  auto const start = [&attributes] {
    auto i = attributes.find("start");
    if (i == attributes.end()) return gcs::StartOffset();
    return gcs::StartOffset(i->second);
  }();

  std::vector<google::cloud::future<google::cloud::Status>> pending;
  for (auto const& entry : client.ListObjectsAndPrefixes(bucket, prefix, start,
                                                         gcs::Delimiter("/"))) {
    ThrowIfNotOkay("listing bucket " + bucket, entry.status());
    if (std::chrono::steady_clock::now() >= deadline) {
      std::string start = absl::visit(
          overloaded{[](std::string const& s) { return s; },
                     [](gcs::ObjectMetadata const& o) { return o.name(); }},
          *entry);
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

    pending.push_back(absl::visit(
        overloaded{
            [&](std::string const& p) {
              // Do not reschedule the same prefix we are processing.
              if (prefix.has_value() && prefix.value() == p) {
                return make_ready_future(Status{});
              }
              return publisher
                  .Publish(pubsub::MessageBuilder{}
                               .InsertAttribute("bucket", bucket)
                               .InsertAttribute("prefix", p)
                               .Build())
                  .then([](auto f) { return f.get().status(); });
            },
            [&](gcs::ObjectMetadata const& o) { return batcher.Push(o); }},
        *entry));
  }
  // publisher.Flush();
  // batcher.Flush();

  when_all(std::move(pending))
      .then([handler = std::move(h), fun = std::string(__func__), bucket,
             prefix](auto f) mutable {
        auto v = f.get();
        auto const success = std::all_of(v.begin(), v.end(),
                                         [](auto&& f) { return f.get().ok(); });
        if (success) return std::move(handler).ack();
        std::move(handler).nack();
      });
}

}  // namespace
