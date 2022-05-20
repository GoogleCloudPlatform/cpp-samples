// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CPP_SAMPLES_SPEECH_API_PARSE_ARGUMENTS_H
#define CPP_SAMPLES_SPEECH_API_PARSE_ARGUMENTS_H

#include <google/cloud/speech/v1/cloud_speech.pb.h>

// Parse the command line arguments, and set the config options accordingly.
// Returns:
//   The audio file path, or nullptr if an error occurred.
struct ParseResult {
  google::cloud::speech::v1::RecognitionConfig config;
  std::string path;
};

ParseResult ParseArguments(int argc, char* argv[]);

#endif  // CPP_SAMPLES_SPEECH_API_PARSE_ARGUMENTS_H
