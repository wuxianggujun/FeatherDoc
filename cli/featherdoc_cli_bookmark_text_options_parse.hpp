#pragma once

#include "featherdoc_cli_image_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct cli_text_source_options {
    std::optional<std::string> text;
    std::optional<std::filesystem::path> text_file;
};

struct bookmark_text_binding_source {
    std::string bookmark_name;
    cli_text_source_options source;
};

struct inspect_bookmarks_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_kind = false;
    bool json_output = false;
};

struct inspect_content_controls_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    bool has_kind = false;
    bool json_output = false;
};

struct replace_content_control_text_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    std::string text;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool has_text = false;
    bool json_output = false;
};

struct set_content_control_form_state_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> tag;
    std::optional<std::string> alias;
    featherdoc::content_control_form_state_options state;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct sync_content_controls_from_custom_xml_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct replace_bookmark_paragraphs_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<cli_text_source_options> paragraph_sources;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct bookmark_table_replacement_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::vector<cli_text_source_options>> row_sources;
    std::optional<std::filesystem::path> output_path;
    bool empty_rows = false;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct replace_content_control_paragraphs_options
    : replace_bookmark_paragraphs_options {
    std::optional<std::string> tag;
    std::optional<std::string> alias;
};

struct content_control_table_replacement_options
    : bookmark_table_replacement_options {
    std::optional<std::string> tag;
    std::optional<std::string> alias;
};

struct replace_content_control_image_options : append_image_options {
    std::optional<std::string> tag;
    std::optional<std::string> alias;
};

struct remove_bookmark_block_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct replace_bookmark_text_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::string text;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool has_text = false;
    bool json_output = false;
};

struct fill_bookmarks_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<bookmark_text_binding_source> binding_sources;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct set_bookmark_block_visibility_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::filesystem::path> output_path;
    bool visible = true;
    bool has_visible = false;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct apply_bookmark_block_visibility_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<featherdoc::bookmark_block_visibility_binding> bindings;
    std::optional<std::filesystem::path> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_bookmarks_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_bookmarks_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_inspect_content_controls_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_content_controls_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_replace_content_control_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_text_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_set_content_control_form_state_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_content_control_form_state_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_sync_content_controls_from_custom_xml_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    sync_content_controls_from_custom_xml_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_bookmark_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_bookmark_paragraphs_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_bookmark_table_replacement_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    bookmark_table_replacement_options &options, bool allow_empty_rows,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_content_control_table_replacement_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    content_control_table_replacement_options &options, bool allow_empty_rows,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_content_control_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_paragraphs_options &options,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_content_control_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_image_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_remove_bookmark_block_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    remove_bookmark_block_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_replace_bookmark_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_bookmark_text_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_fill_bookmarks_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    fill_bookmarks_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_set_bookmark_block_visibility_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_bookmark_block_visibility_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_apply_bookmark_block_visibility_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_bookmark_block_visibility_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_bookmark_image_command_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::string_view command_name, bool allow_floating_layout,
    append_image_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
