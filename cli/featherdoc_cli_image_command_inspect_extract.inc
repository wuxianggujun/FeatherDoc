[[nodiscard]] auto run_inspect_images_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-images expects an input path",
                          json_output);
        return 2;
    }

    inspect_images_options options;
    std::string error_message;
    if (!parse_inspect_images_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "inspect")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.image_index.has_value()) {
        const auto image_it = std::find_if(
            images.begin(), images.end(),
            [&options](const featherdoc::drawing_image_info &image) {
                return image.index == *options.image_index;
            });
        if (image_it == images.end() ||
            !drawing_image_matches_filters(*image_it, options)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail = "drawing image index " +
                                std::to_string(*options.image_index) +
                                " was not found in " +
                                std::string(selected.part.entry_name());
            if (has_inspect_image_filters(options)) {
                error_info.detail +=
                    " for " + describe_inspect_image_filters(options);
            }
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect", "drawing image not found",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_image(selected, *image_it, options, options.json_output);
        return 0;
    }

    inspect_images(selected, filter_drawing_images(images, options), options,
                   options.json_output);
    return 0;
}

[[nodiscard]] auto run_extract_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "extract-image expects an input path and an output path",
                          json_output);
        return 2;
    }

    extract_image_options options;
    std::string error_message;
    if (!parse_extract_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "inspect")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    featherdoc::drawing_image_info target_image{};
    featherdoc::document_error_info selection_error{};
    if (!resolve_selected_drawing_image(images, options, selected.part.entry_name(),
                                        target_image, selection_error)) {
        report_operation_failure(command, "input", "drawing image not found",
                                 selection_error, options.json_output);
        return 1;
    }

    const auto output_path = path_type(std::string(arguments[2]));
    if (!selected.part.extract_drawing_image(target_image.index, output_path)) {
        report_document_error(command, "extract", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command);
        std::cout << ",\"ok\":true";
        write_json_selected_image_part(std::cout, selected);
        std::cout << ",\"output_path\":";
        write_json_string(std::cout, output_path.string());
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, target_image);
        std::cout << "}\n";
    } else {
        print_extract_image_result(selected, target_image, output_path);
    }

    return 0;
}
