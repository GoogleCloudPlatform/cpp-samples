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
#include <arrow/api.h>
#include <arrow/array/data.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/api.h>
#include <arrow/record_batch.h>
#include <arrow/status.h>
#include <format>
#include <iostream>

namespace {

std::shared_ptr<arrow::Schema> GetArrowSchema(
    ::google::cloud::bigquery::storage::v1::ArrowSchema const& schema_in) {
  std::shared_ptr<arrow::Buffer> buffer =
      std::make_shared<arrow::Buffer>(schema_in.serialized_schema());
  arrow::io::BufferReader buffer_reader(buffer);
  arrow::ipc::DictionaryMemo dictionary_memo;
  auto result = arrow::ipc::ReadSchema(&buffer_reader, &dictionary_memo);
  if (!result.ok()) {
    std::cout << "Unable to parse schema\n";
    throw result.status();
  }
  std::shared_ptr<arrow::Schema> schema = result.ValueOrDie();
  return schema;
}

std::shared_ptr<arrow::RecordBatch> GetArrowRecordBatch(
    ::google::cloud::bigquery::storage::v1::ArrowRecordBatch const&
        record_batch_in,
    std::shared_ptr<arrow::Schema> schema) {
  std::shared_ptr<arrow::Buffer> buffer = std::make_shared<arrow::Buffer>(
      record_batch_in.serialized_record_batch());
  arrow::io::BufferReader buffer_reader(buffer);
  arrow::ipc::DictionaryMemo dictionary_memo;
  arrow::ipc::IpcReadOptions read_options;
  auto result = arrow::ipc::ReadRecordBatch(schema, &dictionary_memo,
                                            read_options, &buffer_reader);
  if (!result.ok()) {
    std::cout << "Unable to parse record batch\n";
    throw result.status();
  }
  std::shared_ptr<arrow::RecordBatch> record_batch = result.ValueOrDie();
  return record_batch;
}

void ProcessRowsInArrowFormat(
    ::google::cloud::bigquery::storage::v1::ArrowSchema const& schema_in,
    ::google::cloud::bigquery::storage::v1::ArrowRecordBatch const&
        record_batch_in) {
  std::shared_ptr<arrow::Schema> schema = GetArrowSchema(schema_in);

  std::shared_ptr<arrow::RecordBatch> record_batch =
      GetArrowRecordBatch(record_batch_in, schema);

  // Print information about the record batch.
  std::cout << std::format("Record batch schema is:\n {}\n",
                           record_batch->schema()->ToString());
  std::cout << std::format("Record batch has {} cols and {} rows\n",
                           record_batch->num_columns(),
                           record_batch->num_rows());

  // Print each row and column in the record batch.
  std::cout << std::setfill(' ') << std::setw(7) << "";
  for (std::int64_t col = 0; col < record_batch->num_columns(); ++col) {
    std::cout << std::left << std::setw(12) << record_batch->column_name(col);
  }
  std::cout << "\n";
  // If you want to see what the result looks like without parsing the
  // datatypes, use `record_batch->ToString()` for quick debugging.
  for (std::int64_t row = 0; row < record_batch->num_rows(); ++row) {
    std::cout << std::format("Row {}: ", row);

    for (std::int64_t col = 0; col < record_batch->num_columns(); ++col) {
      std::shared_ptr<arrow::Array> column = record_batch->column(col);
      arrow::Result<std::shared_ptr<arrow::Scalar> > result =
          column->GetScalar(row);
      if (!result.ok()) {
        std::cout << "Unable to parse scalar\n";
        throw result.status();
      }

      std::shared_ptr<arrow::Scalar> scalar = result.ValueOrDie();
      if (scalar->type->id() == arrow::Type::INT64) {
        std::shared_ptr<arrow::Int64Scalar> int64_scalar =
            std::dynamic_pointer_cast<arrow::Int64Scalar>(scalar);
        std::cout << std::left << std::setw(9) << int64_scalar->value << " ";
      } else if (scalar->type->id() == arrow::Type::STRING) {
        std::shared_ptr<arrow::StringScalar> string_scalar =
            std::dynamic_pointer_cast<arrow::StringScalar>(scalar);
        std::cout << std::left << std::setw(9) << string_scalar->view() << " ";
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
      google::cloud::bigquery::storage::v1::DataFormat::ARROW);
  read_session.set_table(table_id);
  auto session =
      client.CreateReadSession(google::cloud::Project(project_id).FullName(),
                               read_session, kMaxReadStreams);
  if (!session) throw std::move(session).status();

  // Read rows from the ReadSession.
  constexpr int kRowOffset = 0;
  auto read_rows = client.ReadRows(session->streams(0).name(), kRowOffset);

  std::int64_t num_rows = 0;
  for (auto const& row : read_rows) {
    if (row.ok()) {
      num_rows += row->row_count();
      ProcessRowsInArrowFormat(session->arrow_schema(),
                               row->arrow_record_batch());
    }
  }

  std::cout << std::format("Read {} rows from table: {}\n", num_rows, table_id);
  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (arrow::Status const& status) {
  std::cerr << "arrow::Status thrown: " << status << "\n";
  return 1;
}
