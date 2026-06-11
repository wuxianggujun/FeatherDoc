#include "featherdoc_cli_image_append_options_parse_support.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <filesystem>

namespace featherdoc_cli {
namespace {

auto read_required_option_value(
    const std::vector<std::string_view> &arguments, std::size_t index,
    std::string_view missing_message, std::string &error_message)
    -> std::optional<std::string_view> {
    if (index + 1U >= arguments.size()) {
        error_message = std::string{missing_message};
        return std::nullopt;
    }
    return arguments[index + 1U];
}

auto parse_uint32_option_value(std::string_view value,
                               std::string_view invalid_message_prefix,
                               std::uint32_t &parsed_value,
                               std::string &error_message) -> bool {
    if (parse_uint32(value, parsed_value)) {
        return true;
    }

    error_message = std::string{invalid_message_prefix} + std::string{value};
    return false;
}

} // namespace

auto parse_append_image_selection_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--part") {
        if (options.has_part) {
            error_message = "duplicate --part option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing value after --part", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        if (!parse_validation_part(*value, options.part)) {
            error_message = "invalid template part: " + std::string{*value};
            return option_parse_result::error;
        }

        options.has_part = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--index") {
        if (options.part_index.has_value()) {
            error_message = "duplicate --index option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing value after --index", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::size_t part_index = 0U;
        if (!parse_index(*value, part_index)) {
            error_message = "invalid part index: " + std::string{*value};
            return option_parse_result::error;
        }

        options.part_index = part_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--section") {
        if (options.section_index.has_value()) {
            error_message = "duplicate --section option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing value after --section", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::size_t section_index = 0U;
        if (!parse_index(*value, section_index)) {
            error_message = "invalid section index: " + std::string{*value};
            return option_parse_result::error;
        }

        options.section_index = section_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--kind") {
        const auto value = read_required_option_value(
            arguments, index, "missing value after --kind", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        if (!parse_reference_kind(*value, options.reference_kind)) {
            error_message = "invalid reference kind: " + std::string{*value};
            return option_parse_result::error;
        }

        options.has_kind = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto parse_append_image_size_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--width") {
        if (options.width_px.has_value()) {
            error_message = "duplicate --width option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing value after --width", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::uint32_t width_px = 0U;
        if (!parse_uint32_option_value(*value, "invalid width: ", width_px,
                                       error_message)) {
            return option_parse_result::error;
        }

        options.width_px = width_px;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--height") {
        if (options.height_px.has_value()) {
            error_message = "duplicate --height option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing value after --height", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        std::uint32_t height_px = 0U;
        if (!parse_uint32_option_value(*value, "invalid height: ", height_px,
                                       error_message)) {
            return option_parse_result::error;
        }

        options.height_px = height_px;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto parse_append_image_crop_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    append_image_crop_options &crop_options, std::string &error_message)
    -> option_parse_result {
    auto *target = static_cast<std::optional<std::uint32_t> *>(nullptr);
    auto missing_message = std::string_view{};
    auto invalid_message = std::string_view{};
    if (argument == "--crop-left") {
        target = &crop_options.left_per_mille;
        missing_message = "missing value after --crop-left";
        invalid_message = "invalid crop left: ";
    } else if (argument == "--crop-top") {
        target = &crop_options.top_per_mille;
        missing_message = "missing value after --crop-top";
        invalid_message = "invalid crop top: ";
    } else if (argument == "--crop-right") {
        target = &crop_options.right_per_mille;
        missing_message = "missing value after --crop-right";
        invalid_message = "invalid crop right: ";
    } else if (argument == "--crop-bottom") {
        target = &crop_options.bottom_per_mille;
        missing_message = "missing value after --crop-bottom";
        invalid_message = "invalid crop bottom: ";
    } else {
        return option_parse_result::not_matched;
    }

    if (target->has_value()) {
        error_message = "duplicate " + std::string{argument} + " option";
        return option_parse_result::error;
    }
    const auto value = read_required_option_value(arguments, index,
                                                 missing_message, error_message);
    if (!value.has_value()) {
        return option_parse_result::error;
    }

    std::uint32_t crop_per_mille = 0U;
    if (!parse_uint32_option_value(*value, invalid_message, crop_per_mille,
                                   error_message)) {
        return option_parse_result::error;
    }

    *target = crop_per_mille;
    options.floating = true;
    ++index;
    return option_parse_result::matched;
}

auto parse_append_image_output_option(
    std::string_view argument, const std::vector<std::string_view> &arguments,
    std::size_t &index, append_image_options &options,
    std::string &error_message) -> option_parse_result {
    if (argument == "--output") {
        if (options.output_path.has_value()) {
            error_message = "duplicate --output option";
            return option_parse_result::error;
        }
        const auto value = read_required_option_value(
            arguments, index, "missing path after --output", error_message);
        if (!value.has_value()) {
            return option_parse_result::error;
        }

        options.output_path = std::filesystem::path(std::string{*value});
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--json") {
        options.json_output = true;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto validate_append_image_size_options(
    const append_image_options &options, std::string &error_message) -> bool {
    if (options.width_px.has_value() == options.height_px.has_value()) {
        return true;
    }

    error_message =
        "append-image requires both --width <px> and --height <px> when scaling";
    return false;
}

auto apply_append_image_crop_options(
    const append_image_crop_options &crop_options, append_image_options &options,
    std::string &error_message) -> bool {
    const bool has_any_crop = crop_options.left_per_mille.has_value() ||
                              crop_options.top_per_mille.has_value() ||
                              crop_options.right_per_mille.has_value() ||
                              crop_options.bottom_per_mille.has_value();
    const bool has_all_crop = crop_options.left_per_mille.has_value() &&
                              crop_options.top_per_mille.has_value() &&
                              crop_options.right_per_mille.has_value() &&
                              crop_options.bottom_per_mille.has_value();
    if (has_any_crop && !has_all_crop) {
        error_message =
            "append-image requires --crop-left/--crop-top/--crop-right/--crop-bottom together";
        return false;
    }

    if (has_all_crop) {
        options.floating_options.crop = featherdoc::floating_image_crop{
            *crop_options.left_per_mille, *crop_options.top_per_mille,
            *crop_options.right_per_mille, *crop_options.bottom_per_mille};
    }

    return true;
}

} // namespace featherdoc_cli
