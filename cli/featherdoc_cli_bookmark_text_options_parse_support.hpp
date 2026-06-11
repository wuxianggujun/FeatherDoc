#pragma once

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_bookmark_text_part_selection_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_part,
    bool &has_kind, bool reject_duplicate_part,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_bookmark_text_output_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::filesystem::path> &output_path,
    std::string &error_message) -> option_parse_result;

[[nodiscard]] auto parse_bookmark_text_binding_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::vector<bookmark_text_binding_source> &binding_sources,
    std::string &error_message) -> option_parse_result;

} // namespace featherdoc_cli
