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
#include <google/cloud/completion_queue.h>
#include <google/cloud/grpc_options.h>
#include <google/cloud/speech/speech_client.h>
#include <coroutine>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace g = ::google::cloud;
namespace speech = ::google::cloud::speech;
using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    speech::v1::StreamingRecognizeRequest,
    speech::v1::StreamingRecognizeResponse>;

auto constexpr kUsage = R"""(Usage:
  streaming_transcribe_singlethread [--bitrate N] audio.(raw|ulaw|flac|amr|awb)
)""";

g::future<void> ReadTranscript(RecognizeStream& stream) {
  while (true) {
    auto response = co_await stream.Read();
    if (!response) co_return;
    for (auto const& result : response->results()) {
      std::cout << "Result stability: " << result.stability() << "\n";
      for (auto const& alternative : result.alternatives()) {
        std::cout << alternative.confidence() << "\t"
                  << alternative.transcript() << "\n";
      }
    }
  }
}

g::future<void> WriteAudio(RecognizeStream& stream,speech::v1::StreamingRecognizeRequest request, std::string const& path,
                           g::CompletionQueue cq) {
  auto file = std::ifstream(path, std::ios::binary);
  while (file) {
    co_await cq.MakeRelativeTimer(std::chrono::seconds(1));
    auto constexpr kChunkSize = 64 * 1024;
    std::vector<char> chunk(kChunkSize);
    file.read(chunk.data(), chunk.size());
    auto const bytes_read = file.gcount();
    if (bytes_read > 0) {
    request.clear_streaming_config();
      request.set_audio_content(chunk.data(), bytes_read);
      std::cout << "Sending " << bytes_read / 1024 << "k bytes." << std::endl;
      if (not co_await stream.Write(request, grpc::WriteOptions())) co_return;
    }
  }
  co_await stream.WritesDone();
}

g::future<g::Status> StreamingTranscribe(g::CompletionQueue cq,
                                         ParseResult args) {
  // Create a Speech client with the default configuration.
  auto client = speech::SpeechClient(speech::MakeSpeechConnection(
      g::Options{}.set<g::GrpcCompletionQueueOption>(cq)));

  speech::v1::StreamingRecognizeRequest request;
  auto& streaming_config = *request.mutable_streaming_config();
  *streaming_config.mutable_config() = std::move(args.config);

  // Get ready to write audio content.  Create the stream, and start it.
  auto stream =
      client.AsyncStreamingRecognize(google::cloud::ExperimentalTag{});

  // The stream can fail to start; `.get()` returns an `false` in this case.
  if (not co_await stream->Start()) co_return co_await stream->Finish();

  // Write the first request, containing the config only.
  if (not co_await stream->Write(request, grpc::WriteOptions{})) {
    // If the initial write fails, we need to collect the state and return
    co_return co_await stream->Finish();
  }

  // Start a coroutine to read the responses
  auto reader = ReadTranscript(*stream);
  auto writer = WriteAudio(*stream, std::move(request), args.path, cq);

  co_await std::move(writer);
  co_await std::move(reader);

  co_return co_await stream->Finish();
}

int main(int argc, char* argv[]) try {
  // Create a CompletionQueue to demux the I/O and other asynchronous
  // operations, and dedicate a thread to it.
  g::CompletionQueue cq;
  auto runner = std::thread{[](auto cq) { cq.Run(); }, cq};

  auto status = StreamingTranscribe(cq, ParseArguments(argc, argv)).get();

  // Shutdown the completion queue
  cq.Shutdown();
  runner.join();

  if (!status.ok()) {
    std::cerr << "Error in transcribe stream: " << status << "\n";
    return 1;
  }
  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard C++ exception thrown: " << ex.what() << "\n"
            << kUsage << "\n";
  return 1;
}
