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

#include "parse_arguments.h"
#include <boost/program_options.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

using ::google::cloud::speech::v1::RecognitionConfig;
namespace po = ::boost::program_options;

ParseResult ParseArguments(int argc, char* argv[]) {
  po::positional_options_description positional;
  positional.add("path", 1);
  po::options_description desc("A Speech-to-Text transcription example");
  desc.add_options()("help", "produce help message")
      //
      ("bitrate", po::value<int>()->default_value(16000),
       "the sample rate in Hz")
      //
      ("language-code", po::value<std::string>()->default_value("en"),
       "the language code for the audio")
      //
      ("path", po::value<std::string>()->required(),
       "the name of an audio file to transcribe. Prefix the path with gs:// to "
       "use objects in GCS.");
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv)
                .options(desc)
                .positional(positional)
                .run(),
            vm);
  po::notify(vm);

  auto const path = vm["path"].as<std::string>();
  auto const bitrate = vm["bitrate"].as<int>();
  auto const language_code = vm["language-code"].as<std::string>();
  // Validate the command-line options.
  if (path.empty()) {
    throw std::runtime_error("The audio file path cannot be empty");
  }
  if (bitrate < 0) {
    throw std::runtime_error(
        "--bitrate option must be a positive number, value=" +
        std::to_string(bitrate));
  }

  ParseResult result;
  result.path = path;
  result.config.set_language_code(language_code);
  result.config.set_sample_rate_hertz(bitrate);

  // Use the audio file extension to configure the encoding.
  auto const ext = [&] {
    auto e = result.path.rfind('.');
    if (e == std::string::npos) return std::string{};
    auto ext = result.path.substr(e);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
  }();

  if (ext.empty() || ext == ".raw") {
    result.config.set_encoding(RecognitionConfig::LINEAR16);
  } else if (ext == ".ulaw") {
    result.config.set_encoding(RecognitionConfig::MULAW);
  } else if (ext == ".flac") {
    result.config.set_encoding(RecognitionConfig::FLAC);
  } else if (ext == ".amr") {
    result.config.set_encoding(RecognitionConfig::AMR);
    result.config.set_sample_rate_hertz(8000);
  } else if (ext == ".awb") {
    result.config.set_encoding(RecognitionConfig::AMR_WB);
    result.config.set_sample_rate_hertz(16000);
  } else {
    result.config.set_encoding(RecognitionConfig::LINEAR16);
  }
  return result;
}
