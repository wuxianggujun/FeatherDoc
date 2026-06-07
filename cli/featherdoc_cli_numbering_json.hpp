#pragma once

#include <featherdoc.hpp>

#include <iosfwd>

namespace featherdoc_cli {

void write_json_numbering_level_definition(
    std::ostream &stream, const featherdoc::numbering_level_definition &definition);

void write_json_numbering_level_override_summary(
    std::ostream &stream,
    const featherdoc::numbering_level_override_summary &level_override);

void write_json_numbering_instance_summary(
    std::ostream &stream,
    const featherdoc::numbering_instance_summary &instance);

void write_json_numbering_catalog_import_summary(
    std::ostream &stream,
    const featherdoc::numbering_catalog_import_summary &summary);

void write_json_paragraph_style_numbering_link(
    std::ostream &stream,
    const featherdoc::paragraph_style_numbering_link &style_link);

} // namespace featherdoc_cli
