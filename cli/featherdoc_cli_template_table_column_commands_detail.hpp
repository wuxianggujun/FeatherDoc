#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_table_mutation_options_parse.hpp"
#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct template_table_column_context {
    featherdoc::Document doc;
    selected_template_part selected;
    std::size_t table_index = 0U;
};

template <typename Options>
auto merge_target_bookmark(parsed_template_table_selector_cell_target &target,
                           const Options &options) -> void {
    if (!target.selector.bookmark_name.has_value() &&
        options.bookmark_name.has_value()) {
        target.selector.bookmark_name = options.bookmark_name;
    }
}

auto validate_target_selector(
    std::string_view command, bool json_output,
    parsed_template_table_selector_cell_target &target,
    std::string &error_message) -> bool;

auto load_template_table_column_context(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const template_table_cell_mutation_options &options,
    const featherdoc::template_table_selector &selector,
    template_table_column_context &context, std::string &error_message) -> bool;

auto report_template_column_failure(std::string_view command,
                                    std::string_view summary,
                                    std::string detail,
                                    const selected_template_part &selected,
                                    bool json_output) -> bool;

auto parse_template_table_column_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message,
    parsed_template_table_selector_cell_target &target,
    template_table_cell_mutation_options &options) -> bool;

} // namespace featherdoc_cli
