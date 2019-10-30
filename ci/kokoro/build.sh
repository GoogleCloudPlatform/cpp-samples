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

# This script is the main test driver of this repo. This repository has multiple
# sub directories and each subdirectory may have tests. For a pull request, it
# runs the tests only if the directory has changes in the pull request. For a
# merge commit, it will run all the tests.

# To run the tests, a sub directory need to have `ci` directory and
# `ci/kokoro/build.sh` executable. See `speech/api` directory for example.

set -eu

NCPU=$(nproc)
export NCPU

if [[ -z "${PROJECT_ROOT+x}" ]]; then
  readonly PROJECT_ROOT="$(cd "$(dirname "$0")/../.."; pwd)"
fi

echo "================================================================"
echo "Change working directory to project root $(date)."
cd "${PROJECT_ROOT}"

readonly CHECK_STYLE="yes"
export CHECK_STYLE

if [[ "${CHECK_STYLE}" ]]; then
  echo "================================================================"
  echo "Running clang-format  $(date)."
  ci/kokoro/check_style.sh
fi

echo "================================================================"
echo "Detecting sub directories that contains ci directory $(date)."
subdirs=()
while IFS= read -r line; do
  subdirs+=( "${line}" )
done < <(find . -mindepth 2 -type d -name ci | sed 's/...$//' | sed 's/^..//')

echo "================================================================"
echo "Found the following subdirs that contains ci directory $(date)."
printf '%s\n' "${subdirs[@]}"

test_dirs=()
if [[ -z "${KOKORO_GITHUB_PULL_REQUEST_NUMBER:-}" ]]; then
  # If it's not a presubmit build for a pull request, run all the tests.
  test_dirs=("${subdirs[@]}")
else
  echo "================================================================"
  echo "Detecting files changed by the pull request $(date)."
  changed_files=()
  while IFS= read -r line; do
    changed_files+=( "${line}" )
  done < <(git --no-pager diff --name-only HEAD $(git merge-base HEAD master))

  echo "================================================================"
  echo "The following files are changed $(date)."
  printf '%s\n' "${changed_files[@]}"
  for subdir in "${subdirs[@]}"
  do
    # quadratic loop!
    for changed_file in "${changed_files[@]}"
    do
      # We'll test the subdir if
      # 1) the root ci directory has changes
      # 2) or the subdir has some changes in the pull request
      if [[ ${changed_file} == ci* ]] \
        || [[ ${changed_file} == "${subdir}"* ]]; then
        test_dirs+=( "${subdir}" )
        break
      fi
    done
  done
fi

echo "================================================================"
echo "We'll run tests for these subdirs $(date)."
printf '%s\n' "${test_dirs[@]}"

exit_status=0

# Run all the tests even if there are failures, then report the error.
set +e
for subdir in "${test_dirs[@]}"
do
  if [ -x "${subdir}/ci/kokoro/build.sh" ];then
    ${subdir}/ci/kokoro/build.sh
    test_status=$?
    if [[ "${test_status}" != 0 ]]; then
      echo "${subdir}/ci/kokoro/build.sh failed with ${test_status}."
      exit_status="${test_status}"
    fi
  else
    echo "Skipped because ${subdir} doesn't have build script $(date)."
  fi
done

exit ${exit_status}
