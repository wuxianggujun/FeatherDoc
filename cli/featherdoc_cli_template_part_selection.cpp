#include "featherdoc_cli_template_part_selection.hpp"

#include "featherdoc_cli_json.hpp"

#include <ostream>
#include <string>

namespace featherdoc_cli {

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
