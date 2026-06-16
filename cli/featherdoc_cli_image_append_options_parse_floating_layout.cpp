#include "featherdoc_cli_image_append_options_parse_support.hpp"
#include "featherdoc_cli_image_append_options_parse_floating_support.hpp"

#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

auto parse_append_image_floating_layout_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--horizontal-reference") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --horizontal-reference",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        if (!parse_floating_image_horizontal_reference(
                *value, options.floating_options.horizontal_reference)) {
            error_message =
                "invalid horizontal reference: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--horizontal-offset") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --horizontal-offset",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::int32_t horizontal_offset_px = 0;
        if (!parse_int32(*value, horizontal_offset_px)) {
            error_message = "invalid horizontal offset: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating_options.horizontal_offset_px = horizontal_offset_px;
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--vertical-reference") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --vertical-reference",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        if (!parse_floating_image_vertical_reference(
                *value, options.floating_options.vertical_reference)) {
            error_message = "invalid vertical reference: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--vertical-offset") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --vertical-offset",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::int32_t vertical_offset_px = 0;
        if (!parse_int32(*value, vertical_offset_px)) {
            error_message = "invalid vertical offset: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating_options.vertical_offset_px = vertical_offset_px;
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--behind-text") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --behind-text",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        bool behind_text = false;
        if (!parse_bool(*value, behind_text)) {
            error_message = "invalid behind-text value: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating_options.behind_text = behind_text;
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--allow-overlap") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --allow-overlap",
            error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        bool allow_overlap = false;
        if (!parse_bool(*value, allow_overlap)) {
            error_message =
                "invalid allow-overlap value: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating_options.allow_overlap = allow_overlap;
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--z-order") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --z-order", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::uint32_t z_order = 0U;
        if (!parse_image_append_floating_uint32_option_value(
                *value, "invalid z-order value: ", z_order, error_message)) {
            return option_parse_result::error;
        }

        options.floating_options.z_order = z_order;
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--wrap-mode") {
        const auto value = read_image_append_floating_option_value(
            arguments, index, "missing value after --wrap-mode", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        if (!parse_floating_image_wrap_mode(*value,
                                            options.floating_options.wrap_mode)) {
            error_message = "invalid wrap mode: " + std::string{*value};
            return option_parse_result::error;
        }

        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--wrap-distance-left" ||
        argument == "--wrap-distance-right" ||
        argument == "--wrap-distance-top" ||
        argument == "--wrap-distance-bottom") {
        const auto missing_message =
            argument == "--wrap-distance-left"     ? "missing value after --wrap-distance-left"
            : argument == "--wrap-distance-right" ? "missing value after --wrap-distance-right"
            : argument == "--wrap-distance-top"   ? "missing value after --wrap-distance-top"
                                                   : "missing value after --wrap-distance-bottom";
        const auto invalid_message =
            argument == "--wrap-distance-left"     ? "invalid wrap distance left: "
            : argument == "--wrap-distance-right" ? "invalid wrap distance right: "
            : argument == "--wrap-distance-top"   ? "invalid wrap distance top: "
                                                   : "invalid wrap distance bottom: ";
        const auto value = read_image_append_floating_option_value(
            arguments, index, missing_message, error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::uint32_t distance_px = 0U;
        if (!parse_image_append_floating_uint32_option_value(
                *value, invalid_message, distance_px, error_message)) {
            return option_parse_result::error;
        }

        if (argument == "--wrap-distance-left") {
            options.floating_options.wrap_distance_left_px = distance_px;
        } else if (argument == "--wrap-distance-right") {
            options.floating_options.wrap_distance_right_px = distance_px;
        } else if (argument == "--wrap-distance-top") {
            options.floating_options.wrap_distance_top_px = distance_px;
        } else {
            options.floating_options.wrap_distance_bottom_px = distance_px;
        }
        options.floating = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

} // namespace featherdoc_cli
