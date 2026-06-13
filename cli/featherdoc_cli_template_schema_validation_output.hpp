#pragma once

#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

namespace featherdoc_cli {

[[nodiscard]] auto validation_part_name(featherdoc::template_schema_part_kind part)
    -> const char *;

void print_template_validation_result(
    const selected_template_part &selected,
    const featherdoc::template_validation_result &result, bool json_output);

void print_template_schema_validation_result(
    const featherdoc::template_schema_validation_result &result, bool json_output);

} // namespace featherdoc_cli
