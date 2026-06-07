#include "featherdoc_cli_numbering_json.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"

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

void write_json_paragraph_style_numbering_link(
    std::ostream &stream,
    const featherdoc::paragraph_style_numbering_link &style_link) {
    stream << "{\"style_id\":";
    write_json_string(stream, style_link.style_id);
    stream << ",\"level\":" << style_link.level << '}';
}

} // namespace featherdoc_cli
