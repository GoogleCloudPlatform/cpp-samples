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

if [ -z "${PROJECT_ID:-}" ]; then
  readonly PROJECT_ID="cloud-devrel-kokoro-resources"
fi

if [[ -z "${PROJECT_ROOT+x}" ]]; then
  readonly PROJECT_ROOT="$(cd "$(dirname "$0")/../.."; pwd)"
fi

echo "================================================================"
echo "Change working directory to project root $(date)."
cd "${PROJECT_ROOT}"

echo "================================================================"
echo "Capture Docker version to troubleshoot $(date)."
docker version
echo "================================================================"

echo "================================================================"
echo "Setup Google Container Registry access $(date)."
gcloud auth configure-docker

readonly CHECK_STYLE_IMAGE="gcr.io/${PROJECT_ID}/cpp/cpp-samples/check_style"

has_cache="false"
# Download the cache
echo "================================================================"
echo "Download existing image (if available) $(date)."
if docker pull "${CHECK_STYLE_IMAGE}:latest"; then
  echo "Existing image successfully downloaded."
  has_cache="true"
fi

build_flags=(
  "-t" "${CHECK_STYLE_IMAGE}:latest"
  "-f" "ci/kokoro/Dockerfile.check_style"
)

if "${has_cache}"; then
  build_flags+=("--cache-from=${CHECK_STYLE_IMAGE}:latest")
fi

echo "================================================================"
echo "Building docker image with the following flags $(date)."
printf '%s\n' "${build_flags[@]}"

if docker build "${build_flags[@]}" ci; then
   update_cache="true"
fi

if "${update_cache}" && [[ -z "${KOKORO_GITHUB_PULL_REQUEST_NUMBER:-}" ]]; then
  echo "================================================================"
  echo "Uploading updated base image $(date)."
  # Do not stop the build on a failure to update the cache.
  docker push "${CHECK_STYLE_IMAGE}:latest" || true
fi

docker_uid="${UID:-0}"

run_flags=(
  "--rm"
  # Mount the current directory (which is the top-level directory for the
  # project) as `/v` inside the docker image, and move to that directory.
  "--volume" "${PWD}:/v"
  "--workdir" "/v"
  # Run the docker script with this user id.
  "--user" "${docker_uid}"
)
echo "================================================================"
echo "Running docker image with the following flags $(date)."
printf '%s\n' "${run_flags[@]}"

docker run "${run_flags[@]}" "${CHECK_STYLE_IMAGE}:latest" \
  "/v/ci/kokoro/in_docker_check_style.sh"
