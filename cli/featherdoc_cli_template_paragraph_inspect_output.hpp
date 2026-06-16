#pragma once

#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <vector>

namespace featherdoc_cli {

void inspect_template_paragraphs(
    const selected_template_part &selected,
    const std::vector<featherdoc::paragraph_inspection_summary> &paragraphs,
    bool json_output);

void inspect_template_paragraph(
    const selected_template_part &selected,
    const featherdoc::paragraph_inspection_summary &paragraph,
    bool json_output);

void inspect_template_runs(
    const selected_template_part &selected, std::size_t paragraph_index,
    const std::vector<featherdoc::run_inspection_summary> &runs,
    bool json_output);

void inspect_template_run(const selected_template_part &selected,
                          std::size_t paragraph_index,
                          const featherdoc::run_inspection_summary &run,
                          bool json_output);

} // namespace featherdoc_cli
