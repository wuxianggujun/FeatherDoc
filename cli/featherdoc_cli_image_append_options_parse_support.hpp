#pragma once

#include "featherdoc_cli_image_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct append_image_crop_options {
    std::optional<std::uint32_t> left_per_mille;
    std::optional<std::uint32_t> top_per_mille;
    std::optional<std::uint32_t> right_per_mille;
    std::optional<std::uint32_t> bottom_per_mille;
};

[[nodiscard]] auto parse_append_image_selection_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto parse_append_image_size_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto parse_append_image_floating_flag_option(
    std::string_view argument, bool &floating_flag_seen,
    append_image_options &options, std::string &error_message)
    -> option_parse_result;
[[nodiscard]] auto parse_append_image_floating_layout_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto parse_append_image_crop_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    append_image_crop_options &crop_options, std::string &error_message)
    -> option_parse_result;
[[nodiscard]] auto parse_append_image_output_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result;
[[nodiscard]] auto validate_append_image_size_options(
    const append_image_options &options, std::string &error_message) -> bool;
[[nodiscard]] auto apply_append_image_crop_options(
    const append_image_crop_options &crop_options, append_image_options &options,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
