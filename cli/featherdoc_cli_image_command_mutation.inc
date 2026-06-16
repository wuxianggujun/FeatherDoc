[[nodiscard]] auto run_replace_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "replace-image expects an input path and an image path",
                          json_output);
        return 2;
    }

    replace_image_options options;
    std::string error_message;
    if (!parse_replace_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "mutate")) {
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

    const auto replacement_path = path_type(std::string(arguments[2]));
    if (!selected.part.replace_drawing_image(target_image.index, replacement_path)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    const auto updated_image_it = std::find_if(
        updated_images.begin(), updated_images.end(),
        [&target_image](const featherdoc::drawing_image_info &image) {
            return image.index == target_image.index;
        });
    if (updated_image_it == updated_images.end()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "updated drawing image index " +
                            std::to_string(target_image.index) +
                            " was not found in " +
                            std::string(selected.part.entry_name());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "updated drawing image not found", error_info,
                                 options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &updated_image_it, &replacement_path,
             &options](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                stream << ",\"replacement_path\":";
                write_json_string(stream, replacement_path.string());
                write_json_inspect_image_filters(stream, options);
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, *updated_image_it);
            });
    } else {
        print_replace_image_result(selected, *updated_image_it, replacement_path,
                                   options.output_path);
    }

    return 0;
}

[[nodiscard]] auto run_remove_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "remove-image expects an input path and image selectors",
                          json_output);
        return 2;
    }

    remove_image_options options;
    std::string error_message;
    if (!parse_remove_image_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "mutate")) {
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

    if (!selected.part.remove_drawing_image(target_image.index)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &target_image, &options](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                write_json_inspect_image_filters(stream, options);
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, target_image);
            });
    } else {
        print_remove_image_result(selected, target_image, options.output_path);
    }

    return 0;
}
