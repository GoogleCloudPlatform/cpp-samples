#!/usr/bin/env bash
#
# Copyright 2019 Google LLC
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

if [ -z "${NCPU:-}" ]; then
  readonly NCPU=$(nproc)
fi

readonly SUBDIR_ROOT="$(cd "$(dirname "$0")/../../"; pwd)"

echo "================================================================"
echo "Change working directory to ${SUBDIR_ROOT} $(date)."
cd "${SUBDIR_ROOT}"

docker_flags=(
  "--build-arg" "NCPU=${NCPU}"
  "-t" "cpp-speech"
)

echo "================================================================"
echo "Building docker image with the following flags $(date)."
printf '%s\n' "${docker_flags[@]}"

docker build "${docker_flags[@]}" .

# TODO(#56) configure service account json file for integration tests.
if [[ -n "${GOOGLE_APPLICATION_CREDENTIALS:-}" ]]; then
  docker run -v "${GOOGLE_APPLICATION_CREDENTIALS}:/home/service-account.json" \
    cpp-speech bash -c "cd /home/speech/api/; make run_tests"
fi
