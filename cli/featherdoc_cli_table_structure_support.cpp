#include "featherdoc_cli_table_structure_support.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto parse_table_structure_index_arg(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::size_t &value,
    bool json_output) -> bool {
    if (parse_index(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_table_structure_table_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, bool json_output) -> bool {
    return parse_table_structure_index_arg(command, arguments, 2U,
                                           "table index", table_index,
                                           json_output);
}

auto parse_table_structure_style_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t option_start, table_cell_style_options &options,
    bool json_output) -> bool {
    std::string error_message;
    if (parse_table_cell_style_options(arguments, option_start, options,
                                       error_message)) {
        return true;
    }

    print_parse_error(command, error_message, json_output);
    return false;
}

auto report_table_structure_failure(std::string_view command,
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

auto report_invalid_table_structure_target(std::string_view command,
                                           std::string_view summary,
                                           bool json_output) -> bool {
    return report_table_structure_failure(command, summary,
                                          "target table handle is not valid",
                                          json_output);
}

auto load_body_table_summary_for_structure(
    std::string_view command, featherdoc::Document &doc,
    std::size_t table_index, featherdoc::table_inspection_summary &table,
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

    return report_table_structure_failure(
        command, "table index is out of range",
        "table index '" + std::to_string(table_index) + "' is out of range",
        json_output);
}

auto validate_table_structure_column_index(
    std::string_view command, std::size_t table_index,
    std::size_t column_index,
    const featherdoc::table_inspection_summary &table, bool json_output)
    -> bool {
    if (column_index < table.column_count) {
        return true;
    }

    return report_table_structure_failure(
        command, "column index is out of range",
        "column index '" + std::to_string(column_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'",
        json_output);
}

auto parse_table_column_width_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t minimum_argument_count, std::string_view usage_message,
    std::size_t &table_index, std::size_t &column_index, bool json_output)
    -> bool {
    if (arguments.size() < minimum_argument_count) {
        print_parse_error(command, std::string(usage_message), json_output);
        return false;
    }

    return parse_table_structure_table_index(command, arguments, table_index,
                                             json_output) &&
           parse_table_structure_index_arg(command, arguments, 3U,
                                           "column index", column_index,
                                           json_output);
}

} // namespace featherdoc_cli
