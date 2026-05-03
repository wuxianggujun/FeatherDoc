#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto json_escape(std::string_view text) -> std::string;
void write_json_string(std::ostream &stream, std::string_view text);

} // namespace featherdoc_cli
