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

#include "gcs_indexing.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace google::cloud::cpp_samples {

namespace gcs = ::google::cloud::storage;
namespace spanner = ::google::cloud::spanner;

using GetField = std::function<spanner::Value(gcs::ObjectMetadata const&)>;

auto const& Columns() {
  static auto const columns = [] {
    auto field = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            return spanner::Value(f(o));
          });
    };

    auto custom = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name), [f = std::move(functor)](
                               gcs::ObjectMetadata const& o) { return f(o); });
    };

    auto optional_string = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            auto s = f(o);
            if (s.empty()) return spanner::Value(absl::optional<std::string>());
            return spanner::Value(std::move(s));
          });
    };

    auto timestamp = [](auto name, auto functor) {
      return std::pair<std::string const, GetField>(
          std::move(name),
          [f = std::move(functor)](gcs::ObjectMetadata const& o) {
            auto tp = f(o);
            if (tp == std::chrono::system_clock::time_point{}) {
              return spanner::Value(absl::optional<spanner::Timestamp>());
            }
            auto ts = spanner::MakeTimestamp(tp).value();
            return spanner::Value(ts);
          });
    };

    return std::map<std::string, GetField>({
        field("name", [](auto const& o) { return o.name(); }),
        field("bucket", [](auto const& o) { return o.bucket(); }),
        field("generation", [](auto const& o) { return o.generation(); }),
        field("metageneration",
              [](auto const& o) { return o.metageneration(); }),
        timestamp("timeCreated",
                  [](auto const& o) { return o.time_created(); }),
        timestamp("updated", [](auto const& o) { return o.updated(); }),
        timestamp("timeDeleted",
                  [](auto const& o) { return o.time_deleted(); }),
        timestamp("customTime", [](auto const& o) { return o.custom_time(); }),
        field("temporaryHold",
              [](auto const& o) { return o.temporary_hold(); }),
        field("eventBasedHold",
              [](auto const& o) { return o.event_based_hold(); }),
        timestamp("retentionExpirationTime",
                  [](auto const& o) { return o.retention_expiration_time(); }),
        field("storageClass", [](auto const& o) { return o.storage_class(); }),
        timestamp("timeStorageClassUpdated",
                  [](auto const& o) { return o.time_storage_class_updated(); }),
        field(
            "size",
            [](auto const& o) { return static_cast<std::int64_t>(o.size()); }),
        field("crc32c", [](auto const& o) { return o.crc32c(); }),
        optional_string("md5Hash", [](auto const& o) { return o.md5_hash(); }),
        optional_string("contentType",
                        [](auto const& o) { return o.content_type(); }),
        optional_string("contentEncoding",
                        [](auto const& o) { return o.content_encoding(); }),
        optional_string("contentDisposition",
                        [](auto const& o) { return o.content_disposition(); }),
        optional_string("contentLanguage",
                        [](auto const& o) { return o.content_language(); }),
        optional_string("cacheControl",
                        [](auto const& o) { return o.cache_control(); }),

        field("metadata",
              [](auto const& o) {
                nlohmann::json json{};
                for (auto const& [k, v] : o.metadata()) json[k] = v;
                return json.dump();
              }),
        custom("owner",
               [](auto const& o) {
                 if (!o.has_owner()) {
                   return spanner::Value(absl::optional<std::string>());
                 }
                 return spanner::Value(nlohmann::json{
                     {"entity", o.owner().entity},
                     {"entityId",
                      o.owner().entity_id}}.dump());
               }),
        field("componentCount",
              [](auto const& o) { return o.component_count(); }),
        optional_string("etag", [](auto const& o) { return o.etag(); }),
        custom("customerEncryption",
               [](auto const& o) {
                 if (!o.has_customer_encryption()) {
                   return spanner::Value(absl::optional<std::string>());
                 }
                 return spanner::Value(nlohmann::json{
                     {"encryptionAlgorithm",
                      o.customer_encryption().encryption_algorithm},
                     {"keySha256", o.customer_encryption().key_sha256}}
                                           .dump());
               }),
        optional_string("kmsKeyName",
                        [](auto const& o) { return o.kms_key_name(); }),
    });
  }();
  return columns;
}

auto Names() {
  static auto const names = [] {
    auto columns = Columns();
    std::vector<std::string> names(columns.size());
    std::transform(columns.begin(), columns.end(), names.begin(),
                   [](auto p) { return p.first; });
    return names;
  }();
  return names;
}

std::size_t ColumnCount() { return Columns().size(); }

spanner::Mutation UpdateObjectMetadata(gcs::ObjectMetadata const& object) {
  auto const& columns = Columns();
  std::vector<spanner::Value> values(columns.size());
  std::transform(columns.begin(), columns.end(), values.begin(),
                 [&object](auto const& p) {
                   auto const& [name, to_value] = p;
                   return to_value(object);
                 });
  return spanner::InsertOrUpdateMutationBuilder("gcs_objects", Names())
      .AddRow(std::move(values))
      .Build();
}

std::string GetEnv(char const* var) {
  auto const* value = std::getenv(var);
  if (value == nullptr) {
    throw std::runtime_error("Environment variable " + std::string(var) +
                             " is not set");
  }
  return value;
}

}  // namespace google::cloud::cpp_samples
