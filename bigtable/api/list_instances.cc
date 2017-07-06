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

#include <grpc++/grpc++.h>
#include <iostream>
#include "google/bigtable/admin/v2/bigtable_instance_admin.grpc.pb.h"

int main(int argc, char* argv[]) try {
  auto creds = grpc::GoogleDefaultCredentials();
  // Notice that Bigtable has separate endpoints for different APIs,
  // we are going to query the list of instances, which is one of the
  // admin APIs, so connect to the bigtableadmin endpoint ...
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableInstanceAdmin;

  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 2) {
    std::cerr << "Usage: list_instances <project_id>" << std::endl;
    return 1;
  }
  char const* project_id = argv[1];

  std::unique_ptr<BigtableInstanceAdmin::Stub> instance_admin(
      BigtableInstanceAdmin::NewStub(channel));
  admin::ListInstancesRequest req;
  req.set_parent(std::string("projects/") + project_id);

  // ... the API may return the list in "pages", it is rare that a
  // project has so many instances that it requires multiple pages,
  // but for completeness sake we document how to do it ...
  int count = 0;
  do {
    grpc::ClientContext context;
    admin::ListInstancesResponse response;
    auto status = instance_admin->ListInstances(&context, req, &response);
    if (not status.ok()) {
      std::cerr << "Error in first ListInstances() request: "
                << status.error_message() << " [" << status.error_code() << "] "
                << status.error_details() << std::endl;
      return 1;
    }
    for (auto const& instance : response.instances()) {
      std::cout << "Instance[" << count << "]: " << instance.name() << ", "
                << instance.display_name() << ", "
                << admin::Instance_State_Name(instance.state()) << ", "
                << admin::Instance_Type_Name(instance.type()) << "\n";
      ++count;
    }
    for (auto const& location : response.failed_locations()) {
      std::cout << "Failed location: " << location << "\n";
    }
    // ... nothing more to display, break the loop ...
    if (response.next_page_token().empty()) {
      break;
    }
    // ... request the next page ...
    req.set_page_token(response.next_page_token());
  } while (true);

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
