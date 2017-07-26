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
#include <iostream>

int main(int argc, char* argv[]) try {
  auto creds = grpc::GoogleDefaultCredentials();
  // Notice that Bigtable has separate endpoints for different APIs,
  // we are going to delete an instance, which requires the admin
  // APIs, so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableInstanceAdmin;

  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 3) {
    std::cerr << "Usage: create_instance <project_id> <instance_id>"
              << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  char const* instance_id = argv[2];

  admin::DeleteInstanceRequest req;
  req.set_name(std::string("projects/") + project_id + "/instances/" +
               instance_id);

  std::unique_ptr<BigtableInstanceAdmin::Stub> instance_admin(
      BigtableInstanceAdmin::NewStub(channel));

  // ... make the request to create an instance, that returns a "long
  // running operation" object ...
  grpc::ClientContext ctx;
  google::protobuf::Empty response;
  auto status = instance_admin->DeleteInstance(&ctx, req, &response);
  if (not status.ok()) {
    std::cerr << "Error in DeleteInstance() request: " << status.error_message()
              << " [" << status.error_code() << "] " << status.error_details()
              << std::endl;
    return 1;
  }

  std::cout << "DeleteInstance() was successful" << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
