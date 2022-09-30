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
using google::cloud::cpp_samples::ColumnCount;
using google::cloud::cpp_samples::GetEnv;
using google::cloud::cpp_samples::UpdateObjectMetadata;

class MutationBatcher {
 public:
  MutationBatcher(spanner::Client client);

  future<Status> Push(gcs::ObjectMetadata const& o);
  // Return the number of mutations processed since the last Flush().
  std::int64_t Flush();

  void ReapBackgroundTasks();

 private:
  void FlushIfNeeded(std::unique_lock<std::mutex> const&);
  void Flush(std::unique_lock<std::mutex> const&);

  struct Item {
    spanner::Mutation mutation;
    promise<Status> done;
  };

  spanner::Client client_;
  std::mutex mu_;
  std::vector<Item> items_;
  std::vector<std::future<void>> background_tasks_;
  std::int64_t mutation_count_ = 0;
};

void IndexGcsPrefix(pubsub::Message m, pubsub::AckHandler h, gcs::Client client,
                    pubsub::Publisher publisher,
                    std::shared_ptr<MutationBatcher> batcher);

// Spanner limits a commit to 20,000 mutations, where each modified column
// counts as a separate "mutation".
auto constexpr kSpannerMutationLimit = 20'000UL;
// Spanner recommends changing at most "a few hundred rows" at a time:
//   https://cloud.google.com/spanner/docs/bulk-loading
auto constexpr kEfficientRowLimit = 512UL;
// The Cloud Pub/Sub service can flow control how many messages
// are delivered to each subscriber.
auto constexpr kMaxOutstandingMessages = 128;
// The Cloud Pub/Sub library can be configured to limit the number of
// messages that are not ack or nacked by the application.
auto constexpr kMaxConcurrency = 256;

}  // namespace

int main(int argc, char* argv[]) try {
  auto gcs_client = gcs::Client();
  auto spanner_client =
      spanner::Client(spanner::MakeConnection(spanner::Database(
          GetEnv("GOOGLE_CLOUD_PROJECT"), GetEnv("SPANNER_INSTANCE"),
          GetEnv("SPANNER_DATABASE"))));

  auto batcher = std::make_shared<MutationBatcher>(spanner_client);

  auto publisher = pubsub::Publisher(pubsub::MakePublisherConnection(
      pubsub::Topic(GetEnv("GOOGLE_CLOUD_PROJECT"), GetEnv("TOPIC_ID")),
      google::cloud::Options{}));

  auto subscriber = pubsub::Subscriber(pubsub::MakeSubscriberConnection(
      pubsub::Subscription(GetEnv("GOOGLE_CLOUD_PROJECT"),
                           GetEnv("SUBSCRIPTION_ID")),
      google::cloud::Options{}
          .set<pubsub::MaxOutstandingMessagesOption>(kMaxOutstandingMessages)
          .set<pubsub::MaxConcurrencyOption>(kMaxConcurrency)));

  std::int64_t last_message_count = 0;
  std::atomic<std::int64_t> message_count{0};
  auto session = subscriber.Subscribe(
      [g = std::move(gcs_client), p = std::move(publisher), b = batcher,
       &message_count](pubsub::Message m, pubsub::AckHandler h) {
        IndexGcsPrefix(std::move(m), std::move(h), g, p, b);
        ++message_count;
      });
  using namespace std::chrono_literals;
  for (auto s = session.wait_for(10s); s != std::future_status::ready;
       s = session.wait_for(10s)) {
    auto const total_messages = message_count.load();
    auto const messages = total_messages - last_message_count;
    last_message_count = total_messages;
    auto const mutations = batcher->Flush();
    if (mutations == 0 && message_count == 0) continue;  // nothing to report
    std::cout << __func__ << "() messages=" << messages
              << ", mutations=" << mutations << std::endl;
  }
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

MutationBatcher::MutationBatcher(spanner::Client client)
    : client_(std::move(client)) {}

future<Status> MutationBatcher::Push(gcs::ObjectMetadata const& o) {
  std::unique_lock lk(mu_);
  // Make room for the new data.
  FlushIfNeeded(lk);
  items_.push_back(Item{UpdateObjectMetadata(o), promise<Status>{}});
  return items_.back().done.get_future();
}

std::int64_t MutationBatcher::Flush() {
  std::unique_lock lk(mu_);
  Flush(lk);
  std::int64_t n = 0;
  std::swap(n, mutation_count_);
  return n;
}

void MutationBatcher::ReapBackgroundTasks() {
  std::unique_lock lk(mu_);
  // Remove any tasks that have completed. This would not be needed if
  // we had a fully asynchronous `AsyncCommit()` function in Cloud Spanner.
  background_tasks_.erase(
      std::remove_if(background_tasks_.begin(), background_tasks_.end(),
                     [](auto& t) {
                       using namespace std::chrono_literals;
                       return t.wait_for(10ms) == std::future_status::ready;
                     }),
      background_tasks_.end());
}

void MutationBatcher::FlushIfNeeded(std::unique_lock<std::mutex> const& lk) {
  if (items_.size() >= kEfficientRowLimit) return Flush(lk);
  if (items_.size() * ColumnCount() >= kSpannerMutationLimit) return Flush(lk);
}

void MutationBatcher::Flush(std::unique_lock<std::mutex> const&) {
  if (items_.empty()) return;
  std::vector<Item> items;
  items.swap(items_);
  mutation_count_ += items.size();
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
                    pubsub::Publisher publisher,
                    std::shared_ptr<MutationBatcher> batcher) {
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
      std::cout << __func__ << "(" << prefix << ") split at " << start
                << std::endl;
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
            [&](gcs::ObjectMetadata const& o) { return batcher->Push(o); }},
        *entry));
  }

  when_all(std::move(pending))
      .then([handler = std::move(h), fun = std::string(__func__), bucket,
             prefix](auto f) mutable {
        auto v = f.get();
        std::vector<google::cloud::Status> results(v.size());
        std::transform(v.begin(), v.end(), results.begin(),
                       [](auto&& g) { return g.get(); });
        auto const success = std::all_of(results.begin(), results.end(),
                                         [](auto&& s) { return s.ok(); });
        if (success) return std::move(handler).ack();
        std::move(handler).nack();
        std::ostringstream os;
        os << "One or more operations failed, first error ";
        for (auto const& s : results) {
          if (s.ok()) continue;
          os << s;
          break;
        }
        LogError(std::move(os).str());
      })
      .then([batcher](auto f) {
        // Once the operations (including any writes to Cloud Spanner) have
        // completed we cleanup the background tasks that might have been
        // created to satisfy the request.
        batcher->ReapBackgroundTasks();
      });
}

}  // namespace
