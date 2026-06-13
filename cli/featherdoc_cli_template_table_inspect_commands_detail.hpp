#pragma once

#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <string>
#include <string_view>

namespace featherdoc_cli {

auto report_template_table_range_failure(
    std::string_view command, std::string_view summary, std::string detail,
    const selected_template_part &selected, bool json_output) -> bool;

template <typename Options>
auto select_inspect_template_part(std::string_view command,
                                  featherdoc::Document &doc,
                                  const Options &options,
                                  selected_template_part &selected,
                                  std::string &error_message) -> bool {
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
