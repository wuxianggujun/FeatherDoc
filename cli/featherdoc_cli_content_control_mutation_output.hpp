#pragma once

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_template_part_selection.hpp"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <vector>

namespace featherdoc_cli {

void write_json_content_control_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_text_options &options, std::size_t replaced);

void print_content_control_text_result(
    const selected_template_part &selected,
    const replace_content_control_text_options &options,
    const std::optional<path_type> &output_path, std::size_t replaced);

void write_json_content_control_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced);

void print_content_control_paragraphs_result(
    const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced);

void write_json_content_control_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images);

void print_content_control_image_result(
    const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images);

void write_json_content_control_form_state_result(
    std::ostream &stream, const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated);

void print_content_control_form_state_result(
    const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated);

void write_json_custom_xml_sync_result(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_result &result);

void print_custom_xml_sync_result(
    const std::optional<path_type> &output_path,
    const featherdoc::custom_xml_data_binding_sync_result &result);

} // namespace featherdoc_cli
