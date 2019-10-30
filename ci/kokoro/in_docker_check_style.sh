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

if [[ -z "${PROJECT_ROOT+x}" ]]; then
  readonly PROJECT_ROOT="$(cd "$(dirname "$0")/../.."; pwd)"
fi

echo "================================================================"
echo "Change working directory to project root $(date)."
cd "${PROJECT_ROOT}"

replace_original_if_changed() {
  if [[ $# != 2 ]]; then
    return 1
  fi

  local original="$1"
  local reformatted="$2"

  if cmp -s "${original}" "${reformatted}"; then
    rm -f "${reformatted}"
  else
    chmod --reference="${original}" "${reformatted}"
    mv -f "${reformatted}" "${original}"
  fi
}

find . \( -name '*.cc' -o -name '*.c' -o -name '*.h' \) -print0 |
  while IFS= read -r -d $'\0' file; do
    clang-format "${file}" >"${file}.tmp"
    replace_original_if_changed "${file}" "${file}.tmp"
  done

# Report any differences created by running the formatting tools. Report any
# differences created by running the formatting tools.

git diff --ignore-submodules=all --color --exit-code .
