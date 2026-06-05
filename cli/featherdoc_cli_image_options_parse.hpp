#pragma once

#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_images_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> relationship_id;
    std::optional<std::string> image_entry_name;
    std::optional<std::size_t> image_index;
    bool has_kind = false;
    bool json_output = false;
};

struct replace_image_options : inspect_images_options {
    std::optional<std::filesystem::path> output_path;
};

struct extract_image_options : inspect_images_options {};

struct remove_image_options : inspect_images_options {
    std::optional<std::filesystem::path> output_path;
};

struct append_image_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> output_path;
    std::optional<std::uint32_t> width_px;
    std::optional<std::uint32_t> height_px;
    featherdoc::floating_image_options floating_options;
    bool floating = false;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_images_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_images_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_image_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_extract_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    extract_image_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_remove_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    remove_image_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_append_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    append_image_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
