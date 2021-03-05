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
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>
#include <google/cloud/storage/client.h>
#include <google/cloud/storage/parallel_upload.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>

namespace {
namespace po = boost::program_options;
namespace gcs = google::cloud::storage;
po::variables_map parse_command_line(int argc, char* argv[]);

using ::gcs_fast_transfers::file_info;
using ::gcs_fast_transfers::format_size;
using ::gcs_fast_transfers::kMiB;

}  // namespace

int main(int argc, char* argv[]) try {
  auto vm = parse_command_line(argc, argv);
  auto const bucket = vm["bucket"].as<std::string>();
  auto const object = vm["object"].as<std::string>();
  auto const source = vm["source"].as<std::string>();

  auto client = gcs::Client::CreateDefaultClient().value();

  std::cout << "Uploading " << source << " to bucket " << bucket
            << " as object " << object << " ..." << std::flush;

  auto const start = std::chrono::steady_clock::now();
  auto const scratch_prefix =
      boost::uuids::to_string(boost::uuids::random_generator_mt19937{}());
  auto metadata =
      gcs::ParallelUploadFile(
          client, source, bucket, object, scratch_prefix, true,
          gcs::MaxStreams(vm["max-streams"].as<int>()),
          gcs::MinStreamSize(vm["minimum-stream-size"].as<std::int64_t>()))
          .value();
  auto const end = std::chrono::steady_clock::now();

  std::cout << "DONE"
            << "\n";
  std::cout << "The upload was successful, the object size is approximately "
            << format_size(metadata.size()) << "\n";

  auto const elapsed_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  auto const elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto const effective_bandwidth_MiBs =
      (static_cast<double>(metadata.size()) / kMiB) /
      (elapsed_us.count() / 1'000'000.0);
  std::cout << "Upload completed in " << elapsed_ms.count() << "ms\n"
            << "Effective bandwidth " << effective_bandwidth_MiBs << " MiB/s\n";

  auto [size, crc32c] = file_info(source);
  if (size != metadata.size()) {
    std::cout << "Uploaded file size mismatch, expected=" << metadata.size()
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
char const* kPositional[] = {"source", "bucket", "object"};

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
  auto const default_min_stream_size = 64 * 1024 * 1024L;
  auto const default_max_streams = [] {
    auto constexpr kFallbackStreamCount = 4;
    auto constexpr kStreamsPerCore = 4;
    auto const count = std::thread::hardware_concurrency();
    if (count == 0) return kFallbackStreamCount;
    return static_cast<int>(count * kStreamsPerCore);
  }();

  po::positional_options_description positional;
  for (auto const* name : kPositional) positional.add(name, 1);
  po::options_description desc(
      "Upload a single GCS object using multiple slices");
  desc.add_options()("help", "produce help message")
      //
      ("source", po::value<std::string>()->required(),
       "set the object file to upload")
      //
      ("bucket", po::value<std::string>()->required(),
       "set the GCS bucket to upload to")
      //
      ("object", po::value<std::string>()->required(),
       "set the GCS object to upload")
      //
      ("max-streams", po::value<int>()->default_value(default_max_streams),
       "number of parallel streams for the upload")
      //
      ("minimum-stream-size",
       po::value<std::int64_t>()->default_value(default_min_stream_size),
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

  if (vm["max-streams"].as<int>() == 0) {
    usage(argv[0], desc, "the --max-streams option cannot be zero");
  }
  if (vm["minimum-stream-size"].as<std::int64_t>() == 0) {
    usage(argv[0], desc, "the --minimum-stream-size option cannot be zero");
  }

  return vm;
}

}  // namespace
