namespace {

using path_type = std::filesystem::path;

auto has_image_filters(const inspect_images_options &options) -> bool {
    return options.relationship_id.has_value() ||
           options.image_entry_name.has_value();
}

auto parse_image_options(const std::vector<std::string_view> &arguments,
                         std::size_t start_index, inspect_images_options &options,
                         std::optional<path_type> *output_path,
                         std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

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

        if (argument == "--image") {
            if (options.image_index.has_value()) {
                error_message = "duplicate --image option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --image";
                return false;
            }

            std::size_t image_index = 0U;
            if (!parse_index(arguments[index + 1U], image_index)) {
                error_message = "invalid image index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.image_index = image_index;
            ++index;
            continue;
        }

        if (argument == "--relationship-id") {
            if (options.relationship_id.has_value()) {
                error_message = "duplicate --relationship-id option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --relationship-id";
                return false;
            }

            const auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--relationship-id must not be empty";
                return false;
            }

            options.relationship_id = value;
            ++index;
            continue;
        }

        if (argument == "--image-entry-name") {
            if (options.image_entry_name.has_value()) {
                error_message = "duplicate --image-entry-name option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --image-entry-name";
                return false;
            }

            const auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--image-entry-name must not be empty";
                return false;
            }

            options.image_entry_name = value;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (output_path == nullptr) {
                error_message = "unknown option: " + std::string(argument);
                return false;
            }
            if (output_path->has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            *output_path = path_type(std::string(arguments[index + 1U]));
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

    return true;
}

} // namespace
