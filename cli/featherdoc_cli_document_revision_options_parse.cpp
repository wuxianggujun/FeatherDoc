#include "featherdoc_cli_document_mutation_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_revision_authoring_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    revision_authoring_options &options, std::string &error_message,
    bool require_text, std::string_view text_forbidden_error,
    bool allow_expected_text,
    std::string_view expected_text_forbidden_error) -> bool {
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

        if (argument == "--author") {
            if (options.has_author) {
                error_message = "duplicate --author option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --author";
                return false;
            }
            options.author = std::string(arguments[index + 1U]);
            options.has_author = true;
            ++index;
            continue;
        }

        if (argument == "--date") {
            if (options.has_date) {
                error_message = "duplicate --date option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --date";
                return false;
            }
            options.date = std::string(arguments[index + 1U]);
            options.has_date = true;
            ++index;
            continue;
        }

        if (argument == "--expected-text") {
            if (!allow_expected_text) {
                error_message = std::string{expected_text_forbidden_error};
                return false;
            }
            if (options.has_expected_text) {
                error_message = "duplicate --expected-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --expected-text";
                return false;
            }
            options.expected_text = std::string(arguments[index + 1U]);
            options.has_expected_text = true;
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

    if (require_text && !options.has_text) {
        error_message = "expected --text <text>";
        return false;
    }
    if (!require_text && options.has_text) {
        error_message = std::string{text_forbidden_error};
        return false;
    }
    return true;
}

auto parse_revision_metadata_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    revision_metadata_mutation_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--author") {
            if (options.metadata.author.has_value() ||
                options.metadata.clear_author) {
                error_message =
                    "--author conflicts with or duplicates --clear-author";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --author";
                return false;
            }
            options.metadata.author = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }
        if (argument == "--clear-author") {
            if (options.metadata.author.has_value() ||
                options.metadata.clear_author) {
                error_message =
                    "--clear-author conflicts with or duplicates --author";
                return false;
            }
            options.metadata.clear_author = true;
            continue;
        }
        if (argument == "--date") {
            if (options.metadata.date.has_value() ||
                options.metadata.clear_date) {
                error_message = "--date conflicts with or duplicates --clear-date";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --date";
                return false;
            }
            options.metadata.date = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }
        if (argument == "--clear-date") {
            if (options.metadata.date.has_value() ||
                options.metadata.clear_date) {
                error_message = "--clear-date conflicts with or duplicates --date";
                return false;
            }
            options.metadata.clear_date = true;
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

    if (!options.metadata.author.has_value() &&
        !options.metadata.date.has_value() && !options.metadata.clear_author &&
        !options.metadata.clear_date) {
        error_message = "expected at least one revision metadata option";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
