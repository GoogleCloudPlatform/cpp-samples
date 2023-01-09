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
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

// [START speech_streaming_recognize]
namespace speech = ::google::cloud::speech;
using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    speech::v1::StreamingRecognizeRequest,
    speech::v1::StreamingRecognizeResponse>;

auto constexpr kUsage = R"""(Usage:
  streaming_transcribe [--bitrate N] audio.(raw|ulaw|flac|amr|awb)
)""";

// Write the audio in 64k chunks at a time, simulating audio content arriving
// from a microphone.
void MicrophoneThreadMain(RecognizeStream& stream,
                          std::string const& file_path) {
  speech::v1::StreamingRecognizeRequest request;
  std::ifstream file_stream(file_path, std::ios::binary);
  auto constexpr kChunkSize = 64 * 1024;
  std::vector<char> chunk(kChunkSize);
  while (true) {
    // Read another chunk from the file.
    file_stream.read(chunk.data(), chunk.size());
    auto const bytes_read = file_stream.gcount();
    // And write the chunk to the stream.
    if (bytes_read > 0) {
      request.set_audio_content(chunk.data(), bytes_read);
      std::cout << "Sending " << bytes_read / 1024 << "k bytes." << std::endl;
      if (!stream.Write(request, grpc::WriteOptions()).get()) break;
    }
    if (!file_stream) {
      // Done reading everything from the file, so done writing to the stream.
      stream.WritesDone().get();
      break;
    }
    // Wait a second before writing the next chunk.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

int main(int argc, char** argv) try {
  // Create a Speech client with the default configuration
  auto client = speech::SpeechClient(speech::MakeSpeechConnection());

  // Parse command line arguments.
  auto args = ParseArguments(argc, argv);
  auto const file_path = args.path;

  speech::v1::StreamingRecognizeRequest request;
  auto& streaming_config = *request.mutable_streaming_config();
  *streaming_config.mutable_config() = args.config;

  // Begin a stream.
  auto stream = client.AsyncStreamingRecognize();
  // The stream can fail to start, and `.get()` returns an error in this case.
  if (!stream->Start().get()) throw stream->Finish().get();
  // Write the first request, containing the config only.
  if (!stream->Write(request, grpc::WriteOptions{}).get()) {
    // Write().get() returns false if the stream is closed.
    throw stream->Finish().get();
  }

  // Simulate a microphone thread using the file as input.
  auto microphone =
      std::thread(MicrophoneThreadMain, std::ref(*stream), file_path);
  // Read responses.
  auto read = [&stream] { return stream->Read().get(); };
  for (auto response = read(); response.has_value(); response = read()) {
    // Dump the transcript of all the results.
    for (auto const& result : response->results()) {
      std::cout << "Result stability: " << result.stability() << "\n";
      for (auto const& alternative : result.alternatives()) {
        std::cout << alternative.confidence() << "\t"
                  << alternative.transcript() << "\n";
      }
    }
  }
  auto status = stream->Finish().get();
  microphone.join();
  if (!status.ok()) throw status;
  return 0;
} catch (google::cloud::Status const& s) {
  std::cerr << "Recognize stream finished with an error: " << s << "\n";
  return 1;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n"
            << kUsage << "\n";
  return 1;
}
// [END speech_streaming_recognize]
