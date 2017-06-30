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
  auto channel = grpc::CreateChannel("bigtableadmin.googleapis.com", creds);

  namespace admin = ::google::bigtable::admin::v2;
  using admin::BigtableInstanceAdmin;

  std::unique_ptr<BigtableInstanceAdmin::Stub> instance_admin(
      BigtableInstanceAdmin::NewStub(channel));
  if (argc != 2) {
    std::cerr << "Usage: list_instances <project_id>" << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  admin::ListInstancesRequest req;
  req.set_parent(std::move(std::string("projects/") + project_id));
  int count = 0;
  do {
    grpc::ClientContext context;
    admin::ListInstancesResponse response;
    auto status = instance_admin->ListInstances(&context, req, &response);
    if (not status.ok()) {
      std::cerr << "Error in first ListInstances() request: "
                << status.error_message() << " [" << status.error_code() << "] "
                << status.error_details() << std::endl;
      break;
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
    if (response.next_page_token() == "") {
      break;
    }
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
