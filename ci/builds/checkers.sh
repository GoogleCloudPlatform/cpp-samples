#!/bin/bash
#
# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euo pipefail

# Runs sed expressions (specified after -e) over the given files, editing them
# in place. This function is exported so it can be run in subshells, such as
# with xargs -P. Example:
#
#   sed_edit -e 's/foo/bar/g' -e 's,baz,blah,' hello.txt
#
function sed_edit() {
  local expressions=()
  local files=()
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -e)
        test $# -gt 1 || return 1
        expressions+=("-e" "$2")
        shift 2
        ;;
      *)
        files+=("$1")
        shift
        ;;
    esac
  done
  local tmp
  tmp="$(mktemp /tmp/checkers.XXXXXX.tmp)"
  for file in "${files[@]}"; do
    sed "${expressions[@]}" "${file}" >"${tmp}"
    if ! cmp -s "${file}" "${tmp}"; then
      chmod --reference="${file}" "${tmp}"
      cp -f "${tmp}" "${file}"
    fi
  done
  rm -f "${tmp}"
}
export -f sed_edit

# The list of files to check.
#
# By default, we check all files in the repository tracked by `git`. To check
# only the files that have changed in a development branch, set
# `GOOGLE_CLOUD_CPP_FAST_CHECKERS=1`.
git_files() {
  if [ -z "${GOOGLE_CLOUD_CPP_FAST_CHECKERS-}" ]; then
    git ls-files "${@}"
  else
    git diff main --name-only --diff-filter=d "${@}"
  fi
}

# This controls the output format from bash's `time` command, which we use
# below to time blocks of the script. A newline is automatically included.
readonly TIMEFORMAT="... %R seconds"

# Use the printf command rather than the shell builtin, which avoids issues
# with bash sometimes buffering output from its builtins. See
# https://github.com/googleapis/google-cloud-cpp/issues/4152
enable -n printf

# Applies whitespace fixes in text files, unless they request no edits. The
# `[D]` character class makes this file not contain the target text itself.
printf "%-50s" "Running whitespace fixes:" >&2
time {
  # Removes trailing whitespace on lines
  expressions=("-e" "'s/[[:blank:]]\+$//'")
  # Removes trailing blank lines (see http://sed.sourceforge.net/sed1line.txt)
  expressions+=("-e" "':x;/^\n*$/{\$d;N;bx;}'")
  # Adds a trailing newline if one doesn't already exist
  expressions+=("-e" "'\$a\'")
  git_files -z | grep -zv 'googleapis.patch$' |
    grep -zv '\.gz$' |
    grep -zv '\.pb$' |
    grep -zv '\.png$' |
    grep -zv '\.raw$' |
    grep -zv '\.flac$' |
    (xargs -r -0 grep -ZPL "\b[D]O NOT EDIT\b" || true) |
    xargs -r -P "$(nproc)" -n 50 -0 bash -c "sed_edit ${expressions[*]} \"\$0\" \"\$@\""
}

# Apply shfmt to format all shell scripts
printf "%-50s" "Running shfmt:" >&2
time {
  git_files -z -- '*.sh' | xargs -r -P "$(nproc)" -n 50 -0 shfmt -w
}

# Apply buildifier to fix the BUILD and .bzl formatting rules.
#    https://github.com/bazelbuild/buildtools/tree/master/buildifier
printf "%-50s" "Running buildifier:" >&2
time {
  git_files -z -- '*.BUILD' '*.bzl' '*.bazel' |
    xargs -r -P "$(nproc)" -n 50 -0 buildifier -mode=fix
}

# The version of clang-format is important, different versions have slightly
# different formatting output (sigh).
printf "%-50s" "Running clang-format:" >&2
time {
  git_files -z -- '*.h' '*.cc' '*.proto' |
    xargs -r -P "$(nproc)" -n 50 -0 clang-format -i
}

# Create a virtual environment and install the correct programs locally.
printf "%-50s" "Installing Python packages:" >&2
VENV_NAME="cpp-samples-checkers"
# List of packages to install.
PACKAGES=(
  mdformat==0.7.17
  cmake-format==0.6.13
)
time {
  # Check if the virtual environment already exists.
  if [[ ! -d "$VENV_NAME" ]]; then
    python3 -m venv "$VENV_NAME"
  fi
  source "$VENV_NAME/bin/activate"

  # Install packages.
  pip install -q "${PACKAGES[@]}"
}

# Apply cmake_format to all the CMake list files.
#     https://github.com/cheshirekow/cmake_format
printf "%-50s" "Running cmake-format:" >&2
time {
  git_files -z -- 'CMakeLists.txt' '**/CMakeLists.txt' '*.cmake' |
    xargs -r -P "$(nproc)" -n 50 -0 cmake-format -i
}

# Format markdown (.md) files.
#   https://github.com/executablebooks/mdformat
# mdformat does `tempfile.mkstemp(); ...; os.replace(tmp_path, path)`,
# which results in the new .md file having mode 0600. So, run a second
# pass to reset the group/other permissions to something more reasonable.
printf "%-50s" "Running markdown formatter:" >&2
time {
  # See `.mdformat.toml` for the configuration parameters.
  git_files -z -- '*.md' | xargs -r -P "$(nproc)" -n 50 -0 mdformat
  git_files -z -- '*.md' | xargs -r -0 chmod go=u-w
}

# Deactivate virtual environment
deactivate

# If there are any diffs, report them and exit with a non-zero status so
# as to break the build. Use a distinctive status so that callers have a
# chance to distinguish formatting updates from other check failures.
git diff --exit-code . || exit 111
