#include "featherdoc_cli_table_position_options_parse_support.hpp"

#include "featherdoc_cli_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_table_position_table_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::vector<std::size_t> &additional_table_indices,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] != "--table") {
        return option_parse_result::not_matched;
    }

    if (index + 1U >= arguments.size()) {
        error_message = "missing value after --table";
        return option_parse_result::error;
    }
    std::size_t table_index = 0U;
    if (!parse_index(arguments[index + 1U], table_index)) {
        error_message =
            "invalid table index: " + std::string(arguments[index + 1U]);
        return option_parse_result::error;
    }
    additional_table_indices.push_back(table_index);
    ++index;
    return option_parse_result::matched;
}

auto parse_table_position_output_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::filesystem::path> &output_path,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] != "--output") {
        return option_parse_result::not_matched;
    }

    if (output_path.has_value()) {
        error_message = "duplicate --output option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing path after --output";
        return option_parse_result::error;
    }

    output_path = std::filesystem::path(std::string(arguments[index + 1U]));
    ++index;
    return option_parse_result::matched;
}

auto parse_table_position_preset_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<table_position_preset> &preset,
    std::string &error_message) -> option_parse_result {
    if (arguments[index] != "--preset") {
        return option_parse_result::not_matched;
    }

    if (preset.has_value()) {
        error_message = "duplicate --preset option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after --preset";
        return option_parse_result::error;
    }

    table_position_preset parsed_preset{};
    if (!parse_table_position_preset(arguments[index + 1U], parsed_preset)) {
        error_message = "invalid table position preset: " +
                        std::string(arguments[index + 1U]);
        return option_parse_result::error;
    }
    preset = parsed_preset;
    ++index;
    return option_parse_result::matched;
}

} // namespace featherdoc_cli
