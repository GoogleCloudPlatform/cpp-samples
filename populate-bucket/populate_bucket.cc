// Copyright 2020 Google LLC
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

#include <boost/program_options.hpp>
#include <crc32c/crc32c.h>
#include <google/cloud/pubsub/publisher.h>
#include <google/cloud/pubsub/subscriber.h>
#include <google/cloud/storage/client.h>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <tuple>
#include <vector>

namespace {
namespace po = boost::program_options;
namespace gcs = google::cloud::storage;
namespace pubsub = google::cloud::pubsub;

std::tuple<po::variables_map, po::options_description> parse_command_line(
    int argc, char* argv[]);

void schedule(po::variables_map const&);
void worker(po::variables_map const&);

}  // namespace

int main(int argc, char* argv[]) try {
  std::cout << "Starting ... " << std::endl;

  auto [vm, desc] = parse_command_line(argc, argv);
  auto help = [d = std::move(desc)](po::variables_map const&) {
    std::cout << "Usage: " << d << "\n";
    return 0;
  };
  if (vm.count("help")) return help(vm);

  using action_function = std::function<void(po::variables_map const&)>;
  std::map<std::string, action_function> const actions{
      {"help", help},
      {"schedule", schedule},
      {"worker", worker},
  };

  auto const action_name = vm["action"].as<std::string>();
  auto const a = actions.find(action_name);
  if (a == actions.end()) {
    std::cerr << "Unknown action " << action_name << "\n";
    std::cerr << desc << "\n";
    return 1;
  }
  std::cout << "Executing " << action_name << " action" << std::endl;
  a->second(vm);

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception caught " << ex.what() << '\n';
  return 1;
}

namespace {
std::string getenv_or_empty(char const* name) {
  auto value = std::getenv(name);
  return value == nullptr ? std::string{} : value;
}

std::tuple<po::variables_map, po::options_description> parse_command_line(
    int argc, char* argv[]) {
  auto const default_object_count = 1'000'000L;
  auto const default_minimum_item_size = 1'000L;

  po::positional_options_description positional;
  positional.add("action", 1);
  po::options_description desc(
      "Populate a GCS Bucket with randomly named objects");
  desc.add_options()("help", "produce help message")
      //
      ("action", po::value<std::string>()->default_value("help")->required(),
       "the execution mode:\n"
       "- `schedule` to setup a number of work items in the task queue\n"
       "- `worker` to run as a worker listening on the task queue\n"
       "- `help` to produce some help\n")
      //
      ("project",
       po::value<std::string>()->default_value(
           getenv_or_empty("GOOGLE_CLOUD_PROJECT")),
       "set the Google Cloud Platform project id")
      //
      ("subscription", po::value<std::string>(),
       "set the Cloud Pub/Sub subscription")
      //
      ("topic", po::value<std::string>(), "set the Cloud Pub/Sub topic")
      //
      ("bucket", po::value<std::string>(), "set the destination bucket name")
      //
      ("object-count", po::value<long>()->default_value(default_object_count),
       "set the number of objects created by the job")
      //
      ("use-hash-prefix", po::value<bool>()->default_value(true),
       "prefix the object names with a hash to avoid hot spots in GCS")
      //
      ("task-size", po::value<long>()->default_value(default_minimum_item_size),
       "each work item created by schedule-job should contain this number of "
       "objects")
      //
      ("concurrency", po::value<int>()->default_value(8),
       "number of parallel handlers to handle work items");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
                .options(desc)
                .positional(positional)
                .run(),
            vm);
  po::notify(vm);
  std::cout << "Arguments parsed" << std::endl;
  return {vm, desc};
}

/// Create a random object name fragment.
std::string random_alphanum_string(std::mt19937_64& gen, int n) {
  char const alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::string result;
  std::generate_n(std::back_inserter(result), n, [&] {
    return alphabet[std::uniform_int_distribution<int>(
        0, sizeof(alphabet) - 1)(gen)];
  });
  return result;
}

/// Prepend a hash to an object name for better performance in GCS.
std::string hashed_name(bool use_hash_prefix, std::string object_name) {
  if (not use_hash_prefix) return std::move(object_name);

  // Just use the last 32-bits of the hash
  auto const hash = crc32c::Crc32c(object_name);

  char buf[16];
  std::snprintf(buf, sizeof(buf), "%08x_", hash);
  return buf + object_name;
}

struct work_item {
  std::string bucket;
  std::string prefix;
  std::int64_t object_count;
  bool use_hash_prefix;
};

work_item parse_message(pubsub::Message const& m) {
  auto attributes = m.attributes();
  return work_item{
      attributes.at("bucket"),
      attributes.at("prefix"),
      std::stoll(attributes.at("object_count")),
      attributes.at("use_hash_prefix") == "true",
  };
}

pubsub::Message format_work_item(work_item wi) {
  return pubsub::MessageBuilder()
      .SetAttributes({
          {"bucket", std::move(wi.bucket)},
          {"prefix", std::move(wi.prefix)},
          {"object_count", std::to_string(wi.object_count)},
          {"use_hash_prefix", wi.object_count ? "true" : "false"},
      })
      .Build();
}

