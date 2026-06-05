#include "featherdoc_cli_page_setup_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_inspect_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_page_setup_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_set_section_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_section_page_setup_options &options, std::string &error_message) -> bool {
    auto parse_twips_option =
        [&](std::size_t &index, std::string_view option_name,
            std::optional<std::uint32_t> &target) -> bool {
        if (target.has_value()) {
            error_message = "duplicate " + std::string(option_name) + " option";
            return false;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after " + std::string(option_name);
            return false;
        }

        std::uint32_t value = 0U;
        if (!parse_uint32(arguments[index + 1U], value)) {
            error_message = "invalid value for " + std::string(option_name) + ": " +
                            std::string(arguments[index + 1U]);
            return false;
        }

        target = value;
        ++index;
        return true;
    };

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--orientation") {
            if (options.orientation.has_value()) {
                error_message = "duplicate --orientation option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --orientation";
                return false;
            }

            featherdoc::page_orientation orientation{};
            if (!parse_page_orientation(arguments[index + 1U], orientation)) {
                error_message = "invalid orientation: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.orientation = orientation;
            ++index;
            continue;
        }

        if (argument == "--width") {
            if (!parse_twips_option(index, argument, options.width_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--height") {
            if (!parse_twips_option(index, argument, options.height_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-top") {
            if (!parse_twips_option(index, argument, options.margin_top_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-bottom") {
            if (!parse_twips_option(index, argument, options.margin_bottom_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-left") {
            if (!parse_twips_option(index, argument, options.margin_left_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-right") {
            if (!parse_twips_option(index, argument, options.margin_right_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-header") {
            if (!parse_twips_option(index, argument, options.margin_header_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-footer") {
            if (!parse_twips_option(index, argument, options.margin_footer_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--page-number-start") {
            if (options.page_number_start.has_value() ||
                options.clear_page_number_start) {
                error_message = "duplicate page number start option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --page-number-start";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid value for --page-number-start: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.page_number_start = value;
            ++index;
            continue;
        }
        if (argument == "--clear-page-number-start") {
            if (options.page_number_start.has_value() ||
                options.clear_page_number_start) {
                error_message = "duplicate page number start option";
                return false;
            }

            options.clear_page_number_start = true;
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

    return true;
}

} // namespace featherdoc_cli
