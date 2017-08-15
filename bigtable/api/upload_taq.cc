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

#include "parse_taq_line.h"

#include <google/bigtable/v2/bigtable.grpc.pb.h>
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>

#include <algorithm>
#include <chrono>
#include <ciso646>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) try {
  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 5) {
    std::cerr
        << "Usage: create_table <project_id> <instance_id> <table> <filename>"
        << std::endl;
    return 1;
  }
  char const* project_id = argv[1];
  char const* instance_id = argv[2];
  char const* table_id = argv[3];
  char const* filename = argv[4];

  auto creds = grpc::GoogleDefaultCredentials();
  // ... notice that Bigtable has separate endpoints for different APIs,
  // we are going to upload some data, so the correct endpoint is:
  auto channel = grpc::CreateChannel("bigtable.googleapis.com", creds);

  // ... save ourselves some typing ...
  namespace bigtable = ::google::bigtable::v2;
  std::unique_ptr<bigtable::Bigtable::Stub> bt_stub(
      bigtable::Bigtable::NewStub(channel));

  std::string table_name = std::string("projects/") + project_id +
                           "/instances/" + instance_id + "/tables/" + table_id;

  // ... we just upload the first max_lines from the input file
  // because otherwise the demo can take hours to finish uploading the
  // data.  For very large uploads the application should use
  // something like Cloud Dataflow, where the upload work is sharded
  // across many clients, and should really take advantage of the
  // batch APIs ...
  int const max_lines = 1000;
  std::ifstream is(filename);
  std::string line;
  std::getline(is, line, '\n');  // ... skip the header ...
  for (int lineno = 1; lineno != max_lines and not is.eof() and is; ++lineno) {
    std::getline(is, line, '\n');
    auto q = bigtable_api_samples::parse_taq_line(lineno, line);
    // ... insert a single row in each call, obviously this is not
    // very efficient, the upload_taq_batch.cc demo shows how to
    // update multiple rows at a time ...
    bigtable::MutateRowRequest request;
    request.set_table_name(table_name);
    request.set_row_key(std::to_string(q.timestamp_ns()) + "/" + q.ticker());
    auto& set_cell = *request.add_mutations()->mutable_set_cell();
    set_cell.set_family_name("taq");
    set_cell.set_column_qualifier("quote");
    std::string value;
    if (not q.SerializeToString(&value)) {
      std::ostringstream os;
      os << "in line #" << lineno << " could not serialize quote";
      throw std::runtime_error(os.str());
    }
    set_cell.set_value(std::move(value));
    // ... we use the timestamp field as a simple revision count in
    // this example, so set it to 0.  The actual timestamp of the
    // quote is stored in the key ...
    set_cell.set_timestamp_micros(0);

    bigtable::MutateRowResponse response;
    grpc::ClientContext client_context;
    auto status = bt_stub->MutateRow(&client_context, request, &response);
    if (not status.ok()) {
      std::cerr << "Error in MutateRow() request: " << status.error_message()
                << " [" << status.error_code() << "] " << status.error_details()
                << std::endl;
      // ... for this demo we just abort on the first error,
      // applications can consider a more interesting error handling
      // logic ...
      return 1;
    }
  }
  std::cout << max_lines << " quotes successfully uploaded" << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
