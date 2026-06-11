#include "featherdoc_cli_table_position_commands_support.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>

namespace featherdoc_cli {

void write_text_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
}

auto report_table_position_failure(std::string_view command,
                                   std::string_view stage,
                                   std::string_view summary,
                                   std::string detail,
                                   bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, stage, summary, error_info,
                                    json_output);
}

auto parse_table_position_targets(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool target_all,
    const std::vector<std::size_t> &additional_table_indices,
    std::vector<std::size_t> &requested_table_indices, bool json_output)
    -> bool {
    if (!target_all) {
        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return false;
        }
        requested_table_indices.push_back(table_index);
    }

    if (target_all && !additional_table_indices.empty()) {
        print_parse_error(command,
                          "--table cannot be combined with all table target",
                          json_output);
        return false;
    }

    for (const auto additional_table_index : additional_table_indices) {
        if (std::find(requested_table_indices.begin(),
                      requested_table_indices.end(),
                      additional_table_index) != requested_table_indices.end()) {
            print_parse_error(command,
                              "duplicate table target: " +
                                  std::to_string(additional_table_index),
                              json_output);
            return false;
        }
        requested_table_indices.push_back(additional_table_index);
    }

    return true;
}

} // namespace featherdoc_cli
