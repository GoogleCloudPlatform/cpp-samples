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

#include <absl/time/time.h>
#include <google/cloud/functions/cloud_event.h>
#include <google/cloud/spanner/client.h>
#include <google/cloud/spanner/mutations.h>
#include <nlohmann/json.hpp>
#include <charconv>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

namespace gcf = ::google::cloud::functions;
namespace spanner = ::google::cloud::spanner;

std::string GetEnv(char const* var) {
  auto const* value = std::getenv(var);
  if (value == nullptr) {
    throw std::runtime_error("Environment variable " + std::string(var) +
                             " is not set");
  }
  return value;
}

using GetField = std::function<spanner::Value(nlohmann::json const&)>;

auto ToSpannerValue(nlohmann::json const& p, std::string const& name,
                    std::int64_t) {
  return spanner::Value(
      static_cast<std::int64_t>(std::stoll(p.value(name, ""))));
}

auto ToSpannerValue(nlohmann::json const& p, std::string const& name,
                    std::string const& v) {
  return spanner::Value(p[name].get_ref<std::string const&>());
}

auto const& Columns() {
  static auto const columns = [] {
    auto required_field = [](auto name, auto default_value) {
      return std::pair<std::string const, GetField>(
          name, [name, v = std::move(default_value)](nlohmann::json const& p) {
            return ToSpannerValue(p, name, v);
          });
    };

    auto optional_field = [](auto name, auto default_value) {
      return std::pair<std::string const, GetField>(
          name, [name, v = std::move(default_value)](nlohmann::json const& p) {
            if (not p.contains(name)) {
              return spanner::Value(absl::optional<decltype(v)>());
            }
            return ToSpannerValue(p, name, v);
          });
    };

    auto object = [](auto name) {
      return std::pair<std::string const, GetField>(
          name, [name](nlohmann::json const& p) {
            if (not p.contains(name)) {
              return spanner::Value(absl::optional<std::string>());
            }
            return spanner::Value(p[name].dump());
          });
    };

    auto timestamp = [](std::string name) {
      return std::pair<std::string const, GetField>(
          name, [name](nlohmann::json const& p) {
            if (not p.contains(name)) {
              return spanner::Value(absl::optional<spanner::Timestamp>());
            }
            auto constexpr kParseSpec = "%Y-%m-%d%ET%H:%M:%E*S%Ez";
            absl::Time t;
            std::string err;
            auto const& value = p[name].get_ref<std::string const&>();
            if (absl::ParseTime(kParseSpec, value, &t, &err)) {
              return spanner::Value(spanner::MakeTimestamp(t).value());
            }
            throw std::runtime_error("timestamp p[" + name + "]=" + value +
                                     ": " + err);
          });
    };

    // Convert from the format described in:
    //
    return std::map<std::string, GetField>({
        required_field("name", std::string{}),
        required_field("bucket", std::string{}),
        required_field("generation", std::int64_t{}),
        required_field("metageneration", std::int64_t{}),
        timestamp("timeCreated"),
        timestamp("updated"),
        timestamp("timeDeleted"),
        timestamp("customTime"),
        optional_field("temporaryHold", false),
        optional_field("eventBasedHold", false),
        timestamp("retentionExpirationTime"),
        optional_field("storageClass", std::string{}),
        timestamp("timeStorageClassUpdated"),
        required_field("size", std::int64_t{}),
        optional_field("crc32c", std::string{}),
        optional_field("md5Hash", std::string{}),
        optional_field("contentType", std::string{}),
        optional_field("contentEncoding", std::string{}),
        optional_field("contentDisposition", std::string{}),
        optional_field("contentLanguage", std::string{}),
        optional_field("cacheControl", std::string{}),
        object("metadata"),
        object("owner"),
        optional_field("componentCount", std::int64_t{}),
        optional_field("etag", std::string{}),
        object("customerEncryption"),
        optional_field("kmsKeyName", std::string{}),
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

spanner::Mutation UpdateObjectMetadata(nlohmann::json const& payload) {
  auto const& columns = Columns();
  std::vector<spanner::Value> values(columns.size());
  std::transform(columns.begin(), columns.end(), values.begin(),
                 [&payload](auto const& p) {
                   auto const& [name, to_value] = p;
                   return to_value(payload);
                 });
  return spanner::InsertOrUpdateMutationBuilder("gcs_objects", Names())
      .AddRow(std::move(values))
      .Build();
}

spanner::Mutation DeleteObjectMetadata(nlohmann::json const& payload) {
  spanner::Key key{
      ToSpannerValue(payload, "bucket", std::string{}),
      ToSpannerValue(payload, "name", std::string{}),
      ToSpannerValue(payload, "generation", std::int64_t{}),
  };
  return spanner::DeleteMutationBuilder(
             "gcs_objects", spanner::KeySet().AddKey(std::move(key)))
      .Build();
}

spanner::Client GetSpannerClient() {
  static auto const client = [&] {
    auto database = spanner::Database(GetEnv("GOOGLE_CLOUD_PROJECT"),
                                      GetEnv("SPANNER_INSTANCE"),
                                      GetEnv("SPANNER_DATABASE"));
    return spanner::Client(spanner::MakeConnection(std::move(database)));
  }();
  return client;
}

}  // namespace

void UpdateGcsIndex(gcf::CloudEvent event) {
  GetSpannerClient()
      .Commit([e = std::move(event)](auto) {
        auto const payload = nlohmann::json::parse(e.data().value_or("{}"));
        if (e.type() == "google.cloud.storage.object.v1.deleted") {
          return spanner::Mutations{DeleteObjectMetadata(payload)};
        }
        return spanner::Mutations{UpdateObjectMetadata(payload)};
      })
      .value();
}
