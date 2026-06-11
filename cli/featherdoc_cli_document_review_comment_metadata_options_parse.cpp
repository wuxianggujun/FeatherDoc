#include "featherdoc_cli_document_mutation_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_comment_metadata_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    comment_metadata_mutation_options &options,
    std::string &error_message) -> bool {
    auto parse_string_option =
        [&](std::size_t &index, const char *option_name,
            const char *clear_option_name, std::optional<std::string> &target,
            bool clear_flag) -> bool {
        if (target.has_value()) {
            error_message = "duplicate " + std::string(option_name) + " option";
            return false;
        }
        if (clear_flag) {
            error_message = std::string(option_name) + " conflicts with " +
                            clear_option_name;
            return false;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after " + std::string(option_name);
            return false;
        }
        target = std::string(arguments[index + 1U]);
        ++index;
        return true;
    };

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--author") {
            if (!parse_string_option(index, "--author", "--clear-author",
                                     options.metadata.author,
                                     options.metadata.clear_author)) {
                return false;
            }
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
        if (argument == "--initials") {
            if (!parse_string_option(index, "--initials", "--clear-initials",
                                     options.metadata.initials,
                                     options.metadata.clear_initials)) {
                return false;
            }
            continue;
        }
        if (argument == "--clear-initials") {
            if (options.metadata.initials.has_value() ||
                options.metadata.clear_initials) {
                error_message =
                    "--clear-initials conflicts with or duplicates --initials";
                return false;
            }
            options.metadata.clear_initials = true;
            continue;
        }
        if (argument == "--date") {
            if (!parse_string_option(index, "--date", "--clear-date",
                                     options.metadata.date,
                                     options.metadata.clear_date)) {
                return false;
            }
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
        !options.metadata.initials.has_value() &&
        !options.metadata.date.has_value() && !options.metadata.clear_author &&
        !options.metadata.clear_initials && !options.metadata.clear_date) {
        error_message = "expected at least one comment metadata option";
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
