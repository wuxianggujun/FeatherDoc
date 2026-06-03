#pragma once

#include <featherdoc.hpp>

#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_page_orientation(
    std::string_view text, featherdoc::page_orientation &orientation) -> bool;
[[nodiscard]] auto parse_list_kind(std::string_view text,
                                   featherdoc::list_kind &kind) -> bool;
[[nodiscard]] auto parse_reference_kind(
    std::string_view text, featherdoc::section_reference_kind &reference_kind)
    -> bool;
[[nodiscard]] auto parse_floating_image_horizontal_reference(
    std::string_view text,
    featherdoc::floating_image_horizontal_reference &reference) -> bool;
[[nodiscard]] auto parse_floating_image_vertical_reference(
    std::string_view text,
    featherdoc::floating_image_vertical_reference &reference) -> bool;
[[nodiscard]] auto parse_floating_image_wrap_mode(
    std::string_view text, featherdoc::floating_image_wrap_mode &mode) -> bool;

} // namespace featherdoc_cli
