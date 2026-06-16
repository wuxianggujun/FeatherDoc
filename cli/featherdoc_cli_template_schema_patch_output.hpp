#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <iosfwd>

namespace featherdoc_cli {

void write_json_template_schema_patch_selector(
    std::ostream &stream, const template_schema_patch_target_selector &selector);

void write_json_template_schema_patch_document(
    std::ostream &stream, const template_schema_patch_document &document);

} // namespace featherdoc_cli
