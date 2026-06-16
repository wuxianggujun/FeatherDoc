#pragma once

#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>

namespace featherdoc_cli {

struct selected_template_part {
    featherdoc::TemplatePart part;
    validation_part_family family = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
};

[[nodiscard]] auto select_template_part(
    featherdoc::Document &doc, validation_part_family part,
    const std::optional<std::size_t> &part_index,
    const std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind reference_kind,
    selected_template_part &selected, std::string &error_message) -> bool;

[[nodiscard]] auto select_mutable_template_part(
    featherdoc::Document &doc, validation_part_family part,
    const std::optional<std::size_t> &part_index,
    const std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind reference_kind,
    selected_template_part &selected, std::string &error_message) -> bool;

void write_json_selected_template_part(std::ostream &stream,
                                       const selected_template_part &selected);
void print_selected_template_part(std::ostream &stream,
                                  const selected_template_part &selected);

} // namespace featherdoc_cli
