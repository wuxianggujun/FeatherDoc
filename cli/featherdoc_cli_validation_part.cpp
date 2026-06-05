#include "featherdoc_cli_validation_part.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_validation_part(std::string_view text,
                           validation_part_family &part) -> bool {
    if (text == "body") {
        part = validation_part_family::body;
        return true;
    }
    if (text == "header") {
        part = validation_part_family::header;
        return true;
    }
    if (text == "footer") {
        part = validation_part_family::footer;
        return true;
    }
    if (text == "section-header") {
        part = validation_part_family::section_header;
        return true;
    }
    if (text == "section-footer") {
        part = validation_part_family::section_footer;
        return true;
    }

    return false;
}

auto validation_part_name(validation_part_family family) -> const char * {
    switch (family) {
    case validation_part_family::body:
        return "body";
    case validation_part_family::header:
        return "header";
    case validation_part_family::footer:
        return "footer";
    case validation_part_family::section_header:
        return "section-header";
    case validation_part_family::section_footer:
        return "section-footer";
    }

    return "unknown";
}

auto validate_template_part_selection(
    validation_part_family part, const std::optional<std::size_t> &part_index,
    const std::optional<std::size_t> &section_index, bool has_kind,
    std::string_view operation_label, std::string &error_message) -> bool {
    switch (part) {
    case validation_part_family::body:
        if (part_index.has_value()) {
            error_message = "--index is only supported by header/footer " +
                            std::string(operation_label);
            return false;
        }
        if (section_index.has_value()) {
            error_message =
                "--section is only supported by section-header/section-footer";
            return false;
        }
        if (has_kind) {
            error_message =
                "--kind is only supported by section-header/section-footer";
            return false;
        }
        break;
    case validation_part_family::header:
    case validation_part_family::footer:
        if (section_index.has_value()) {
            error_message =
                "--section is only supported by section-header/section-footer";
            return false;
        }
        if (has_kind) {
            error_message =
                "--kind is only supported by section-header/section-footer";
            return false;
        }
        break;
    case validation_part_family::section_header:
    case validation_part_family::section_footer:
        if (!section_index.has_value()) {
            error_message = "section-header/section-footer " +
                            std::string(operation_label) +
                            " requires --section <index>";
            return false;
        }
        if (part_index.has_value()) {
            error_message = "--index is only supported by header/footer " +
                            std::string(operation_label);
            return false;
        }
        break;
    }

    return true;
}

} // namespace featherdoc_cli
