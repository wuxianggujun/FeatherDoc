#pragma once

#include "featherdoc_cli_image_options_parse.hpp"

#include <featherdoc.hpp>

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

void write_json_drawing_image_summary(
    std::ostream &stream, const featherdoc::drawing_image_info &image);
void print_drawing_image_summary(std::ostream &stream,
                                 const featherdoc::drawing_image_info &image);

[[nodiscard]] auto has_inspect_image_filters(
    const inspect_images_options &options) -> bool;
[[nodiscard]] auto has_drawing_image_selector(
    const inspect_images_options &options) -> bool;
[[nodiscard]] auto drawing_image_matches_filters(
    const featherdoc::drawing_image_info &image,
    const inspect_images_options &options) -> bool;
[[nodiscard]] auto filter_drawing_images(
    const std::vector<featherdoc::drawing_image_info> &images,
    const inspect_images_options &options)
    -> std::vector<featherdoc::drawing_image_info>;
[[nodiscard]] auto describe_inspect_image_filters(
    const inspect_images_options &options) -> std::string;

void write_json_inspect_image_filters(std::ostream &stream,
                                      const inspect_images_options &options);
void print_inspect_image_filters(std::ostream &stream,
                                 const inspect_images_options &options);

[[nodiscard]] auto resolve_selected_drawing_image(
    const std::vector<featherdoc::drawing_image_info> &images,
    const inspect_images_options &options, std::string_view part_entry_name,
    featherdoc::drawing_image_info &selected_image,
    featherdoc::document_error_info &error_info) -> bool;

} // namespace featherdoc_cli
