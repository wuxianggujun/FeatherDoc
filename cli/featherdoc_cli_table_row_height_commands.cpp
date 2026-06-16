#include "featherdoc_cli_table_row_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_row_support.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_row_height_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    table_row_target target;
    if (!parse_body_table_row_target(
            command, arguments, 6U,
            "set-table-row-height expects an input path, a table index, a row "
            "index, a height in twips, and a height rule",
            target, json_output)) {
        return 2;
    }

    std::uint32_t height_twips = 0U;
    if (!parse_uint32(arguments[4], height_twips)) {
        print_parse_error(command,
                          "invalid height twips: " +
                              std::string(arguments[4]),
                          json_output);
        return 2;
    }

    featherdoc::row_height_rule height_rule =
        featherdoc::row_height_rule::automatic;
    if (!parse_row_height_rule_text(arguments[5], height_rule)) {
        print_parse_error(command,
                          "invalid row height rule: " +
                              std::string(arguments[5]),
                          json_output);
        return 2;
    }

    table_cell_style_options options;
    if (!parse_body_table_row_mutation_options(command, arguments, 6U, options,
                                               json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    featherdoc::TableRow row;
    if (!load_mutable_body_table_row(command, arguments, options, target, doc,
                                     row)) {
        return 1;
    }

    if (!row.set_height_twips(height_twips, height_rule)) {
        report_body_table_row_failure(command, "failed to set table row height",
                                      "target table row handle is not valid",
                                      options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target, height_twips, height_rule](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index
                       << ",\"height_twips\":" << height_twips
                       << ",\"height_rule\":";
                write_json_string(stream, row_height_rule_name(height_rule));
            });
    }

    return 0;
}

auto run_clear_table_row_height_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    table_row_target target;
    if (!parse_body_table_row_target(
            command, arguments, 4U,
            "clear-table-row-height expects an input path, a table index, and "
            "a row index",
            target, json_output)) {
        return 2;
    }

    table_cell_style_options options;
    if (!parse_body_table_row_mutation_options(command, arguments, 4U, options,
                                               json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    featherdoc::TableRow row;
    if (!load_mutable_body_table_row(command, arguments, options, target, doc,
                                     row)) {
        return 1;
    }

    if (!row.clear_height()) {
        report_body_table_row_failure(command,
                                      "failed to clear table row height",
                                      "target table row handle is not valid",
                                      options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [target](std::ostream &stream) {
                stream << ",\"table_index\":" << target.table_index
                       << ",\"row_index\":" << target.row_index;
            });
    }

    return 0;
}

} // namespace featherdoc_cli
