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

#include <getopt.h>
#include "parse_arguments.h"
#include "google/cloud/speech/v1beta1/cloud_speech.pb.h"

using google::cloud::speech::v1beta1::RecognitionConfig;

char* ParseArguments(int argc, char** argv, RecognitionConfig* config) {
  // Parse the bit rate from the --bitrate command line option.
  static struct option long_options[] = {
    {"bitrate", required_argument, 0,  'b' },
    {0,         0,                 0,  0 }
  };
  config->set_sample_rate(16000);  // Default sample rate.
  int opt;
  int option_index = 0;
  while ((opt = getopt_long(argc, argv, "b:", long_options,
                            &option_index)) != -1) {
    switch (opt) {
      case 'b':
        config->set_sample_rate(atoi(optarg));
        if (0 == config->sample_rate())
          return nullptr;
        break;
      default: /* '?' */
        return nullptr;
    }
  }
  if (optind == argc) {
    // Missing the audio file path.
    return nullptr;
  }
  // Choose the encoding by examining the audio file extension.
  char* ext = strrchr(argv[optind], '.');
  if (!ext || 0 == strcasecmp(ext, ".raw")) {
    config->set_encoding(RecognitionConfig::LINEAR16);
  } else if (0 == strcasecmp(ext, ".ulaw")) {
    config->set_encoding(RecognitionConfig::MULAW);
  } else if (0 == strcasecmp(ext, ".flac")) {
    config->set_encoding(RecognitionConfig::FLAC);
  } else if (0 == strcasecmp(ext, ".amr")) {
    config->set_encoding(RecognitionConfig::AMR);
    config->set_sample_rate(8000);
  } else if (0 == strcasecmp(ext, ".awb")) {
    config->set_encoding(RecognitionConfig::AMR_WB);
    config->set_sample_rate(16000);
  } else {
    config->set_encoding(RecognitionConfig::LINEAR16);
  }
  return argv[optind];
}
