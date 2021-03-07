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
#include <google/cloud/speech/v1/cloud_speech.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <strings.h>

using google::cloud::speech::v1::RecognizeRequest;
using google::cloud::speech::v1::RecognizeResponse;
using google::cloud::speech::v1::Speech;

static const char kUsage[] =
    "Usage:\n"
    "   transcribe [--bitrate N] audio.(raw|ulaw|flac|amr|awb)\n";

int main(int argc, char** argv) {
  // [START speech_sync_recognize]
  // [START speech_sync_recognize_gcs]
  // Create a Speech Stub connected to the speech service.
  auto creds = grpc::GoogleDefaultCredentials();
  auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
  std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));
  // Parse command line arguments.
  RecognizeRequest request;
  char* file_path = ParseArguments(argc, argv, request.mutable_config());
  if (nullptr == file_path) {
    std::cerr << kUsage;
    return -1;
  }
  // [END speech_sync_recognize_gcs]
  // [END speech_sync_recognize]
  if (0 == strncasecmp("gs:/", file_path, 4)) {
    // [START speech_sync_recognize_gcs]
    // Pass the Google Cloud Storage URI to the request.
    request.mutable_audio()->set_uri(file_path);
    // [END speech_sync_recognize_gcs]
  } else {
    // [START speech_sync_recognize]
    // Load the audio file from disk into the request.
    request.mutable_audio()->mutable_content()->assign(
        std::istreambuf_iterator<char>(std::ifstream(file_path).rdbuf()),
        std::istreambuf_iterator<char>());
    // [END speech_sync_recognize]
  }
  // [START speech_sync_recognize]
  // [START speech_sync_recognize_gcs]
  // Send audio content using Recognize().
  grpc::ClientContext context;
  RecognizeResponse response;
  grpc::Status rpc_status = speech->Recognize(&context, request, &response);
  if (!rpc_status.ok()) {
    // Report the RPC failure.
    std::cerr << rpc_status.error_message() << std::endl;
    return -1;
  }
  // Dump the transcript of all the results.
  for (int r = 0; r < response.results_size(); ++r) {
    const auto& result = response.results(r);
    for (int a = 0; a < result.alternatives_size(); ++a) {
      const auto& alternative = result.alternatives(a);
      std::cout << alternative.confidence() << "\t" << alternative.transcript()
                << std::endl;
    }
  }
  // [END speech_sync_recognize_gcs]
  // [END speech_sync_recognize]
  return 0;
}
