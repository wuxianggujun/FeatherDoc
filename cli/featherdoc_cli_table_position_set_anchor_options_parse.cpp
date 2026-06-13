#include "featherdoc_cli_table_position_set_options_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_table_position_anchor_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    table_position_options &options, std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--horizontal-reference") {
        if (options.has_horizontal_reference) {
            error_message = "duplicate --horizontal-reference option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --horizontal-reference";
            return option_parse_result::error;
        }
        if (!parse_table_position_horizontal_reference(
                arguments[index + 1U], options.position.horizontal_reference)) {
            error_message = "invalid horizontal reference: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.has_horizontal_reference = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--horizontal-offset") {
        if (options.has_horizontal_offset) {
            error_message = "duplicate --horizontal-offset option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --horizontal-offset";
            return option_parse_result::error;
        }
        if (!parse_int32(arguments[index + 1U],
                         options.position.horizontal_offset_twips)) {
            error_message = "invalid horizontal offset twips: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.has_horizontal_offset = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--horizontal-spec") {
        if (options.has_horizontal_spec) {
            error_message = "duplicate --horizontal-spec option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --horizontal-spec";
            return option_parse_result::error;
        }
        featherdoc::table_position_horizontal_spec spec{};
        if (!parse_table_position_horizontal_spec(arguments[index + 1U], spec)) {
            error_message = "invalid horizontal spec: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.position.horizontal_spec = spec;
        options.has_horizontal_spec = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--vertical-reference") {
        if (options.has_vertical_reference) {
            error_message = "duplicate --vertical-reference option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --vertical-reference";
            return option_parse_result::error;
        }
        if (!parse_table_position_vertical_reference(
                arguments[index + 1U], options.position.vertical_reference)) {
            error_message = "invalid vertical reference: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.has_vertical_reference = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--vertical-offset") {
        if (options.has_vertical_offset) {
            error_message = "duplicate --vertical-offset option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --vertical-offset";
            return option_parse_result::error;
        }
        if (!parse_int32(arguments[index + 1U],
                         options.position.vertical_offset_twips)) {
            error_message = "invalid vertical offset twips: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.has_vertical_offset = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--vertical-spec") {
        if (options.has_vertical_spec) {
            error_message = "duplicate --vertical-spec option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --vertical-spec";
            return option_parse_result::error;
        }
        featherdoc::table_position_vertical_spec spec{};
        if (!parse_table_position_vertical_spec(arguments[index + 1U], spec)) {
            error_message = "invalid vertical spec: " +
                            std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }
        options.position.vertical_spec = spec;
        options.has_vertical_spec = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
