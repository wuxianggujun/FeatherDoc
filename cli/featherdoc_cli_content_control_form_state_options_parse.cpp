#include "featherdoc_cli_content_control_form_state_options_parse.hpp"

#include "featherdoc_cli_content_control_form_state_options_parse_support.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto form_state_option_result_ok(option_parse_result result) -> bool {
    return result != option_parse_result::error;
}

auto form_state_option_matched(option_parse_result result) -> bool {
    return result == option_parse_result::matched;
}

} // namespace

auto parse_set_content_control_form_state_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_content_control_form_state_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        auto result = parse_template_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, error_message);
        if (!form_state_option_result_ok(result)) {
            return false;
        }
        if (form_state_option_matched(result)) {
            continue;
        }

        const auto argument = arguments[index];
        result = parse_content_control_form_state_selector_option(
            argument, arguments, index, options, error_message);
        if (!form_state_option_result_ok(result)) {
            return false;
        }
        if (form_state_option_matched(result)) {
            continue;
        }

        result = parse_content_control_form_state_value_option(
            argument, arguments, index, options, error_message);
        if (!form_state_option_result_ok(result)) {
            return false;
        }
        if (form_state_option_matched(result)) {
            continue;
        }

        result = parse_content_control_form_state_output_option(
            argument, arguments, index, options, error_message);
        if (!form_state_option_result_ok(result)) {
            return false;
        }
        if (form_state_option_matched(result)) {
            continue;
        }

        error_message = "unknown option: " + std::string{argument};
        return false;
    }

    return validate_content_control_form_state_options(options, error_message) &&
           validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

} // namespace featherdoc_cli
