#include "featherdoc_cli_document_mutation_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_review_note_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    review_note_mutation_options &options, bool require_reference_text,
    bool require_note_text, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--reference-text") {
            if (options.has_reference_text) {
                error_message = "duplicate --reference-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --reference-text";
                return false;
            }
            options.reference_text = std::string(arguments[index + 1U]);
            options.has_reference_text = true;
            ++index;
            continue;
        }

        if (argument == "--note-text") {
            if (options.has_note_text) {
                error_message = "duplicate --note-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --note-text";
                return false;
            }
            options.note_text = std::string(arguments[index + 1U]);
            options.has_note_text = true;
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

    if (require_reference_text && !options.has_reference_text) {
        error_message = "expected --reference-text <text>";
        return false;
    }
    if (require_note_text && !options.has_note_text) {
        error_message = "expected --note-text <text>";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
