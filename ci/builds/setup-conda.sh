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

# [START cpp_setup_conda_install]
conda config --add channels conda-forge
conda config --set channel_priority strict
conda install -y -c conda-forge cmake ninja cxx-compiler google-cloud-cpp libgoogle-cloud
# [END cpp_setup_conda_install]

cmake -G Ninja -S /workspace/setup -B /var/tmp/build/setup-conda 
cmake --build /var/tmp/build/setup-conda
