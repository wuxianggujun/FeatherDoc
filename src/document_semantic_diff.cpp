#include "featherdoc.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc {

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info = {code, std::move(detail), std::move(entry_name), xml_offset};
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}

auto semantic_optional_string(const std::optional<std::string> &value)
    -> std::string {
    return value.has_value() ? *value : std::string{};
}

template <typename Value>
auto semantic_optional_numeric(const std::optional<Value> &value) -> std::string {
    return value.has_value() ? std::to_string(*value) : std::string{};
}

auto semantic_bool(const std::optional<bool> &value) -> std::string {
    if (!value.has_value()) {
        return {};
    }
    return *value ? "true" : "false";
}

auto semantic_paragraph_alignment(
    const std::optional<featherdoc::paragraph_alignment> &alignment)
    -> std::string {
    if (!alignment.has_value()) {
        return {};
    }

    switch (*alignment) {
    case featherdoc::paragraph_alignment::left:
        return "left";
    case featherdoc::paragraph_alignment::center:
        return "center";
    case featherdoc::paragraph_alignment::right:
        return "right";
    case featherdoc::paragraph_alignment::justified:
        return "justified";
    case featherdoc::paragraph_alignment::distribute:
        return "distribute";
    }

    return {};
}

auto semantic_content_control_kind_name(featherdoc::content_control_kind kind)
    -> std::string_view {
    switch (kind) {
    case featherdoc::content_control_kind::block:
        return "block";
    case featherdoc::content_control_kind::run:
        return "run";
    case featherdoc::content_control_kind::table_row:
        return "table_row";
    case featherdoc::content_control_kind::table_cell:
        return "table_cell";
    case featherdoc::content_control_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_content_control_form_kind_name(
    featherdoc::content_control_form_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::content_control_form_kind::rich_text:
        return "rich_text";
    case featherdoc::content_control_form_kind::plain_text:
        return "plain_text";
    case featherdoc::content_control_form_kind::picture:
        return "picture";
    case featherdoc::content_control_form_kind::checkbox:
        return "checkbox";
    case featherdoc::content_control_form_kind::drop_down_list:
        return "drop_down_list";
    case featherdoc::content_control_form_kind::combo_box:
        return "combo_box";
    case featherdoc::content_control_form_kind::date:
        return "date";
    case featherdoc::content_control_form_kind::repeating_section:
        return "repeating_section";
    case featherdoc::content_control_form_kind::group:
        return "group";
    case featherdoc::content_control_form_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_drawing_image_placement_name(
    featherdoc::drawing_image_placement placement) -> std::string_view {
    switch (placement) {
    case featherdoc::drawing_image_placement::inline_object:
        return "inline";
    case featherdoc::drawing_image_placement::anchored_object:
        return "anchored";
    }

    return "inline";
}

auto semantic_table_position_value(
    const std::optional<featherdoc::table_position> &position) -> std::string {
    if (!position.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "horizontal_reference="
           << static_cast<int>(position->horizontal_reference)
           << ";horizontal_offset=" << position->horizontal_offset_twips
           << ";horizontal_spec=";
    if (position->horizontal_spec.has_value()) {
        stream << static_cast<int>(*position->horizontal_spec);
    }
    stream << ";vertical_reference="
           << static_cast<int>(position->vertical_reference)
           << ";vertical_offset=" << position->vertical_offset_twips
           << ";vertical_spec=";
    if (position->vertical_spec.has_value()) {
        stream << static_cast<int>(*position->vertical_spec);
    }
    stream << ";left=" << semantic_optional_numeric(position->left_from_text_twips)
           << ";right=" << semantic_optional_numeric(position->right_from_text_twips)
           << ";top=" << semantic_optional_numeric(position->top_from_text_twips)
           << ";bottom=" << semantic_optional_numeric(position->bottom_from_text_twips)
           << ";overlap=";
    if (position->overlap.has_value()) {
        stream << static_cast<int>(*position->overlap);
    }
    return stream.str();
}

auto semantic_column_widths_value(
    const std::vector<std::optional<std::uint32_t>> &widths) -> std::string {
    std::ostringstream stream;
    for (std::size_t index = 0; index < widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        if (widths[index].has_value()) {
            stream << *widths[index];
        } else {
            stream << "null";
        }
    }
    return stream.str();
}

auto semantic_paragraph_value(
    const featherdoc::paragraph_inspection_summary &paragraph) -> std::string {
    std::ostringstream stream;
    stream << "text=" << paragraph.text << "\nstyle="
           << semantic_optional_string(paragraph.style_id) << "\nbidi="
           << semantic_bool(paragraph.bidi) << "\nalignment="
           << semantic_paragraph_alignment(paragraph.alignment)
           << "\nindent_left="
           << semantic_optional_numeric(paragraph.indent_left_twips)
           << "\nindent_right="
           << semantic_optional_numeric(paragraph.indent_right_twips)
           << "\nfirst_line_indent="
           << semantic_optional_numeric(paragraph.first_line_indent_twips)
           << "\nhanging_indent="
           << semantic_optional_numeric(paragraph.hanging_indent_twips)
           << "\nruns=" << paragraph.run_count;
    if (paragraph.numbering.has_value()) {
        stream << "\nnumbering="
               << semantic_optional_numeric(paragraph.numbering->num_id) << ':'
               << semantic_optional_numeric(paragraph.numbering->level) << ':'
               << semantic_optional_numeric(paragraph.numbering->definition_id) << ':'
               << semantic_optional_string(paragraph.numbering->definition_name);
    }
    return stream.str();
}

auto semantic_table_value(
    const featherdoc::table_inspection_summary &table) -> std::string {
    std::ostringstream stream;
    stream << "text=" << table.text << "\nstyle="
           << semantic_optional_string(table.style_id) << "\nrows="
           << table.row_count << "\ncolumns=" << table.column_count
           << "\nwidth=" << semantic_optional_numeric(table.width_twips)
           << "\ncolumn_widths="
           << semantic_column_widths_value(table.column_widths)
           << "\nposition=" << semantic_table_position_value(table.position);
    return stream.str();
}

auto semantic_floating_options_value(
    const std::optional<featherdoc::floating_image_options> &options)
    -> std::string {
    if (!options.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "offset=" << options->horizontal_offset_px << ','
           << options->vertical_offset_px << ";behind="
           << (options->behind_text ? "true" : "false") << ";overlap="
           << (options->allow_overlap ? "true" : "false") << ";z="
           << options->z_order << ";wrap_left="
           << options->wrap_distance_left_px << ";wrap_right="
           << options->wrap_distance_right_px << ";wrap_top="
           << options->wrap_distance_top_px << ";wrap_bottom="
           << options->wrap_distance_bottom_px;
    if (options->crop.has_value()) {
        stream << ";crop=" << options->crop->left_per_mille << ','
               << options->crop->top_per_mille << ','
               << options->crop->right_per_mille << ','
               << options->crop->bottom_per_mille;
    }
    return stream.str();
}

auto semantic_image_value(const featherdoc::drawing_image_info &image,
                          const featherdoc::document_semantic_diff_options &options)
    -> std::string {
    std::ostringstream stream;
    stream << "placement=" << semantic_drawing_image_placement_name(image.placement)
           << "\nentry=" << image.entry_name << "\ndisplay="
           << image.display_name << "\ncontent_type=" << image.content_type
           << "\nwidth=" << image.width_px << "\nheight=" << image.height_px
           << "\nfloating=" << semantic_floating_options_value(image.floating_options);
    if (options.compare_image_relationship_ids) {
        stream << "\nrelationship_id=" << image.relationship_id;
    }
    return stream.str();
}

auto semantic_list_items_value(
    const std::vector<featherdoc::content_control_list_item> &items)
    -> std::string {
    std::ostringstream stream;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << items[index].display_text << '=' << items[index].value;
    }
    return stream.str();
}

auto semantic_content_control_value(
    const featherdoc::content_control_summary &content_control,
    const featherdoc::document_semantic_diff_options &options) -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_content_control_kind_name(content_control.kind)
           << "\nform="
           << semantic_content_control_form_kind_name(content_control.form_kind)
           << "\ntag=" << semantic_optional_string(content_control.tag)
           << "\nalias=" << semantic_optional_string(content_control.alias)
           << "\nlock=" << semantic_optional_string(content_control.lock)
           << "\nbinding_store="
           << semantic_optional_string(content_control.data_binding_store_item_id)
           << "\nbinding_xpath="
           << semantic_optional_string(content_control.data_binding_xpath)
           << "\nbinding_prefix="
           << semantic_optional_string(content_control.data_binding_prefix_mappings)
           << "\nchecked=" << semantic_bool(content_control.checked)
           << "\ndate_format="
           << semantic_optional_string(content_control.date_format)
           << "\ndate_locale="
           << semantic_optional_string(content_control.date_locale)
           << "\nselected="
           << semantic_optional_numeric(content_control.selected_list_item)
           << "\nitems="
           << semantic_list_items_value(content_control.list_items)
           << "\nplaceholder="
           << (content_control.showing_placeholder ? "true" : "false")
           << "\ntext=" << content_control.text;
    if (options.compare_content_control_ids) {
        stream << "\nid=" << semantic_optional_string(content_control.id);
    }
    return stream.str();
}


auto semantic_field_kind_name(featherdoc::field_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::field_kind::page:
        return "page";
    case featherdoc::field_kind::total_pages:
        return "total_pages";
    case featherdoc::field_kind::table_of_contents:
        return "table_of_contents";
    case featherdoc::field_kind::reference:
        return "reference";
    case featherdoc::field_kind::page_reference:
        return "page_reference";
    case featherdoc::field_kind::style_reference:
        return "style_reference";
    case featherdoc::field_kind::document_property:
        return "document_property";
    case featherdoc::field_kind::date:
        return "date";
    case featherdoc::field_kind::hyperlink:
        return "hyperlink";
    case featherdoc::field_kind::sequence:
        return "sequence";
    case featherdoc::field_kind::index:
        return "index";
    case featherdoc::field_kind::index_entry:
        return "index_entry";
    case featherdoc::field_kind::custom:
        return "custom";
    }

    return "custom";
}

auto semantic_field_summary_value(const featherdoc::field_summary &field)
    -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_field_kind_name(field.kind)
           << "\ninstruction=" << field.instruction
           << "\nresult_text=" << field.result_text
           << "\ndirty=" << (field.dirty ? "true" : "false")
           << "\nlocked=" << (field.locked ? "true" : "false")
           << "\ncomplex=" << (field.complex ? "true" : "false")
           << "\ndepth=" << field.depth;
    return stream.str();
}


auto semantic_style_kind_name(featherdoc::style_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::style_kind::paragraph:
        return "paragraph";
    case featherdoc::style_kind::character:
        return "character";
    case featherdoc::style_kind::table:
        return "table";
    case featherdoc::style_kind::numbering:
        return "numbering";
    case featherdoc::style_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_list_kind_name(featherdoc::list_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "decimal";
}

auto semantic_numbering_level_value(
    const featherdoc::numbering_level_definition &level) -> std::string {
    std::ostringstream stream;
    stream << "level=" << level.level << ";kind="
           << semantic_list_kind_name(level.kind) << ";start=" << level.start
           << ";text_pattern=" << level.text_pattern;
    return stream.str();
}

auto semantic_numbering_instance_value(
    const featherdoc::numbering_instance_summary &instance) -> std::string {
    std::ostringstream stream;
    stream << "id=" << instance.instance_id << ";overrides=";
    for (std::size_t index = 0U; index < instance.level_overrides.size(); ++index) {
        const auto &override = instance.level_overrides[index];
        if (index != 0U) {
            stream << '|';
        }
        stream << override.level << ':'
               << semantic_optional_numeric(override.start_override) << ':';
        if (override.level_definition.has_value()) {
            stream << semantic_numbering_level_value(*override.level_definition);
        }
    }
    return stream.str();
}

auto semantic_style_numbering_value(
    const std::optional<featherdoc::style_summary::numbering_summary> &numbering)
    -> std::string {
    if (!numbering.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "num_id=" << semantic_optional_numeric(numbering->num_id)
           << ";level=" << semantic_optional_numeric(numbering->level)
           << ";definition_id="
           << semantic_optional_numeric(numbering->definition_id)
           << ";definition_name="
           << semantic_optional_string(numbering->definition_name);
    if (numbering->instance.has_value()) {
        stream << ";instance="
               << semantic_numbering_instance_value(*numbering->instance);
    }
    return stream.str();
}

auto semantic_style_summary_value(const featherdoc::style_summary &style)
    -> std::string {
    std::ostringstream stream;
    stream << "style_id=" << style.style_id << "\nname=" << style.name
           << "\nbased_on=" << semantic_optional_string(style.based_on)
           << "\nkind=" << semantic_style_kind_name(style.kind)
           << "\ntype=" << style.type_name
           << "\nnumbering=" << semantic_style_numbering_value(style.numbering)
           << "\ndefault=" << (style.is_default ? "true" : "false")
           << "\ncustom=" << (style.is_custom ? "true" : "false")
           << "\nsemi_hidden="
           << (style.is_semi_hidden ? "true" : "false")
           << "\nunhide_when_used="
           << (style.is_unhide_when_used ? "true" : "false")
           << "\nquick_format=" << (style.is_quick_format ? "true" : "false");
    return stream.str();
}

auto semantic_numbering_definition_summary_value(
    const featherdoc::numbering_definition_summary &definition) -> std::string {
    std::ostringstream stream;
    stream << "definition_id=" << definition.definition_id << "\nname="
           << definition.name << "\nlevels=";
    for (std::size_t index = 0U; index < definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << semantic_numbering_level_value(definition.levels[index]);
    }
    stream << "\ninstance_ids=";
    for (std::size_t index = 0U; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "\ninstances=";
    for (std::size_t index = 0U; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << '|';
        }
        stream << semantic_numbering_instance_value(definition.instances[index]);
    }
    return stream.str();
}


auto semantic_review_note_kind_name(featherdoc::review_note_kind kind)
    -> std::string_view {
    switch (kind) {
    case featherdoc::review_note_kind::footnote:
        return "footnote";
    case featherdoc::review_note_kind::endnote:
        return "endnote";
    case featherdoc::review_note_kind::comment:
        return "comment";
    }

    return "footnote";
}

auto semantic_revision_kind_name(featherdoc::revision_kind kind) -> std::string_view {
    switch (kind) {
    case featherdoc::revision_kind::insertion:
        return "insertion";
    case featherdoc::revision_kind::deletion:
        return "deletion";
    case featherdoc::revision_kind::move_from:
        return "move_from";
    case featherdoc::revision_kind::move_to:
        return "move_to";
    case featherdoc::revision_kind::paragraph_property_change:
        return "paragraph_property_change";
    case featherdoc::revision_kind::run_property_change:
        return "run_property_change";
    case featherdoc::revision_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto semantic_review_note_summary_value(
    const featherdoc::review_note_summary &note) -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_review_note_kind_name(note.kind)
           << "\nid=" << note.id
           << "\nauthor=" << semantic_optional_string(note.author)
           << "\ninitials=" << semantic_optional_string(note.initials)
           << "\ndate=" << semantic_optional_string(note.date)
           << "\nanchor_text=" << semantic_optional_string(note.anchor_text)
           << "\nresolved=" << (note.resolved ? "true" : "false")
           << "\nparent_index=" << semantic_optional_numeric(note.parent_index)
           << "\nparent_id=" << semantic_optional_string(note.parent_id)
           << "\ntext=" << note.text;
    return stream.str();
}

auto semantic_revision_summary_value(const featherdoc::revision_summary &revision)
    -> std::string {
    std::ostringstream stream;
    stream << "kind=" << semantic_revision_kind_name(revision.kind)
           << "\nid=" << revision.id
           << "\nauthor=" << semantic_optional_string(revision.author)
           << "\ndate=" << semantic_optional_string(revision.date)
           << "\npart=" << revision.part_entry_name
           << "\ntext=" << revision.text;
    return stream.str();
}


auto semantic_page_orientation_value(featherdoc::page_orientation orientation)
    -> std::string_view {
    switch (orientation) {
    case featherdoc::page_orientation::portrait:
        return "portrait";
    case featherdoc::page_orientation::landscape:
        return "landscape";
    }

    return "portrait";
}

auto semantic_section_part_value(
    const featherdoc::section_part_inspection_summary &part) -> std::string {
    std::ostringstream stream;
    stream << "has_default=" << semantic_bool(part.has_default)
           << ";has_first=" << semantic_bool(part.has_first)
           << ";has_even=" << semantic_bool(part.has_even)
           << ";default_linked="
           << semantic_bool(part.default_linked_to_previous)
           << ";first_linked=" << semantic_bool(part.first_linked_to_previous)
           << ";even_linked=" << semantic_bool(part.even_linked_to_previous)
           << ";default_entry=" << semantic_optional_string(part.default_entry_name)
           << ";first_entry=" << semantic_optional_string(part.first_entry_name)
           << ";even_entry=" << semantic_optional_string(part.even_entry_name)
           << ";resolved_default="
           << semantic_optional_string(part.resolved_default_entry_name)
           << ";resolved_first="
           << semantic_optional_string(part.resolved_first_entry_name)
           << ";resolved_even="
           << semantic_optional_string(part.resolved_even_entry_name)
           << ";resolved_default_section="
           << semantic_optional_numeric(part.resolved_default_section_index)
           << ";resolved_first_section="
           << semantic_optional_numeric(part.resolved_first_section_index)
           << ";resolved_even_section="
           << semantic_optional_numeric(part.resolved_even_section_index);
    return stream.str();
}

auto semantic_section_page_setup_value(
    const std::optional<featherdoc::section_page_setup> &page_setup) -> std::string {
    if (!page_setup.has_value()) {
        return "implicit";
    }

    std::ostringstream stream;
    stream << "orientation=" << semantic_page_orientation_value(page_setup->orientation)
           << ";width=" << page_setup->width_twips
           << ";height=" << page_setup->height_twips
           << ";margin_top=" << page_setup->margins.top_twips
           << ";margin_right=" << page_setup->margins.right_twips
           << ";margin_bottom=" << page_setup->margins.bottom_twips
           << ";margin_left=" << page_setup->margins.left_twips
           << ";margin_header=" << page_setup->margins.header_twips
           << ";margin_footer=" << page_setup->margins.footer_twips
           << ";page_number_start="
           << semantic_optional_numeric(page_setup->page_number_start);
    return stream.str();
}

auto semantic_section_value(
    const featherdoc::section_inspection_summary &section,
    const std::optional<featherdoc::section_page_setup> &page_setup)
    -> std::string {
    std::ostringstream stream;
    stream << "even_and_odd="
           << semantic_bool(section.even_and_odd_headers_enabled)
           << "\ndifferent_first_page="
           << semantic_bool(section.different_first_page_enabled)
           << "\npage_setup=" << semantic_section_page_setup_value(page_setup)
           << "\nheader=" << semantic_section_part_value(section.header)
           << "\nfooter=" << semantic_section_part_value(section.footer);
    return stream.str();
}


struct semantic_template_part_view {
    featherdoc::template_schema_part_selector target{};
    std::string entry_name;
    std::optional<std::size_t> resolved_from_section_index;
    TemplatePart part;
};

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    featherdoc::section_reference_kind reference_kind, TemplatePart part) {
    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::string{part.entry_name()},
                                                std::nullopt,
                                                std::move(part)});
}

void append_semantic_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind,
    std::optional<std::size_t> part_index, TemplatePart part) {
    append_semantic_template_part(
        parts, part_kind, part_index, std::nullopt,
        featherdoc::section_reference_kind::default_reference, std::move(part));
}

