#include "featherdoc_cli_numbering_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_set_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition-name") {
            if (options.definition_name.has_value()) {
                error_message = "duplicate --definition-name option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition-name";
                return false;
            }

            options.definition_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--numbering-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --numbering-level";
                return false;
            }

            featherdoc::numbering_level_definition definition;
            if (!parse_numbering_level_definition(arguments[index + 1U], definition,
                                                  error_message)) {
                return false;
            }

            options.levels.push_back(std::move(definition));
            ++index;
            continue;
        }

        if (argument == "--style-level") {
            if (options.style_level.has_value()) {
                error_message = "duplicate --style-level option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid style level: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.style_level = value;
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

    if (!options.definition_name.has_value()) {
        error_message = "missing --definition-name <name>";
        return false;
    }
    if (options.levels.empty()) {
        error_message =
            "expected at least one --numbering-level "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }

    return true;
}

auto parse_ensure_style_linked_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_style_linked_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition-name") {
            if (options.definition_name.has_value()) {
                error_message = "duplicate --definition-name option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition-name";
                return false;
            }

            options.definition_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--numbering-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --numbering-level";
                return false;
            }

            featherdoc::numbering_level_definition definition;
            if (!parse_numbering_level_definition(arguments[index + 1U], definition,
                                                  error_message)) {
                return false;
            }

            options.levels.push_back(std::move(definition));
            ++index;
            continue;
        }

        if (argument == "--style-link") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-link";
                return false;
            }

            featherdoc::paragraph_style_numbering_link style_link;
            if (!parse_paragraph_style_numbering_link(arguments[index + 1U],
                                                      style_link,
                                                      error_message)) {
                return false;
            }

            options.style_links.push_back(std::move(style_link));
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

    if (!options.definition_name.has_value()) {
        error_message = "missing --definition-name <name>";
        return false;
    }
    if (options.levels.empty()) {
        error_message =
            "expected at least one --numbering-level "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }
    if (options.style_links.empty()) {
        error_message = "expected at least one --style-link <style-id>:<level>";
        return false;
    }

    return true;
}

auto parse_clear_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_numbering_options &options,
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
