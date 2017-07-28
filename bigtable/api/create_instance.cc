// Copyright 2017 Google Inc.
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

#include <google/bigtable/admin/v2/bigtable_instance_admin.grpc.pb.h>
#include <google/longrunning/operations.grpc.pb.h>
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>

#include <algorithm>
#include <chrono>
#include <ciso646>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) try {
  auto creds = grpc::GoogleDefaultCredentials();
  // Notice that Bigtable has separate endpoints for different APIs,
  // we are going to query the create an instance, which is one of the
  // admin APIs, so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableInstanceAdmin;

  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 5) {
    std::cerr << "Usage: create_instance <project_id> <instance_id> "
              << "<cluster_id> <zone>" << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  char const* instance_id = argv[2];
  char const* cluster_id = argv[3];
  char const* zone = argv[4];

  admin::CreateInstanceRequest req;
  req.set_parent(std::string("projects/") + project_id);
  req.set_instance_id(instance_id);

  // ... in this demo we only create DEVELOPMENT instances ...
  auto& instance = *req.mutable_instance();
  instance.set_display_name(instance_id);
  instance.set_type(admin::Instance::DEVELOPMENT);

  auto& cluster = (*req.mutable_clusters())[cluster_id];
  cluster.set_location(std::string("projects/") + project_id + "/locations/" +
                       zone);

  std::unique_ptr<BigtableInstanceAdmin::Stub> instance_admin(
      BigtableInstanceAdmin::NewStub(channel));
  std::unique_ptr<google::longrunning::Operations::Stub> operations(
      google::longrunning::Operations::NewStub(channel));

  // ... make the request to create an instance, that returns a "long
  // running operation" object ...
  grpc::ClientContext create_context;
  google::longrunning::Operation operation;
  auto status =
      instance_admin->CreateInstance(&create_context, req, &operation);
  if (not status.ok()) {
    std::cerr << "Error in CreateInstance() request: " << status.error_message()
              << " [" << status.error_code() << "] " << status.error_details()
              << std::endl;
    return 1;
  }

  // ... we poll the long running operation object at most 100 times,
  // using some exponential backoff to be nice ...
  using namespace ::std::chrono_literals;
  auto const initial_wait = 100ms;  // probably too fast
  auto const maximum_wait = 3min;
  auto const maximum_iterations = 100;

  auto wait = initial_wait;
  bool done = operation.done();
  for (int i = 0; i != maximum_iterations and not done; ++i) {
    // ... wait a little bit before trying again ...
    std::this_thread::sleep_for(wait);
    // ... check the status of the operation ...
    google::longrunning::GetOperationRequest opreq;
    opreq.set_name(operation.name());
    grpc::ClientContext ctx;
    google::longrunning::Operation response;

    auto status = operations->GetOperation(&ctx, opreq, &response);
    if (not status.ok()) {
      std::cerr << "Error in GetOperation() request, will try again: "
                << status.error_message() << " [" << status.error_code() << "] "
                << status.error_details() << std::endl;
    } else if (response.done()) {
      done = true;
      operation.Swap(&response);
      break;
    }

    wait = wait * 2;
    if (wait > maximum_wait) {
      wait = maximum_wait;
    }
  }
  // ... the operation did not complete ...
  if (not operation.done()) {
    std::cerr << "Timeout waiting for CreateInstance() operation: "
              << operation.name() << std::endl;
    return 1;
  }
  // ... at this point the answer is in the 'reponse' variable ...
  if (operation.has_error()) {
    // ... the creation failed (but not immediately), report the error
    // ...
    auto const& error = operation.error();
    std::string details;
    (void)google::protobuf::TextFormat::PrintToString(error, &details);
    std::cerr << "Error reported in CreateInstance() operation: "
              << operation.name() << " - " << error.message() << " ["
              << error.code() << "] " << details << std::endl;
    return 1;
  }
  if (not operation.has_response()) {
    std::cerr << "Missing response() field in successful CreateInstance() long "
              << "running operation:" << operation.name() << std::endl;
    return 1;
  }

  admin::Instance result;
  if (not operation.response().UnpackTo(&result)) {
    std::string text;
    (void)google::protobuf::TextFormat::PrintToString(operation, &text);
    std::cerr << "CreateInstance() operation was successful but returned"
              << " an unexpected response=" << text << std::endl;
    return 1;
  }

  std::string text;
  (void)google::protobuf::TextFormat::PrintToString(result, &text);
  std::cout << "CreateInstance() operation (" << operation.name()
            << ") is successful with result=" << text << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
