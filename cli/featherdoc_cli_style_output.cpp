#include "featherdoc_cli_style_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_style_numbering_summary(
    std::ostream &stream,
    const featherdoc::style_summary::numbering_summary &numbering) {
    stream << "{\"num_id\":";
    write_json_optional_u32(stream, numbering.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, numbering.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, numbering.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, numbering.definition_name);
    stream << ",\"instance\":";
    if (numbering.instance.has_value()) {
        write_json_numbering_instance_summary(stream, *numbering.instance);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_style_usage_hit_reference(
    std::ostream &stream,
    const featherdoc::style_usage_hit_reference &reference) {
    stream << "{\"section_index\":" << reference.section_index
           << ",\"reference_kind\":";
    write_json_string(stream,
                      featherdoc::to_xml_reference_type(reference.reference_kind));
    stream << '}';
}

void write_json_style_usage_breakdown(
    std::ostream &stream, const featherdoc::style_usage_breakdown &usage) {
    stream << "{\"paragraph_count\":" << usage.paragraph_count
           << ",\"run_count\":" << usage.run_count
           << ",\"table_count\":" << usage.table_count
           << ",\"total_count\":" << usage.total_count() << '}';
}

void write_json_style_usage_hit(std::ostream &stream,
                                const featherdoc::style_usage_hit &hit) {
    stream << "{\"part\":";
    write_json_string(stream, style_usage_part_kind_name(hit.part));
    stream << ",\"entry_name\":";
    write_json_string(stream, hit.entry_name);
    stream << ",\"section_index\":";
    if (hit.section_index.has_value()) {
        stream << *hit.section_index;
    } else {
        stream << "null";
    }
    stream << ",\"ordinal\":" << hit.ordinal
           << ",\"node_ordinal\":" << hit.node_ordinal << ",\"kind\":";
    write_json_string(stream, style_usage_hit_kind_name(hit.kind));
    stream << ",\"references\":[";
    for (std::size_t index = 0; index < hit.references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_usage_hit_reference(stream, hit.references[index]);
    }
    stream << ']';
    stream << '}';
}

void print_style_numbering_inline(
    std::ostream &stream,
    const std::optional<featherdoc::style_summary::numbering_summary>
        &numbering) {
    if (!numbering.has_value()) {
        stream << "none";
        return;
    }

    stream << '(';
    stream << "level=";
    if (numbering->level.has_value()) {
        stream << *numbering->level;
    } else {
        stream << "unknown";
    }
    stream << ", num_id=";
    if (numbering->num_id.has_value()) {
        stream << *numbering->num_id;
    } else {
        stream << "unknown";
    }
    stream << ", definition_id=";
    if (numbering->definition_id.has_value()) {
        stream << *numbering->definition_id;
    } else {
        stream << "unknown";
    }
    stream << ", definition_name=";
    if (numbering->definition_name.has_value()) {
        stream << *numbering->definition_name;
    } else {
        stream << "none";
    }
    if (numbering->instance.has_value()) {
        stream << ", override_count="
               << numbering->instance->level_overrides.size();
    }
    stream << ')';
}

void print_numbering_level_override_summary(
    std::ostream &stream,
    const featherdoc::numbering_level_override_summary &level_override) {
    stream << "level=" << level_override.level << " start_override=";
    if (level_override.start_override.has_value()) {
        stream << *level_override.start_override;
    } else {
        stream << "none";
    }

    if (level_override.level_definition.has_value()) {
        stream << " definition_kind="
               << list_kind_name(level_override.level_definition->kind)
               << " definition_start=" << level_override.level_definition->start
               << " definition_text_pattern="
               << level_override.level_definition->text_pattern;
    }
}

} // namespace

void write_json_style_summary(std::ostream &stream,
                              const featherdoc::style_summary &style) {
    stream << "{\"style_id\":";
    write_json_string(stream, style.style_id);
    stream << ",\"name\":";
    write_json_string(stream, style.name);
    stream << ",\"based_on\":";
    write_json_optional_string(stream, style.based_on);
    stream << ",\"kind\":";
    write_json_string(stream, style_kind_name(style.kind));
    stream << ",\"type\":";
    write_json_string(stream, style.type_name);
    stream << ",\"numbering\":";
    if (style.numbering.has_value()) {
        write_json_style_numbering_summary(stream, *style.numbering);
    } else {
        stream << "null";
    }
    stream << ",\"is_default\":" << json_bool(style.is_default)
           << ",\"is_custom\":" << json_bool(style.is_custom)
           << ",\"is_semi_hidden\":" << json_bool(style.is_semi_hidden)
           << ",\"is_unhide_when_used\":" << json_bool(style.is_unhide_when_used)
           << ",\"is_quick_format\":" << json_bool(style.is_quick_format)
           << '}';
}

void write_json_style_usage_summary(
    std::ostream &stream, const featherdoc::style_usage_summary &usage) {
    stream << "{\"style_id\":";
    write_json_string(stream, usage.style_id);
    stream << ",\"paragraph_count\":" << usage.paragraph_count
           << ",\"run_count\":" << usage.run_count
           << ",\"table_count\":" << usage.table_count
           << ",\"total_count\":" << usage.total_count()
           << ",\"body\":";
    write_json_style_usage_breakdown(stream, usage.body);
    stream << ",\"header\":";
    write_json_style_usage_breakdown(stream, usage.header);
    stream << ",\"footer\":";
    write_json_style_usage_breakdown(stream, usage.footer);
    stream << ",\"hits\":[";
    for (std::size_t index = 0; index < usage.hits.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_usage_hit(stream, usage.hits[index]);
    }
    stream << ']';
    stream << '}';
}

void print_style_summary(std::ostream &stream,
                         const featherdoc::style_summary &style) {
    stream << "id=" << style.style_id << " name=" << style.name
           << " type=" << style.type_name
           << " kind=" << style_kind_name(style.kind) << " based_on=";
    if (style.based_on.has_value()) {
        stream << *style.based_on;
    } else {
        stream << "none";
    }
    stream << " default=" << yes_no(style.is_default)
           << " custom=" << yes_no(style.is_custom)
           << " semi_hidden=" << yes_no(style.is_semi_hidden)
           << " unhide_when_used=" << yes_no(style.is_unhide_when_used)
           << " quick_format=" << yes_no(style.is_quick_format)
           << " numbering=";
    print_style_numbering_inline(stream, style.numbering);
}

void inspect_style(
    const featherdoc::style_summary &style,
    const std::optional<featherdoc::style_usage_summary> &usage,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"style\":";
        write_json_style_summary(std::cout, style);
        if (usage.has_value()) {
            std::cout << ",\"usage\":";
            write_json_style_usage_summary(std::cout, *usage);
        }
        std::cout << "}\n";
        return;
    }

    std::cout << "style_id: " << style.style_id << '\n';
    std::cout << "name: " << style.name << '\n';
    std::cout << "type: " << style.type_name << '\n';
    std::cout << "kind: " << style_kind_name(style.kind) << '\n';
    std::cout << "based_on: ";
    if (style.based_on.has_value()) {
        std::cout << *style.based_on;
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    std::cout << "default: " << yes_no(style.is_default) << '\n';
    std::cout << "custom: " << yes_no(style.is_custom) << '\n';
    std::cout << "semi_hidden: " << yes_no(style.is_semi_hidden) << '\n';
    std::cout << "unhide_when_used: " << yes_no(style.is_unhide_when_used)
              << '\n';
    std::cout << "quick_format: " << yes_no(style.is_quick_format) << '\n';
    std::cout << "numbering: ";
    print_style_numbering_inline(std::cout, style.numbering);
    std::cout << '\n';
    if (style.numbering.has_value() && style.numbering->instance.has_value()) {
        std::cout << "numbering_instance_id: "
                  << style.numbering->instance->instance_id << '\n';
        std::cout << "numbering_override_count: "
                  << style.numbering->instance->level_overrides.size()
                  << '\n';
        for (std::size_t index = 0;
             index < style.numbering->instance->level_overrides.size();
             ++index) {
            std::cout << "numbering_override[" << index << "]: ";
            print_numbering_level_override_summary(
                std::cout, style.numbering->instance->level_overrides[index]);
            std::cout << '\n';
        }
    }

    if (usage.has_value()) {
        std::cout << "usage_paragraphs: " << usage->paragraph_count << '\n';
        std::cout << "usage_runs: " << usage->run_count << '\n';
        std::cout << "usage_tables: " << usage->table_count << '\n';
        std::cout << "usage_total: " << usage->total_count() << '\n';
        std::cout << "usage_body_paragraphs: " << usage->body.paragraph_count
                  << '\n';
        std::cout << "usage_body_runs: " << usage->body.run_count << '\n';
        std::cout << "usage_body_tables: " << usage->body.table_count << '\n';
        std::cout << "usage_body_total: " << usage->body.total_count() << '\n';
        std::cout << "usage_header_paragraphs: "
                  << usage->header.paragraph_count << '\n';
        std::cout << "usage_header_runs: " << usage->header.run_count << '\n';
        std::cout << "usage_header_tables: " << usage->header.table_count
                  << '\n';
        std::cout << "usage_header_total: " << usage->header.total_count()
                  << '\n';
        std::cout << "usage_footer_paragraphs: "
                  << usage->footer.paragraph_count << '\n';
        std::cout << "usage_footer_runs: " << usage->footer.run_count << '\n';
        std::cout << "usage_footer_tables: " << usage->footer.table_count
                  << '\n';
        std::cout << "usage_footer_total: " << usage->footer.total_count()
                  << '\n';
        std::cout << "usage_hits: " << usage->hits.size() << '\n';
        for (std::size_t index = 0; index < usage->hits.size(); ++index) {
            const auto &hit = usage->hits[index];
            std::cout << "usage_hit[" << index << "]: part="
                      << style_usage_part_kind_name(hit.part)
                      << " entry_name=" << hit.entry_name
                      << " ordinal=" << hit.ordinal << " section_index=";
            if (hit.section_index.has_value()) {
                std::cout << *hit.section_index;
            } else {
                std::cout << '-';
            }
            std::cout << " kind=" << style_usage_hit_kind_name(hit.kind)
                      << " references=" << hit.references.size();
            for (std::size_t reference_index = 0;
                 reference_index < hit.references.size(); ++reference_index) {
                const auto &reference = hit.references[reference_index];
                std::cout << " ref[" << reference_index
                          << "]=section:" << reference.section_index
                          << ",kind:"
                          << featherdoc::to_xml_reference_type(
                                 reference.reference_kind);
            }
            std::cout << '\n';
        }
    }
}

void write_json_style_prune_plan(std::ostream &stream,
                                 const featherdoc::style_prune_plan &plan) {
    stream << ",\"scanned_style_count\":" << plan.scanned_style_count
           << ",\"protected_style_count\":" << plan.protected_style_count
           << ",\"removable_style_count\":" << plan.removable_style_ids.size()
           << ",\"removable_style_ids\":";
    write_json_strings(stream, plan.removable_style_ids);
}

void print_style_prune_plan(const featherdoc::style_prune_plan &plan) {
    std::cout << "styles_scanned: " << plan.scanned_style_count << '\n'
              << "styles_protected: " << plan.protected_style_count << '\n'
              << "removable_styles: " << plan.removable_style_ids.size()
              << '\n';
    for (std::size_t index = 0U; index < plan.removable_style_ids.size();
         ++index) {
        std::cout << "removable_style[" << index
                  << "]: " << plan.removable_style_ids[index] << '\n';
    }
}

void write_json_style_prune_summary(
    std::ostream &stream, const featherdoc::style_prune_summary &summary) {
    stream << ",\"scanned_style_count\":" << summary.scanned_style_count
           << ",\"protected_style_count\":" << summary.protected_style_count
           << ",\"removed_style_count\":" << summary.removed_style_ids.size()
           << ",\"removed_style_ids\":";
    write_json_strings(stream, summary.removed_style_ids);
}

void print_style_prune_summary(const featherdoc::style_prune_summary &summary) {
    std::cout << "styles_scanned: " << summary.scanned_style_count << '\n'
              << "styles_protected: " << summary.protected_style_count << '\n'
              << "removed_styles: " << summary.removed_style_ids.size() << '\n';
    for (std::size_t index = 0U; index < summary.removed_style_ids.size();
         ++index) {
        std::cout << "removed_style[" << index
                  << "]: " << summary.removed_style_ids[index] << '\n';
    }
}

} // namespace featherdoc_cli
