#pragma once

#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

void write_json_bookmark_support_summary(
    std::ostream &stream, const featherdoc::bookmark_summary &bookmark);
void print_bookmark_support_summary(
    std::ostream &stream, const featherdoc::bookmark_summary &bookmark);
void write_json_selected_bookmark_part(
    std::ostream &stream, const selected_template_part &selected);
void print_selected_bookmark_part(const selected_template_part &selected);
void print_bookmark_identity(const selected_template_part &selected,
                             const featherdoc::bookmark_summary &bookmark);

[[nodiscard]] auto select_bookmark_part(
    std::string_view command, featherdoc::Document &doc,
    validation_part_family part, const std::optional<std::size_t> &part_index,
    const std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind reference_kind, bool json_output,
    selected_template_part &selected, std::string &error_message) -> bool;

[[nodiscard]] auto report_bookmark_input_error(
    std::string_view command, bool json_output,
    const std::string &error_message) -> int;

} // namespace featherdoc_cli
