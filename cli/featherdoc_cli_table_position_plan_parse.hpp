#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class table_position_preset {
    paragraph_callout,
    page_corner,
    margin_anchor,
};

struct table_position_table_fingerprint {
    std::size_t table_index = 0U;
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> width_twips;
    std::size_t row_count = 0U;
    std::size_t column_count = 0U;
    std::vector<std::optional<std::uint32_t>> column_widths;
    std::string text;
};

struct parsed_table_position_plan_file {
    std::filesystem::path input_path;
    table_position_preset preset{table_position_preset::paragraph_callout};
    std::size_t table_count = 0U;
    std::vector<std::size_t> automatic_table_indices;
    std::optional<std::filesystem::path> resolved_output_path;
    std::vector<table_position_table_fingerprint> table_fingerprints;
};

[[nodiscard]] auto table_position_preset_name(
    table_position_preset preset) noexcept -> std::string_view;
[[nodiscard]] auto parse_table_position_preset(
    std::string_view text, table_position_preset &preset) -> bool;
[[nodiscard]] auto read_table_position_plan_file(
    const std::filesystem::path &plan_path,
    parsed_table_position_plan_file &plan, std::string &error_message) -> bool;

} // namespace featherdoc_cli
