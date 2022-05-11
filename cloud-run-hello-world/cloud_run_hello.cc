// Copyright 2020 Google LLC
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

// [START cloudrun_helloworld_service]
// [START cloud_run_hello_world]
#include <google/cloud/functions/framework.h>
#include <cstdlib>

namespace gcf = ::google::cloud::functions;

auto hello_world_http() {
  return gcf::MakeFunction([](gcf::HttpRequest const& /*request*/) {
    std::string greeting = "Hello ";
    auto const* target = std::getenv("TARGET");
    greeting += target == nullptr ? "World" : target;
    greeting += "\n";

    return gcf::HttpResponse{}
        .set_header("Content-Type", "text/plain")
        .set_payload(greeting);
  });
}

int main(int argc, char* argv[]) {
  return gcf::Run(argc, argv, hello_world_http());
}

// [END cloud_run_hello_world]
// [END cloudrun_helloworld_service]
