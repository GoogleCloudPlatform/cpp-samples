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
#include <google/longrunning/operations.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <string>
#include <thread>

using google::cloud::speech::v1::LongRunningRecognizeRequest;
using google::cloud::speech::v1::LongRunningRecognizeResponse;
using google::cloud::speech::v1::Speech;

static const char kUsage[] =
    "Usage:\n"
    "   async_transcribe [--bitrate N] "
    "gs://bucket/audio.(raw|ulaw|flac|amr|awb)\n";

int main(int argc, char** argv) {
  // [START speech_async_recognize_gcs]
  // Create a Speech stub connected to the speech service.
  auto creds = grpc::GoogleDefaultCredentials();
  auto channel = grpc::CreateChannel("speech.googleapis.com", creds);
  std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));
  // Create a long operations stub so we can check the progress of the async
  // request.
  std::unique_ptr<google::longrunning::Operations::Stub> long_operations(
      google::longrunning::Operations::NewStub(channel));
  // Parse command line arguments.
  LongRunningRecognizeRequest request;
  char* file_path = ParseArguments(argc, argv, request.mutable_config());
  if (nullptr == file_path) {
    std::cerr << kUsage;
    return -1;
  }
  // Pass the Google Cloud Storage URI to the request.
  request.mutable_audio()->set_uri(file_path);
  // Call LongRunningRecognize().
  grpc::ClientContext context;
  google::longrunning::Operation op;
  grpc::Status rpc_status =
      speech->LongRunningRecognize(&context, request, &op);
  if (!rpc_status.ok()) {
    // Report the RPC failure.
    std::cerr << rpc_status.error_message() << std::endl;
    return -1;
  }
  // Wait for the operation to complete.  Check the status once per second.
  std::cout << "Waiting for operation " << op.name() << " to complete.";
  google::longrunning::GetOperationRequest get_op_request;
  get_op_request.set_name(op.name());
  while (!op.done()) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    grpc::ClientContext op_context;
    rpc_status =
        long_operations->GetOperation(&op_context, get_op_request, &op);
    if (!rpc_status.ok()) {
      // Report the RPC failure.
      std::cerr << rpc_status.error_message() << std::endl;
      return -1;
    }
  }
  std::cout << std::endl;
  // Unpack the response.
  if (!op.response().Is<LongRunningRecognizeResponse>()) {
    std::cerr << "The operation completed, but did not contain a "
              << "LongRunningRecognizeResponse.";
    return -1;
  }
  LongRunningRecognizeResponse response;
  op.response().UnpackTo(&response);
  // Dump the transcript of all the results.
  for (int r = 0; r < response.results_size(); ++r) {
    const auto& result = response.results(r);
    for (int a = 0; a < result.alternatives_size(); ++a) {
      const auto& alternative = result.alternatives(a);
      std::cout << alternative.confidence() << "\t" << alternative.transcript()
                << std::endl;
    }
  }
  // [END speech_async_recognize_gcs]
  return 0;
}
