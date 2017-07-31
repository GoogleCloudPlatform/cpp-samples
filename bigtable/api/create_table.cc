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
#include <google/longrunning/operations.grpc.pb.h>
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>

#include <ciso646>
#include <iostream>

int main(int argc, char* argv[]) try {
  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 4) {
    std::cerr << "Usage: create_table <project_id> <instance_id> <table>"
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

  // ... notice that Bigtable has separate endpoints for different APIs,
  // we are going to create a table, which is one of the admin APIs,
  // so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableTableAdmin;
  std::unique_ptr<BigtableTableAdmin::Stub> table_admin(
      BigtableTableAdmin::NewStub(channel));

  admin::CreateTableRequest req;
  req.set_parent(std::string("projects/") + project_id + "/instances/" +
                 instance_id);
  req.set_table_id(table_id);

  // ... in this demo we create a table with two column families
  // "quotes" and "trades", vaguely movivated by financial markets
  // data ...
  auto& quotes = (*req.mutable_table()->mutable_column_families())["quotes"];
  quotes.mutable_gc_rule()->set_max_num_versions(1);

  auto& trades = (*req.mutable_table()->mutable_column_families())["trades"];
  trades.mutable_gc_rule()->set_max_num_versions(2);

  auto& taq = (*req.mutable_table()->mutable_column_families())["taq"];
  taq.mutable_gc_rule()->set_max_num_versions(1);

  grpc::ClientContext ctx;
  admin::Table resp;
  auto status = table_admin->CreateTable(&ctx, req, &resp);
  if (not status.ok()) {
    std::cerr << "Error in CreateTable() request: " << status.error_message()
              << " [" << status.error_code() << "] " << status.error_details()
              << std::endl;
    return 1;
  }

  std::string text;
  (void)google::protobuf::TextFormat::PrintToString(resp, &text);
  std::cout << "CreateTable() operation was successful with result=" << text
            << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
