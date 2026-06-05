#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_page_setup_options {
    std::optional<std::size_t> section_index;
    bool json_output = false;
};

struct set_section_page_setup_options {
    std::optional<featherdoc::page_orientation> orientation;
    std::optional<std::uint32_t> width_twips;
    std::optional<std::uint32_t> height_twips;
    std::optional<std::uint32_t> margin_top_twips;
    std::optional<std::uint32_t> margin_bottom_twips;
    std::optional<std::uint32_t> margin_left_twips;
    std::optional<std::uint32_t> margin_right_twips;
    std::optional<std::uint32_t> margin_header_twips;
    std::optional<std::uint32_t> margin_footer_twips;
    std::optional<std::uint32_t> page_number_start;
    bool clear_page_number_start = false;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_page_setup_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_section_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_section_page_setup_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
