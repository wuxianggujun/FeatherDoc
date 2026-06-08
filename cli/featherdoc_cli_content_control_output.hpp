#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

void write_json_content_control_part_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::optional<std::string> &tag,
    const std::optional<std::string> &alias);

void print_content_control_common_result(
    const selected_template_part &selected, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias,
    const std::optional<path_type> &output_path, std::size_t replaced);

} // namespace featherdoc_cli
