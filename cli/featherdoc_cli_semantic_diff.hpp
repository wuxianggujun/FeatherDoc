#pragma once

#include "featherdoc_cli_semantic_diff_options_parse.hpp"

#include <featherdoc.hpp>

namespace featherdoc_cli {

void output_semantic_diff_result(
    const featherdoc::document_semantic_diff_result &result,
    const semantic_diff_options &options);

[[nodiscard]] auto run_semantic_diff_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
