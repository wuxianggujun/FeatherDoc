#include "featherdoc_cli_template_part_selection.hpp"

#include "featherdoc_cli_json.hpp"

#include <ostream>
#include <string>

namespace featherdoc_cli {

auto select_template_part(featherdoc::Document &doc,
                          validation_part_family part,
                          const std::optional<std::size_t> &part_index,
                          const std::optional<std::size_t> &section_index,
                          featherdoc::section_reference_kind reference_kind,
                          selected_template_part &selected,
                          std::string &error_message) -> bool {
    selected = {};
    selected.family = part;
    selected.part_index = part_index;
    selected.section_index = section_index;

    switch (part) {
    case validation_part_family::body:
        selected.part = doc.body_template();
        if (!selected.part) {
            error_message = "body template not available";
            return false;
        }
        break;
    case validation_part_family::header:
        selected.part_index = part_index.value_or(0U);
        selected.part = doc.header_template(*selected.part_index);
        if (!selected.part) {
            error_message = "header part not found";
            return false;
        }
        break;
    case validation_part_family::footer:
        selected.part_index = part_index.value_or(0U);
        selected.part = doc.footer_template(*selected.part_index);
        if (!selected.part) {
            error_message = "footer part not found";
            return false;
        }
        break;
    case validation_part_family::section_header:
        selected.reference_kind = reference_kind;
        selected.part = doc.section_header_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section header part not found";
            return false;
        }
        break;
    case validation_part_family::section_footer:
        selected.reference_kind = reference_kind;
        selected.part = doc.section_footer_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section footer part not found";
            return false;
        }
        break;
    }

    return true;
}

auto select_mutable_template_part(featherdoc::Document &doc,
                                  validation_part_family part,
                                  const std::optional<std::size_t> &part_index,
                                  const std::optional<std::size_t> &section_index,
                                  featherdoc::section_reference_kind reference_kind,
                                  selected_template_part &selected,
                                  std::string &error_message) -> bool {
    selected = {};
    selected.family = part;
    selected.part_index = part_index;
    selected.section_index = section_index;

    switch (part) {
    case validation_part_family::body:
        selected.part = doc.body_template();
        if (!selected.part) {
            error_message = "body template not available";
            return false;
        }
        break;
    case validation_part_family::header:
        selected.part_index = part_index.value_or(0U);
        if (*selected.part_index == 0U && doc.header_count() == 0U) {
            auto &paragraphs = doc.ensure_header_paragraphs();
            if (!paragraphs.has_next()) {
                error_message = "failed to materialize header part";
                return false;
            }
        }
        selected.part = doc.header_template(*selected.part_index);
        if (!selected.part) {
            error_message = "header part not found";
            return false;
        }
        break;
    case validation_part_family::footer:
        selected.part_index = part_index.value_or(0U);
        if (*selected.part_index == 0U && doc.footer_count() == 0U) {
            auto &paragraphs = doc.ensure_footer_paragraphs();
            if (!paragraphs.has_next()) {
                error_message = "failed to materialize footer part";
                return false;
            }
        }
        selected.part = doc.footer_template(*selected.part_index);
        if (!selected.part) {
            error_message = "footer part not found";
            return false;
        }
        break;
    case validation_part_family::section_header: {
        selected.reference_kind = reference_kind;
        auto &paragraphs = doc.ensure_section_header_paragraphs(*section_index,
                                                                reference_kind);
        if (!paragraphs.has_next()) {
            error_message = "failed to materialize section header part";
            return false;
        }
        selected.part = doc.section_header_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section header part not found";
            return false;
        }
        break;
    }
    case validation_part_family::section_footer: {
        selected.reference_kind = reference_kind;
        auto &paragraphs = doc.ensure_section_footer_paragraphs(*section_index,
                                                                reference_kind);
        if (!paragraphs.has_next()) {
            error_message = "failed to materialize section footer part";
            return false;
        }
        selected.part = doc.section_footer_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section footer part not found";
            return false;
        }
        break;
    }
    }

    return true;
}

void write_json_selected_template_part(std::ostream &stream,
                                       const selected_template_part &selected) {
    stream << "\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
}

void print_selected_template_part(std::ostream &stream,
                                  const selected_template_part &selected) {
    stream << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        stream << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        stream << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(*selected.reference_kind)
               << '\n';
    }
    stream << "entry_name: " << selected.part.entry_name() << '\n';
}

} // namespace featherdoc_cli
