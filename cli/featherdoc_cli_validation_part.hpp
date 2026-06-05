#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

enum class validation_part_family {
    body,
    header,
    footer,
    section_header,
    section_footer,
};

[[nodiscard]] auto parse_validation_part(std::string_view text,
                                         validation_part_family &part) -> bool;
[[nodiscard]] auto validation_part_name(validation_part_family family) -> const char *;
[[nodiscard]] auto validate_template_part_selection(
    validation_part_family part, const std::optional<std::size_t> &part_index,
    const std::optional<std::size_t> &section_index, bool has_kind,
    std::string_view operation_label, std::string &error_message) -> bool;

} // namespace featherdoc_cli
