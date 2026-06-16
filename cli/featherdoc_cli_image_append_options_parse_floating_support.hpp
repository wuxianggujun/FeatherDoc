#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto read_image_append_floating_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string_view missing_message, std::string &error_message)
    -> std::optional<std::string_view>;

[[nodiscard]] auto parse_image_append_floating_uint32_option_value(
    std::string_view value, std::string_view invalid_message_prefix,
    std::uint32_t &parsed_value, std::string &error_message) -> bool;

} // namespace featherdoc_cli
