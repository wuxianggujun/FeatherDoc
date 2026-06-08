#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <featherdoc.hpp>

#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto build_exported_template_schema(
    featherdoc::Document &doc, bool section_targets, bool resolved_section_targets,
    exported_template_schema_result &result, std::string_view command,
    bool json_output) -> bool;

} // namespace featherdoc_cli
