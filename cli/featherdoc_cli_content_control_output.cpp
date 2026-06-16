#include "featherdoc_cli_content_control_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

namespace featherdoc_cli {
namespace {

void write_json_content_control_selector(
    std::ostream &stream, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
    stream << ",\"selector\":{\"kind\":";
    write_json_string(stream, tag.has_value() ? "tag" : "alias");
    stream << ",\"value\":";
    write_json_string(stream, tag.has_value() ? *tag : *alias);
    stream << '}';
}

} // namespace

void write_json_content_control_part_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
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
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    write_json_content_control_selector(stream, tag, alias);
}

void print_content_control_common_result(
    const selected_template_part &selected, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias,
    const std::optional<path_type> &output_path, std::size_t replaced) {
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
    std::cout << "selector_kind: " << (tag.has_value() ? "tag" : "alias")
              << '\n';
    std::cout << "selector_value: " << (tag.has_value() ? *tag : *alias)
              << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
}

} // namespace featherdoc_cli
