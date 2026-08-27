#pragma once

#include "featherdoc_cli_paragraph_inspect_output.hpp"

#include <featherdoc/fwd.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto load_body_paragraph_summaries(
    featherdoc::Document &document,
    std::vector<inspected_body_paragraph> &paragraphs,
    std::string &error_message) -> bool;

[[nodiscard]] auto load_body_run_summaries(
    featherdoc::Document &document, std::size_t paragraph_index,
    std::vector<inspected_body_run> &runs, bool &paragraph_found,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
