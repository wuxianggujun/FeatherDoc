#include "featherdoc_cli_template_schema_options_parse.hpp"

#include "featherdoc_cli_template_schema_options_parse_support.hpp"

namespace featherdoc_cli {

auto parse_patch_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    patch_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (!parse_template_schema_path_option(arguments, index,
                                                  "--patch-file",
                                                  options.patch_path,
                                                  error_message)) {
                return false;
            }
            continue;
        }

        if (argument == "--output") {
            if (!parse_template_schema_path_option(arguments, index, "--output",
                                                  options.output_path,
                                                  error_message)) {
                return false;
            }
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
            if (!parse_template_schema_path_option(arguments, index,
                                                  "--patch-file",
                                                  options.patch_path,
                                                  error_message)) {
                return false;
            }
            continue;
        }

        if (argument == "--output-patch") {
            if (!parse_template_schema_path_option(
                    arguments, index, "--output-patch",
                    options.output_patch_path, error_message)) {
                return false;
            }
            continue;
        }

        if (argument == "--review-json") {
            if (!parse_template_schema_path_option(
                    arguments, index, "--review-json", options.review_json_path,
                    error_message)) {
                return false;
            }
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
        options.right_schema_path =
            std::filesystem::path(std::string(argument));
    }

    if (options.patch_path.has_value() == options.right_schema_path.has_value()) {
        error_message = "preview-template-schema-patch expects exactly one of "
                        "--patch-file <path> or <right-schema.json>";
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
            if (!parse_template_schema_path_option(arguments, index, "--output",
                                                  options.output_path,
                                                  error_message)) {
                return false;
            }
            continue;
        }

        if (argument == "--review-json") {
            if (!parse_template_schema_path_option(
                    arguments, index, "--review-json", options.review_json_path,
                    error_message)) {
                return false;
            }
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
