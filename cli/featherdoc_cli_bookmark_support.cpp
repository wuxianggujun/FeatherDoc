#include "featherdoc_cli_bookmark_support.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <iostream>
#include <ostream>
#include <string>

namespace featherdoc_cli {

void write_json_bookmark_support_summary(
    std::ostream &stream, const featherdoc::bookmark_summary &bookmark) {
    stream << "{\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark_name);
    stream << ",\"occurrence_count\":" << bookmark.occurrence_count
           << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.kind));
    stream << ",\"is_duplicate\":" << json_bool(bookmark.is_duplicate()) << '}';
}

void print_bookmark_support_summary(
    std::ostream &stream, const featherdoc::bookmark_summary &bookmark) {
    stream << "name=" << bookmark.bookmark_name
           << " occurrences=" << bookmark.occurrence_count
           << " kind=" << bookmark_kind_name(bookmark.kind)
           << " duplicate=" << yes_no(bookmark.is_duplicate());
}

void write_json_selected_bookmark_part(std::ostream &stream,
                                       const selected_template_part &selected) {
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
        write_json_string(
            stream, featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
}

void print_selected_bookmark_part(const selected_template_part &selected) {
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
}

void print_bookmark_identity(const selected_template_part &selected,
                             const featherdoc::bookmark_summary &bookmark) {
    print_selected_bookmark_part(selected);
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count
              << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate())
              << '\n';
}

auto select_bookmark_part(std::string_view command, featherdoc::Document &doc,
                          validation_part_family part,
                          const std::optional<std::size_t> &part_index,
                          const std::optional<std::size_t> &section_index,
                          featherdoc::section_reference_kind reference_kind,
                          bool json_output, selected_template_part &selected,
                          std::string &error_message) -> bool {
    if (select_template_part(doc, part, part_index, section_index,
                             reference_kind, selected, error_message)) {
        return true;
    }

    report_operation_failure(command, "mutate", error_message, doc.last_error(),
                             json_output);
    return false;
}

auto report_bookmark_input_error(std::string_view command, bool json_output,
                                 const std::string &error_message) -> int {
    if (json_output) {
        write_json_command_error(std::cerr, command, "input", error_message);
    } else {
        std::cerr << error_message << '\n';
    }
    return 1;
}

} // namespace featherdoc_cli
