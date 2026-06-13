#include "document_semantic_diff_values.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc {

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

} // namespace featherdoc