void append_resolved_semantic_section_template_part(
    std::vector<semantic_template_part_view> &parts,
    featherdoc::template_schema_part_kind part_kind, std::size_t section_index,
    featherdoc::section_reference_kind reference_kind,
    const std::optional<std::string> &resolved_entry_name,
    std::optional<std::size_t> resolved_from_section_index, TemplatePart part) {
    if (!resolved_entry_name.has_value()) {
        return;
    }

    auto entry_name = std::string{part.entry_name()};
    if (entry_name.empty() || entry_name != *resolved_entry_name) {
        return;
    }

    auto target = featherdoc::template_schema_part_selector{};
    target.part = part_kind;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    parts.push_back(semantic_template_part_view{target,
                                                std::move(entry_name),
                                                resolved_from_section_index,
                                                std::move(part)});
}

auto collect_semantic_template_parts(
    Document &document,
    const featherdoc::document_semantic_diff_options &options)
    -> std::vector<semantic_template_part_view> {
    auto parts = std::vector<semantic_template_part_view>{};
    append_semantic_template_part(parts,
                                  featherdoc::template_schema_part_kind::body,
                                  std::nullopt, document.body_template());

    const auto header_count = document.header_count();
    for (std::size_t index = 0U; index < header_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::header, index,
            document.header_template(index));
    }

    const auto footer_count = document.footer_count();
    for (std::size_t index = 0U; index < footer_count; ++index) {
        append_semantic_template_part(
            parts, featherdoc::template_schema_part_kind::footer, index,
            document.footer_template(index));
    }

    if (!options.compare_resolved_section_template_parts) {
        return parts;
    }

    const auto sections = document.inspect_sections();
    if (document.last_error().code) {
        return parts;
    }

    for (const auto &section : sections.sections) {
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.header.resolved_default_entry_name,
            section.header.resolved_default_section_index,
            document.section_header_template(
                section.header.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::first_page,
            section.header.resolved_first_entry_name,
            section.header.resolved_first_section_index,
            document.section_header_template(
                section.header.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_header,
            section.index, featherdoc::section_reference_kind::even_page,
            section.header.resolved_even_entry_name,
            section.header.resolved_even_section_index,
            document.section_header_template(
                section.header.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::default_reference,
            section.footer.resolved_default_entry_name,
            section.footer.resolved_default_section_index,
            document.section_footer_template(
                section.footer.resolved_default_section_index.value_or(section.index),
                featherdoc::section_reference_kind::default_reference));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::first_page,
            section.footer.resolved_first_entry_name,
            section.footer.resolved_first_section_index,
            document.section_footer_template(
                section.footer.resolved_first_section_index.value_or(section.index),
                featherdoc::section_reference_kind::first_page));
        append_resolved_semantic_section_template_part(
            parts, featherdoc::template_schema_part_kind::section_footer,
            section.index, featherdoc::section_reference_kind::even_page,
            section.footer.resolved_even_entry_name,
            section.footer.resolved_even_section_index,
            document.section_footer_template(
                section.footer.resolved_even_section_index.value_or(section.index),
                featherdoc::section_reference_kind::even_page));
    }

    return parts;
}

