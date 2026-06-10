#include "featherdoc_cli_document_mutation_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_hyperlink_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    hyperlink_mutation_options &options, bool require_text_and_target,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--text") {
            if (options.has_text) {
                error_message = "duplicate --text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }
            options.text = std::string(arguments[index + 1U]);
            options.has_text = true;
            ++index;
            continue;
        }

        if (argument == "--target") {
            if (options.has_target) {
                error_message = "duplicate --target option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --target";
                return false;
            }
            options.target = std::string(arguments[index + 1U]);
            options.has_target = true;
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

    if (require_text_and_target && !options.has_text) {
        error_message = "expected --text <text>";
        return false;
    }
    if (require_text_and_target && !options.has_target) {
        error_message = "expected --target <url>";
        return false;
    }
    return true;
}

auto parse_omml_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    omml_mutation_options &options, bool require_xml,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--xml") {
            if (options.has_xml) {
                error_message = "duplicate --xml option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --xml";
                return false;
            }
            options.xml = std::string(arguments[index + 1U]);
            options.has_xml = true;
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

    if (require_xml && !options.has_xml) {
        error_message = "expected --xml <omml-xml>";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
