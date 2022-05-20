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
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using google::cloud::speech::v1::Speech;
using google::cloud::speech::v1::StreamingRecognizeRequest;
using google::cloud::speech::v1::StreamingRecognizeResponse;

// [START speech_streaming_recognize]
static const char kUsage[] =
    "Usage:\n"
    "   streaming_transcribe "
    "[--bitrate N] audio.(raw|ulaw|flac|amr|awb)\n";

// Write the audio in 64k chunks at a time, simulating audio content arriving
// from a microphone.
static void MicrophoneThreadMain(
    grpc::ClientReaderWriterInterface<StreamingRecognizeRequest,
                                      StreamingRecognizeResponse>* streamer,
    std::string const& file_path) {
  StreamingRecognizeRequest request;
  std::ifstream file_stream(file_path, std::ios::binary);
  auto const chunk_size = 64 * 1024;
  std::vector<char> chunk(chunk_size);
  while (true) {
    // Read another chunk from the file.
    file_stream.read(chunk.data(), chunk.size());
    auto const bytes_read = file_stream.gcount();
    // And write the chunk to the stream.
    if (bytes_read > 0) {
      request.set_audio_content(chunk.data(), bytes_read);
      std::cout << "Sending " << bytes_read / 1024 << "k bytes." << std::endl;
      streamer->Write(request);
    }
    if (!file_stream) {
      // Done reading everything from the file, so done writing to the stream.
      streamer->WritesDone();
      break;
    }
    // Wait a second before writing the next chunk.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

int main(int argc, char** argv) try {
  // Create a Speech Stub connected to the speech service.
  auto creds = grpc::GoogleDefaultCredentials();
  auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
  std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));

  // Parse command line arguments.
  auto args = ParseArguments(argc, argv);
  auto const file_path = args.path;

  StreamingRecognizeRequest request;
  auto& streaming_config = *request.mutable_streaming_config();
  *streaming_config.mutable_config() = args.config;

  // Begin a stream.
  grpc::ClientContext context;
  auto streamer = speech->StreamingRecognize(&context);
  // Write the first request, containing the config only.
  streaming_config.set_interim_results(true);
  streamer->Write(request);
  // The microphone thread writes the audio content.
  std::thread microphone_thread(&MicrophoneThreadMain, streamer.get(),
                                file_path);
  // Read responses.
  StreamingRecognizeResponse response;
  while (streamer->Read(&response)) {  // Returns false when no more to read.
    // Dump the transcript of all the results.
    for (auto const& result : response.results()) {
      std::cout << "Result stability: " << result.stability() << "\n";
      for (auto const& alternative : result.alternatives()) {
        std::cout << alternative.confidence() << "\t"
                  << alternative.transcript() << "\n";
      }
    }
  }
  grpc::Status status = streamer->Finish();
  microphone_thread.join();
  if (!status.ok()) {
    // Report the RPC failure.
    std::cerr << status.error_message() << std::endl;
    return -1;
  }
  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n"
            << kUsage << "\n";
  return 1;
}
// [END speech_streaming_recognize]