auto is_counted_semantic_template_part(
    const featherdoc::template_schema_part_selector &target) -> bool {
    return target.part == featherdoc::template_schema_part_kind::header ||
           target.part == featherdoc::template_schema_part_kind::footer;
}

auto semantic_template_part_targets_equal(
    const featherdoc::template_schema_part_selector &left,
    const featherdoc::template_schema_part_selector &right) -> bool {
    return left.part == right.part && left.part_index == right.part_index &&
           left.section_index == right.section_index &&
           left.reference_kind == right.reference_kind;
}

auto find_matching_semantic_template_part(
    const std::vector<semantic_template_part_view> &parts,
    const std::vector<bool> &matched,
    const featherdoc::template_schema_part_selector &target)
    -> std::optional<std::size_t> {
    for (std::size_t index = 0U; index < parts.size(); ++index) {
        if (!matched[index] &&
            semantic_template_part_targets_equal(parts[index].target, target)) {
            return index;
        }
    }

    return std::nullopt;
}

void accumulate_semantic_template_part_summary(
    featherdoc::document_semantic_diff_category_summary &summary,
    const featherdoc::document_semantic_diff_part_result &part_result) {
    if (part_result.different()) {
        ++summary.changed_count;
    } else {
        ++summary.unchanged_count;
    }
}


