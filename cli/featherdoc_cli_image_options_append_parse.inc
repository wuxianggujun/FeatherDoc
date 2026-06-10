auto parse_append_image_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                append_image_options &options,
                                std::string &error_message) -> bool {
    std::optional<std::uint32_t> crop_left_per_mille;
    std::optional<std::uint32_t> crop_top_per_mille;
    std::optional<std::uint32_t> crop_right_per_mille;
    std::optional<std::uint32_t> crop_bottom_per_mille;
    bool floating_flag_seen = false;

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (options.has_part) {
                error_message = "duplicate --part option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_part = true;
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U],
                                      options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--width") {
            if (options.width_px.has_value()) {
                error_message = "duplicate --width option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --width";
                return false;
            }

            std::uint32_t width_px = 0U;
            if (!parse_uint32(arguments[index + 1U], width_px)) {
                error_message = "invalid width: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.width_px = width_px;
            ++index;
            continue;
        }

        if (argument == "--height") {
            if (options.height_px.has_value()) {
                error_message = "duplicate --height option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --height";
                return false;
            }

            std::uint32_t height_px = 0U;
            if (!parse_uint32(arguments[index + 1U], height_px)) {
                error_message = "invalid height: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.height_px = height_px;
            ++index;
            continue;
        }

        if (argument == "--floating") {
            if (floating_flag_seen) {
                error_message = "duplicate --floating option";
                return false;
            }

            options.floating = true;
            floating_flag_seen = true;
            continue;
        }

        if (argument == "--horizontal-reference") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --horizontal-reference";
                return false;
            }

            if (!parse_floating_image_horizontal_reference(
                    arguments[index + 1U],
                    options.floating_options.horizontal_reference)) {
                error_message = "invalid horizontal reference: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--horizontal-offset") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --horizontal-offset";
                return false;
            }

            std::int32_t horizontal_offset_px = 0;
            if (!parse_int32(arguments[index + 1U], horizontal_offset_px)) {
                error_message = "invalid horizontal offset: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.horizontal_offset_px = horizontal_offset_px;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--vertical-reference") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --vertical-reference";
                return false;
            }

            if (!parse_floating_image_vertical_reference(
                    arguments[index + 1U],
                    options.floating_options.vertical_reference)) {
                error_message = "invalid vertical reference: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--vertical-offset") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --vertical-offset";
                return false;
            }

            std::int32_t vertical_offset_px = 0;
            if (!parse_int32(arguments[index + 1U], vertical_offset_px)) {
                error_message = "invalid vertical offset: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.vertical_offset_px = vertical_offset_px;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--behind-text") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --behind-text";
                return false;
            }

            bool behind_text = false;
            if (!parse_bool(arguments[index + 1U], behind_text)) {
                error_message = "invalid behind-text value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.behind_text = behind_text;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--allow-overlap") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --allow-overlap";
                return false;
            }

            bool allow_overlap = false;
            if (!parse_bool(arguments[index + 1U], allow_overlap)) {
                error_message = "invalid allow-overlap value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.allow_overlap = allow_overlap;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--z-order") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --z-order";
                return false;
            }

            std::uint32_t z_order = 0U;
            if (!parse_uint32(arguments[index + 1U], z_order)) {
                error_message = "invalid z-order value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.z_order = z_order;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--wrap-mode") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --wrap-mode";
                return false;
            }

            if (!parse_floating_image_wrap_mode(arguments[index + 1U],
                                                options.floating_options.wrap_mode)) {
                error_message = "invalid wrap mode: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--wrap-distance-left") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --wrap-distance-left";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid wrap distance left: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.wrap_distance_left_px = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--wrap-distance-right") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --wrap-distance-right";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid wrap distance right: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.wrap_distance_right_px = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--wrap-distance-top") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --wrap-distance-top";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid wrap distance top: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.wrap_distance_top_px = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--wrap-distance-bottom") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --wrap-distance-bottom";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid wrap distance bottom: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.floating_options.wrap_distance_bottom_px = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--crop-left") {
            if (crop_left_per_mille.has_value()) {
                error_message = "duplicate --crop-left option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --crop-left";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid crop left: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            crop_left_per_mille = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--crop-top") {
            if (crop_top_per_mille.has_value()) {
                error_message = "duplicate --crop-top option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --crop-top";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid crop top: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            crop_top_per_mille = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--crop-right") {
            if (crop_right_per_mille.has_value()) {
                error_message = "duplicate --crop-right option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --crop-right";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid crop right: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            crop_right_per_mille = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--crop-bottom") {
            if (crop_bottom_per_mille.has_value()) {
                error_message = "duplicate --crop-bottom option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --crop-bottom";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid crop bottom: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            crop_bottom_per_mille = value;
            options.floating = true;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.width_px.has_value() != options.height_px.has_value()) {
        error_message =
            "append-image requires both --width <px> and --height <px> when scaling";
        return false;
    }

    const bool has_any_crop = crop_left_per_mille.has_value() ||
                              crop_top_per_mille.has_value() ||
                              crop_right_per_mille.has_value() ||
                              crop_bottom_per_mille.has_value();
    const bool has_all_crop = crop_left_per_mille.has_value() &&
                              crop_top_per_mille.has_value() &&
                              crop_right_per_mille.has_value() &&
                              crop_bottom_per_mille.has_value();
    if (has_any_crop && !has_all_crop) {
        error_message =
            "append-image requires --crop-left/--crop-top/--crop-right/--crop-bottom together";
        return false;
    }

    if (has_all_crop) {
        options.floating_options.crop = featherdoc::floating_image_crop{
            *crop_left_per_mille, *crop_top_per_mille, *crop_right_per_mille,
            *crop_bottom_per_mille};
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}
