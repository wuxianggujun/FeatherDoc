#include "featherdoc_cli_template_table_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_table_inspect_commands_detail.hpp"
#include "featherdoc_cli_template_table_inspect_output.hpp"
#include "featherdoc_cli_template_table_resolve.hpp"

#include <featherdoc.hpp>

#include <string>

namespace featherdoc_cli {

auto run_inspect_template_tables_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-template-tables expects an input path",
                          json_output);
        return 2;
    }

    template_inspect_tables_options options;
    std::string error_message;
    if (!parse_template_inspect_tables_options(arguments, 2U, options,
                                               error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_inspect_template_part(command, doc, options, selected,
                                      error_message)) {
        return 1;
    }

    if (options.table_index.has_value() || options.bookmark_name.has_value()) {
        std::size_t table_index = 0U;
        if (!resolve_template_table_index(doc, selected, options.table_index,
                                          options.bookmark_name, table_index,
                                          command, options.json_output,
                                          "inspect")) {
            return 1;
        }

        const auto table = selected.part.inspect_table(table_index);
        if (!table.has_value()) {
            report_template_table_range_failure(
                command, "table index is out of range",
                "table index '" + std::to_string(table_index) +
                    "' is out of range",
                selected, options.json_output);
            return 1;
        }

        inspect_template_table(selected, *table, options.json_output);
        return 0;
    }

    const auto tables = selected.part.inspect_tables();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_tables(selected, tables, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