struct semantic_field_value {
    std::string path;
    std::string value;
};

auto is_semantic_field_name(std::string_view text) -> bool {
    if (text.empty()) {
        return false;
    }
    for (const auto ch : text) {
        const auto valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_';
        if (!valid) {
            return false;
        }
    }
    return true;
}

auto try_expand_semicolon_fields(std::string_view path, std::string_view value,
                                 std::vector<semantic_field_value> &fields)
    -> bool {
    if (value.find(';') == std::string_view::npos) {
        return false;
    }

    auto expanded = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find(';', start);
        const auto segment = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        if (segment.empty()) {
            return false;
        }
        const auto equals = segment.find('=');
        if (equals == std::string_view::npos || equals == 0U) {
            return false;
        }
        const auto name = segment.substr(0U, equals);
        if (!is_semantic_field_name(name)) {
            return false;
        }
        auto child_path = std::string{path};
        child_path.push_back('.');
        child_path.append(name.data(), name.size());
        expanded.push_back(semantic_field_value{
            std::move(child_path),
            std::string{segment.substr(equals + 1U)}});
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    if (expanded.size() < 2U) {
        return false;
    }
    fields.insert(fields.end(), expanded.begin(), expanded.end());
    return true;
}

auto parse_semantic_field_values(std::string_view value)
    -> std::vector<semantic_field_value> {
    auto top_level = std::vector<semantic_field_value>{};
    std::size_t start = 0U;
    while (start <= value.size()) {
        const auto end = value.find('\n', start);
        const auto line = value.substr(
            start, end == std::string_view::npos ? std::string_view::npos
                                                 : end - start);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos &&
            is_semantic_field_name(line.substr(0U, equals))) {
            top_level.push_back(semantic_field_value{
                std::string{line.substr(0U, equals)},
                std::string{line.substr(equals + 1U)}});
        } else if (!top_level.empty()) {
            top_level.back().value.push_back('\n');
            top_level.back().value.append(line.data(), line.size());
        } else if (!line.empty()) {
            top_level.push_back(semantic_field_value{"value", std::string{line}});
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    auto fields = std::vector<semantic_field_value>{};
    for (const auto &field : top_level) {
        if (!try_expand_semicolon_fields(field.path, field.value, fields)) {
            fields.push_back(field);
        }
    }
    return fields;
}

auto find_semantic_field_value(const std::vector<semantic_field_value> &fields,
                               std::string_view path) -> std::optional<std::string> {
    for (const auto &field : fields) {
        if (field.path == path) {
            return field.value;
        }
    }
    return std::nullopt;
}

auto semantic_field_changes(std::string_view left_value, std::string_view right_value)
    -> std::vector<featherdoc::document_semantic_diff_field_change> {
    const auto left_fields = parse_semantic_field_values(left_value);
    const auto right_fields = parse_semantic_field_values(right_value);
    auto changes = std::vector<featherdoc::document_semantic_diff_field_change>{};

    for (const auto &left_field : left_fields) {
        const auto right_field = find_semantic_field_value(right_fields, left_field.path);
        if (!right_field.has_value() || *right_field != left_field.value) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                left_field.path, left_field.value,
                right_field.value_or(std::string{})});
        }
    }

    for (const auto &right_field : right_fields) {
        if (!find_semantic_field_value(left_fields, right_field.path).has_value()) {
            changes.push_back(featherdoc::document_semantic_diff_field_change{
                right_field.path, std::string{}, right_field.value});
        }
    }

    return changes;
}

