#include "featherdoc_cli_template_schema_options_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_export_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--section-targets") {
            if (options.resolved_section_targets) {
                error_message =
                    "--section-targets conflicts with --resolved-section-targets";
                return false;
            }
            options.section_targets = true;
            continue;
        }

        if (argument == "--resolved-section-targets") {
            if (options.section_targets) {
                error_message =
                    "--resolved-section-targets conflicts with --section-targets";
                return false;
            }
            options.resolved_section_targets = true;
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

    return true;
}

auto parse_normalize_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    normalize_template_schema_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

    return true;
}

auto parse_lint_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    lint_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_repair_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_template_schema_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

    return true;
}

auto parse_merge_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    merge_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

        if (argument.starts_with("--")) {
            error_message = "unknown option: " + std::string(argument);
            return false;
        }

        options.schema_paths.emplace_back(std::string(argument));
    }

    if (options.schema_paths.size() < 2U) {
        error_message = "merge-template-schema expects at least two schema paths";
        return false;
    }

    return true;
}

auto parse_patch_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    patch_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (options.patch_path.has_value()) {
                error_message = "duplicate --patch-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --patch-file";
                return false;
            }

            options.patch_path = path_type(std::string(arguments[index + 1U]));
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

    if (!options.patch_path.has_value()) {
        error_message = "missing required --patch-file <path> option";
        return false;
    }

    return true;
}

auto parse_preview_template_schema_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    preview_template_schema_patch_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (options.patch_path.has_value()) {
                error_message = "duplicate --patch-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --patch-file";
                return false;
            }

            options.patch_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output-patch") {
            if (options.output_patch_path.has_value()) {
                error_message = "duplicate --output-patch option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output-patch";
                return false;
            }

            options.output_patch_path =
                path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--review-json") {
            if (options.review_json_path.has_value()) {
                error_message = "duplicate --review-json option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --review-json";
                return false;
            }

            options.review_json_path =
                path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument.starts_with("--")) {
            error_message = "unknown option: " + std::string(argument);
            return false;
        }

        if (options.right_schema_path.has_value()) {
            error_message = "duplicate right schema path";
            return false;
        }
        options.right_schema_path = path_type(std::string(argument));
    }

    if (options.patch_path.has_value() == options.right_schema_path.has_value()) {
        error_message = "preview-template-schema-patch expects exactly one of "
                        "--patch-file <path> or <right-schema.json>";
        return false;
    }

    return true;
}

auto parse_diff_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    diff_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--fail-on-diff") {
            options.fail_on_diff = true;
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

auto parse_build_template_schema_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    build_template_schema_patch_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

        if (argument == "--review-json") {
            if (options.review_json_path.has_value()) {
                error_message = "duplicate --review-json option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --review-json";
                return false;
            }

            options.review_json_path =
                path_type(std::string(arguments[index + 1U]));
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

    return true;
}

auto parse_check_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--schema-file") {
            if (options.schema_path.has_value()) {
                error_message = "duplicate --schema-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --schema-file";
                return false;
            }

            options.schema_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--section-targets") {
            if (options.resolved_section_targets) {
                error_message =
                    "--section-targets conflicts with --resolved-section-targets";
                return false;
            }
            options.section_targets = true;
            continue;
        }

        if (argument == "--resolved-section-targets") {
            if (options.section_targets) {
                error_message =
                    "--resolved-section-targets conflicts with --section-targets";
                return false;
            }
            options.resolved_section_targets = true;
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

    if (!options.schema_path.has_value()) {
        error_message = "missing required --schema-file <path> option";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
