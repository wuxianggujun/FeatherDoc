#include "featherdoc_cli_image_options_parse.hpp"

#include "featherdoc_cli_image_append_options_parse_support.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto append_image_option_result_ok(option_parse_result result) -> bool {
    return result != option_parse_result::error;
}

auto append_image_option_matched(option_parse_result result) -> bool {
    return result == option_parse_result::matched;
}

} // namespace

auto parse_append_image_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                append_image_options &options,
                                std::string &error_message) -> bool {
    auto crop_options = append_image_crop_options{};
    bool floating_flag_seen = false;

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];

        auto result = parse_append_image_selection_option(
            argument, arguments, index, options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        result = parse_append_image_size_option(argument, arguments, index,
                                                options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        result = parse_append_image_floating_flag_option(
            argument, floating_flag_seen, options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        result = parse_append_image_floating_layout_option(
            argument, arguments, index, options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        result = parse_append_image_crop_option(
            argument, arguments, index, options, crop_options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        result = parse_append_image_output_option(argument, arguments, index,
                                                  options, error_message);
        if (!append_image_option_result_ok(result)) {
            return false;
        }
        if (append_image_option_matched(result)) {
            continue;
        }

        error_message = "unknown option: " + std::string{argument};
        return false;
    }

    return validate_append_image_size_options(options, error_message) &&
           apply_append_image_crop_options(crop_options, options, error_message) &&
           validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

} // namespace featherdoc_cli