auto make_semantic_sequence_change(std::string_view field)
    -> featherdoc::document_semantic_diff_change {
    auto change = featherdoc::document_semantic_diff_change{};
    change.field = std::string{field};
    return change;
}

void append_semantic_added_change(
    std::string_view field, const std::vector<std::string> &right_values,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.added_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::added;
    change.right_index = right_index;
    change.right_value = right_values[right_index];
    changes.push_back(std::move(change));
}

void append_semantic_removed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    std::size_t left_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.removed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::removed;
    change.left_index = left_index;
    change.left_value = left_values[left_index];
    changes.push_back(std::move(change));
}

void append_semantic_changed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::size_t left_index,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.changed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::changed;
    change.left_index = left_index;
    change.right_index = right_index;
    change.left_value = left_values[left_index];
    change.right_value = right_values[right_index];
    change.field_changes = semantic_field_changes(change.left_value, change.right_value);
    changes.push_back(std::move(change));
}

void compare_semantic_values_by_index(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto item_count = std::max(left_values.size(), right_values.size());
    for (std::size_t item_index = 0; item_index < item_count; ++item_index) {
        if (item_index >= left_values.size()) {
            append_semantic_added_change(field, right_values, item_index, summary,
                                         changes);
            continue;
        }

        if (item_index >= right_values.size()) {
            append_semantic_removed_change(field, left_values, item_index, summary,
                                           changes);
            continue;
        }

        if (left_values[item_index] != right_values[item_index]) {
            append_semantic_changed_change(field, left_values, right_values,
                                           item_index, item_index, summary,
                                           changes);
        } else {
            ++summary.unchanged_count;
        }
    }
}

