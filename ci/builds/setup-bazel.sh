#!/bin/bash
#
# Copyright 2023 Google LLC
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

set -euo pipefail

args=(
    "--test_output=errors"
    "--verbose_failures=true"
    "--keep_going"
    "--experimental_convenience_symlinks=ignore"
    "--cache_test_results=auto"
)
if [[ -n "${BAZEL_REMOTE_CACHE:-}" ]]; then
    args+=("--remote_cache=${BAZEL_REMOTE_CACHE}")
    args+=("--google_default_credentials")
    # See https://docs.bazel.build/versions/main/remote-caching.html#known-issues
    # and https://github.com/bazelbuild/bazel/issues/3360
    args+=("--experimental_guard_against_concurrent_changes")
fi

# Make some attempts to download dependencies. This is a common source of
# flakes, and worth doing for less spurious failures. If they all fail we just
# let the `bazel build` invocation below try its luck.
for i in 1 2 3; do
  if env -C /workspace/setup bazel fetch ...; then
    break
  fi
  sleep 60
done

env -C /workspace/setup bazel build ...
