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
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>

#include <ciso646>
#include <iostream>

int main(int argc, char* argv[]) try {
  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 3) {
    std::cerr << "Usage: list_tables <project_id> <instance_id>"
              << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  char const* instance_id = argv[2];

  // ... use the default credentials, this works automatically if your
  // GCE instance has the Bigtable APIs enabled.  You can also set
  // GOOGLE_APPLICATION_CREDENTIALS to work outside GCE, more details
  // at:
  //  https://grpc.io/docs/guides/auth.html
  auto creds = grpc::GoogleDefaultCredentials();

  // ... notice that Bigtable has separate endpoints for different
  // APIs, we are going to list tables in an instance, which is one of the admin
  // APIs, so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableTableAdmin;
  std::unique_ptr<BigtableTableAdmin::Stub> table_admin(
      BigtableTableAdmin::NewStub(channel));

  admin::ListTablesRequest req;
  req.set_parent(std::string("projects/") + project_id + "/instances/" +  instance_id);
  req.set_view(admin::Table::NAME_ONLY);

  // ... the API may return the list in "pages", it is rare that an
  // instance has enough tables to require more than a page, but for
  // completeness sake we document how to do it ...
  int count = 0;
  do {
    grpc::ClientContext ctx;
    admin::ListTablesResponse resp;
    auto status = table_admin->ListTables(&ctx, req, &resp);
    if (not status.ok()) {
      std::cerr << "Error in ListTables() request: " << status.error_message()
		<< " [" << status.error_code() << "] " << status.error_details()
		<< std::endl;
      return 1;
    }
    for (auto const& table : resp.tables()) {
      std::string text;
      (void)google::protobuf::TextFormat::PrintToString(table, &text);
      std::cout << "Table[" << count << "]: " << table.name() << ", details="
		<< text << "\n";
      ++count;
    }
    // ... nothing more to display, break the loop ...
    if (resp.next_page_token().empty()) {
      break;
    }
    // ... request the next page ...
    req.set_page_token(resp.next_page_token());
  } while(true);

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
