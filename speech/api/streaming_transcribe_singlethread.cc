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

class Handler : public std::enable_shared_from_this<Handler> {
 public:
  static std::shared_ptr<Handler> Create(google::cloud::CompletionQueue cq,
                                         ParseResult args) {
    return std::shared_ptr<Handler>(
        new Handler(std::move(cq), std::move(args)));
  }

  g::future<g::Status> Start(speech::SpeechClient& client) {
    // Get ready to write audio content.  Create the stream, and start it.
    stream_ = client.AsyncStreamingRecognize();
    // The stream can fail to start; `.get()` returns `false` in this case.
    if (!stream_->Start().get()) return StartFailure();
    // Write the first request, containing the config only.
    if (!stream_->Write(request_, grpc::WriteOptions{}).get()) {
      // Write().get() returns false if the stream is closed.
      return StartFailure();
    }
    auto self = shared_from_this();
    // This creates a series of reads; when each read completes successfully a
    // new one starts.
    stream_->Read().then([self](auto f) { self->OnRead(f.get()); });
    // This creates a series of timer -> write -> timer -> ... steps.
    cq_.MakeRelativeTimer(std::chrono::seconds(1)).then([self](auto f) {
      self->OnTimer(f.get().status());
    });
    return done_.get_future();
  }

 private:
  Handler(google::cloud::CompletionQueue cq, ParseResult args)
      : cq_(std::move(cq)), file_(args.path, std::ios::binary) {
    auto& streaming_config = *request_.mutable_streaming_config();
    *streaming_config.mutable_config() = std::move(args.config);
  }

  g::future<g::Status> StartFailure() {
    auto self = shared_from_this();
    writing_ = false;
    reading_ = false;
    stream_->Finish().then([self](auto f) { self->OnFinish(f.get()); });
    return done_.get_future();
  }

  void OnFinish(g::Status s) { done_.set_value(s); }

  void OnTimer(g::Status s) {
    // On a timer error the completion queue is not usable so just return.
    if (!s.ok()) return;
    auto self = shared_from_this();
    auto constexpr kChunkSize = 64 * 1024;
    std::vector<char> chunk(kChunkSize);
    file_.read(chunk.data(), chunk.size());
    auto const bytes_read = file_.gcount();
    if (bytes_read > 0) {
      request_.clear_streaming_config();
      request_.set_audio_content(chunk.data(), bytes_read);
      std::cout << "Sending " << bytes_read / 1024 << "k bytes." << std::endl;
      stream_->Write(request_, grpc::WriteOptions()).then([self](auto f) {
        self->OnWrite(f.get());
      });
    }
  }

  void OnRead(absl::optional<speech::v1::StreamingRecognizeResponse> response) {
    if (!response.has_value()) return CloseReadSide();
    // Dump the transcript of all the results.
    for (auto const& result : response->results()) {
      std::cout << "Result stability: " << result.stability() << "\n";
      for (auto const& alternative : result.alternatives()) {
        std::cout << alternative.confidence() << "\t"
                  << alternative.transcript() << "\n";
      }
    }
    auto self = shared_from_this();
    stream_->Read().then([self](auto f) { self->OnRead(f.get()); });
  }

  void OnWrite(bool ok) {
    if (!ok) return CloseWriteSide();
    auto self = shared_from_this();
    // If the file is not usable, start the process to close the stream.
    if (!file_) {
      stream_->WritesDone().then(
          [self](auto f) { self->OnWritesDone(f.get()); });
      return;
    }
    // Schedule a new timer to read more data.
    cq_.MakeRelativeTimer(std::chrono::seconds(1)).then([&](auto f) {
      self->OnTimer(f.get().status());
    });
  }

  void OnWritesDone(bool) { CloseWriteSide(); }

  void CloseWriteSide() {
    writing_ = false;
    if (!reading_) return Close();
  }

  void CloseReadSide() {
    reading_ = false;
    if (!writing_) return Close();
  }

  void Close() {
    auto self = shared_from_this();
    stream_->Finish().then([self](auto f) { self->OnFinish(f.get()); });
  }

  google::cloud::CompletionQueue cq_;
  speech::v1::StreamingRecognizeRequest request_;
  std::unique_ptr<RecognizeStream> stream_;
  std::ifstream file_;
  bool writing_ = true;
  bool reading_ = true;
  g::promise<g::Status> done_;
};

int main(int argc, char** argv) try {
  // Create a CompletionQueue to demux the I/O and other asynchronous
  // operations, and dedicate a thread to it.
  g::CompletionQueue cq;
  auto runner = std::thread{[](auto cq) { cq.Run(); }, cq};

  // Create a Speech client with the default configuration.
  auto client = speech::SpeechClient(speech::MakeSpeechConnection(
      g::Options{}.set<g::GrpcCompletionQueueOption>(cq)));

  // Create a handler for the stream and run it until closed.
  auto handler = Handler::Create(cq, ParseArguments(argc, argv));
  auto status = handler->Start(client).get();

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
