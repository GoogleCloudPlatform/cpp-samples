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

#include <boost/program_options.hpp>
#include <google/cloud/storage/client.h>
#include <cstdint>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace {
namespace po = boost::program_options;
namespace gcs = google::cloud::storage;
std::tuple<po::variables_map, po::options_description> parse_command_line(
    int argc, char* argv[]);
std::string format_size(std::int64_t size);
auto constexpr kKiB = std::int64_t(1024);
auto constexpr kMiB = 1024 * kKiB;
auto constexpr kGiB = 1024 * kMiB;
auto constexpr kTiB = 1024 * kGiB;
auto constexpr kPiB = 1024 * kTiB;
}  // namespace

int main(int argc, char* argv[]) try {
  auto [vm, desc] = parse_command_line(argc, argv);
  auto help = [d = std::move(desc)](po::variables_map const&) {
    std::cout << "Usage: " << d << "\n";
    return 0;
  };
  if (vm.count("help")) return help(vm);

  for (std::string opt : {"bucket", "object", "destination"}) {
    if (vm.count(opt) != 0) continue;
    if (not vm[opt].as<std::string>().empty()) continue;
    throw std::runtime_error("the --" + opt + " option cannot be empty");
  }
  auto const bucket = vm["bucket"].as<std::string>();
  auto const object = vm["object"].as<std::string>();
  auto const destination = vm["destination"].as<std::string>();
  auto const desired_thread_count = vm["thread-count"].as<int>();
  if (desired_thread_count == 0) {
    throw std::runtime_error("the --thread-count option cannot be zero");
  }
  auto const minimum_slice_size = vm["minimum-slice-size"].as<std::int64_t>();

  auto client = gcs::Client::CreateDefaultClient().value();
  auto metadata = client.GetObjectMetadata(bucket, object).value();

  auto [slice_size, thread_count] = [&]() -> std::pair<std::int64_t, int> {
    auto const thread_slice = metadata.size() / desired_thread_count;
    if (thread_slice >= minimum_slice_size) {
      return {thread_slice, desired_thread_count};
    }
    auto const threads = metadata.size() / minimum_slice_size;
    if (threads == 0) {
      return {metadata.size(), 1};
    }
    return {minimum_slice_size, static_cast<int>(threads)};
  }();

  std::cout << "Downloading " << object << " from bucket " << bucket
            << " to file " << destination << "\nThis object has "
            << format_size(metadata.size())
            << " in size. It will be downloaded in " << thread_count
            << " slices, each approximately " << format_size(slice_size)
            << " in size" << std::endl;

  auto task = [](std::int64_t offset, std::int64_t length,
                 std::string const& bucket, std::string const& object,
                 int fd) {
    auto client = gcs::Client::CreateDefaultClient().value();
    auto is = client.ReadObject(bucket, object,
                                gcs::ReadRange(offset, offset + length));

    std::vector<char> buffer(1024 * 1024L);
    std::int64_t count = 0;
    std::int64_t write_offset = offset;
    do {
      is.read(buffer.data(), buffer.size());
      if (is.bad()) break;
      count += is.gcount();
      ::pwrite(fd, buffer.data(), is.gcount(), write_offset);
      write_offset += is.gcount();
    } while (not is.eof());
    std::ostringstream result;
    result << "Downloaded range [" << offset << ","
           << std::to_string(offset + length) << "] got " << count << "/"
           << length << " bytes";
    return std::move(result).str();
  };

  auto const start = std::chrono::steady_clock::now();
  std::vector<std::future<std::string>> tasks;
  auto const fd = ::open(destination.c_str(), O_CREAT | O_TRUNC | O_WRONLY);
  for (std::int64_t offset = 0; offset < metadata.size();
       offset += slice_size) {
    auto const current_slice_size =
        std::min<std::int64_t>(slice_size, metadata.size() - offset);
    tasks.push_back(std::async(std::launch::async, task, offset,
                               current_slice_size, bucket, object,
                               fd));
  }

  for (auto& t : tasks) {
    std::cout << t.get() << "\n";
  }
  ::close(fd);

  auto const end = std::chrono::steady_clock::now();
  auto const elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  auto const elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto const effective_bandwidth_MiBs =
      (static_cast<double>(metadata.size()) / kMiB) /
      (elapsed_us.count() / 1'000'000.0);
  std::cout << "Download completed in " << elapsed_ms.count() << "ms\n"
            << "Effective bandwidth " << effective_bandwidth_MiBs << " MiB/s\n";

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown C++ exception thrown" << std::endl;
  return 1;
}

namespace {
std::tuple<po::variables_map, po::options_description> parse_command_line(
    int argc, char* argv[]) {
  auto const default_minimum_slice_size = 64 * 1024 * 1024L;
  auto const default_thread_count = [] {
    auto constexpr kFallbackThreadCount = 2;
    auto constexpr kThreadsPerCore = 2;
    auto const count = std::thread::hardware_concurrency();
    if (count == 0) return kFallbackThreadCount;
    return static_cast<int>(count * kThreadsPerCore);
  }();

  po::positional_options_description positional;
  positional.add("bucket", 1);
  positional.add("object", 1);
  positional.add("destination", 1);
  po::options_description desc(
      "Download a single GCS object using multiple slices");
  desc.add_options()("help", "produce help message")
      //
      ("bucket", po::value<std::string>(),
       "set the GCS bucket to download from")
      //
      ("object", po::value<std::string>(),
       "set the GCS object to download from")
      //
      ("destination", po::value<std::string>(),
       "set the destination file to download into")
      //
      ("thread-count", po::value<int>()->default_value(default_thread_count),
       "number of parallel handlers to handle work items")
      //
      ("minimum-slice-size",
       po::value<std::int64_t>()->default_value(default_minimum_slice_size),
       "minimum slice size");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
                .options(desc)
                .positional(positional)
                .run(),
            vm);
  po::notify(vm);
  return {vm, desc};
}

std::string format_size(std::int64_t size) {
  struct range_definition {
    std::int64_t max_value;
    std::int64_t scale;
    char const* units;
  } ranges[] = {
      {kKiB, 1, "Bytes"},  {kMiB, kKiB, "KiB"}, {kGiB, kMiB, "MiB"},
      {kTiB, kGiB, "GiB"}, {kPiB, kTiB, "TiB"},
  };
  for (auto d : ranges) {
    if (size < d.max_value) return std::to_string(size / d.scale) + d.units;
  }
  return std::to_string(size / kPiB) + "PiB";
}

}  // namespace
