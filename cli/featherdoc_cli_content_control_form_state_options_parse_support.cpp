#include "featherdoc_cli_content_control_form_state_options_parse_support.hpp"

#include <filesystem>
#include <optional>
#include <utility>

namespace featherdoc_cli {
namespace {

auto content_control_form_state_has_cli_changes(
    const featherdoc::content_control_form_state_options &state) -> bool {
    return state.lock.has_value() || state.clear_lock ||
           state.clear_data_binding || state.data_binding_store_item_id.has_value() ||
           state.data_binding_xpath.has_value() ||
           state.data_binding_prefix_mappings.has_value() ||
           state.checked.has_value() || state.selected_list_item.has_value() ||
           state.date_text.has_value() || state.date_format.has_value() ||
           state.date_locale.has_value();
}

auto read_required_form_state_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string missing_message, std::string &error_message)
    -> std::optional<std::string_view> {
    if (index + 1U >= arguments.size()) {
        error_message = std::move(missing_message);
        return std::nullopt;
    }
    return arguments[index + 1U];
}

auto parse_non_empty_string_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name, std::optional<std::string> &target,
    std::string &error_message) -> bool {
    if (target.has_value()) {
        error_message = "duplicate " + std::string{option_name} + " option";
        return false;
    }

    const auto value = read_required_form_state_option_value(
        arguments, index, "missing value after " + std::string{option_name},
        error_message);
    if (!value.has_value()) {
        return false;
    }

    auto parsed_value = std::string{*value};
    if (parsed_value.empty()) {
        error_message = std::string{option_name} + " expects a non-empty value";
        return false;
    }

    target = std::move(parsed_value);
    ++index;
    return true;
}

auto validate_content_control_form_state_selector(
    const std::optional<std::string> &tag,
    const std::optional<std::string> &alias, std::string &error_message) -> bool {
    if (!tag.has_value() && !alias.has_value()) {
        error_message = "set-content-control-form-state expects --tag or --alias";
        return false;
    }
    if (tag.has_value() && alias.has_value()) {
        error_message = "--tag cannot be combined with --alias";
        return false;
    }
    return true;
}

} // namespace

auto parse_content_control_form_state_selector_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--tag") {
        return parse_non_empty_string_option(arguments, index, "--tag",
                                             options.tag, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    if (argument == "--alias") {
        return parse_non_empty_string_option(arguments, index, "--alias",
                                             options.alias, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    return option_parse_result::not_matched;
}

auto parse_content_control_form_state_output_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--output") {
        if (options.output_path.has_value()) {
            error_message = "duplicate --output option";
            return option_parse_result::error;
        }
        const auto value = read_required_form_state_option_value(
            arguments, index, "missing path after --output", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        options.output_path = std::filesystem::path(std::string{*value});
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--json") {
        options.json_output = true;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto validate_content_control_form_state_options(
    const set_content_control_form_state_options &options,
    std::string &error_message) -> bool {
    if (!validate_content_control_form_state_selector(options.tag, options.alias,
                                                      error_message)) {
        return false;
    }
    if (!content_control_form_state_has_cli_changes(options.state)) {
        error_message =
            "set-content-control-form-state expects at least one form-state option";
        return false;
    }
    if (options.state.lock.has_value() && options.state.clear_lock) {
        error_message = "--lock cannot be combined with --clear-lock";
        return false;
    }
    const bool has_data_binding_options =
        options.state.data_binding_store_item_id.has_value() ||
        options.state.data_binding_xpath.has_value() ||
        options.state.data_binding_prefix_mappings.has_value();
    if (options.state.clear_data_binding && has_data_binding_options) {
        error_message =
            "--clear-data-binding cannot be combined with --data-binding-* options";
        return false;
    }
    if (has_data_binding_options &&
        (!options.state.data_binding_store_item_id.has_value() ||
         !options.state.data_binding_xpath.has_value())) {
        error_message =
            "data binding requires --data-binding-store-item-id and --data-binding-xpath";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
