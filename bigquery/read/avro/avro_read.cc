// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cloud/bigquery/storage/v1/bigquery_read_client.h"
#include "google/cloud/project.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <avro/Compiler.hh>
#include <avro/DataFile.hh>
#include <avro/Decoder.hh>
#include <avro/Generic.hh>
#include <avro/GenericDatum.hh>
#include <avro/Stream.hh>
#include <avro/ValidSchema.hh>

namespace {

avro::ValidSchema GetAvroSchema(
    ::google::cloud::bigquery::storage::v1::AvroSchema const& schema) {
  // Create a valid reader schema.
  std::istringstream schema_bytes(schema.schema(), std::ios::binary);
  avro::ValidSchema valid_schema;
  avro::compileJsonSchema(schema_bytes, valid_schema);

  // [optional] Write the schema to a file. This could be useful if you want to
  // re-use the schema elsewhere.
  std::ofstream output("schema.avsc");
  if (output.is_open()) {
    valid_schema.toJson(output);
    output.close();
  } else {
    std::cerr << "Error opening the file!" << std::endl;
  }
  return valid_schema;
}

void ProcessRowsInAvroFormat(
    avro::ValidSchema const& valid_schema,
    ::google::cloud::bigquery::storage::v1::AvroRows const& rows,
    std::int64_t row_count) {
  // Get an avro decoder.
  std::stringstream row_bytes(rows.serialized_binary_rows(), std::ios::binary);
  std::unique_ptr<avro::InputStream> in = avro::istreamInputStream(row_bytes);
  avro::DecoderPtr decoder =
      avro::validatingDecoder(valid_schema, avro::binaryDecoder());
  decoder->init(*in);

  for (auto i = 0; i < row_count; ++i) {
    std::cout << "Row " << i << " ";
    avro::GenericDatum datum(valid_schema);
    avro::decode(*decoder, datum);
    if (datum.type() == avro::AVRO_RECORD) {
      const avro::GenericRecord& record = datum.value<avro::GenericRecord>();
      std::cout << "(" << record.fieldCount() << "): ";
      for (auto i = 0; i < record.fieldCount(); i++) {
        const avro::GenericDatum& datum = record.fieldAt(i);

        switch (datum.type()) {
          case avro::AVRO_STRING:
            std::cout << std::left << std::setw(15)
                      << datum.value<std::string>();
            break;
          case avro::AVRO_INT:
            std::cout << std::left << std::setw(15) << datum.value<int>();
            break;
          case avro::AVRO_LONG:
            std::cout << std::left << std::setw(15) << datum.value<long>();
            break;
            // Depending on the table you are reading, you might need to add
            // cases for other datatypes here. The schema will tell you what
            // datatypes need to be handled.
          default:
            std::cout << std::left << std::setw(15) << "UNDEFINED " << " ";
        }
      }
    }
    std::cout << "\n";
  }
}

}  // namespace

int main(int argc, char* argv[]) try {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <project-id> <dataset-name> <table-name>\n";
    return 1;
  }

  std::string const project_id = argv[1];
  std::string const dataset_name = argv[2];
  std::string const table_name = argv[3];

  std::string const table_id = "projects/" + project_id + "/datasets/" +
                               dataset_name + "/tables/" + table_name;

  // Create a namespace alias to make the code easier to read.
  namespace bigquery_storage = ::google::cloud::bigquery_storage_v1;
  constexpr int kMaxReadStreams = 1;
  // Create the ReadSession.
  auto client = bigquery_storage::BigQueryReadClient(
      bigquery_storage::MakeBigQueryReadConnection());
  ::google::cloud::bigquery::storage::v1::ReadSession read_session;
  read_session.set_data_format(
      google::cloud::bigquery::storage::v1::DataFormat::AVRO);
  read_session.set_table(table_id);
  auto session =
      client.CreateReadSession(google::cloud::Project(project_id).FullName(),
                               read_session, kMaxReadStreams);
  if (!session) throw std::move(session).status();

  // Get Avro schema.
  avro::ValidSchema valid_schema = GetAvroSchema(session->avro_schema());

  // Read rows from the ReadSession.
  constexpr int kRowOffset = 0;
  auto read_rows = client.ReadRows(session->streams(0).name(), kRowOffset);

  std::int64_t num_rows = 0;
  std::int64_t num_responses = 0;
  for (auto const& read_rows_response : read_rows) {
    if (read_rows_response.ok()) {
      num_rows += read_rows_response->row_count();
      ProcessRowsInAvroFormat(valid_schema, read_rows_response->avro_rows(),
                              read_rows_response->row_count());
      ++num_responses;
    }
  }

  std::cout << "Read " << num_responses << " responses(s) and " << num_rows
            << " total row(s) from table: " << table_id << "\n";

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (avro::Exception const& e) {
  std::cerr << "avro::Exception thrown: " << e.what() << "\n";
  return 1;
}
