#include "featherdoc_cli_document_mutation_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_comment_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    comment_mutation_options &options, bool require_selected_text,
    bool require_comment_text, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--selected-text") {
            if (options.has_selected_text) {
                error_message = "duplicate --selected-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --selected-text";
                return false;
            }
            options.selected_text = std::string(arguments[index + 1U]);
            options.has_selected_text = true;
            ++index;
            continue;
        }

        if (argument == "--comment-text") {
            if (options.has_comment_text) {
                error_message = "duplicate --comment-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --comment-text";
                return false;
            }
            options.comment_text = std::string(arguments[index + 1U]);
            options.has_comment_text = true;
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

        if (argument == "--initials") {
            if (options.has_initials) {
                error_message = "duplicate --initials option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --initials";
                return false;
            }
            options.initials = std::string(arguments[index + 1U]);
            options.has_initials = true;
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

    if (require_selected_text && !options.has_selected_text) {
        error_message = "expected --selected-text <text>";
        return false;
    }
    if (require_comment_text && !options.has_comment_text) {
        error_message = "expected --comment-text <text>";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
