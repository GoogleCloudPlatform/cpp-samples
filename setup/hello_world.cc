// Copyright 2023 Google LLC
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

// [START cpp_hello_world]
#include "google/cloud/storage/client.h"
#include <iostream>
#include <stdexcept>
#include <string>

void HelloWorld(std::string const& bucket_name) {
  namespace gcs = ::google::cloud::storage;
  auto client = gcs::Client();

  auto const object_name = std::string{"hello-world.txt"};
  auto metadata = client.InsertObject(bucket_name, object_name, "Hello World!");
  if (!metadata) throw std::move(metadata).status();

  auto is = client.ReadObject(bucket_name, object_name);
  std::cout << std::string{std::istreambuf_iterator<char>{is}, {}} << "\n";
  if (is.bad()) throw google::cloud::Status(is.status());
}
// [END cpp_hello_world]

int main(int argc, char* argv[]) try {
  if (argc != 2) {
    std::cerr << "Usage: hello_world <bucket-name>\n";
    return 1;
  }
  HelloWorld(argv[1]);

} catch (google::cloud::Status const& status) {
  std::cerr << "google::cloud::Status thrown: " << status << "\n";
  return 1;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception thrown: " << ex.what() << "\n";
  return 1;
}
