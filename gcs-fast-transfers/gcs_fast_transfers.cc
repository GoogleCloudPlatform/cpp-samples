// Copyright 2021 Google LLC
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

#include "gcs_fast_transfers.h"
#include <boost/endian/buffers.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <crc32c/crc32c.h>
#include <fstream>
#include <limits>
#include <vector>

namespace gcs_fast_transfers {

std::string format_size(std::int64_t size) {
  struct range_definition {
    std::int64_t max_value;
    std::int64_t scale;
    char const* units;
  } ranges[] = {
      {kKiB, 1, "Bytes"},  {kMiB, kKiB, "KiB"}, {kGiB, kMiB, "MiB"},
      {kTiB, kGiB, "GiB"}, {kPiB, kTiB, "TiB"},
  };
  for (auto d : ranges) {
    if (size < d.max_value) return std::to_string(size / d.scale) + d.units;
  }
  return std::to_string(size / kPiB) + "PiB";
}

std::pair<std::int64_t, std::string> file_info(std::string const& filename) {
  std::ifstream is(filename, std::ios::binary);
  std::vector<char> buffer(1024 * 1024L);
  std::uint32_t crc32c = 0;
  std::int64_t size = 0;
  do {
    is.read(buffer.data(), buffer.size());
    if (is.bad()) break;
    crc32c = crc32c::Extend(
        crc32c, reinterpret_cast<std::uint8_t*>(buffer.data()), is.gcount());
    size += is.gcount();
  } while (not is.eof());

  static_assert(std::numeric_limits<unsigned char>::digits == 8,
                "This program assumes an 8-bit char");
  boost::endian::big_uint32_buf_at buf(crc32c);
  return {size, cppcodec::base64_rfc4648::encode(
                    std::string(buf.data(), buf.data() + sizeof(buf)))};
}

}  // namespace gcs_fast_transfers
