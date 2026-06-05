#include "featherdoc_cli_pdf_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_export_pdf_options(const std::vector<std::string_view> &arguments,
                              std::size_t start_index,
                              export_pdf_options &options,
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
            options.output_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--font-file") {
            if (options.font_file_path.has_value()) {
                error_message = "duplicate --font-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --font-file";
                return false;
            }
            options.font_file_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--cjk-font-file") {
            if (options.cjk_font_file_path.has_value()) {
                error_message = "duplicate --cjk-font-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --cjk-font-file";
                return false;
            }
            options.cjk_font_file_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--font-map") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing mapping after --font-map";
                return false;
            }
            const auto mapping = arguments[index + 1U];
            const auto separator = mapping.find('=');
            if (separator == std::string_view::npos || separator == 0U ||
                separator + 1U >= mapping.size()) {
                error_message =
                    "--font-map expects <font-family>=<font-file>";
                return false;
            }
            options.font_mappings.emplace_back(
                std::string(mapping.substr(0U, separator)),
                std::filesystem::path(
                    std::string(mapping.substr(separator + 1U))));
            ++index;
            continue;
        }

        if (argument == "--title") {
            if (options.title.has_value()) {
                error_message = "duplicate --title option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --title";
                return false;
            }
            options.title = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--creator") {
            if (options.creator.has_value()) {
                error_message = "duplicate --creator option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --creator";
                return false;
            }
            options.creator = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--render-headers-and-footers") {
            options.render_headers_and_footers = true;
            continue;
        }

        if (argument == "--render-inline-images") {
            options.render_inline_images = true;
            continue;
        }

        if (argument == "--expand-header-footer-page-placeholders") {
            options.expand_header_footer_page_placeholders = true;
            continue;
        }

        if (argument == "--no-font-subset") {
            options.subset_unicode_fonts = false;
            continue;
        }

        if (argument == "--no-system-font-fallbacks") {
            options.use_system_font_fallbacks = false;
            continue;
        }

        if (argument == "--summary-json") {
            if (options.summary_json_path.has_value()) {
                error_message = "duplicate --summary-json option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --summary-json";
                return false;
            }
            options.summary_json_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
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

    if (!options.output_path.has_value()) {
        error_message = "export-pdf requires --output <path>";
        return false;
    }

    return true;
}

auto parse_import_pdf_options(const std::vector<std::string_view> &arguments,
                              std::size_t start_index,
                              import_pdf_options &options,
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
            options.output_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--import-table-candidates-as-tables") {
            options.import_table_candidates_as_tables = true;
            continue;
        }

        if (argument == "--min-table-continuation-confidence") {
            if (options.min_table_continuation_confidence.has_value()) {
                error_message =
                    "duplicate --min-table-continuation-confidence option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message =
                    "missing value after --min-table-continuation-confidence";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid value after --min-table-continuation-confidence";
                return false;
            }
            options.min_table_continuation_confidence = value;
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

    if (!options.output_path.has_value()) {
        error_message = "import-pdf requires --output <path>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
