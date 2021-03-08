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

#include "gcs_fast_transfers.h"
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <google/cloud/storage/client.h>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
// Posix headers last.
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

namespace {
namespace po = boost::program_options;
namespace gcs = google::cloud::storage;
po::variables_map parse_command_line(int argc, char* argv[]);
int check_system_call(std::string const& name, int result);

std::vector<std::int64_t> compute_slices(std::int64_t object_size,
                                         po::variables_map const& vm) {
  auto const minimum_slice_size = vm["minimum-slice-size"].as<std::int64_t>();
  auto const thread_count = vm["thread-count"].as<int>();

  std::vector<std::int64_t> result;
  auto const thread_slice = object_size / thread_count;
  if (thread_slice >= minimum_slice_size) {
    std::fill_n(std::back_inserter(result), thread_count, thread_slice);
    // If the object size is not a multiple of the thread count we may need
    // to add any excess bytes to the last slice.
    result.back() += object_size % thread_count;
    return result;
  }
  for (; object_size > 0; object_size -= minimum_slice_size) {
    result.push_back(std::min(minimum_slice_size, object_size));
  }
  return result;
}

std::string task(std::int64_t offset, std::int64_t length,
                 std::string const& bucket, std::string const& object, int fd) {
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
    check_system_call("pwrite()",
                      ::pwrite(fd, buffer.data(), is.gcount(), write_offset));
    write_offset += is.gcount();
  } while (not is.eof());
  return fmt::format("Download range [{}, {}] got {}/{} bytes", offset,
                     offset + length, count, length);
}

using ::gcs_fast_transfers::file_info;
using ::gcs_fast_transfers::format_size;
using ::gcs_fast_transfers::kMiB;

}  // namespace

int main(int argc, char* argv[]) try {
  auto vm = parse_command_line(argc, argv);
  auto const bucket = vm["bucket"].as<std::string>();
  auto const object = vm["object"].as<std::string>();
  auto const destination = vm["destination"].as<std::string>();

  auto client = gcs::Client::CreateDefaultClient().value();
  auto metadata = client.GetObjectMetadata(bucket, object).value();

  auto slices = compute_slices(metadata.size(), vm);

  std::cout << "Downloading " << object << " from bucket " << bucket
            << " to file " << destination << "\n";
  std::cout << "This object size is approximately "
            << format_size(metadata.size()) << ". It will be downloaded in "
            << slices.size() << " slices." << std::endl;

  auto const start = std::chrono::steady_clock::now();
  auto constexpr kOpenFlags = O_CREAT | O_TRUNC | O_WRONLY;
  auto constexpr kOpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
  auto const fd = check_system_call(
      "open()", ::open(destination.c_str(), kOpenFlags, kOpenMode));

  std::vector<std::future<std::string>> tasks(slices.size());
  std::int64_t offset = 0;
  std::transform(slices.begin(), slices.end(), tasks.begin(), [&](auto length) {
    auto f = std::async(std::launch::async, task, offset, length, bucket,
                        object, fd);
    offset += length;
    return f;
  });

  for (auto& t : tasks) std::cout << t.get() << "\n";
  check_system_call("close(fd)", ::close(fd));

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

  auto [size, crc32c] = file_info(destination);
  if (size != metadata.size()) {
    std::cout << "Downloaded file size mismatch, expected=" << metadata.size()
              << ", got=" << size << std::endl;
    return 1;
  }

  if (crc32c != metadata.crc32c()) {
    std::cout << "Download file CRC32C mismatch, expected=" << metadata.crc32c()
              << ", got=" << crc32c << std::endl;
    return 1;
  }

  std::cout << "File size and CRC32C match expected values" << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown C++ exception thrown" << std::endl;
  return 1;
}

namespace {
char const* kPositional[] = {"bucket", "object", "destination"};

[[noreturn]] void usage(std::string const& argv0,
                        po::options_description const& desc,
                        std::string const& message = {}) {
  auto exit_status = EXIT_SUCCESS;
  if (not message.empty()) {
    exit_status = EXIT_FAILURE;
    std::cout << "Error: " << message << "\n";
  }

  // format positional args
  auto const positional_names =
      std::accumulate(std::begin(kPositional), std::end(kPositional),
                      std::string{" [options]"}, [](auto a, auto const& b) {
                        a += ' ';
                        a += b;
                        return a;
                      });

  // print usage + options help, and exit normally
  std::cout << "usage: " << argv0 << positional_names << "\n\n" << desc << "\n";
  std::exit(exit_status);
}

po::variables_map parse_command_line(int argc, char* argv[]) {
  auto const default_minimum_slice_size = 64 * 1024 * 1024L;
  auto const default_thread_count = [] {
    auto constexpr kFallbackThreadCount = 2;
    auto constexpr kThreadsPerCore = 2;
    auto const count = std::thread::hardware_concurrency();
    if (count == 0) return kFallbackThreadCount;
    return static_cast<int>(count * kThreadsPerCore);
  }();

  po::positional_options_description positional;
  for (auto const* name : kPositional) positional.add(name, 1);
  po::options_description desc(
      "Download a single GCS object using multiple slices");
  desc.add_options()("help", "produce help message")
      //
      ("bucket", po::value<std::string>()->required(),
       "set the GCS bucket to download from")
      //
      ("object", po::value<std::string>()->required(),
       "set the GCS object to download")
      //
      ("destination", po::value<std::string>()->required(),
       "set the destination file to download into")
      //
      ("thread-count", po::value<int>()->default_value(default_thread_count),
       "number of parallel streams for the download")
      //
      ("minimum-slice-size",
       po::value<std::int64_t>()->default_value(default_minimum_slice_size),
       "minimum slice size");

  // parse the input into the map
  po::variables_map vm;

  // run notify() for all registered options in the map
  try {
    po::parsed_options parsed = po::command_line_parser(argc, argv)
                                    .options(desc)
                                    .positional(positional)
                                    .run();
    po::store(parsed, vm);
    po::notify(vm);
  } catch (std::exception const& ex) {
    // if required arguments are missing but help is desired, just print help
    if (vm.count("help") > 0 or argc == 1) usage(argv[0], desc);
    usage(argv[0], desc, ex.what());
  }

  if (vm.count("help") != 0) usage(argv[0], desc);

  for (std::string opt : kPositional) {
    if (not vm[opt].as<std::string>().empty()) continue;
    usage(argv[0], desc, fmt::format("the {} argument cannot be empty", opt));
  }

  if (vm["thread-count"].as<int>() == 0) {
    usage(argv[0], desc, "the --thread-count option cannot be zero");
  }
  if (vm["minimum-slice-size"].as<std::int64_t>() == 0) {
    usage(argv[0], desc, "the --minimum-slice-size option cannot be zero");
  }

  return vm;
}

int check_system_call(std::string const& name, int result) {
  if (result >= 0) return result;
  auto err = errno;
  throw std::runtime_error(
      fmt::format("Error in {}() - return value={}, error=[{}] {}", name,
                  result, err, strerror(err)));
}

}  // namespace
