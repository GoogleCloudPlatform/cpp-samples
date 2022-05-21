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
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace speech = ::google::cloud::speech;

auto constexpr kUsage = R"""(Usage:
  transcribe [--bitrate N] audio.(raw|ulaw|flac|amr|awb)
)""";

int main(int argc, char** argv) try {
  // [START speech_sync_recognize]
  // [START speech_sync_recognize_gcs]
  // Create a Speech client with the default configuration
  auto client = speech::SpeechClient(speech::MakeSpeechConnection());
  // Parse command line arguments.
  auto const args = ParseArguments(argc, argv);
  speech::v1::RecognizeRequest request;
  *request.mutable_config() = args.config;
  // [END speech_sync_recognize_gcs]
  // [END speech_sync_recognize]
  if (args.path.rfind("gs://", 0) == 0) {
    // [START speech_sync_recognize_gcs]
    // Pass the Google Cloud Storage URI to the request.
    request.mutable_audio()->set_uri(args.path);
    // [END speech_sync_recognize_gcs]
  } else {
    // [START speech_sync_recognize]
    // Load the audio file from disk into the request.
    auto content =
        std::string{std::istreambuf_iterator<char>(
                        std::ifstream(args.path, std::ios::binary).rdbuf()),
                    {}};
    request.mutable_audio()->mutable_content()->assign(std::move(content));
    // [END speech_sync_recognize]
  }
  // [START speech_sync_recognize]
  // [START speech_sync_recognize_gcs]
  // Send audio content using Recognize().
  auto response = client.Recognize(request);
  if (!response) throw std::move(response).status();
  // Dump the transcript of all the results.
  for (auto const& result : response->results()) {
    for (auto const& alternative : result.alternatives()) {
      std::cout << alternative.confidence() << "\t" << alternative.transcript()
                << "\n";
    }
  }
  // [END speech_sync_recognize_gcs]
  // [END speech_sync_recognize]
  return 0;
} catch (google::cloud::Status const& s) {
  std::cerr << "Recognize failed with: " << s << "\n";
  return 1;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n"
            << kUsage << "\n";
  return 1;
}
