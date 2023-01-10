// Copyright 2022 Google LLC
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

#include <google/cloud/bigquery/bigquery_write_client.h>
#include <iostream>
#include <schema.pb.h>

namespace bq = ::google::cloud::bigquery;
using bq::storage::v1::AppendRowsRequest;
using bq::storage::v1::AppendRowsResponse;

auto constexpr kRowsPerMessage = 10;
bq::storage::v1::ProtoRows MakeSampleRows(int start_id, int count);

// Writes a few rows to a (hard-coded) test table.  Create the table with
//   bq mk cpp_samples
//   bq mk cpp_samples.hello
//   bq update cpp_samples.hello schema.json
int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: single_threaded_write <project-id>\n";
    return 1;
  }
  auto const project_id = std::string{argv[1]};
  auto client = bq::BigQueryWriteClient(bq::MakeBigQueryWriteConnection());

  auto stream = client.AsyncAppendRows();
  auto handle_broken_stream = [&stream](char const* where) {
    auto status = stream->Finish().get();
    std::cerr << "Unexpected streaming RPC error in " << where << ": " << status
              << "\n";
    return 1;
  };

  if (!stream->Start().get()) return handle_broken_stream("Start()");
  for (int i = 0; i != 5; ++i) {
    AppendRowsRequest request;
    if (i == 0) {
      // Only the first request needs to include this information.
      request.set_write_stream(
          std::string{"projects/"} + project_id +
          "/datasets/cpp_samples/tables/singers/streams/_default");
      Singers::GetDescriptor()->CopyTo(request.mutable_proto_rows()
                                           ->mutable_writer_schema()
                                           ->mutable_proto_descriptor());
    }
    *request.mutable_proto_rows()->mutable_rows() =
        MakeSampleRows(i * kRowsPerMessage, kRowsPerMessage);
    auto write = stream->Write(request, grpc::WriteOptions{}).get();
    // Abort the upload if the stream is closed unexpectedly.
    if (!write) return handle_broken_stream("Write()");

    auto response = stream->Read().get();
    // Abort the upload if the stream is closed unexpectedly.
    if (!response.has_value()) return handle_broken_stream("Read()");

    if (response->has_error()) {
      // In this example we simply abort the write, though applications could
      // recover, for example, using a separate stream to retry.
      std::cerr << "Error uploading data on message " << i
                << ". The full error is " << response->error().DebugString()
                << "\n";
      break;
    }
    if (!response->row_errors().empty()) {
      std::cerr << "Error uploading data on message " << i
                << ". Some rows had errors\n";
      for (auto const& e : response->row_errors()) {
        std::cerr << "  " << e.DebugString() << "\n";
      }
      break;
    }
    if (response->has_append_result() &&
        response->append_result().has_offset()) {
      std::cout << "Data successfully inserted at offset "
                << response->append_result().offset().value() << "\n";
    }
    // There are additional informational messages to handle.
    if (response->has_updated_schema()) {
      // In this example this is very unlikely.
      std::cout << "Table schema change reported, new schema is "
                << response->updated_schema().DebugString() << "\n";
    }
  }
  stream->WritesDone().get();
  auto status = stream->Finish().get();
  if (!status.ok()) {
    std::cerr << "Error in write stream: " << status << "\n";
    return 1;
  }
  return 0;
}

bq::storage::v1::ProtoRows MakeSampleRows(int start_id, int count) {
  bq::storage::v1::ProtoRows rows;
  for (int id = start_id; id != start_id + count; ++id) {
    Singers singer;
    singer.set_singerid(id);
    singer.set_firstname("first name (" + std::to_string(id) + ")");
    singer.set_lastname("last name (" + std::to_string(id) + ")");
    rows.add_serialized_rows(singer.SerializeAsString());
  }
  return rows;
}
