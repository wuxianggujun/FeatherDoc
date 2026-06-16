#include "featherdoc_cli_table_position_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_commands_support.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"
#include "featherdoc_cli_table_position_output.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_set_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "set-table-position expects an input path and a table index",
                          json_output);
        return 2;
    }

    const auto target_all = arguments[2] == "all";

    table_position_options options;
    std::string error_message;
    if (!parse_table_position_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::size_t> requested_table_indices;
    if (!parse_table_position_targets(command, arguments, target_all,
                                      options.additional_table_indices,
                                      requested_table_indices,
                                      options.json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::vector<std::size_t> mutated_table_indices;
    if (!mutate_table_positions(
            command, doc, target_all, requested_table_indices,
            options.json_output, "failed to set table position",
            [&options](featherdoc::Table &table) {
                return table.set_position(options.position);
            },
            mutated_table_indices)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&mutated_table_indices, &options](std::ostream &stream) {
                stream << ",\"table_indices\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << mutated_table_indices[index];
                }
                stream << "],\"positions\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    write_json_table_position(stream, options.position);
                }
                stream << ']';
            });
    }

    return 0;
}

auto run_clear_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "clear-table-position expects an input path and a table index",
            json_output);
        return 2;
    }

    const auto target_all = arguments[2] == "all";

    table_position_target_options options;
    std::string error_message;
    if (!parse_table_position_target_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::vector<std::size_t> requested_table_indices;
    if (!parse_table_position_targets(command, arguments, target_all,
                                      options.additional_table_indices,
                                      requested_table_indices,
                                      options.json_output)) {
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    std::vector<std::size_t> mutated_table_indices;
    if (!mutate_table_positions(
            command, doc, target_all, requested_table_indices,
            options.json_output, "failed to clear table position",
            [](featherdoc::Table &table) { return table.clear_position(); },
            mutated_table_indices)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&mutated_table_indices](std::ostream &stream) {
                stream << ",\"table_indices\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << mutated_table_indices[index];
                }
                stream << "],\"positions\":[";
                for (std::size_t index = 0;
                     index < mutated_table_indices.size(); ++index) {
                    if (index > 0U) {
                        stream << ',';
                    }
                    stream << "null";
                }
                stream << ']';
            });
    }

    return 0;
}

} // namespace featherdoc_cli
