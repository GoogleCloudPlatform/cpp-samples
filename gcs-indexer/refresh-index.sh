#!/usr/bin/env bash
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eu

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <prefix> [prefix ...]"
  echo "  A prefix can be a bucket name (without the gs://)"
  echo "  or a bucket name and a object name prefix, separated by /"
  exit 1
fi

if [[ -z "${GOOGLE_CLOUD_PROJECT:-}" ]]; then
  echo "You must set GOOGLE_CLOUD_PROJECT to the project id hosting the GCS" \
      "metadata indexer"
  exit 1
fi

GOOGLE_CLOUD_PROJECT="${GOOGLE_CLOUD_PROJECT:-}"
readonly GOOGLE_CLOUD_PROJECT
GOOGLE_CLOUD_SPANNER_INSTANCE="${GOOGLE_CLOUD_SPANNER_INSTANCE:-gcs-indexer}"
readonly GOOGLE_CLOUD_SPANNER_INSTANCE
GOOGLE_CLOUD_SPANNER_DATABASE="${GOOGLE_CLOUD_SPANNER_DATABASE:-gcs-indexer-db}"
readonly GOOGLE_CLOUD_SPANNER_DATABASE

# Create a GKE job to refresh each prefix
for prefix in "${@}"; do
  ./refresh_index_for_bucket_config.py \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--instance=${GOOGLE_CLOUD_SPANNER_INSTANCE}" \
      "--database=${GOOGLE_CLOUD_SPANNER_DATABASE}" \
      "${prefix}" | kubectl apply -f -
done

exit 0
