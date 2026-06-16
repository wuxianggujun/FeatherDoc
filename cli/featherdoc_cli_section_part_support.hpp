#pragma once

#include "featherdoc_cli_command_support.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class section_part_family {
    header,
    footer,
};

struct section_part_command_options {
    bool inherit_header_footer = true;
    std::optional<path_type> output_path;
    bool json_output = false;
};

[[nodiscard]] auto section_part_name(section_part_family family) -> const char *;

[[nodiscard]] auto part_family_for_command(std::string_view command)
    -> section_part_family;

[[nodiscard]] auto parse_section_part_command_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    bool allow_no_inherit, section_part_command_options &options,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
