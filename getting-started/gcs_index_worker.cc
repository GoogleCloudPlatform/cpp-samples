// Copyright 2021 Google LLC
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

#include <google/cloud/functions/cloud_event.h>
#include <google/cloud/spanner/client.h>
#include <stdexcept>
#include <string>

namespace gcf = ::google::cloud::functions;
namespace spanner = ::google::cloud::spanner;

namespace {
auto GetSpannerDatabase() {
  auto getenv = [](char const* var) {
    auto const* value = std::getenv(var);
    if (value == nullptr) {
      throw std::runtime_error("Environment variable " + std::string(var) +
                               " is not set");
    }
    return std::string(value);
  };
  return spanner::Database(getenv("GOOGLE_CLOUD_PROJECT"),
                           getenv("SPANNER_INSTANCE"),
                           getenv("SPANNER_DATABASE"));
}
}  // namespace

void GcsIndexer(gcf::CloudEvent /*event*/) {  // NOLINT
  // TODO(coryan) - do interesting stuff....
}
