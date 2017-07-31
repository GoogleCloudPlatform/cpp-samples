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

#include <google/bigtable/v2/bigtable.grpc.pb.h>
#include <google/rpc/status.pb.h>
#include <google/protobuf/text_format.h>
#include <grpc++/grpc++.h>

#include <algorithm>
#include <chrono>
#include <ciso646>
#include <fstream>
#include <sstream>
#include <thread>

namespace {
// ... save ourselves some typing ...
namespace bigtable = ::google::bigtable::v2;

// Parse a line from the souce file ane turn it into a KV pair.
std::pair<std::string, std::string> parse_taq_line(int lineno,
                                                   std::string const& line);

// Perform a Bigtable::MutateRows() request until all mutations complete.
void mutate_with_retries(bigtable::Bigtable::Stub& bt_stub,
			 bigtable::MutateRowsRequest& req);

}  // anonymous namespace

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
  int const max_lines = 1000000;
  int const report_progress_rate = 20000;
  // ... we upload batch_size rows at a time, nothing magical about
  // 1024, just a nice round number picked by the author ...
  int const batch_size = 1024;

  std::ifstream is(filename);
  std::string line;
  std::getline(is, line, '\n');  // ... skip the header ...
  bigtable::MutateRowsRequest req;
  req.set_table_name(table_name);
  for (int lineno = 1; lineno != max_lines and not is.eof() and is; ++lineno) {
    std::getline(is, line, '\n');
    auto kv = parse_taq_line(lineno, line);
    // ... add one more entry to the batch request ...
    auto& entry = *req.add_entries();
    entry.set_row_key(std::move(kv.first));
    auto& set = *entry.add_mutations()->mutable_set_cell();
    set.set_family_name("taq");
    set.set_column_qualifier("message");
    set.set_value(std::move(kv.second));
    // ... we use the timestamp field as a simple revision count in
    // this example, so set it to 0.  The actual timestamp of the
    // quote is stored in the key ...
    set.set_timestamp_micros(0);

    if (req.entries_size() >= batch_size) {
      mutate_with_retries(*bt_stub, req);
    }
    if (lineno % report_progress_rate == 0) {
      std::cout << lineno << " quotes uploaded so far" << std::endl;
    }
  }
  // ... CS101: the last batch needs to be uploaded too ...
  mutate_with_retries(*bt_stub, req);
  std::cout << max_lines << " quotes successfully uploaded" << std::endl;

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
void throw_on_iostream_error(std::istream& is) {
  if (is.good()) {
    return;
  }
  throw std::runtime_error("parsing error");
}

// TODO(coryan) - this should return a proto representing the TAQ data.
std::pair<std::string, std::string> parse_taq_line(
    int lineno, std::string const& line) try {
  // The data is in pipe separated fields, starting with:
  // Time: in HHMMSSNNNNNNNNN format (hours, minutes, seconds, nanoseconds).
  // Exchange: a single character
  // Symbol: a string
  // Bid_Price: float
  // Bid_Size: integer
  // Offer_Price: float
  // Offer_Size: integer
  // ... and many other fields we ignore in this demo.
  std::istringstream tokens(line);
  std::string tk;
  throw_on_iostream_error(
      std::getline(tokens, tk, '|'));  // fetch the timestamp

  // ... the key will be a combination of timestamp and symbol, to
  // avoid hotspotting by either ...
  std::string key = tk + "/";
  throw_on_iostream_error(
      std::getline(tokens, tk, '|'));  // ignore the exchange
  std::string symbol;
  throw_on_iostream_error(
      std::getline(tokens, symbol, '|'));  // fetch the symbol
  key += tk;

  // ... the value is built using a proto
  TAQ quote;
  throw_on_iostream_error(std::getline(tokens, tk, '|'));
  quote.set_bid_px(std::stod(tk));
  throw_on_iostream_error(std::getline(tokens, tk, '|'));
  quote.set_bid_qty(std::stol(tk));
  throw_on_iostream_error(std::getline(tokens, tk, '|'));
  quote.set_offer_px(std::stod(tk));
  throw_on_iostream_error(std::getline(tokens, tk, '|'));
  quote.set_offer_qty(std::stol(tk));

  std::string value;
  if (not quote.SerializeToString(&value)) {
    std::ostringstream os;
    os << "could not serialize quote at line #" << lineno << "(" << line << ")";
    throw std::runtime_error(os.str());
  }

  return std::make_pair(std::move(key), std::move(value));
} catch (std::exception const& ex) {
  std::ostringstream os;
  os << ex.what() << " in line #" << lineno << " (" << line << ")";
  throw std::runtime_error(os.str());
}

bool should_retry(int code) {
  return (code == grpc::ABORTED or code == grpc::UNAVAILABLE
	  or code == grpc::DEADLINE_EXCEEDED);
}

void mutate_with_retries(bigtable::Bigtable::Stub& bt_stub,
			 bigtable::MutateRowsRequest& req) {
  using namespace std::chrono_literals;
  // These should be parameters in a real application, but in a demon we can hardcode all kinds of stuff ...
  int const max_retries = 100;
  // ... do an exponential backoff for retries ...
  auto const initial_backoff = 10ms;
  auto const maximum_backoff = 5min;

  auto backoff = initial_backoff;
  char const* retry_msg = "retrying .";
  for (int i = 0; i != max_retries; ++i) {
    // ... a variable to prepare the next retry ...
    bigtable::MutateRowsRequest tmp;
    // ... a variable to accumulate any permanent errors in this request ...
    std::ostringstream os;

    // ... MutateRows() is a streaming RPC call, make the call and read from the stream ...
    grpc::ClientContext ctx;
    auto stream = bt_stub.MutateRows(&ctx, req);
    bigtable::MutateRowsResponse resp;
    while (stream->Read(&resp)) {
      // ... save the partial results to either `tmp` or `os` ...
      for (auto const& entry : resp.entries()) {
	auto& status = entry.status();
	if (status.code() == grpc::OK) {
	  continue;
	}
	if (not should_retry(status.code())) {
	  // ... permanent error, save all of them for later ...
	  std::string details;
	  (void)google::protobuf::TextFormat::PrintToString(entry, &details);
	  os << "permanent error for #" << entry.index()
	     << ": " << status.message() << " [" << status.code() << "] "
	     << details << "\n";
	} else {
	  tmp.add_entries()->Swap(req.mutable_entries(entry.index()));
	}
      }
    }
    if (not os.str().empty()) {
      // ... there was at least one permanent error, abort ...
      throw std::runtime_error(os.str());
    }
    if (tmp.entries_size() == 0) {
      // ... nothing to retry, just return ...
      if (i > 0) {
	std::cout << " done" << std::endl;
      }
      req.mutable_entries()->Clear();
      return;
    }
    // ... prepare the next request ...
    tmp.mutable_table_name()->swap(*req.mutable_table_name());
    tmp.Swap(&req);
    backoff = backoff * 2;
    if (backoff > maximum_backoff) {
      backoff = maximum_backoff;
    }
    // ... we should randomize this sleep to avoid synchronized
    // backoffs when running multiple clients, that is beyond the
    // scope of a simple demo like this one ...
    std::this_thread::sleep_for(backoff);
    // ... give the user some sense of progress ...
    std::cout << retry_msg << std::flush;
    retry_msg = ".";
  }
  throw std::runtime_error("Could not complete mutation after maximum retries");
}

}  // anonymous namespace
