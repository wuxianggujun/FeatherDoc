#include "featherdoc_cli_field_complex_options_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_complex_field_option(
    std::string_view command, std::string_view argument,
    const std::vector<std::string_view> &arguments, std::size_t &index,
    append_field_options &options, bool is_complex,
    std::string &error_message) -> field_complex_option_parse_result {
    if (argument == "--instruction") {
        if (!is_complex) {
            error_message = std::string(command) + " does not support --instruction";
            return field_complex_option_parse_result::error;
        }
        if (options.instruction.has_value()) {
            error_message = "duplicate --instruction option";
            return field_complex_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --instruction";
            return field_complex_option_parse_result::error;
        }
        options.instruction = std::string(arguments[index + 1U]);
        ++index;
        return field_complex_option_parse_result::handled;
    }

    if (argument == "--instruction-before") {
        if (!is_complex) {
            error_message =
                std::string(command) + " does not support --instruction-before";
            return field_complex_option_parse_result::error;
        }
        if (options.instruction_before.has_value()) {
            error_message = "duplicate --instruction-before option";
            return field_complex_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --instruction-before";
            return field_complex_option_parse_result::error;
        }
        options.instruction_before = std::string(arguments[index + 1U]);
        ++index;
        return field_complex_option_parse_result::handled;
    }

    if (argument == "--instruction-after") {
        if (!is_complex) {
            error_message =
                std::string(command) + " does not support --instruction-after";
            return field_complex_option_parse_result::error;
        }
        if (options.instruction_after.has_value()) {
            error_message = "duplicate --instruction-after option";
            return field_complex_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --instruction-after";
            return field_complex_option_parse_result::error;
        }
        options.instruction_after = std::string(arguments[index + 1U]);
        ++index;
        return field_complex_option_parse_result::handled;
    }

    if (argument == "--nested-instruction") {
        if (!is_complex) {
            error_message =
                std::string(command) + " does not support --nested-instruction";
            return field_complex_option_parse_result::error;
        }
        if (options.nested_instruction.has_value()) {
            error_message = "duplicate --nested-instruction option";
            return field_complex_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --nested-instruction";
            return field_complex_option_parse_result::error;
        }
        options.nested_instruction = std::string(arguments[index + 1U]);
        ++index;
        return field_complex_option_parse_result::handled;
    }

    if (argument == "--nested-result-text") {
        if (!is_complex) {
            error_message =
                std::string(command) + " does not support --nested-result-text";
            return field_complex_option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing text after --nested-result-text";
            return field_complex_option_parse_result::error;
        }
        options.nested_result_text = std::string(arguments[index + 1U]);
        ++index;
        return field_complex_option_parse_result::handled;
    }

    return field_complex_option_parse_result::not_handled;
}

auto validate_complex_field_options(const append_field_options &options,
                                    std::string &error_message) -> bool {
    const auto has_plain_instruction = options.instruction.has_value();
    const auto has_nested_instruction = options.nested_instruction.has_value();
    if (has_plain_instruction &&
        (options.instruction_before.has_value() || has_nested_instruction ||
         options.instruction_after.has_value())) {
        error_message =
            "append-complex-field --instruction cannot be combined with nested instruction options";
        return false;
    }
    if (!has_plain_instruction && !has_nested_instruction) {
        error_message =
            "append-complex-field requires --instruction or --nested-instruction";
        return false;
    }
    if (has_nested_instruction && (!options.instruction_before.has_value() ||
                                   !options.instruction_after.has_value())) {
        error_message =
            "append-complex-field nested mode requires --instruction-before and --instruction-after";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
