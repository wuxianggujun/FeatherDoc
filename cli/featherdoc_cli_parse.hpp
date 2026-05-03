#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto has_json_flag(const std::vector<std::string_view> &arguments)
    -> bool;
[[nodiscard]] auto parse_index(std::string_view text, std::size_t &value) -> bool;
[[nodiscard]] auto parse_uint32(std::string_view text, std::uint32_t &value) -> bool;
[[nodiscard]] auto parse_int32(std::string_view text, std::int32_t &value) -> bool;
[[nodiscard]] auto parse_bool(std::string_view text, bool &value) -> bool;

} // namespace featherdoc_cli