auto can_align_semantic_values(
    std::size_t left_count, std::size_t right_count,
    const featherdoc::document_semantic_diff_options &options) -> bool {
    if (!options.align_sequences_by_content) {
        return false;
    }

    const auto row_count = left_count + 1U;
    const auto column_count = right_count + 1U;
    if (row_count == 0U || column_count == 0U) {
        return false;
    }
    return row_count <= options.alignment_cell_limit / column_count;
}

void compare_semantic_values_by_content_alignment(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto left_count = left_values.size();
    const auto right_count = right_values.size();
    const auto column_count = right_count + 1U;
    auto cost = std::vector<std::size_t>((left_count + 1U) * column_count, 0U);
    const auto cost_index = [column_count](std::size_t left_position,
                                           std::size_t right_position) {
        return left_position * column_count + right_position;
    };

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        cost[cost_index(left_position, 0U)] = left_position;
    }
    for (std::size_t right_position = 1U; right_position <= right_count;
         ++right_position) {
        cost[cost_index(0U, right_position)] = right_position;
    }

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        for (std::size_t right_position = 1U; right_position <= right_count;
             ++right_position) {
            const auto substitution_cost =
                left_values[left_position - 1U] == right_values[right_position - 1U]
                    ? 0U
                    : 1U;
            auto best_cost = cost[cost_index(left_position - 1U,
                                             right_position - 1U)] +
                             substitution_cost;
            const auto removal_cost =
                cost[cost_index(left_position - 1U, right_position)] + 1U;
            if (removal_cost < best_cost) {
                best_cost = removal_cost;
            }
            const auto addition_cost =
                cost[cost_index(left_position, right_position - 1U)] + 1U;
            if (addition_cost < best_cost) {
                best_cost = addition_cost;
            }
            cost[cost_index(left_position, right_position)] = best_cost;
        }
    }

    auto aligned_changes = std::vector<featherdoc::document_semantic_diff_change>{};
    std::size_t left_position = left_count;
    std::size_t right_position = right_count;
    while (left_position > 0U || right_position > 0U) {
        const auto current_cost = cost[cost_index(left_position, right_position)];
        if (left_position > 0U && right_position > 0U) {
            const auto left_index = left_position - 1U;
            const auto right_index = right_position - 1U;
            const auto substitution_cost =
                left_values[left_index] == right_values[right_index] ? 0U : 1U;
            const auto diagonal_cost =
                cost[cost_index(left_position - 1U, right_position - 1U)] +
                substitution_cost;
            if (current_cost == diagonal_cost) {
                if (substitution_cost == 0U) {
                    ++summary.unchanged_count;
                } else {
                    append_semantic_changed_change(field, left_values, right_values,
                                                   left_index, right_index, summary,
                                                   aligned_changes);
                }
                --left_position;
                --right_position;
                continue;
            }
        }

        if (left_position > 0U &&
            current_cost == cost[cost_index(left_position - 1U, right_position)] +
                                1U) {
            append_semantic_removed_change(field, left_values, left_position - 1U,
                                           summary, aligned_changes);
            --left_position;
            continue;
        }

        append_semantic_added_change(field, right_values, right_position - 1U,
                                     summary, aligned_changes);
        --right_position;
    }

    std::reverse(aligned_changes.begin(), aligned_changes.end());
    changes.insert(changes.end(), aligned_changes.begin(), aligned_changes.end());
}

template <typename Item, typename Fingerprint>
void compare_semantic_sequence(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Fingerprint fingerprint,
    const featherdoc::document_semantic_diff_options &options,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto left_values = std::vector<std::string>{};
    auto right_values = std::vector<std::string>{};
    left_values.reserve(left_items.size());
    right_values.reserve(right_items.size());
    for (const auto &left_item : left_items) {
        left_values.push_back(fingerprint(left_item));
    }
    for (const auto &right_item : right_items) {
        right_values.push_back(fingerprint(right_item));
    }

    if (can_align_semantic_values(left_values.size(), right_values.size(), options)) {
        compare_semantic_values_by_content_alignment(left_values, right_values, field,
                                                     summary, changes);
        return;
    }

    compare_semantic_values_by_index(left_values, right_values, field, summary,
                                     changes);
}

