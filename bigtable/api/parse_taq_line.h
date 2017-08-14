// Copyright 2017 Google Inc.
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

#ifndef parse_taq_line_h
#define parse_taq_line_h

#include "taq.pb.h"

#include <utility>

namespace bigtable_api_samples {
// Parse a line from a TAQ file and convert it to a quote.
//
// TAQ files are delimiter (using '|' as the delimiter) separated text
// files, using this format:
//
// timestamp|exchange|ticker|bid price|bid qty|offer price|offer qty|...
// 093000123456789|K|GOOG|800.00|100|900.00|200|...
// ...
// END|20161024|78721395|||||||||||||||||||||||||
//
// The first line is a header, it defines the fields, each line
// contains all the data for a quote, in this example we are only
// interested in the first few fields, the last line is indicated by
// the 'END' marker, it contains the date (timestamps are relative to
// midnight on this date), and the total number of lines.
Quote parse_taq_line(int lineno, std::string const& line);
} // namespace bigtable_api_samples

#endif // parse_taq_line_h
