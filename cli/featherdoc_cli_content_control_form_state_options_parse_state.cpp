#include "featherdoc_cli_content_control_form_state_options_parse_support.hpp"

#include <optional>
#include <utility>

namespace featherdoc_cli {
namespace {

auto read_required_state_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string missing_message, std::string &error_message)
    -> std::optional<std::string_view> {
    if (index + 1U >= arguments.size()) {
        error_message = std::move(missing_message);
        return std::nullopt;
    }
    return arguments[index + 1U];
}

auto parse_non_empty_state_string_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name, std::optional<std::string> &target,
    std::string &error_message) -> bool {
    if (target.has_value()) {
        error_message = "duplicate " + std::string{option_name} + " option";
        return false;
    }

    const auto value = read_required_state_option_value(
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

auto parse_optional_state_string_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name, std::optional<std::string> &target,
    std::string &error_message) -> bool {
    if (target.has_value()) {
        error_message = "duplicate " + std::string{option_name} + " option";
        return false;
    }

    const auto value = read_required_state_option_value(
        arguments, index, "missing value after " + std::string{option_name},
        error_message);
    if (!value.has_value()) {
        return false;
    }

    target = std::string{*value};
    ++index;
    return true;
}

auto parse_clear_state_flag(std::string_view option_name, bool &target,
                            std::string &error_message) -> bool {
    if (target) {
        error_message = "duplicate " + std::string{option_name} + " option";
        return false;
    }
    target = true;
    return true;
}

} // namespace

auto parse_content_control_form_state_value_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, set_content_control_form_state_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--lock") {
        return parse_non_empty_state_string_option(
                   arguments, index, "--lock", options.state.lock, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    if (argument == "--clear-lock") {
        return parse_clear_state_flag("--clear-lock", options.state.clear_lock,
                                      error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    if (argument == "--checked") {
        if (options.state.checked.has_value()) {
            error_message = "duplicate --checked option";
            return option_parse_result::error;
        }
        const auto value = read_required_state_option_value(
            arguments, index, "missing value after --checked", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }
        bool checked = false;
        if (!parse_bool(*value, checked)) {
            error_message = "invalid --checked value: " + std::string{*value};
            return option_parse_result::error;
        }
        options.state.checked = checked;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--selected-item") {
        return parse_non_empty_state_string_option(
                   arguments, index, "--selected-item",
                   options.state.selected_list_item, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    if (argument == "--date-text") {
        return parse_optional_state_string_option(
                   arguments, index, "--date-text", options.state.date_text,
                   error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    if (argument == "--date-format") {
        return parse_optional_state_string_option(
                   arguments, index, "--date-format", options.state.date_format,
                   error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    if (argument == "--date-locale") {
        return parse_optional_state_string_option(
                   arguments, index, "--date-locale", options.state.date_locale,
                   error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    if (argument == "--clear-data-binding") {
        return parse_clear_state_flag("--clear-data-binding",
                                      options.state.clear_data_binding,
                                      error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    if (argument == "--data-binding-store-item-id") {
        return parse_non_empty_state_string_option(
                   arguments, index, "--data-binding-store-item-id",
                   options.state.data_binding_store_item_id, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    if (argument == "--data-binding-xpath") {
        return parse_non_empty_state_string_option(
                   arguments, index, "--data-binding-xpath",
                   options.state.data_binding_xpath, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }
    if (argument == "--data-binding-prefix-mappings") {
        return parse_non_empty_state_string_option(
                   arguments, index, "--data-binding-prefix-mappings",
                   options.state.data_binding_prefix_mappings, error_message)
                   ? option_parse_result::matched
                   : option_parse_result::error;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
