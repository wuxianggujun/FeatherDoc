#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_numbering_catalog_diff.hpp"
#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"

#include <featherdoc.hpp>

#include <optional>
#include <ostream>
#include <string>

namespace featherdoc_cli {

void write_json_numbering_catalog(std::ostream &stream,
                                  const featherdoc::numbering_catalog &catalog);

[[nodiscard]] auto write_numbering_catalog_file(
    const path_type &output_path,
    const featherdoc::numbering_catalog &catalog,
    std::string &error_message) -> bool;

void print_exported_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const std::optional<path_type> &output_path, bool json_output);

void print_patched_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const numbering_catalog_patch_summary &summary,
    const std::optional<path_type> &output_path, bool json_output);

void print_linted_numbering_catalog_result(
    const numbering_catalog_lint_result &result, bool json_output);

void print_numbering_catalog_diff_result(
    const numbering_catalog_diff_result &result, bool json_output);

void print_checked_numbering_catalog_result(
    const path_type &catalog_path,
    const numbering_catalog_lint_result &baseline_lint,
    const numbering_catalog_lint_result &generated_lint,
    const numbering_catalog_diff_result &diff,
    const std::optional<path_type> &output_path, bool json_output);

} // namespace featherdoc_cli