/// Create all the work items to populate a bucket
void schedule(boost::program_options::variables_map const& vm) {
  std::cout << "Scheduling jobs through work queue" << std::endl;

  if (vm.count("project") == 0 or vm["project"].as<std::string>().empty()) {
    throw std::runtime_error("the `schedule` action requires --project");
  }
  if (vm.count("topic") == 0) {
    throw std::runtime_error("the `schedule` action requires --topic");
  }
  if (vm.count("bucket") == 0) {
    throw std::runtime_error("the `schedule` action requires --bucket");
  }
  auto const bucket = vm["bucket"].as<std::string>();
  auto const object_count = vm["object-count"].as<long>();
  auto const use_hash_prefix = vm["use-hash-prefix"].as<bool>();
  auto const task_size = vm["task-size"].as<long>();
  auto const project_id = vm["project"].as<std::string>();
  auto const topic_id = vm["topic"].as<std::string>();

  auto const topic = pubsub::Topic(project_id, topic_id);
  auto publisher = pubsub::Publisher(
      pubsub::MakePublisherConnection(topic, google::cloud::Options{}));

  auto make_prefix = [g = std::mt19937_64(std::random_device{}())](
                         long offset) mutable {
    std::ostringstream os;
    os << "name-" << random_alphanum_string(g, 32) << "-offset-" << std::setw(8)
       << std::setfill('0') << std::hex << static_cast<std::int64_t>(offset);
    return std::move(os).str();
  };
  std::vector<google::cloud::future<google::cloud::Status>> pending_publish;

  std::cout << "Generating work items" << std::flush;
  auto next_report = object_count / 10;
  for (long offset = 0; offset < object_count; offset += task_size) {
    if (offset >= next_report) {
      std::cout << '.' << std::flush;
      next_report += object_count / 10;
    }
    auto prefix = make_prefix(offset);
    auto const task_objects_count =
        (std::min)(task_size, object_count - offset);
    pending_publish.push_back(
        publisher
            .Publish(format_work_item(
                work_item{bucket, prefix, task_objects_count, use_hash_prefix}))
            .then([](auto f) { return f.get().status(); }));
  }
  std::cout << "DONE" << std::endl;
  std::map<google::cloud::StatusCode, std::int64_t> error_count;
  for (auto& f : pending_publish) {
    auto const code = f.get().code();
    if (code == google::cloud::StatusCode::kOk) continue;
    ++error_count[code];
  }
  if (error_count.empty()) return;
  std::cerr << "Errors publishing messages: ";
  std::int64_t total_count = 0;
  for (auto [code, count] : error_count) {
    std::cerr << "  " << code << ": " << count << "\n";
    total_count += count;
  }
  throw std::runtime_error("Errors publishing messages, count=" +
                           std::to_string(total_count));
}

std::string create_contents(work_item const& wi, long index) {
  std::ostringstream os;
  os << "Prefix: " << wi.prefix << "\nUse Hash Prefix: " << std::boolalpha
     << wi.use_hash_prefix << "\nObject Index: " << index << "\n";
  return std::move(os).str();
}

void process_one_item(gcs::Client client, pubsub::Message const& m) {
  auto wi = parse_message(m);
  for (std::int64_t i = 0; i != wi.object_count; ++i) {
    auto object_name = wi.prefix + "/object-" + std::to_string(i);
    auto hashed = hashed_name(wi.use_hash_prefix, std::move(object_name));
    client.InsertObject(wi.bucket, hashed, create_contents(wi, i)).value();
  }
}

/// Run the worker thread for a GKE batch job.
void worker(boost::program_options::variables_map const& vm) {
  std::cout << "Running in worker mode" << std::endl;

  if (vm.count("project") == 0 or vm["project"].as<std::string>().empty()) {
    throw std::runtime_error("the `schedule` action requires --subscription");
  }
  if (vm.count("subscription") == 0) {
    throw std::runtime_error("the `schedule` action requires --subscription");
  }
  auto const concurrency = vm["concurrency"].as<int>();
  auto const project_id = vm["project"].as<std::string>();
  auto const subscription_id = vm["subscription"].as<std::string>();

  using namespace std::chrono_literals;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;
  auto const subscription = pubsub::Subscription(project_id, subscription_id);
  auto subscriber = pubsub::Subscriber(pubsub::MakeSubscriberConnection(
      subscription,
      google::cloud::Options{}
          .set<pubsub::MaxOutstandingMessagesOption>(concurrency)
          .set<pubsub::MaxOutstandingBytesOption>(concurrency * 1024)
          .set<pubsub::MaxConcurrencyOption>(concurrency)
          .set<pubsub::MaxDeadlineTimeOption>(300s)
          .set<google::cloud::GrpcBackgroundThreadPoolSizeOption>(
              concurrency)));

  std::atomic<std::int64_t> latency{0};
  std::atomic<std::int64_t> attempts{0};
  std::atomic<std::int64_t> counter{0};
  auto handler = [&, cl = gcs::Client::CreateDefaultClient().value()](
                     pubsub::Message const& m, pubsub::AckHandler h) {
    auto const start = std::chrono::steady_clock::now();
    process_one_item(cl, m);
    auto const elapsed = std::chrono::steady_clock::now() - start;
    latency.fetch_add(duration_cast<milliseconds>(elapsed).count());
    attempts.fetch_add(h.delivery_attempt());
    ++counter;
    std::move(h).ack();
  };

  using namespace std::chrono_literals;
  using google::cloud::StatusCode;
  auto session = subscriber.Subscribe(handler);
  auto total = counter.exchange(0);
  auto mean = [&total](std::int64_t v) -> std::int64_t {
    if (total == 0) return 0;
    return v / total;
  };
  while (session.wait_for(30s) == std::future_status::timeout) {
    auto last = counter.exchange(0);
    total += last;
    std::cout << "Processed " << last << " work items"
              << ", latency=" << mean(latency.load())
              << ", attempts=" << mean(attempts.load()) << ", count=" << total
              << std::endl;
  }

  auto status = session.get();
  std::cout << "Session finished with " << status << std::endl;

  std::ostringstream os;
  os << "Unrecoverable error in Subscriber::Subscribe " << status;
  throw std::runtime_error(std::move(os).str());
}

}  // namespace