template <typename Item, typename Key, typename Fingerprint>
void compare_semantic_sequence_by_key(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Key key, Fingerprint fingerprint,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto matched_right_items = std::vector<bool>(right_items.size(), false);
    for (std::size_t left_index = 0U; left_index < left_items.size(); ++left_index) {
        const auto left_key = key(left_items[left_index]);
        auto right_index = std::optional<std::size_t>{};
        for (std::size_t candidate_index = 0U; candidate_index < right_items.size();
             ++candidate_index) {
            if (!matched_right_items[candidate_index] &&
                key(right_items[candidate_index]) == left_key) {
                right_index = candidate_index;
                break;
            }
        }

        if (!right_index.has_value()) {
            auto values = std::vector<std::string>{fingerprint(left_items[left_index])};
            append_semantic_removed_change(field, values, 0U, summary, changes);
            changes.back().left_index = left_index;
            continue;
        }

        matched_right_items[*right_index] = true;
        const auto left_value = fingerprint(left_items[left_index]);
        const auto right_value = fingerprint(right_items[*right_index]);
        if (left_value != right_value) {
            auto left_values = std::vector<std::string>{left_value};
            auto right_values = std::vector<std::string>{right_value};
            append_semantic_changed_change(field, left_values, right_values, 0U, 0U,
                                           summary, changes);
            changes.back().left_index = left_index;
            changes.back().right_index = *right_index;
        } else {
            ++summary.unchanged_count;
        }
    }

    for (std::size_t right_index = 0U; right_index < right_items.size(); ++right_index) {
        if (!matched_right_items[right_index]) {
            auto values = std::vector<std::string>{fingerprint(right_items[right_index])};
            append_semantic_added_change(field, values, 0U, summary, changes);
            changes.back().right_index = right_index;
        }
    }
}

} // namespace

