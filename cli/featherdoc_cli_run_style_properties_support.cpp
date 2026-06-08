#include "featherdoc_cli_run_style_properties_support.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>

namespace featherdoc_cli {

auto open_document_for_command(std::string_view command,
                               const std::vector<std::string_view> &arguments,
                               featherdoc::Document &doc,
                               bool json_output) -> bool {
    return open_document(path_type(std::string(arguments[1])), doc, command,
                         json_output);
}

void write_json_run_properties_summary(std::ostream &stream,
                                       const run_properties_summary &summary) {
    stream << "{\"font_family\":";
    write_json_optional_string(stream, summary.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, summary.east_asia_font_family);
    stream << ",\"language\":";
    write_json_optional_string(stream, summary.language);
    stream << ",\"east_asia_language\":";
    write_json_optional_string(stream, summary.east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_optional_string(stream, summary.bidi_language);
    stream << ",\"rtl\":";
    write_json_optional_bool(stream, summary.rtl);
    stream << ",\"paragraph_bidi\":";
    write_json_optional_bool(stream, summary.paragraph_bidi);
    stream << '}';
}

void print_run_properties_summary(std::ostream &stream,
                                  const run_properties_summary &summary) {
    stream << "font_family: ";
    if (summary.font_family.has_value()) {
        stream << *summary.font_family;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "east_asia_font_family: ";
    if (summary.east_asia_font_family.has_value()) {
        stream << *summary.east_asia_font_family;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "language: ";
    if (summary.language.has_value()) {
        stream << *summary.language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "east_asia_language: ";
    if (summary.east_asia_language.has_value()) {
        stream << *summary.east_asia_language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "bidi_language: ";
    if (summary.bidi_language.has_value()) {
        stream << *summary.bidi_language;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "rtl: ";
    if (summary.rtl.has_value()) {
        stream << yes_no(*summary.rtl);
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "paragraph_bidi: ";
    if (summary.paragraph_bidi.has_value()) {
        stream << yes_no(*summary.paragraph_bidi);
    } else {
        stream << "none";
    }
    stream << '\n';
}

void write_json_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary) {
    stream << "{\"based_on\":";
    write_json_optional_string(stream, summary.based_on);
    stream << ",\"next_style\":";
    write_json_optional_string(stream, summary.next_style);
    stream << ",\"outline_level\":";
    if (summary.outline_level.has_value()) {
        stream << *summary.outline_level;
    } else {
        stream << "null";
    }
    stream << '}';
}

void print_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary) {
    stream << "based_on: ";
    if (summary.based_on.has_value()) {
        stream << *summary.based_on;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "next_style: ";
    if (summary.next_style.has_value()) {
        stream << *summary.next_style;
    } else {
        stream << "none";
    }
    stream << '\n';

    stream << "outline_level: ";
    if (summary.outline_level.has_value()) {
        stream << *summary.outline_level;
    } else {
        stream << "none";
    }
    stream << '\n';
}

void write_json_cleared_fields(std::ostream &stream,
                               const std::vector<std::string> &cleared_fields) {
    stream << '[';
    for (std::size_t index = 0U; index < cleared_fields.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, cleared_fields[index]);
    }
    stream << ']';
}

void append_materialized_style_run_property(
    std::vector<materialized_style_run_property_summary> &materialized_properties,
    std::string_view current_style_id, std::string_view field_name,
    const featherdoc::resolved_style_string_property &property) {
    if (!property.value.has_value() || !property.source_style_id.has_value() ||
        *property.source_style_id == current_style_id) {
        return;
    }

    materialized_properties.push_back(
        {std::string(field_name), *property.source_style_id});
}

void append_materialized_style_run_property(
    std::vector<materialized_style_run_property_summary> &materialized_properties,
    std::string_view current_style_id, std::string_view field_name,
    const featherdoc::resolved_style_bool_property &property) {
    if (!property.value.has_value() || !property.source_style_id.has_value() ||
        *property.source_style_id == current_style_id) {
        return;
    }

    materialized_properties.push_back(
        {std::string(field_name), *property.source_style_id});
}

auto collect_materialized_style_run_properties(
    std::string_view style_id,
    const featherdoc::resolved_style_properties_summary &resolved)
    -> std::vector<materialized_style_run_property_summary> {
    auto properties = std::vector<materialized_style_run_property_summary>{};
    append_materialized_style_run_property(properties, style_id, "font_family",
                                          resolved.run_font_family);
    append_materialized_style_run_property(properties, style_id,
                                          "east_asia_font_family",
                                          resolved.run_east_asia_font_family);
    append_materialized_style_run_property(properties, style_id, "language",
                                          resolved.run_language);
    append_materialized_style_run_property(properties, style_id,
                                          "east_asia_language",
                                          resolved.run_east_asia_language);
    append_materialized_style_run_property(properties, style_id, "bidi_language",
                                          resolved.run_bidi_language);
    append_materialized_style_run_property(properties, style_id, "rtl",
                                          resolved.run_rtl);
    append_materialized_style_run_property(properties, style_id,
                                          "paragraph_bidi",
                                          resolved.paragraph_bidi);
    return properties;
}

void write_json_materialized_style_run_properties(
    std::ostream &stream,
    const std::vector<materialized_style_run_property_summary>
        &materialized_properties) {
    stream << '[';
    for (std::size_t index = 0U; index < materialized_properties.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        stream << "{\"field\":";
        write_json_string(stream, materialized_properties[index].field);
        stream << ",\"source_style_id\":";
        write_json_string(stream, materialized_properties[index].source_style_id);
        stream << '}';
    }
    stream << ']';
}

auto resolve_style_properties_for_command(
    std::string_view command, featherdoc::Document &doc,
    const std::string &style_id, bool json_output)
    -> std::optional<featherdoc::resolved_style_properties_summary> {
    const auto resolved = doc.resolve_style_properties(style_id);
    if (!resolved.has_value()) {
        report_document_error(command, "inspect", doc.last_error(), json_output);
    }
    return resolved;
}

} // namespace featherdoc_cli
