#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

struct export_pdf_options {
    std::optional<std::filesystem::path> output_path;
    std::optional<std::filesystem::path> font_file_path;
    std::optional<std::filesystem::path> cjk_font_file_path;
    std::vector<std::pair<std::string, std::filesystem::path>> font_mappings;
    bool use_system_font_fallbacks = true;
    std::optional<std::string> title;
    std::optional<std::string> creator;
    bool render_headers_and_footers = false;
    bool render_inline_images = false;
    bool expand_header_footer_page_placeholders = false;
    bool subset_unicode_fonts = true;
    std::optional<std::filesystem::path> summary_json_path;
    bool json_output = false;
};

struct import_pdf_options {
    std::optional<std::filesystem::path> output_path;
    bool import_table_candidates_as_tables = false;
    std::optional<std::uint32_t> min_table_continuation_confidence;
    bool json_output = false;
};

[[nodiscard]] auto parse_export_pdf_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_pdf_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_import_pdf_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    import_pdf_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
