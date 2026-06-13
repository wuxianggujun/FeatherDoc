#include "featherdoc_cli_table_position_options_parse.hpp"

#include "featherdoc_cli_table_position_plan_build.hpp"

namespace featherdoc_cli {

void apply_table_position_preset_defaults(table_position_options &options) {
    if (!options.preset.has_value()) {
        return;
    }

    const auto preset_position = make_table_position_preset(*options.preset);
    if (!options.has_horizontal_reference) {
        options.position.horizontal_reference =
            preset_position.horizontal_reference;
        options.has_horizontal_reference = true;
    }
    if (!options.has_horizontal_offset) {
        options.position.horizontal_offset_twips =
            preset_position.horizontal_offset_twips;
        options.has_horizontal_offset = true;
    }
    if (!options.has_horizontal_spec &&
        preset_position.horizontal_spec.has_value()) {
        options.position.horizontal_spec = preset_position.horizontal_spec;
        options.has_horizontal_spec = true;
    }
    if (!options.has_vertical_reference) {
        options.position.vertical_reference = preset_position.vertical_reference;
        options.has_vertical_reference = true;
    }
    if (!options.has_vertical_offset) {
        options.position.vertical_offset_twips =
            preset_position.vertical_offset_twips;
        options.has_vertical_offset = true;
    }
    if (!options.has_vertical_spec && preset_position.vertical_spec.has_value()) {
        options.position.vertical_spec = preset_position.vertical_spec;
        options.has_vertical_spec = true;
    }
    if (!options.position.left_from_text_twips.has_value()) {
        options.position.left_from_text_twips =
            preset_position.left_from_text_twips;
    }
    if (!options.position.right_from_text_twips.has_value()) {
        options.position.right_from_text_twips =
            preset_position.right_from_text_twips;
    }
    if (!options.position.top_from_text_twips.has_value()) {
        options.position.top_from_text_twips =
            preset_position.top_from_text_twips;
    }
    if (!options.position.bottom_from_text_twips.has_value()) {
        options.position.bottom_from_text_twips =
            preset_position.bottom_from_text_twips;
    }
    if (!options.position.overlap.has_value()) {
        options.position.overlap = preset_position.overlap;
    }
}

} // namespace featherdoc_cli
