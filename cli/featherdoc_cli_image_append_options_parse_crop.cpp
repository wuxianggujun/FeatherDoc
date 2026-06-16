#include "featherdoc_cli_image_append_options_parse_support.hpp"

namespace featherdoc_cli {

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
