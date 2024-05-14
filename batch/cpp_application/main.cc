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
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) try {
  if (argc != 6) {
    std::cerr << "Usage: " << argv[0]
              << " <project-id> <region-id> <job-id> <job-json-file> "
                 "<repository-name>\n";
    return 1;
  }

  namespace batch = ::google::cloud::batch_v1;

  std::string const project_id = argv[1];
  auto const location = google::cloud::Location(argv[1], argv[2]);
  std::string const job_id = argv[3];
  std::string const job_file = argv[4];
  std::string const repository_name = argv[5];

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

  // Modify the job for the containerized application
  auto container = job.mutable_task_groups()
                       ->at(0)
                       .mutable_task_spec()
                       ->mutable_runnables()
                       ->at(0)
                       .mutable_container();
  std::string image_uri = location.location_id() + "-docker.pkg.dev/" +
                          location.project_id() + "/" + repository_name +
                          "/application-image:latest";
  container->set_image_uri(image_uri);

  // Create the cloud batch client.
  auto client = batch::BatchServiceClient(batch::MakeBatchServiceConnection());

  // Create a job.
  auto response = client.CreateJob(location.FullName(), job, job_id);

  if (response.status().code() != google::cloud::StatusCode::kOk) {
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

  // Poll the service using exponential backoff to check if job is ready and
  // print once job is complete.
  const auto kMinPollingInterval = std::chrono::minutes(2);
  const auto kMaxPollingInterval = std::chrono::minutes(4);
  const auto kMaxPollingTime = std::chrono::minutes(10);

  auto current_time = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(current_time);
  std::cout << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %X]")
            << " Begin polling for job status\n";

  const auto start_time = current_time;
  auto delay = kMinPollingInterval;
  while (current_time <= start_time + kMaxPollingTime) {
    auto polling_response =
        client.GetJob("projects/" + location.project_id() + "/locations/" +
                      location.location_id() + "/jobs/" + job_id);
    if (polling_response.status().code() != google::cloud::StatusCode::kOk) {
      throw std::move(polling_response).status();
    }

    switch (polling_response.value().status().state()) {
      // 4 = google::cloud::batch::v1::JobStatus::State::SUCCEEDED
      case 4:
        std::cout << "Job succeeded!\n";
        return 0;
        // 5 = google::cloud::batch::v1::JobStatus::State::FAILED
      case 5:
        std::cout << "Job failed!\n";
        return 0;
        // 8 = google::cloud::batch::v1::JobStatus::State::CANCELLED
      case 8:
        std::cout << "Job cancelled!\n";
        return 0;
    }
    in_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::cout << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %X]")
              << " Job status: "
              << google::cloud::batch::v1::JobStatus_State_Name(
                     polling_response.value().status().state())
              << "\n"
              << "Current delay: " << delay.count() << " minute(s)\n";
    std::this_thread::sleep_for(
        std::chrono::duration_cast<std::chrono::milliseconds>(delay));
    delay = (std::min)(delay * 2, kMaxPollingInterval);
    current_time = std::chrono::system_clock::now();
  }
  in_time_t =
      std::chrono::system_clock::to_time_t(start_time + kMaxPollingTime);
  std::cout << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %X]")
            << " Max polling time passed\n";

  return 0;
} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (google::protobuf::util::Status const& status) {
  std::cerr << "google::protobuf::util::Status thrown: " << status << "\n";
  return 1;
}
