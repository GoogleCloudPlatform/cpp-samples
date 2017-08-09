#ifndef parse_taq_line_h
#define parse_taq_line_h

#include "taq.pb.h"

#include <utility>

namespace bigtable_api_samples {
// Parse a line from a TAQ file and convert it to a quote.
Quote parse_taq_line(int lineno, std::string const& line);
} // namespace bigtable_api_samples

#endif // parse_taq_line_h
