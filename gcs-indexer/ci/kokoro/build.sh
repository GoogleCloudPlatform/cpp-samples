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

if [[ -z "${SUBPROJECT_ROOT+x}" ]]; then
  readonly SUBPROJECT_ROOT="$(cd "$(dirname "$0")/../.."; pwd)"
fi
cd "${SUBPROJECT_ROOT}"

# Verify the code compiles with Docker.
if type -t docker>/dev/null; then
  echo "================================================================"
  echo "$(date -u): building Docker container for the code."
  docker build -t unused-tag -f Dockerfile .
  echo "================================================================"
else
  echo "Docker not found, skipping Docker tests."
fi

if type -t cmake>/dev/null && type -t ninja>/dev/null; then
  echo "================================================================"
  echo "$(date -u): building the code with CMake."
  cmake -Hsuper -Bcmake-out -GNinja
  cmake --build cmake-out
else
  echo "Missing cmake and ninja, skipping CMake Super Build."
fi
