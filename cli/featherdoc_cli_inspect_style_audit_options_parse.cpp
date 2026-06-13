#include "featherdoc_cli_inspect_style_options_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_audit_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--fail-on-issue") {
            options.fail_on_issue = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_audit_table_style_regions_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_regions_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--style") {
            if (options.style_id.has_value()) {
                error_message = "duplicate --style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing style id after --style";
                return false;
            }
            options.style_id = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--fail-on-issue") {
            options.fail_on_issue = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.style_id.has_value() && options.style_id->empty()) {
        error_message = "--style requires a non-empty style id";
        return false;
    }

    return true;
}

auto parse_audit_table_style_inheritance_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_inheritance_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--style") {
            if (options.style_id.has_value()) {
                error_message = "duplicate --style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing style id after --style";
                return false;
            }
            options.style_id = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--fail-on-issue") {
            options.fail_on_issue = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.style_id.has_value() && options.style_id->empty()) {
        error_message = "--style requires a non-empty style id";
        return false;
    }

    return true;
}

auto parse_audit_table_style_quality_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    audit_table_style_quality_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--fail-on-issue") {
            options.fail_on_issue = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
