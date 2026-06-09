#include "featherdoc_cli_content_control_rich_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

auto filter_content_control_selector_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::optional<std::string> &tag, std::optional<std::string> &alias,
    std::vector<std::string_view> &filtered_arguments,
    std::string &error_message) -> bool {
    filtered_arguments.clear();
    filtered_arguments.reserve(arguments.size() - start_index);

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--tag") {
            if (tag.has_value()) {
                error_message = "duplicate --tag option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --tag";
                return false;
            }
            auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--tag expects a non-empty value";
                return false;
            }
            tag = std::move(value);
            ++index;
            continue;
        }

        if (argument == "--alias") {
            if (alias.has_value()) {
                error_message = "duplicate --alias option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --alias";
                return false;
            }
            auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--alias expects a non-empty value";
                return false;
            }
            alias = std::move(value);
            ++index;
            continue;
        }

        filtered_arguments.push_back(argument);
    }

    return true;
}

auto validate_content_control_selector(
    std::string_view command, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias, std::string &error_message) -> bool {
    if (!tag.has_value() && !alias.has_value()) {
        error_message = std::string(command) + " expects --tag or --alias";
        return false;
    }
    if (tag.has_value() && alias.has_value()) {
        error_message = "--tag cannot be combined with --alias";
        return false;
    }
    return true;
}

} // namespace

auto parse_replace_content_control_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_paragraphs_options &options,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_replace_bookmark_paragraphs_options(
            filtered_arguments, 0U, options, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        "replace-content-control-paragraphs", options.tag, options.alias,
        error_message);
}

auto parse_content_control_table_replacement_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    content_control_table_replacement_options &options, bool allow_empty_rows,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_bookmark_table_replacement_options(
            filtered_arguments, 0U, options, allow_empty_rows, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        allow_empty_rows ? "replace-content-control-table-rows"
                         : "replace-content-control-table",
        options.tag, options.alias, error_message);
}

auto parse_replace_content_control_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_image_options &options,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_bookmark_image_command_options(
            filtered_arguments, 0U, "replace-content-control-image", false,
            options, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        "replace-content-control-image", options.tag, options.alias,
        error_message);
}

} // namespace featherdoc_cli
