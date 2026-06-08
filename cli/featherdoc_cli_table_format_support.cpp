#include "featherdoc_cli_table_format_support.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <system_error>

namespace featherdoc_cli {

auto parse_index_arg(std::string_view command,
                     const std::vector<std::string_view> &arguments,
                     std::size_t argument_index, std::string_view label,
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

auto parse_table_index(std::string_view command,
                       const std::vector<std::string_view> &arguments,
                       std::size_t &table_index, bool json_output) -> bool {
    return parse_index_arg(command, arguments, 2U, "table index", table_index,
                           json_output);
}

auto parse_table_cell_location(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, std::size_t &row_index, std::size_t &cell_index,
    bool json_output) -> bool {
    return parse_index_arg(command, arguments, 2U, "table index", table_index,
                           json_output) &&
           parse_index_arg(command, arguments, 3U, "row index", row_index,
                           json_output) &&
           parse_index_arg(command, arguments, 4U, "cell index", cell_index,
                           json_output);
}

auto parse_uint32_arg(std::string_view command,
                      const std::vector<std::string_view> &arguments,
                      std::size_t argument_index, std::string_view label,
                      std::uint32_t &value, bool json_output) -> bool {
    if (parse_uint32(arguments[argument_index], value)) {
        return true;
    }

    print_parse_error(command,
                      "invalid " + std::string(label) + ": " +
                          std::string(arguments[argument_index]),
                      json_output);
    return false;
}

auto parse_style_options(std::string_view command,
                         const std::vector<std::string_view> &arguments,
                         std::size_t option_start,
                         table_cell_style_options &options,
                         bool json_output) -> bool {
    std::string error_message;
    if (parse_table_cell_style_options(arguments, option_start, options,
                                       error_message)) {
        return true;
    }

    print_parse_error(command, error_message, json_output);
    return false;
}

auto parse_border_options(std::string_view command,
                          const std::vector<std::string_view> &arguments,
                          std::size_t option_start,
                          table_cell_border_options &options,
                          bool json_output) -> bool {
    std::string error_message;
    if (!parse_table_cell_border_options(arguments, option_start, options,
                                         error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    if (!options.style.has_value()) {
        print_parse_error(command, "missing --style option", json_output);
        return false;
    }

    return true;
}

auto make_border_definition(const table_cell_border_options &options)
    -> featherdoc::border_definition {
    featherdoc::border_definition border{};
    border.style = *options.style;
    if (options.size_eighth_points.has_value()) {
        border.size_eighth_points = *options.size_eighth_points;
    }
    if (options.color.has_value()) {
        border.color = *options.color;
    }
    if (options.space_points.has_value()) {
        border.space_points = *options.space_points;
    }
    return border;
}

auto report_invalid_table_target(std::string_view command,
                                 std::string_view summary, bool json_output)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "target table handle is not valid";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto report_invalid_table_cell_target(std::string_view command,
                                      std::string_view summary,
                                      bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "target table cell handle is not valid";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

void write_json_table_location(std::ostream &stream,
                               std::size_t table_index) {
    stream << ",\"table_index\":" << table_index;
}

void write_json_table_cell_location(std::ostream &stream,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    std::size_t cell_index) {
    write_json_table_location(stream, table_index);
    stream << ",\"row_index\":" << row_index
           << ",\"cell_index\":" << cell_index;
}

void write_json_uint_field(std::ostream &stream, std::string_view field_name,
                           std::uint32_t value) {
    stream << ",\"" << field_name << "\":" << value;
}

void write_json_border_definition(std::ostream &stream,
                                  const featherdoc::border_definition &border) {
    stream << ",\"style\":";
    write_json_string(stream, border_style_name(border.style));
    stream << ",\"size_eighth_points\":" << border.size_eighth_points
           << ",\"color\":";
    write_json_string(stream, border.color);
    stream << ",\"space_points\":" << border.space_points;
}

} // namespace featherdoc_cli
