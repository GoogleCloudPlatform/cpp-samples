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

#include "taq.pb.h"

#include <google/bigtable/admin/v2/bigtable_table_admin.grpc.pb.h>
#include <google/bigtable/v2/bigtable.grpc.pb.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <google/rpc/status.pb.h>
#include <grpc++/grpc++.h>

#include <algorithm>
#include <ciso646>
#include <deque>
#include <fstream>
#include <sstream>
#include <thread>

// We want to show the correct way to receive a Bigtable cell value in
// ReadRows().  Bigtable can break down cell values across multiple
// messages, which requires reassembly of the result.  We assume a
// table has been populated using the upload_taq_batch example, query
// a few of the rows and calculate an (un)interesting value derived
// from that data.
int main(int argc, char* argv[]) try {
  // ... a more interesting application would use getopt(3),
  // getopt_long(3), or Boost.Options to parse the command-line, we
  // want to keep things simple in the example ...
  if (argc != 5) {
    std::cerr
        << "Usage: create_table <project_id> <instance_id> <table_id> <date>"
        << std::endl;
    return 1;
  }
  std::string const project_id = argv[1];
  std::string const instance_id = argv[2];
  std::string const table_id = argv[3];
  std::string const yyyymmdd = argv[4];

  // ... save ourselves some typing ...
  namespace bigtable = ::google::bigtable::v2;

  // ... use the default credentials, this works automatically if your
  // GCE instance has the Bigtable APIs enabled.  You can also set
  // GOOGLE_APPLICATION_CREDENTIALS to work outside GCE, more details
  // at:
  //  https://grpc.io/docs/guides/auth.html
  auto creds = grpc::GoogleDefaultCredentials();

  // ... create the table if needed ...
  std::string table_name = "projects/" + project_id +
                           "/instances/" + instance_id + "/tables/" + table_id;

  // ... notice that Bigtable has separate endpoints for different APIs,
  // we are going to upload some data, so the correct endpoint is:
  auto channel = grpc::CreateChannel("bigtable.googleapis.com", creds);

  std::unique_ptr<bigtable::Bigtable::Stub> bt_stub(
      bigtable::Bigtable::NewStub(channel));

  // ... once the table is created and the data uploaded we can make
  // the query ...
  bigtable::ReadRowsRequest request;
  request.set_table_name(table_name);

  // ... show how to use filters, there are probably several ways to do this ...
  auto& chain = *request.mutable_filter()->mutable_chain();
  // ... first only accept the "taq" column family ...
  chain.add_filters()->set_family_name_regex_filter("taq");
  // ... then only accept the "quotes" column ...
  chain.add_filters()->set_column_qualifier_regex_filter("quotes");

  // ... the magic values "A" and "AA" happen to be the first two
  // symbols in the TAQ file used in these examples.  That provides
  // enough complexity to make the example interesting, but not so
  // much that it becomes overwhelming.  A future example will show
  // how to read multiple families and cells ...
  // TODO(coryan) update this comment when that example is written ...
  request.mutable_rows()->add_row_keys(yyyymmdd + "/A");
  request.mutable_rows()->add_row_keys(yyyymmdd + "/AA");

  grpc::ClientContext context;
  auto stream = bt_stub->ReadRows(&context, request);

  std::string current_row_key;
  std::string current_column_family;
  std::string current_column;
  std::string current_value;
  // ... receive the streaming response ...
  bigtable::ReadRowsResponse response;
  while (stream->Read(&response)) {
    for (auto& cell_chunk : *response.mutable_chunks()) {
      if (not cell_chunk.row_key().empty()) {
	current_row_key = cell_chunk.row_key();
      }
      if (cell_chunk.has_family_name()) {
	current_column_family = cell_chunk.family_name().value();
	if (current_column_family != "taq") {
	  throw std::runtime_error("strange, only 'taq' family name expected in the query");
	}
      }
      if (cell_chunk.has_qualifier()) {
	current_column = cell_chunk.qualifier().value();
	if (current_column != "quotes") {
	  throw std::runtime_error("strange, only 'quotes' column expected in the query");
	}
      }
      if (cell_chunk.timestamp_micros() != 0) {
	throw std::runtime_error("strange, only the 0 timestamp expected in the query");
      }
      if (cell_chunk.value_size() > 0) {
	current_value.reserve(cell_chunk.value_size());
      }
      current_value.append(cell_chunk.value());
      if (cell_chunk.commit_row()) {
	// ... process this cell, we want to convert the value into a Quotes proto ...
	Quotes quotes;
	if (not quotes.ParseFromString(current_value)) {
	  std::cerr << current_row_key << ": ParseFromString() failed" << std::endl;
	} else if (quotes.bid_px_size() != quotes.offer_px_size() or quotes.bid_px_size() == 0) {
	  std::cerr << current_row_key << ": mismatched or zero sizes bid="
		    << quotes.bid_px_size() << ", offer=" << quotes.offer_px_size()
		    << std::endl;
	} else {
	  double bid_px_sum =
	    std::accumulate(quotes.bid_px().begin(), quotes.bid_px().end(), 0);
	  double offer_px_sum =
	    std::accumulate(quotes.offer_px().begin(), quotes.offer_px().end(), 0);
	  double average_spread =
	    (offer_px_sum - bid_px_sum) / quotes.offer_px_size();
	  std::cout << current_row_key << ": average spread="
		    << average_spread << ", count=" << quotes.offer_px_size()
		    << std::endl;
	}
      }
      if (cell_chunk.reset_row()) {
	current_value.clear();
      }
    }
  }

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
}  // anonymous namespace
