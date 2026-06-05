#include "featherdoc_cli_numbering_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_numbering_level_definition(
    std::string_view text, featherdoc::numbering_level_definition &definition,
    std::string &error_message) -> bool {
    const auto first_separator = text.find(':');
    const auto second_separator =
        first_separator == std::string_view::npos
            ? std::string_view::npos
            : text.find(':', first_separator + 1U);
    const auto third_separator =
        second_separator == std::string_view::npos
            ? std::string_view::npos
            : text.find(':', second_separator + 1U);
    if (first_separator == std::string_view::npos ||
        second_separator == std::string_view::npos ||
        third_separator == std::string_view::npos) {
        error_message =
            "invalid --numbering-level value: expected "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }

    const auto level_text = text.substr(0U, first_separator);
    const auto kind_text =
        text.substr(first_separator + 1U, second_separator - first_separator - 1U);
    const auto start_text =
        text.substr(second_separator + 1U, third_separator - second_separator - 1U);
    const auto text_pattern = text.substr(third_separator + 1U);
    if (text_pattern.empty()) {
        error_message =
            "invalid --numbering-level value: text pattern must not be empty";
        return false;
    }

    std::uint32_t level = 0U;
    if (!parse_uint32(level_text, level)) {
        error_message = "invalid numbering level: " + std::string(level_text);
        return false;
    }

    featherdoc::list_kind kind{};
    if (!parse_list_kind(kind_text, kind)) {
        error_message = "invalid numbering kind: " + std::string(kind_text);
        return false;
    }

    std::uint32_t start = 0U;
    if (!parse_uint32(start_text, start)) {
        error_message = "invalid numbering start: " + std::string(start_text);
        return false;
    }

    definition.kind = kind;
    definition.start = start;
    definition.level = level;
    definition.text_pattern = std::string(text_pattern);
    return true;
}

auto parse_paragraph_style_numbering_link(
    std::string_view text, featherdoc::paragraph_style_numbering_link &style_link,
    std::string &error_message) -> bool {
    const auto separator = text.find(':');
    if (separator == std::string_view::npos) {
        error_message =
            "invalid --style-link value: expected <style-id>:<level>";
        return false;
    }

    const auto style_id = text.substr(0U, separator);
    const auto level_text = text.substr(separator + 1U);
    if (style_id.empty()) {
        error_message = "invalid --style-link value: style id must not be empty";
        return false;
    }
    if (level_text.empty()) {
        error_message = "invalid --style-link value: level must not be empty";
        return false;
    }

    std::uint32_t level = 0U;
    if (!parse_uint32(level_text, level)) {
        error_message = "invalid style link level: " + std::string(level_text);
        return false;
    }

    style_link.style_id = std::string(style_id);
    style_link.level = level;
    return true;
}

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

auto parse_ensure_numbering_definition_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_numbering_definition_options &options, std::string &error_message)
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

auto parse_set_paragraph_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition") {
            if (options.definition_id.has_value()) {
                error_message = "duplicate --definition option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid numbering definition id: " +
                    std::string(arguments[index + 1U]);
                return false;
            }

            options.definition_id = value;
            ++index;
            continue;
        }

        if (argument == "--level") {
            if (options.level.has_value()) {
                error_message = "duplicate --level option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid numbering level: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.level = value;
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

    if (!options.definition_id.has_value()) {
        error_message = "missing --definition <id>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
