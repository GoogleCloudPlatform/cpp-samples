// Copyright 2016 Google Inc.
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

#include "parse_arguments.h"
#include <google/cloud/speech/speech_client.h>
#include <iostream>
#include <string>

namespace speech = ::google::cloud::speech;

static const char kUsage[] =
    "Usage:\n"
    "   async_transcribe [--bitrate N] "
    "gs://bucket/audio.(raw|ulaw|flac|amr|awb)\n";

int main(int argc, char** argv) try {
  // [START speech_async_recognize_gcs]
  // Create a Speech client with the default configuration
  auto client = speech::SpeechClient(speech::MakeSpeechConnection());
  // Parse command line arguments.
  auto args = ParseArguments(argc, argv);
  auto const file_path = args.path;
  speech::v1::LongRunningRecognizeRequest request;
  *request.mutable_config() = args.config;

  // Pass the Google Cloud Storage URI to the request.
  request.mutable_audio()->set_uri(file_path);
  // Call LongRunningRecognize().
  auto future = client.LongRunningRecognize(request);
  // Wait for the operation to complete.  Check the status once per second.
  std::cout << "Waiting for operation to complete ";
  while (future.wait_for(std::chrono::seconds(1)) ==
         std::future_status::timeout) {
    std::cout << '.' << std::flush;
  }
  std::cout << "DONE\n";
  auto response = future.get();
  // If the is an error just report it:
  if (!response) {
    std::cerr << "Error in LongRunningRecognize: " << response.status() << "\n";
    return 1;
  }
  // Dump the transcript of all the results.
  for (auto const& result : response->results()) {
    for (auto const& alternative : result.alternatives()) {
      std::cout << alternative.confidence() << "\t" << alternative.transcript()
                << "\n";
    }
  }
  // [END speech_async_recognize_gcs]
  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n"
            << kUsage << "\n";
  return 1;
}
