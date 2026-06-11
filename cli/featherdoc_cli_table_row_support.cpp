#include "featherdoc_cli_table_row_support.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto report_body_table_row_failure(std::string_view command,
                                   std::string_view summary,
                                   std::string detail, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto parse_body_table_index(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            std::size_t argument_index,
                            std::string_view label,
                            std::size_t &value, bool json_output) -> bool {
    if (parse_index(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_body_table_row_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    table_row_target &target, bool json_output) -> bool {
    if (arguments.size() < minimum_argument_count) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    if (!parse_body_table_index(command, arguments, 2U, "table index",
                                target.table_index, json_output)) {
        return false;
    }

    if (!parse_body_table_index(command, arguments, 3U, "row index",
                                target.row_index, json_output)) {
        return false;
    }

    return true;
}

auto parse_body_table_row_mutation_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t options_start_index, table_cell_style_options &options,
    bool json_output) -> bool {
    std::string error_message;
    if (!parse_table_cell_style_options(arguments, options_start_index, options,
                                        error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

auto resolve_body_table_for_row(std::string_view command,
                                featherdoc::Document &doc,
                                std::size_t table_index,
                                featherdoc::Table &table, bool json_output)
    -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next(); ++current_index) {
        table.next();
    }

    if (table.has_next()) {
        return true;
    }

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto resolve_body_table_row_for_row(std::string_view command,
                                    featherdoc::Document &doc,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    featherdoc::TableRow &row,
                                    bool json_output) -> bool {
    featherdoc::Table table;
    if (!resolve_body_table_for_row(command, doc, table_index, table,
                                    json_output)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next(); ++current_index) {
        row.next();
    }

    if (row.has_next()) {
        return true;
    }

    return report_body_table_row_failure(
        command, "row index is out of range",
        "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto load_body_table_summary(std::string_view command, featherdoc::Document &doc,
                             std::size_t table_index,
                             featherdoc::table_inspection_summary &table,
                             bool json_output) -> bool {
    const auto inspected_table = doc.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, json_output);
        return false;
    }

    if (inspected_table.has_value()) {
        table = *inspected_table;
        return true;
    }

    return report_body_table_row_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto load_mutable_body_table_row(std::string_view command,
                                 const std::vector<std::string_view> &arguments,
                                 const table_cell_style_options &options,
                                 const table_row_target &target,
                                 featherdoc::Document &doc,
                                 featherdoc::TableRow &row) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_row_for_row(command, doc, target.table_index,
                                          target.row_index, row,
                                          options.json_output);
}

} // namespace featherdoc_cli
