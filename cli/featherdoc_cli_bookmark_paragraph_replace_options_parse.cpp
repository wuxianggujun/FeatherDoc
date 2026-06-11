#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_replace_bookmark_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_bookmark_paragraphs_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (options.has_part) {
                error_message = "duplicate --part option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_part = true;
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

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

            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--paragraph") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph";
                return false;
            }

            cli_text_source_options source;
            source.text = std::string(arguments[index + 1U]);
            options.paragraph_sources.push_back(std::move(source));
            ++index;
            continue;
        }

        if (argument == "--paragraph-file") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --paragraph-file";
                return false;
            }

            cli_text_source_options source;
            source.text_file = path_type(std::string(arguments[index + 1U]));
            options.paragraph_sources.push_back(std::move(source));
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

    if (options.paragraph_sources.empty()) {
        error_message =
            "expected at least one --paragraph <text> or --paragraph-file <path>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index,
                                            options.has_kind, "mutation",
                                            error_message);
}

} // namespace featherdoc_cli
