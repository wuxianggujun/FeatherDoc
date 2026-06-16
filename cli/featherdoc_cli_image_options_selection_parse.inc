auto parse_inspect_images_options(const std::vector<std::string_view> &arguments,
                                  std::size_t start_index,
                                  inspect_images_options &options,
                                  std::string &error_message) -> bool {
    if (!parse_image_options(arguments, start_index, options, nullptr,
                             error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_replace_image_options(const std::vector<std::string_view> &arguments,
                                 std::size_t start_index,
                                 replace_image_options &options,
                                 std::string &error_message) -> bool {
    if (!parse_image_options(arguments, start_index,
                             static_cast<inspect_images_options &>(options),
                             &options.output_path, error_message)) {
        return false;
    }

    if (!options.image_index.has_value() && !has_image_filters(options)) {
        error_message =
            "replace-image requires --image <index>, --relationship-id <id>, or "
            "--image-entry-name <path>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

auto parse_extract_image_options(const std::vector<std::string_view> &arguments,
                                 std::size_t start_index,
                                 extract_image_options &options,
                                 std::string &error_message) -> bool {
    if (!parse_image_options(arguments, start_index,
                             static_cast<inspect_images_options &>(options),
                             nullptr, error_message)) {
        return false;
    }

    if (!options.image_index.has_value() && !has_image_filters(options)) {
        error_message =
            "extract-image requires --image <index>, --relationship-id <id>, or "
            "--image-entry-name <path>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_remove_image_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                remove_image_options &options,
                                std::string &error_message) -> bool {
    if (!parse_image_options(arguments, start_index,
                             static_cast<inspect_images_options &>(options),
                             &options.output_path, error_message)) {
        return false;
    }

    if (!options.image_index.has_value() && !has_image_filters(options)) {
        error_message =
            "remove-image requires --image <index>, --relationship-id <id>, or "
            "--image-entry-name <path>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}
