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

#include <google/bigtable/admin/v2/bigtable_table_admin.grpc.pb.h>
#include <grpc++/grpc++.h>

#include <ciso646>
#include <iostream>

int main(int argc, char* argv[]) try {
  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 4) {
    std::cerr << "Usage: delete_table <project_id> <instance_id> <table>"
              << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  char const* instance_id = argv[2];
  char const* table_id = argv[3];

  // ... use the default credentials, this works automatically if your
  // GCE instance has the Bigtable APIs enabled.  You can also set
  // GOOGLE_APPLICATION_CREDENTIALS to work outside GCE, more details
  // at:
  //  https://grpc.io/docs/guides/auth.html
  auto creds = grpc::GoogleDefaultCredentials();

  // ... notice that Bigtable has separate endpoints for different
  // APIs, we are going to delete a table, which is one of the admin
  // APIs, so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableTableAdmin;
  std::unique_ptr<BigtableTableAdmin::Stub> table_admin(
      BigtableTableAdmin::NewStub(channel));

  admin::DeleteTableRequest req;
  req.set_name(std::string("projects/") + project_id + "/instances/" +  instance_id + "/tables/" + table_id);
  grpc::ClientContext ctx;
  google::protobuf::Empty resp;
  auto status = table_admin->DeleteTable(&ctx, req, &resp);
  if (not status.ok()) {
    std::cerr << "Error in DeleteTable() request: " << status.error_message()
              << " [" << status.error_code() << "] " << status.error_details()
              << std::endl;
    return 1;
  }

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
