#include "featherdoc_cli_section_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_set_update_fields_on_open_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_update_fields_on_open_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--enable" || argument == "--disable") {
            if (options.enabled.has_value()) {
                error_message = "--enable and --disable are mutually exclusive";
                return false;
            }
            options.enabled = argument == "--enable";
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

    if (!options.enabled.has_value()) {
        error_message = "set-update-fields-on-open requires --enable or --disable";
        return false;
    }

    return true;
}

auto parse_section_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    bool require_text_source, bool allow_output, bool allow_json,
    section_text_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U],
                                      options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            ++index;
            continue;
        }

        if (allow_output && argument == "--output") {
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
            if (!allow_json) {
                error_message = "--json is not supported by this command";
                return false;
            }

            options.json_output = true;
            continue;
        }

        if (argument == "--text") {
            if (!require_text_source) {
                error_message = "--text is only supported by set-section-* commands";
                return false;
            }
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }

            options.text = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--text-file") {
            if (!require_text_source) {
                error_message =
                    "--text-file is only supported by set-section-* commands";
                return false;
            }
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --text-file";
                return false;
            }

            options.text_file = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (require_text_source &&
        !options.text.has_value() && !options.text_file.has_value()) {
        error_message = "expected --text <text> or --text-file <path>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
