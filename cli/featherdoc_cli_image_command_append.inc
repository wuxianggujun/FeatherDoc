[[nodiscard]] auto run_append_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "append-image expects an input path and an image path",
                          json_output);
        return 2;
    }

    append_image_options options;
    std::string error_message;
    if (!parse_append_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_mutable_image_part(command, arguments, options, doc, selected,
                                 error_message)) {
        return 1;
    }

    const auto existing_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    const auto image_path = path_type(std::string(arguments[2]));
    bool success = false;
    if (options.floating) {
        if (options.width_px.has_value()) {
            success = selected.part.append_floating_image(
                image_path, *options.width_px, *options.height_px,
                options.floating_options);
        } else {
            success =
                selected.part.append_floating_image(image_path, options.floating_options);
        }
    } else if (options.width_px.has_value()) {
        success = selected.part.append_image(image_path, *options.width_px,
                                             *options.height_px);
    } else {
        success = selected.part.append_image(image_path);
    }

    if (!success) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    if (updated_images.size() <= existing_images.size()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "appended drawing image was not found in " +
                            std::string(selected.part.entry_name());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "appended drawing image not found", error_info,
                                 options.json_output);
        return 1;
    }

    const auto &appended_image = updated_images.back();
    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &appended_image, &image_path](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                stream << ",\"image_path\":";
                write_json_string(stream, image_path.string());
                stream << ",\"floating\":"
                       << json_bool(appended_image.floating_options.has_value());
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, appended_image);
            });
    } else {
        print_append_image_result(selected, appended_image, image_path,
                                  options.output_path);
    }

    return 0;
}