std::optional<featherdoc::document_semantic_diff_result>
Document::compare_semantic(
    const Document &other,
    featherdoc::document_semantic_diff_options options) const {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before comparing documents",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (!other.is_open()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "right document must be opened before semantic comparison");
        return std::nullopt;
    }

    auto &left = const_cast<Document &>(*this);
    auto &right = const_cast<Document &>(other);
    auto result = featherdoc::document_semantic_diff_result{};

    if (options.compare_paragraphs) {
        const auto left_paragraphs = left.inspect_paragraphs();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_paragraphs = right.inspect_paragraphs();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_paragraphs, right_paragraphs, "paragraph", semantic_paragraph_value,
            options, result.paragraphs, result.paragraph_changes);
    }

    if (options.compare_tables) {
        const auto left_tables = left.inspect_tables();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_tables = right.inspect_tables();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_tables, right_tables, "table",
                                  semantic_table_value, options, result.tables,
                                  result.table_changes);
    }

    if (options.compare_images) {
        const auto left_images = this->drawing_images();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_images = other.drawing_images();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_images, right_images, "image",
            [&options](const featherdoc::drawing_image_info &image) {
                return semantic_image_value(image, options);
            },
            options, result.images, result.image_changes);
    }

    if (options.compare_content_controls) {
        const auto left_controls = this->list_content_controls();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_controls = other.list_content_controls();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(
            left_controls, right_controls, "content_control",
            [&options](const featherdoc::content_control_summary &content_control) {
                return semantic_content_control_value(content_control, options);
            },
            options, result.content_controls, result.content_control_changes);
    }

    if (options.compare_fields) {
        const auto left_fields = left.body_template().list_fields();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_fields = right.body_template().list_fields();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_fields, right_fields, "field",
                                  semantic_field_summary_value, options,
                                  result.fields, result.field_changes);
    }

    if (options.compare_styles) {
        const auto left_styles = left.list_styles();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_styles = right.list_styles();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_styles, right_styles, "style",
            [](const featherdoc::style_summary &style) { return style.style_id; },
            semantic_style_summary_value, result.styles, result.style_changes);
    }

    if (options.compare_numbering) {
        const auto left_numbering = left.list_numbering_definitions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_numbering = right.list_numbering_definitions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence_by_key(
            left_numbering, right_numbering, "numbering",
            [](const featherdoc::numbering_definition_summary &definition) {
                return definition.definition_id;
            },
            semantic_numbering_definition_summary_value, result.numbering,
            result.numbering_changes);
    }

    if (options.compare_footnotes) {
        const auto left_footnotes = left.list_footnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_footnotes = right.list_footnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_footnotes, right_footnotes, "footnote",
                                  semantic_review_note_summary_value, options,
                                  result.footnotes, result.footnote_changes);
    }

    if (options.compare_endnotes) {
        const auto left_endnotes = left.list_endnotes();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_endnotes = right.list_endnotes();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_endnotes, right_endnotes, "endnote",
                                  semantic_review_note_summary_value, options,
                                  result.endnotes, result.endnote_changes);
    }

    if (options.compare_comments) {
        const auto left_comments = left.list_comments();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_comments = right.list_comments();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_comments, right_comments, "comment",
                                  semantic_review_note_summary_value, options,
                                  result.comments, result.comment_changes);
    }

    if (options.compare_revisions) {
        const auto left_revisions = left.list_revisions();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_revisions = right.list_revisions();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }
        compare_semantic_sequence(left_revisions, right_revisions, "revision",
                                  semantic_revision_summary_value, options,
                                  result.revisions, result.revision_changes);
    }

    if (options.compare_template_parts) {
        auto left_parts = collect_semantic_template_parts(left, options);
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        auto right_parts = collect_semantic_template_parts(right, options);
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        for (const auto &part : left_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.left_count;
            }
        }
        for (const auto &part : right_parts) {
            if (is_counted_semantic_template_part(part.target)) {
                ++result.template_parts.right_count;
            }
        }

        auto matched_right_parts = std::vector<bool>(right_parts.size(), false);
        for (auto &left_part : left_parts) {
            const auto right_index = find_matching_semantic_template_part(
                right_parts, matched_right_parts, left_part.target);
            if (!right_index.has_value()) {
                if (is_counted_semantic_template_part(left_part.target)) {
                    ++result.template_parts.removed_count;
                }
                continue;
            }

            auto &right_part = right_parts[*right_index];
            auto part_result = featherdoc::document_semantic_diff_part_result{};
            part_result.target = left_part.target;
            part_result.entry_name = left_part.entry_name;
            if (right_part.entry_name != left_part.entry_name &&
                !right_part.entry_name.empty()) {
                part_result.entry_name += " -> ";
                part_result.entry_name += right_part.entry_name;
            }
            if (left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_header ||
                left_part.target.part ==
                    featherdoc::template_schema_part_kind::section_footer) {
                part_result.left_resolved_from_section_index =
                    left_part.resolved_from_section_index;
                part_result.right_resolved_from_section_index =
                    right_part.resolved_from_section_index;
            }

            if (options.compare_paragraphs) {
                const auto left_paragraphs = left_part.part.inspect_paragraphs();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_paragraphs = right_part.part.inspect_paragraphs();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_paragraphs, right_paragraphs, "paragraph",
                    semantic_paragraph_value, options, part_result.paragraphs,
                    part_result.paragraph_changes);
            }

            if (options.compare_tables) {
                const auto left_tables = left_part.part.inspect_tables();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_tables = right_part.part.inspect_tables();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_tables, right_tables, "table",
                                          semantic_table_value, options,
                                          part_result.tables,
                                          part_result.table_changes);
            }

            if (options.compare_images) {
                const auto left_images = left_part.part.drawing_images();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_images = right_part.part.drawing_images();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_images, right_images, "image",
                    [&options](const featherdoc::drawing_image_info &image) {
                        return semantic_image_value(image, options);
                    },
                    options, part_result.images, part_result.image_changes);
            }

            if (options.compare_content_controls) {
                const auto left_controls = left_part.part.list_content_controls();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_controls = right_part.part.list_content_controls();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(
                    left_controls, right_controls, "content_control",
                    [&options](
                        const featherdoc::content_control_summary &content_control) {
                        return semantic_content_control_value(content_control, options);
                    },
                    options, part_result.content_controls,
                    part_result.content_control_changes);
            }

            if (options.compare_fields) {
                const auto left_fields = left_part.part.list_fields();
                if (this->last_error_info.code) {
                    return std::nullopt;
                }
                const auto right_fields = right_part.part.list_fields();
                if (other.last_error().code) {
                    this->last_error_info = other.last_error();
                    return std::nullopt;
                }
                compare_semantic_sequence(left_fields, right_fields, "field",
                                          semantic_field_summary_value, options,
                                          part_result.fields,
                                          part_result.field_changes);
            }

            if (is_counted_semantic_template_part(part_result.target)) {
                accumulate_semantic_template_part_summary(result.template_parts,
                                                          part_result);
            }
            result.template_part_results.push_back(std::move(part_result));
            matched_right_parts[*right_index] = true;
        }

        for (std::size_t index = 0U; index < right_parts.size(); ++index) {
            if (!matched_right_parts[index] &&
                is_counted_semantic_template_part(right_parts[index].target)) {
                ++result.template_parts.added_count;
            }
        }
    }

    if (options.compare_sections) {
        const auto left_sections = left.inspect_sections();
        if (this->last_error_info.code) {
            return std::nullopt;
        }
        const auto right_sections = right.inspect_sections();
        if (other.last_error().code) {
            this->last_error_info = other.last_error();
            return std::nullopt;
        }

        auto left_section_values = std::vector<std::string>{};
        auto right_section_values = std::vector<std::string>{};
        left_section_values.reserve(left_sections.sections.size());
        right_section_values.reserve(right_sections.sections.size());
        for (const auto &section : left_sections.sections) {
            auto page_setup = this->get_section_page_setup(section.index);
            if (this->last_error_info.code) {
                return std::nullopt;
            }
            left_section_values.push_back(semantic_section_value(section, page_setup));
        }
        for (const auto &section : right_sections.sections) {
            auto page_setup = other.get_section_page_setup(section.index);
            if (other.last_error().code) {
                this->last_error_info = other.last_error();
                return std::nullopt;
            }
            right_section_values.push_back(semantic_section_value(section, page_setup));
        }

        result.sections.left_count = left_section_values.size();
        result.sections.right_count = right_section_values.size();
        if (can_align_semantic_values(left_section_values.size(),
                                      right_section_values.size(), options)) {
            compare_semantic_values_by_content_alignment(
                left_section_values, right_section_values, "section", result.sections,
                result.section_changes);
        } else {
            compare_semantic_values_by_index(left_section_values, right_section_values,
                                             "section", result.sections,
                                             result.section_changes);
        }
    }

    this->last_error_info.clear();
    return result;
}

} // namespace featherdoc
