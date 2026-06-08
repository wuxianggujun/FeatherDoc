#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <iosfwd>

namespace featherdoc_cli {

void write_json_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target);

void print_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target);

} // namespace featherdoc_cli
