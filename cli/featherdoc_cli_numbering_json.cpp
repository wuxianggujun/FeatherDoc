#include "featherdoc_cli_numbering_json.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"

#include <cstddef>
#include <ostream>

namespace featherdoc_cli {

void write_json_numbering_level_definition(
    std::ostream &stream,
    const featherdoc::numbering_level_definition &definition) {
    stream << "{\"level\":" << definition.level << ",\"kind\":";
    write_json_string(stream, list_kind_name(definition.kind));
    stream << ",\"start\":" << definition.start << ",\"text_pattern\":";
    write_json_string(stream, definition.text_pattern);
    stream << '}';
}

void write_json_numbering_level_override_summary(
    std::ostream &stream,
    const featherdoc::numbering_level_override_summary &level_override) {
    stream << "{\"level\":" << level_override.level << ",\"start_override\":";
    write_json_optional_u32(stream, level_override.start_override);
    stream << ",\"level_definition\":";
    if (level_override.level_definition.has_value()) {
        write_json_numbering_level_definition(stream, *level_override.level_definition);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_numbering_instance_summary(
    std::ostream &stream,
    const featherdoc::numbering_instance_summary &instance) {
    stream << "{\"instance_id\":" << instance.instance_id << ",\"level_overrides\":[";
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    instance.level_overrides[index]);
    }
    stream << "]}";
}

void write_json_imported_numbering_definition_summary(
    std::ostream &stream,
    const featherdoc::imported_numbering_definition_summary &definition) {
    stream << "{\"name\":";
    write_json_string(stream, definition.name);
    stream << ",\"definition_id\":" << definition.definition_id
           << ",\"instance_ids\":[";
    for (std::size_t index = 0; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "]}";
}

void write_json_numbering_catalog_import_summary(
    std::ostream &stream,
    const featherdoc::numbering_catalog_import_summary &summary) {
    stream << "\"input_definition_count\":" << summary.input_definition_count
           << ",\"imported_definition_count\":" << summary.imported_definition_count
           << ",\"imported_instance_count\":" << summary.imported_instance_count
           << ",\"imported_definitions\":[";
    for (std::size_t index = 0; index < summary.definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_imported_numbering_definition_summary(stream,
                                                         summary.definitions[index]);
    }
    stream << ']';
}

void write_json_paragraph_style_numbering_link(
    std::ostream &stream,
    const featherdoc::paragraph_style_numbering_link &style_link) {
    stream << "{\"style_id\":";
    write_json_string(stream, style_link.style_id);
    stream << ",\"level\":" << style_link.level << '}';
}

} // namespace featherdoc_cli
