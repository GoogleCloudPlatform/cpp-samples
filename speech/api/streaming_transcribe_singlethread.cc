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
#include <grpc++/grpc++.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "parse_arguments.h"
#include "google/cloud/speech/v1/cloud_speech.grpc.pb.h"

using google::cloud::speech::v1::RecognitionConfig;
using google::cloud::speech::v1::Speech;
using google::cloud::speech::v1::StreamingRecognizeRequest;
using google::cloud::speech::v1::StreamingRecognizeResponse;

static const char usage[] =
    "Usage:\n"
    "   streaming_transcribe_singlethread "
    "[--bitrate N] audio.(raw|ulaw|flac|amr|awb)\n";

int main(int argc, char** argv) {
  // Create a Speech Stub connected to the speech service.
  auto creds = grpc::GoogleDefaultCredentials();
  auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
  std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));
  // Parse command line arguments.
  StreamingRecognizeRequest request;
  auto* streaming_config = request.mutable_streaming_config();
  char* file_path =
      ParseArguments(argc, argv, streaming_config->mutable_config());
  if (nullptr == file_path) {
    std::cerr << usage;
    return -1;
  }
  // Many things are happening at once:
  // 1. Writing the initial request to the stream.
  // 2. Writing chunks of audio content to the stream.
  // 3. Reading responses from the stream.
  // 4. Shutting down the stream.
  // So, use a completion queue and tags to track them.
  grpc::CompletionQueue cq;
  struct Tag {
    bool happening_now;  // Gets set to false when the operation completes.
    const char* name;
  };
  Tag create_stream = { true, "create stream" };
  Tag reading = { false, "reading" };
  Tag writing = { false, "writing" };
  Tag writes_done = { false, "writes done" };
  Tag finishing = { false, "finishing" };
  grpc::Status status;
  bool server_closed_stream = false;
  // Create the stream reader/writer.
  grpc::ClientContext context;
  auto streamer = speech->AsyncStreamingRecognize(
      &context, &cq, &create_stream);

  bool ok = false;
  Tag* tag = nullptr;
  // Block until the creation of the stream is done, we cannot start
  // writing until that happens ...
  if (cq.Next((void**)&tag, &ok)) {
    std::cout << tag->name << " completed." << std::endl;
    tag->happening_now = false;
    if (tag != &create_stream) {
      std::cerr << "Expected create_stream in cq." << std::endl;
      return -1;
    }
    if (!ok) {
      std::cerr << "Stream closed while creating it." << std::endl;
      return -1;
    }
  } else {
    std::cerr << "The completion queue unexpectedly shutdown or timedout." << std::endl;
    return -1;
  }

  StreamingRecognizeResponse response;
  // Write the first request, containing the config only.
  streaming_config->set_interim_results(true);
  writing.happening_now = true;
  streamer->Write(request, &writing);
  // Get ready to write audio content.  Open the file, allocate a chunk, and
  // start a timer.  Use the timer to simulate a microphone, where the audio
  // content is arriving in one chunk per second.
  std::ifstream file_stream(file_path);
  const size_t chunk_size = 64 * 1024;
  std::vector<char> chunk(chunk_size);
  std::chrono::system_clock::time_point next_write_time_point =
      std::chrono::system_clock::time_point::min();
  bool writes_completed = false;
  do {
    if (!reading.happening_now && !finishing.happening_now) {
      reading.happening_now = true;
      // AsyncNext(), called below, will return the reading tag when the read
      // completes.
      streamer->Read(&response, &reading);
    }
    auto now = std::chrono::system_clock::now();
    if (now >= next_write_time_point && !writing.happening_now) {
      // Time to write another chunk of the file on the stream.
      std::streamsize bytes_read =
          file_stream.rdbuf()->sgetn(&chunk[0], chunk.size());
      request.clear_streaming_config();
      request.set_audio_content(&chunk[0], bytes_read);
      std::cout << "Sending " << bytes_read / 1024 << "k bytes." << std::endl;
      writing.happening_now = true;
      streamer->Write(request, &writing);
      if (bytes_read < chunk.size()) {
        // Done writing.
	writes_completed = true;
        next_write_time_point =  // Never write again.
            std::chrono::system_clock::time_point::max();
      } else {
        // Schedule the next write for one second from now.
        next_write_time_point = now + std::chrono::duration<long>(1);
      }
    }
    // Wait for a pending operation to complete.  Identify the operation that
    // completed by examining tag.
    switch (cq.AsyncNext((void**)&tag, &ok, next_write_time_point)) {
      case grpc::CompletionQueue::SHUTDOWN:
        std::cerr << "The completion queue unexpectedly shutdown." << std::endl;
        return -1;
      case grpc::CompletionQueue::GOT_EVENT:
        std::cout << tag->name << " completed." << std::endl;
        tag->happening_now = false;
        if (tag == &reading) {
          // Dump the transcript of all the results.
          for (int r = 0; r < response.results_size(); ++r) {
            auto result = response.results(r);
            std::cout << "Result stability: " << result.stability()
                      << std::endl;
            for (int a = 0; a < result.alternatives_size(); ++a) {
              auto alternative = result.alternatives(a);
              std::cout << alternative.confidence() << "\t"
                        << alternative.transcript() << std::endl;
            }
          }
        }
	if (tag == &writing && writes_completed) {
	  // After the last Write, send a WritesDone() ...
	  writes_done.happening_now = true;
          // AsyncNext(), called above, will return the writes_done tag when
          // the writes_done operation completes.
	  streamer->WritesDone(&writes_done);
	}
        if (!ok) {
          server_closed_stream = true;
          finishing.happening_now = true;
          // AsyncNext() will return the finishing tag when the finishing
          // operation completes.
          streamer->Finish(&status, &finishing);
        }
        break;
      case grpc::CompletionQueue::TIMEOUT:
        break;  // Time to send another chunk of audio content.
    }
  } while (!server_closed_stream || finishing.happening_now);
  if (!status.ok()) {
    // Report the RPC failure.
    std::cerr << status.error_message() << std::endl;
    return -1;
  }
  return 0;
}
