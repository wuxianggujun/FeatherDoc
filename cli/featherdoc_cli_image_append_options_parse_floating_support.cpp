#include "featherdoc_cli_image_append_options_parse_floating_support.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto read_image_append_floating_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string_view missing_message, std::string &error_message)
    -> std::optional<std::string_view> {
    if (index + 1U >= arguments.size()) {
        error_message = std::string{missing_message};
        return std::nullopt;
    }
    return arguments[index + 1U];
}

auto parse_image_append_floating_uint32_option_value(
    std::string_view value, std::string_view invalid_message_prefix,
    std::uint32_t &parsed_value, std::string &error_message) -> bool {
    if (parse_uint32(value, parsed_value)) {
        return true;
    }

    error_message = std::string{invalid_message_prefix} + std::string{value};
    return false;
}

} // namespace featherdoc_cli
