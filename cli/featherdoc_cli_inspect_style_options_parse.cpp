#include "featherdoc_cli_inspect_style_options_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_inspect_options(const std::vector<std::string_view> &arguments,
                           std::size_t start_index, inspect_options &options,
                           std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(arguments[index]);
        return false;
    }

    return true;
}

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

auto parse_plan_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool {
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

auto parse_apply_table_style_quality_fixes_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_style_quality_fixes_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--look-only") {
            options.look_only = true;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.look_only) {
        error_message = "apply-table-style-quality-fixes requires --look-only";
        return false;
    }
    if (!options.output_path.has_value()) {
        error_message = "apply-table-style-quality-fixes requires --output <path>";
        return false;
    }

    return true;
}

auto parse_repair_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--plan-only") {
            options.plan_only = true;
            continue;
        }

        if (argument == "--apply") {
            options.apply = true;
            continue;
        }

        if (argument == "--catalog-file") {
            if (options.catalog_path.has_value()) {
                error_message = "duplicate --catalog-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --catalog-file";
                return false;
            }

            options.catalog_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.plan_only && options.apply) {
        error_message = "--plan-only and --apply are mutually exclusive";
        return false;
    }
    if (!options.apply) {
        options.plan_only = true;
    }
    if (options.plan_only && options.output_path.has_value()) {
        error_message = "--output requires --apply";
        return false;
    }
    if (options.catalog_path.has_value() && !options.apply) {
        error_message = "--catalog-file requires --apply";
        return false;
    }
    if (options.apply && !options.output_path.has_value()) {
        error_message = "repair-style-numbering --apply requires --output <path>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
