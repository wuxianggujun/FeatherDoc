#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <iosfwd>

namespace featherdoc_cli {

void write_json_template_schema_lint_issue(
    std::ostream &stream, const template_schema_lint_issue &issue);

void write_json_exported_template_schema_skipped_bookmark(
    std::ostream &stream,
    const exported_template_schema_skipped_bookmark &bookmark);

void print_template_schema_patch_selector_summary(
    std::ostream &stream, const template_schema_patch_target_selector &selector);

} // namespace featherdoc_cli
