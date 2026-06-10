#include "featherdoc_cli_field_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_field_complex_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

namespace {

#include "featherdoc_cli_field_parse_traits.inc"

#include "featherdoc_cli_field_parse_common_options.inc"

#include "featherdoc_cli_field_parse_caption_sequence_options.inc"

#include "featherdoc_cli_field_parse_index_toc_options.inc"

#include "featherdoc_cli_field_parse_reference_options.inc"

#include "featherdoc_cli_field_parse_state_options.inc"

#include "featherdoc_cli_field_parse_argument_options.inc"

#include "featherdoc_cli_field_parse_validate.inc"

} // namespace

auto parse_append_field_options(std::string_view command,
                                const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                append_field_options &options,
                                std::string &error_message) -> bool {
    const auto traits = make_append_field_command_traits(command);

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];

        auto result = parse_common_append_field_option(
            command, argument, arguments, index, traits, options, error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        const auto complex_option_result =
            parse_complex_field_option(command, argument, arguments, index,
                                       options, traits.is_complex, error_message);
        if (complex_option_result == field_complex_option_parse_result::error) {
            return false;
        }
        if (complex_option_result == field_complex_option_parse_result::handled) {
            continue;
        }

        result = parse_caption_sequence_field_option(
            command, argument, arguments, index, traits, options, error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        result = parse_index_toc_field_option(command, argument, arguments, index,
                                              traits, options, error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        result = parse_reference_field_option(command, argument, arguments, index,
                                              traits, options, error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        result = parse_state_field_option(command, argument, traits, options,
                                          error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        result = parse_field_argument(command, argument, traits, options,
                                      error_message);
        if (result == field_option_parse_result::error) {
            return false;
        }
        if (result == field_option_parse_result::handled) {
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_append_field_options(command, traits, options, error_message);
}

#include "featherdoc_cli_field_parse_command_names.inc"

} // namespace featherdoc_cli
