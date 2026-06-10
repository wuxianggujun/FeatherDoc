auto is_image_command(std::string_view command) -> bool {
    return command == "inspect-images" || command == "extract-image" ||
           command == "replace-image" || command == "remove-image" ||
           command == "append-image";
}

auto run_image_command(std::string_view command,
                       const std::vector<std::string_view> &arguments,
                       featherdoc::Document &doc) -> int {
    if (command == "inspect-images") {
        return run_inspect_images_command(command, arguments, doc);
    }
    if (command == "extract-image") {
        return run_extract_image_command(command, arguments, doc);
    }
    if (command == "replace-image") {
        return run_replace_image_command(command, arguments, doc);
    }
    if (command == "remove-image") {
        return run_remove_image_command(command, arguments, doc);
    }
    if (command == "append-image") {
        return run_append_image_command(command, arguments, doc);
    }

    return 2;
}
