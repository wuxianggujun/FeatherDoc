#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <featherdoc.hpp>

namespace featherdoc_cli {

[[nodiscard]] auto to_template_schema_part_kind(validation_part_family family)
    -> featherdoc::template_schema_part_kind;

[[nodiscard]] auto to_template_schema(const exported_template_schema_result &result)
    -> featherdoc::template_schema;

[[nodiscard]] auto to_template_schema_patch(
    const template_schema_patch_document &document) -> featherdoc::template_schema_patch;

} // namespace featherdoc_cli
