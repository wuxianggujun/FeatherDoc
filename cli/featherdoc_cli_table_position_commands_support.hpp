#pragma once

#include "featherdoc_cli_command_support.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

void write_text_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values);

auto report_table_position_failure(
    std::string_view command, std::string_view stage,
    std::string_view summary, std::string detail, bool json_output) -> bool;

[[nodiscard]] auto parse_table_position_targets(
    std::string_view command, const std::vector<std::string_view> &arguments,
    bool target_all, const std::vector<std::size_t> &additional_table_indices,
    std::vector<std::size_t> &requested_table_indices, bool json_output)
    -> bool;

template <typename Mutator>
auto mutate_table_positions(
    std::string_view command, featherdoc::Document &doc, bool target_all,
    const std::vector<std::size_t> &requested_table_indices, bool json_output,
    std::string_view failure_summary, Mutator &&mutator,
    std::vector<std::size_t> &mutated_table_indices) -> bool {
    if (target_all) {
        std::size_t table_index = 0U;
        for (featherdoc::Table table = doc.tables(); table.has_next();
             table.next(), ++table_index) {
            if (!mutator(table)) {
                return report_table_position_failure(
                    command, "mutate", failure_summary,
                    "target table handle is not valid", json_output);
            }
            mutated_table_indices.push_back(table_index);
        }

        if (mutated_table_indices.empty()) {
            return report_table_position_failure(command, "mutate",
                                                 "no body tables found",
                                                 "no body tables found",
                                                 json_output);
        }
        return true;
    }

    for (const auto table_index : requested_table_indices) {
        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                json_output)) {
            return false;
        }

        if (!mutator(table)) {
            return report_table_position_failure(
                command, "mutate", failure_summary,
                "target table handle is not valid", json_output);
        }
        mutated_table_indices.push_back(table_index);
    }

    return true;
}

} // namespace featherdoc_cli
