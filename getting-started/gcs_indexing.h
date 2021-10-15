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

#ifndef CPP_SAMPLES_GETTING_STARTED_GCS_INDEXING_H
#define CPP_SAMPLES_GETTING_STARTED_GCS_INDEXING_H

#include <google/cloud/spanner/mutations.h>
#include <google/cloud/storage/object_metadata.h>
#include <string>
#include <vector>

namespace google::cloud::cpp_samples {

std::size_t ColumnCount();

google::cloud::spanner::Mutation UpdateObjectMetadata(
    google::cloud::storage::ObjectMetadata const& object);

std::string GetEnv(char const* var);

}  // namespace google::cloud::cpp_samples

#endif  // CPP_SAMPLES_GETTING_STARTED_GCS_INDEXING_H
