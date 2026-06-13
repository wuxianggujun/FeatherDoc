#include "featherdoc_cli_image_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <algorithm>
#include <ostream>
#include <string>
#include <system_error>

namespace featherdoc_cli {
namespace {

void write_json_floating_image_crop(
    std::ostream &stream, const featherdoc::floating_image_crop &crop) {
    stream << "{\"left_per_mille\":" << crop.left_per_mille
           << ",\"top_per_mille\":" << crop.top_per_mille
           << ",\"right_per_mille\":" << crop.right_per_mille
           << ",\"bottom_per_mille\":" << crop.bottom_per_mille << '}';
}

void write_json_floating_image_options(
    std::ostream &stream, const featherdoc::floating_image_options &options) {
    stream << "{\"horizontal_reference\":";
    write_json_string(stream,
                      floating_image_horizontal_reference_name(
                          options.horizontal_reference));
    stream << ",\"horizontal_offset_px\":" << options.horizontal_offset_px
           << ",\"vertical_reference\":";
    write_json_string(stream,
                      floating_image_vertical_reference_name(
                          options.vertical_reference));
    stream << ",\"vertical_offset_px\":" << options.vertical_offset_px
           << ",\"behind_text\":" << json_bool(options.behind_text)
           << ",\"allow_overlap\":" << json_bool(options.allow_overlap)
           << ",\"z_order\":" << options.z_order
           << ",\"wrap_mode\":";
    write_json_string(stream, floating_image_wrap_mode_name(options.wrap_mode));
    stream << ",\"wrap_distance_left_px\":" << options.wrap_distance_left_px
           << ",\"wrap_distance_right_px\":" << options.wrap_distance_right_px
           << ",\"wrap_distance_top_px\":" << options.wrap_distance_top_px
           << ",\"wrap_distance_bottom_px\":" << options.wrap_distance_bottom_px
           << ",\"crop\":";
    if (options.crop.has_value()) {
        write_json_floating_image_crop(stream, *options.crop);
    } else {
        stream << "null";
    }
    stream << '}';
}

} // namespace

void write_json_drawing_image_summary(
    std::ostream &stream, const featherdoc::drawing_image_info &image) {
    stream << "{\"index\":" << image.index << ",\"placement\":";
    write_json_string(stream, drawing_image_placement_name(image.placement));
    stream << ",\"relationship_id\":";
    write_json_string(stream, image.relationship_id);
    stream << ",\"entry_name\":";
    write_json_string(stream, image.entry_name);
    stream << ",\"display_name\":";
    write_json_string(stream, image.display_name);
    stream << ",\"content_type\":";
    write_json_string(stream, image.content_type);
    stream << ",\"width_px\":" << image.width_px
           << ",\"height_px\":" << image.height_px
           << ",\"floating_options\":";
    if (image.floating_options.has_value()) {
        write_json_floating_image_options(stream, *image.floating_options);
    } else {
        stream << "null";
    }
    stream << '}';
}

void print_drawing_image_summary(std::ostream &stream,
                                 const featherdoc::drawing_image_info &image) {
    stream << "index=" << image.index
           << " placement=" << drawing_image_placement_name(image.placement)
           << " relationship_id=" << image.relationship_id
           << " entry_name=" << image.entry_name
           << " display_name=" << image.display_name
           << " content_type=" << image.content_type
           << " width_px=" << image.width_px
           << " height_px=" << image.height_px;
    if (!image.floating_options.has_value()) {
        stream << " floating=no";
        return;
    }

    const auto &floating_options = *image.floating_options;
    stream << " floating=yes"
           << " horizontal_reference="
           << floating_image_horizontal_reference_name(
                  floating_options.horizontal_reference)
           << " horizontal_offset_px=" << floating_options.horizontal_offset_px
           << " vertical_reference="
           << floating_image_vertical_reference_name(
                  floating_options.vertical_reference)
           << " vertical_offset_px=" << floating_options.vertical_offset_px
           << " behind_text=" << yes_no(floating_options.behind_text)
           << " allow_overlap=" << yes_no(floating_options.allow_overlap)
           << " z_order=" << floating_options.z_order
           << " wrap_mode="
           << floating_image_wrap_mode_name(floating_options.wrap_mode)
           << " wrap_distance_left_px="
           << floating_options.wrap_distance_left_px
           << " wrap_distance_right_px="
           << floating_options.wrap_distance_right_px
           << " wrap_distance_top_px=" << floating_options.wrap_distance_top_px
           << " wrap_distance_bottom_px="
           << floating_options.wrap_distance_bottom_px;
    if (floating_options.crop.has_value()) {
        stream << " crop=(" << floating_options.crop->left_per_mille << ','
               << floating_options.crop->top_per_mille << ','
               << floating_options.crop->right_per_mille << ','
               << floating_options.crop->bottom_per_mille << ')';
    } else {
        stream << " crop=none";
    }
}

auto has_inspect_image_filters(const inspect_images_options &options) -> bool {
    return options.relationship_id.has_value() ||
           options.image_entry_name.has_value();
}

auto has_drawing_image_selector(const inspect_images_options &options) -> bool {
    return options.image_index.has_value() || has_inspect_image_filters(options);
}

auto drawing_image_matches_filters(const featherdoc::drawing_image_info &image,
                                   const inspect_images_options &options) -> bool {
    if (options.relationship_id.has_value() &&
        image.relationship_id != *options.relationship_id) {
        return false;
    }
    if (options.image_entry_name.has_value() &&
        image.entry_name != *options.image_entry_name) {
        return false;
    }
    return true;
}

auto filter_drawing_images(
    const std::vector<featherdoc::drawing_image_info> &images,
    const inspect_images_options &options)
    -> std::vector<featherdoc::drawing_image_info> {
    if (!has_inspect_image_filters(options)) {
        return images;
    }

    std::vector<featherdoc::drawing_image_info> filtered_images;
    filtered_images.reserve(images.size());
    for (const auto &image : images) {
        if (drawing_image_matches_filters(image, options)) {
            filtered_images.push_back(image);
        }
    }
    return filtered_images;
}

auto describe_inspect_image_filters(const inspect_images_options &options)
    -> std::string {
    std::string description;
    if (options.relationship_id.has_value()) {
        description += "relationship_id '" + *options.relationship_id + "'";
    }
    if (options.image_entry_name.has_value()) {
        if (!description.empty()) {
            description += " and ";
        }
        description += "image_entry_name '" + *options.image_entry_name + "'";
    }
    return description;
}

void write_json_inspect_image_filters(std::ostream &stream,
                                      const inspect_images_options &options) {
    if (!has_inspect_image_filters(options)) {
        return;
    }

    stream << ",\"filters\":{";
    bool needs_separator = false;
    if (options.relationship_id.has_value()) {
        stream << "\"relationship_id\":";
        write_json_string(stream, *options.relationship_id);
        needs_separator = true;
    }
    if (options.image_entry_name.has_value()) {
        if (needs_separator) {
            stream << ',';
        }
        stream << "\"image_entry_name\":";
        write_json_string(stream, *options.image_entry_name);
    }
    stream << '}';
}

void print_inspect_image_filters(std::ostream &stream,
                                 const inspect_images_options &options) {
    if (options.relationship_id.has_value()) {
        stream << "relationship_id_filter: " << *options.relationship_id << '\n';
    }
    if (options.image_entry_name.has_value()) {
        stream << "image_entry_name_filter: " << *options.image_entry_name << '\n';
    }
}

auto resolve_selected_drawing_image(
    const std::vector<featherdoc::drawing_image_info> &images,
    const inspect_images_options &options, std::string_view part_entry_name,
    featherdoc::drawing_image_info &selected_image,
    featherdoc::document_error_info &error_info) -> bool {
    error_info.clear();

    if (options.image_index.has_value()) {
        const auto image_it = std::find_if(
            images.begin(), images.end(),
            [&options](const featherdoc::drawing_image_info &image) {
                return image.index == *options.image_index;
            });
        if (image_it == images.end() ||
            !drawing_image_matches_filters(*image_it, options)) {
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail = "drawing image index " +
                                std::to_string(*options.image_index) +
                                " was not found in " +
                                std::string(part_entry_name);
            if (has_inspect_image_filters(options)) {
                error_info.detail += " for " +
                                     describe_inspect_image_filters(options);
            }
            error_info.entry_name = std::string(part_entry_name);
            return false;
        }

        selected_image = *image_it;
        return true;
    }

    const auto filtered_images = filter_drawing_images(images, options);
    if (filtered_images.empty()) {
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "drawing image";
        if (has_inspect_image_filters(options)) {
            error_info.detail += " matching " +
                                 describe_inspect_image_filters(options);
        }
        error_info.detail += " was not found in " + std::string(part_entry_name);
        error_info.entry_name = std::string(part_entry_name);
        return false;
    }

    if (filtered_images.size() > 1U) {
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "drawing image selection matched " +
                            std::to_string(filtered_images.size()) +
                            " images in " + std::string(part_entry_name) +
                            "; refine with --image, --relationship-id, or "
                            "--image-entry-name";
        error_info.entry_name = std::string(part_entry_name);
        return false;
    }

    selected_image = filtered_images.front();
    return true;
}

} // namespace featherdoc_cli
