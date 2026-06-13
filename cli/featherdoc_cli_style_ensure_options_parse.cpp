#include "featherdoc_cli_style_ensure_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_common_options_parse.hpp"
#include "featherdoc_cli_style_ensure_table_options_parse.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

namespace {

using path_type = std::filesystem::path;

template <typename Options>
auto parse_style_output_option(const std::vector<std::string_view> &arguments,
                               std::size_t &index, Options &options,
                               std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--output") {
        if (options.output_path.has_value()) {
            error_message = "duplicate --output option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing path after --output";
            return option_parse_result::error;
        }

        options.output_path = path_type(std::string(arguments[index + 1U]));
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--json") {
        options.json_output = true;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace

auto parse_ensure_paragraph_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_paragraph_style_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (argument == "--next-style") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --next-style";
                return false;
            }

            options.definition.next_style = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--paragraph-bidi") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph-bidi";
                return false;
            }

            bool value = false;
            if (!parse_bool(arguments[index + 1U], value)) {
                error_message = "invalid --paragraph-bidi value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.definition.paragraph_bidi = value;
            ++index;
            continue;
        }

        if (argument == "--outline-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --outline-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid outline level: " +
                    std::string(arguments[index + 1U]);
                return false;
            }

            options.definition.outline_level = value;
            ++index;
            continue;
        }

        if (const auto result = parse_style_output_option(
                arguments, index, options, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_character_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_character_style_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_run_style_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (const auto result = parse_style_output_option(
                arguments, index, options, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

auto parse_ensure_table_style_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_table_style_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (const auto result = parse_style_catalog_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_ensure_table_style_option(
                arguments, index, options.definition, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        const auto argument = arguments[index];
        if (const auto result = parse_style_output_option(
                arguments, index, options, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.definition.name.empty()) {
        error_message = "missing required --name <name>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
