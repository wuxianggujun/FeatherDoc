#pragma once

#include "featherdoc_cli_semantic_diff_options_parse.hpp"

#include <featherdoc.hpp>

namespace featherdoc_cli {

void output_semantic_diff_result(
    const featherdoc::document_semantic_diff_result &result,
    const semantic_diff_options &options);

} // namespace featherdoc_cli
