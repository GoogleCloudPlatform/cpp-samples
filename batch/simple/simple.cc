// Copyright 2024 Google LLC
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

#include <google/cloud/batch/v1/batch_client.h>
#include <google/cloud/location.h>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) try {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0]
              << " <project-id> <region-id> <job-id> <job-json-file>\n";
    return 1;
  }

  namespace batch = ::google::cloud::batch_v1;

  std::string const project_id = argv[1];
  auto const location = google::cloud::Location(argv[1], argv[2]);
  std::string const job_id = argv[3];
  std::string const job_file = argv[4];

  // Parse the json and convert into protobuf format.
  std::ifstream file(job_file, std::ios::in);
  if (!file.is_open()) {
    std::cout << "Failed to open JSON file: " << job_file << '\n';
    return 0;
  }
  auto contents = std::string{std::istreambuf_iterator<char>(file), {}};
  google::cloud::batch::v1::Job job;
  google::protobuf::util::JsonParseOptions options;
  google::protobuf::util::Status status =
      google::protobuf::util::JsonStringToMessage(contents, &job, options);
  if (!status.ok()) throw status;

  // Create the cloud batch client.
  auto client = batch::BatchServiceClient(batch::MakeBatchServiceConnection());

  // Create a job.
  auto response = client.CreateJob(location.FullName(), job, job_id);

  if (!response) {
    if (response.status().code() ==
        google::cloud::StatusCode::kResourceExhausted) {
      std::cout << "There already exists a job for the parent `"
                << location.FullName() << "` and job_id: `" << job_id
                << "`. Please try again with a new job id.\n";
      return 0;
    }
    throw std::move(response).status();
  }

  // On success, print the job.
  std::cout << "Job : " << response->DebugString() << "\n";
  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (google::protobuf::util::Status const& status) {
  std::cerr << "google::protobuf::util::Status thrown: " << status << "\n";
  return 1;
}
