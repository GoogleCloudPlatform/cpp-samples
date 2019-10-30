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

if [ -z "${PROJECT_ID:-}" ]; then
  readonly PROJECT_ID="cloud-devrel-kokoro-resources"
fi

readonly SUBDIR_ROOT="$(cd "$(dirname "$0")/../../"; pwd)"

echo "================================================================"
echo "Change working directory to ${SUBDIR_ROOT} $(date)."
cd "${SUBDIR_ROOT}"

echo "================================================================"
echo "Setup Google Container Registry access $(date)."
gcloud auth configure-docker

readonly DEV_IMAGE="gcr.io/${PROJECT_ID}/cpp/cpp-samples/iot-devtools"
readonly IMAGE="gcr.io/${PROJECT_ID}/cpp/cpp-samples/iot-sample"

has_cache="false"
# Download the cache for local run and pull request.
if [[ -z "${KOKORO_JOB_NAME:-}" ]] ||
   [[ -n "${KOKORO_GITHUB_PULL_REQUEST_NUMBER:-}" ]]; then
  echo "================================================================"
  echo "Download existing image (if available) $(date)."
  if docker pull "${DEV_IMAGE}:latest"; then
    echo "Existing image successfully downloaded."
    has_cache="true"
  fi
fi

echo "================================================================"
echo "Build base image with minimal development tools $(date)."
update_cache="false"

devtools_flags=(
  # Build only for dependencies
  "--target" "devtools"
  "--build-arg" "NCPU=${NCPU}"
  "-t" "${DEV_IMAGE}:latest"
  "-f" "docker/Dockerfile"
)

if "${has_cache}"; then
  devtools_flags+=("--cache-from=${DEV_IMAGE}:latest")
fi

if [[ -n "${KOKORO_JOB_NAME:-}" ]] && \
   [[ -z "${KOKORO_GITHUB_PULL_REQUEST_NUMBER:-}" ]]; then
  devtools_flags+=("--no-cache")
fi

echo "================================================================"
echo "Building docker image with the following flags $(date)."
printf '%s\n' "${devtools_flags[@]}"

if docker build "${devtools_flags[@]}" .; then
   update_cache="true"
fi

if "${update_cache}" && [[ -z "${KOKORO_GITHUB_PULL_REQUEST_NUMBER:-}" ]]; then
  echo "================================================================"
  echo "Uploading updated base image $(date)."
  # Do not stop the build on a failure to update the cache.
  docker push "${DEV_IMAGE}:latest" || true
fi

build_flags=(
  "--build-arg" "NCPU=${NCPU}"
  "-t" "${IMAGE}:latest"
  "--cache-from=${DEV_IMAGE}:latest"
  "--target=cpp-iot"
  "-f" "docker/Dockerfile"
)

echo "================================================================"
echo "Building docker image with the following flags $(date)."
printf '%s\n' "${build_flags[@]}"

docker build "${build_flags[@]}" .

# TODO(#65) Run actual tests somehow

docker run --rm "${IMAGE}:latest" /src/mqtt_ciotc --help
