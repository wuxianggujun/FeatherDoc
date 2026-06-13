#include "featherdoc_cli_template_schema_target_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << "{\"" << template_slot_source_json_name(requirement.source) << "\":";
    write_json_string(stream, requirement.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, template_slot_kind_name(requirement.kind));
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << ",\"count\":" << *requirement.min_occurrences;
    } else {
        if (!requirement.required) {
            stream << ",\"required\":false";
        }
        if (requirement.min_occurrences.has_value()) {
            stream << ",\"min\":" << *requirement.min_occurrences;
        }
        if (requirement.max_occurrences.has_value()) {
            stream << ",\"max\":" << *requirement.max_occurrences;
        }
    }
    stream << '}';
}

void print_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << template_slot_source_text_name(requirement.source) << '='
           << requirement.bookmark_name << " kind="
           << template_slot_kind_name(requirement.kind);
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << " count=" << *requirement.min_occurrences;
        return;
    }

    stream << " required=" << yes_no(requirement.required);
    if (requirement.min_occurrences.has_value()) {
        stream << " min=" << *requirement.min_occurrences;
    }
    if (requirement.max_occurrences.has_value()) {
        stream << " max=" << *requirement.max_occurrences;
    }
}

} // namespace

void write_json_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(target.part));
    if (target.part_index.has_value()) {
        stream << ",\"index\":" << *target.part_index;
    }
    if (target.section_index.has_value()) {
        stream << ",\"section\":" << *target.section_index;
    }
    if (target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*target.reference_kind));
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *target.resolved_from_section_index;
    }
    if (target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"slots\":[";
    for (std::size_t slot_index = 0U; slot_index < target.requirements.size();
         ++slot_index) {
        if (slot_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_requirement(
            stream, target.requirements[slot_index]);
    }
    stream << "]}";
}

void print_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "part: " << validation_part_name(target.part) << '\n';
    if (target.part_index.has_value()) {
        stream << "part_index: " << *target.part_index << '\n';
    }
    if (target.section_index.has_value()) {
        stream << "section: " << *target.section_index << '\n';
    }
    if (target.reference_kind.has_value()) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(*target.reference_kind) << '\n';
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << "resolved_from_section: " << *target.resolved_from_section_index
               << '\n';
    }
    stream << "linked_to_previous: " << yes_no(target.linked_to_previous) << '\n';
    stream << "slot_count: " << target.requirements.size() << '\n';
    for (std::size_t index = 0U; index < target.requirements.size(); ++index) {
        stream << "slot[" << index << "]: ";
        print_exported_template_schema_requirement(stream, target.requirements[index]);
        stream << '\n';
    }
}

} // namespace featherdoc_cli
