#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct export_numbering_catalog_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct import_numbering_catalog_options {
    std::optional<std::filesystem::path> catalog_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct check_numbering_catalog_options {
    std::optional<std::filesystem::path> catalog_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct patch_numbering_catalog_options {
    std::optional<std::filesystem::path> patch_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

[[nodiscard]] auto parse_export_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_numbering_catalog_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_import_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    import_numbering_catalog_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_check_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_numbering_catalog_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_patch_numbering_catalog_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    patch_numbering_catalog_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
