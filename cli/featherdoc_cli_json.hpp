#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto json_escape(std::string_view text) -> std::string;
void write_json_string(std::ostream &stream, std::string_view text);
void write_json_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values);
void write_json_strings(std::ostream &stream,
                        const std::vector<std::string> &values);
void write_json_lines(std::ostream &stream,
                      const std::vector<std::string> &lines);
void write_json_optional_string(std::ostream &stream,
                                const std::optional<std::string> &value);
void write_json_optional_u32(std::ostream &stream,
                             const std::optional<std::uint32_t> &value);
void write_json_optional_double(std::ostream &stream,
                                const std::optional<double> &value);
void write_json_optional_bool(std::ostream &stream,
                              const std::optional<bool> &value);
void write_json_optional_size(std::ostream &stream,
                              const std::optional<std::size_t> &value);
void write_json_optional_u32_value(
    std::ostream &stream, const std::optional<std::uint32_t> &value);
void write_json_optional_bool_value(std::ostream &stream,
                                    const std::optional<bool> &value);

} // namespace featherdoc_cli
