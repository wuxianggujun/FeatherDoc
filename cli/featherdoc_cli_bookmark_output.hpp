#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

void inspect_bookmarks(const selected_template_part &selected,
                       const std::vector<featherdoc::bookmark_summary> &bookmarks,
                       bool json_output);

void inspect_bookmark(const selected_template_part &selected,
                      const featherdoc::bookmark_summary &bookmark,
                      bool json_output);

void write_json_bookmark_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images);

void print_bookmark_image_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::optional<path_type> &output_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images);

void write_json_bookmark_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs, std::size_t replaced);

void print_bookmark_paragraphs_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs,
    const std::optional<path_type> &output_path, std::size_t replaced);

void write_json_bookmark_block_removal_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::size_t removed);

void print_bookmark_block_removal_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, std::size_t removed);

void write_json_bookmark_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::string_view text,
    std::size_t replaced);

void print_bookmark_text_result(const selected_template_part &selected,
                                const featherdoc::bookmark_summary &bookmark,
                                std::string_view text,
                                const std::optional<path_type> &output_path,
                                std::size_t replaced);

void write_json_bookmark_fill_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const featherdoc::bookmark_fill_result &result);

void print_bookmark_fill_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_fill_result &result);

} // namespace featherdoc_cli
