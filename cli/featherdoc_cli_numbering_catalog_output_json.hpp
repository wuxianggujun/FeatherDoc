#pragma once

#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto numbering_catalog_instance_count(
    const featherdoc::numbering_catalog &catalog) -> std::size_t;

void write_json_numbering_catalog_definition(
    std::ostream &stream,
    const featherdoc::numbering_catalog_definition &definition);

void write_json_numbering_catalog_patch_summary(
    std::ostream &stream, const numbering_catalog_patch_summary &summary);

void write_json_numbering_catalog_lint_issues(
    std::ostream &stream,
    const std::vector<numbering_catalog_lint_issue> &issues);

} // namespace featherdoc_cli
