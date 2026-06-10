void write_json_selected_image_part(std::ostream &stream,
                                    const selected_template_part &selected) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream, featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
}

void print_selected_image_part(const selected_template_part &selected) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
}

void inspect_images(const selected_template_part &selected,
                    const std::vector<featherdoc::drawing_image_info> &images,
                    const inspect_images_options &options, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"count\":" << images.size() << ",\"images\":[";
        for (std::size_t index = 0; index < images.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_drawing_image_summary(std::cout, images[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_image_part(selected);
    print_inspect_image_filters(std::cout, options);
    std::cout << "images: " << images.size() << '\n';
    for (std::size_t index = 0; index < images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, images[index]);
        std::cout << '\n';
    }
}

void inspect_image(const selected_template_part &selected,
                   const featherdoc::drawing_image_info &image,
                   const inspect_images_options &options, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, image);
        std::cout << "}\n";
        return;
    }

    print_selected_image_part(selected);
    print_inspect_image_filters(std::cout, options);
    std::cout << "image_index: " << image.index << '\n';
    std::cout << "placement: " << drawing_image_placement_name(image.placement)
              << '\n';
    std::cout << "relationship_id: " << image.relationship_id << '\n';
    std::cout << "image_entry_name: " << image.entry_name << '\n';
    std::cout << "display_name: " << image.display_name << '\n';
    std::cout << "content_type: " << image.content_type << '\n';
    std::cout << "width_px: " << image.width_px << '\n';
    std::cout << "height_px: " << image.height_px << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (!image.floating_options.has_value()) {
        return;
    }

    const auto &floating_options = *image.floating_options;
    std::cout << "horizontal_reference: "
              << floating_image_horizontal_reference_name(
                     floating_options.horizontal_reference)
              << '\n';
    std::cout << "horizontal_offset_px: " << floating_options.horizontal_offset_px
              << '\n';
    std::cout << "vertical_reference: "
              << floating_image_vertical_reference_name(
                     floating_options.vertical_reference)
              << '\n';
    std::cout << "vertical_offset_px: " << floating_options.vertical_offset_px
              << '\n';
    std::cout << "behind_text: " << yes_no(floating_options.behind_text) << '\n';
    std::cout << "allow_overlap: " << yes_no(floating_options.allow_overlap)
              << '\n';
    std::cout << "z_order: " << floating_options.z_order << '\n';
    std::cout << "wrap_mode: "
              << floating_image_wrap_mode_name(floating_options.wrap_mode)
              << '\n';
    std::cout << "wrap_distance_left_px: "
              << floating_options.wrap_distance_left_px << '\n';
    std::cout << "wrap_distance_right_px: "
              << floating_options.wrap_distance_right_px << '\n';
    std::cout << "wrap_distance_top_px: "
              << floating_options.wrap_distance_top_px << '\n';
    std::cout << "wrap_distance_bottom_px: "
              << floating_options.wrap_distance_bottom_px << '\n';
    if (!floating_options.crop.has_value()) {
        std::cout << "crop: none\n";
        return;
    }

    std::cout << "crop_left_per_mille: " << floating_options.crop->left_per_mille
              << '\n';
    std::cout << "crop_top_per_mille: " << floating_options.crop->top_per_mille
              << '\n';
    std::cout << "crop_right_per_mille: "
              << floating_options.crop->right_per_mille << '\n';
    std::cout << "crop_bottom_per_mille: "
              << floating_options.crop->bottom_per_mille << '\n';
}

void print_extract_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &output_path) {
    print_selected_image_part(selected);
    std::cout << "output_path: " << output_path.string() << '\n';
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_replace_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &replacement_path,
                                const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    std::cout << "replacement_path: " << replacement_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_remove_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed_image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_append_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const path_type &image_path,
                               const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    std::cout << "image_path: " << image_path.string() << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}
