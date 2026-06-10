[[nodiscard]] auto load_image_part(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const inspect_images_options &options, featherdoc::Document &doc,
    selected_template_part &selected, std::string &error_message,
    std::string_view operation) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, operation, error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

[[nodiscard]] auto load_mutable_image_part(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const append_image_options &options, featherdoc::Document &doc,
    selected_template_part &selected, std::string &error_message) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    if (!select_mutable_template_part(doc, options.part, options.part_index,
                                      options.section_index,
                                      options.reference_kind, selected,
                                      error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}
