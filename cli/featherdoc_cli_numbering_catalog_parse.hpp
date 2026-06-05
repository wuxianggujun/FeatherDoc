#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_json_numbering_catalog_uint32(
    std::string_view content, std::size_t &index, std::uint32_t &value,
    std::string_view member_name, std::string &error_message) -> bool;
[[nodiscard]] auto parse_json_numbering_catalog_optional_uint32(
    std::string_view content, std::size_t &index,
    std::optional<std::uint32_t> &value, std::string_view member_name,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_numbering_catalog_level_definition(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_level_definition &definition,
    std::string &error_message) -> bool;
[[nodiscard]] auto read_numbering_catalog_file(
    const std::filesystem::path &catalog_path,
    featherdoc::numbering_catalog &catalog, std::string &error_message) -> bool;

} // namespace featherdoc_cli