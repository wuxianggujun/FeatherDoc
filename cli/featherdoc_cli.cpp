#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_diff.hpp"
#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_options_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"
#include "featherdoc_cli_numbering_catalog_commands.hpp"
#include "featherdoc_cli_numbering_inspect_commands.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_bookmark_commands.hpp"
#include "featherdoc_cli_content_control_commands.hpp"
#include "featherdoc_cli_page_setup_commands.hpp"
#include "featherdoc_cli_paragraph_inspect_commands.hpp"
#include "featherdoc_cli_paragraph_numbering_commands.hpp"
#include "featherdoc_cli_paragraph_run_commands.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_document_content_commands.hpp"
#include "featherdoc_cli_semantic_diff.hpp"
#include "featherdoc_cli_semantic_diff_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_commands.hpp"
#include "featherdoc_cli_style_ensure_commands.hpp"
#include "featherdoc_cli_style_inspect_commands.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_cell_style_commands.hpp"
#include "featherdoc_cli_table_column_commands.hpp"
#include "featherdoc_cli_table_format_commands.hpp"
#include "featherdoc_cli_table_inspect_commands.hpp"
#include "featherdoc_cli_table_merge_commands.hpp"
#include "featherdoc_cli_table_row_commands.hpp"
#include "featherdoc_cli_table_row_summary.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"
#include "featherdoc_cli_table_structure_commands.hpp"
#include "featherdoc_cli_table_style_commands.hpp"
#include "featherdoc_cli_table_text_commands.hpp"
#include "featherdoc_cli_template_table_inspect_commands.hpp"
#include "featherdoc_cli_template_table_column_commands.hpp"
#include "featherdoc_cli_template_table_json_patch_commands.hpp"
#include "featherdoc_cli_template_table_merge_commands.hpp"
#include "featherdoc_cli_template_table_row_commands.hpp"
#include "featherdoc_cli_template_table_text_commands.hpp"
#include "featherdoc_cli_pdf_commands.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"
#include "featherdoc_cli_table_position_commands.hpp"
#include "featherdoc_cli_table_position_output.hpp"
#include "featherdoc_cli_table_position_plan_build.hpp"
#include "featherdoc_cli_table_position_plan_parse.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_style_refactor_commands.hpp"
#include "featherdoc_cli_style_numbering_commands.hpp"
#include "featherdoc_cli_review_commands.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"
#include "featherdoc_cli_run_recipe_commands.hpp"
#include "featherdoc_cli_section_part_commands.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
#include "featherdoc_cli_template_schema_model.hpp"
#include "featherdoc_cli_template_schema_commands.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_options_parse.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"
#include "featherdoc_cli_template_schema_patch_ops.hpp"
#include "featherdoc_cli_template_schema_patch_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_field_commands.hpp"
#include "featherdoc_cli_image_commands.hpp"
#include "featherdoc_cli_image_output.hpp"
#include "featherdoc_cli_image_options_parse.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_inspect_table_item_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_usage.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using featherdoc_cli::json_escape;
using featherdoc_cli::apply_numbering_catalog_patch;
using featherdoc_cli::apply_template_schema_patch;
using featherdoc_cli::bookmark_kind_name;
using featherdoc_cli::border_style_name;
using featherdoc_cli::cell_border_edge_name;
using featherdoc_cli::cell_margin_edge_name;
using featherdoc_cli::cell_text_direction_name;
using featherdoc_cli::cell_vertical_alignment_name;
using featherdoc_cli::changed_numbering_catalog_definition;
using featherdoc_cli::changed_numbering_catalog_level;
using featherdoc_cli::changed_numbering_catalog_override;
using featherdoc_cli::check_numbering_catalog_options;
using featherdoc_cli::content_control_form_kind_name;
using featherdoc_cli::content_control_kind_name;
using featherdoc_cli::changed_template_schema_target;
using featherdoc_cli::build_table_position_preset_plan;
using featherdoc_cli::build_template_schema_patch_options;
using featherdoc_cli::check_template_schema_options;
using featherdoc_cli::compare_template_schema_target;
using featherdoc_cli::compare_template_schema_target_identity;
using featherdoc_cli::compare_template_schema_target_selector;
using featherdoc_cli::compare_template_slot_requirement;
using featherdoc_cli::compare_template_slot_requirement_shape;
using featherdoc_cli::describe_table_position_fingerprint_differences;
using featherdoc_cli::diff_numbering_catalogs;
using featherdoc_cli::exported_template_schema_result;
using featherdoc_cli::exported_template_schema_skipped_bookmark;
using featherdoc_cli::exported_template_schema_target;
using featherdoc_cli::export_numbering_catalog_options;
using featherdoc_cli::export_template_schema_options;
using featherdoc_cli::format_paragraph_text;
using featherdoc_cli::has_json_flag;
using featherdoc_cli::is_bookmark_command;
using featherdoc_cli::is_content_control_command;
using featherdoc_cli::is_document_content_command;
using featherdoc_cli::is_field_command;
using featherdoc_cli::is_image_command;
using featherdoc_cli::is_numbering_catalog_command;
using featherdoc_cli::is_numbering_inspect_command;
using featherdoc_cli::is_page_setup_command;
using featherdoc_cli::is_review_command;
using featherdoc_cli::is_style_inspect_command;
using featherdoc_cli::is_style_numbering_command;
using featherdoc_cli::is_style_refactor_command;
using featherdoc_cli::is_table_cell_style_command;
using featherdoc_cli::is_table_format_command;
using featherdoc_cli::is_table_position_command;
using featherdoc_cli::is_table_style_command;
using featherdoc_cli::is_table_structure_command;
using featherdoc_cli::is_template_schema_command;
using featherdoc_cli::is_paragraph_inspect_command;
using featherdoc_cli::inspect_table_cells_options;
using featherdoc_cli::inspect_table_rows_options;
using featherdoc_cli::table_merge_direction;
using featherdoc_cli::table_merge_direction_name;
using featherdoc_cli::table_cell_text_options;
using featherdoc_cli::merge_table_cells_options;
using featherdoc_cli::unmerge_table_cells_options;
using featherdoc_cli::table_cell_style_options;
using featherdoc_cli::append_table_row_options;
using featherdoc_cli::table_cell_border_options;
using featherdoc_cli::parse_table_merge_direction;
using featherdoc_cli::parse_table_cell_text_options;
using featherdoc_cli::parse_merge_table_cells_options;
using featherdoc_cli::parse_unmerge_table_cells_options;
using featherdoc_cli::parse_table_cell_style_options;
using featherdoc_cli::parse_append_table_row_options;
using featherdoc_cli::parse_table_cell_border_options;
using featherdoc_cli::validate_template_options;
using featherdoc_cli::validate_template_schema_options;
using featherdoc_cli::validate_template_schema_target_options;
using featherdoc_cli::simple_document_mutation_options;
using featherdoc_cli::review_note_mutation_options;
using featherdoc_cli::comment_mutation_options;
using featherdoc_cli::comment_metadata_mutation_options;
using featherdoc_cli::revision_authoring_options;
using featherdoc_cli::revision_metadata_mutation_options;
using featherdoc_cli::semantic_diff_options;
using featherdoc_cli::cli_text_source_options;
using featherdoc_cli::bookmark_text_binding_source;
using featherdoc_cli::import_numbering_catalog_options;
using featherdoc_cli::json_bool;
using featherdoc_cli::list_kind_name;
using featherdoc_cli::lint_numbering_catalog;
using featherdoc_cli::lint_template_schema;
using featherdoc_cli::lint_template_schema_options;
using featherdoc_cli::lower_ascii_copy;
using featherdoc_cli::make_table_position_preset;
using featherdoc_cli::make_table_position_table_fingerprint;
using featherdoc_cli::make_template_schema_patch_target_selector;
using featherdoc_cli::make_template_schema_slot_update;
using featherdoc_cli::merge_template_schema_options;
using featherdoc_cli::normalize_word_part_entry;
using featherdoc_cli::numbering_catalog_lint_issue;
using featherdoc_cli::numbering_catalog_lint_issue_kind;
using featherdoc_cli::numbering_catalog_lint_issue_name;
using featherdoc_cli::numbering_catalog_lint_result;
using featherdoc_cli::numbering_catalog_diff_result;
using featherdoc_cli::numbering_catalog_instance_diff_result;
using featherdoc_cli::optional_display_value;
using featherdoc_cli::optional_size_display_value;
using featherdoc_cli::output_semantic_diff_result;
using featherdoc_cli::page_orientation_name;
using featherdoc_cli::parse_border_style_text;
using featherdoc_cli::parse_cell_border_edge_text;
using featherdoc_cli::parse_cell_margin_edge_text;
using featherdoc_cli::parse_cell_text_direction_text;
using featherdoc_cli::parse_cell_vertical_alignment_text;
using featherdoc_cli::parse_floating_image_horizontal_reference;
using featherdoc_cli::parse_floating_image_vertical_reference;
using featherdoc_cli::parse_floating_image_wrap_mode;
using featherdoc_cli::parse_bool;
using featherdoc_cli::option_parse_result;
using featherdoc_cli::parse_export_template_schema_options;
using featherdoc_cli::parse_index;
using featherdoc_cli::parse_inspect_table_cells_options;
using featherdoc_cli::parse_inspect_table_rows_options;
using featherdoc_cli::parse_simple_document_mutation_options;
using featherdoc_cli::parse_review_note_mutation_options;
using featherdoc_cli::parse_comment_mutation_options;
using featherdoc_cli::parse_comment_metadata_mutation_options;
using featherdoc_cli::parse_revision_authoring_options;
using featherdoc_cli::parse_revision_metadata_mutation_options;
using featherdoc_cli::parse_semantic_diff_options;
using featherdoc_cli::parse_validate_template_options;
using featherdoc_cli::parse_validate_template_schema_options;
using featherdoc_cli::parse_int32;
using featherdoc_cli::parse_build_template_schema_patch_options;
using featherdoc_cli::parse_check_numbering_catalog_options;
using featherdoc_cli::parse_check_template_schema_options;
using featherdoc_cli::parse_diff_template_schema_options;
using featherdoc_cli::parse_export_numbering_catalog_options;
using featherdoc_cli::parse_import_numbering_catalog_options;
using featherdoc_cli::parse_lint_template_schema_options;
using featherdoc_cli::parse_merge_template_schema_options;
using featherdoc_cli::parse_normalize_template_schema_options;
using featherdoc_cli::parse_patch_numbering_catalog_options;
using featherdoc_cli::parse_patch_template_schema_options;
using featherdoc_cli::parse_preview_template_schema_patch_options;
using featherdoc_cli::parse_repair_template_schema_options;
using featherdoc_cli::parse_section_text_options;
using featherdoc_cli::parse_list_kind;
using featherdoc_cli::parse_numbering_catalog_level_definition;
using featherdoc_cli::parse_table_position_preset;
using featherdoc_cli::parse_page_orientation;
using featherdoc_cli::parse_reference_kind;
using featherdoc_cli::parse_row_height_rule_text;
using featherdoc_cli::parse_table_alignment_text;
using featherdoc_cli::parse_table_border_edge_text;
using featherdoc_cli::parse_table_layout_mode_text;
using featherdoc_cli::parse_table_style_border_edge_text;
using featherdoc_cli::parse_table_style_border_style_text;
using featherdoc_cli::parse_table_style_cell_text_direction_text;
using featherdoc_cli::parse_table_style_cell_vertical_alignment_text;
using featherdoc_cli::parse_table_style_margin_edge_text;
using featherdoc_cli::parse_table_style_paragraph_alignment_text;
using featherdoc_cli::parse_table_style_paragraph_line_spacing_rule_text;
using featherdoc_cli::parse_table_overlap_text;
using featherdoc_cli::parse_apply_table_position_plan_options;
using featherdoc_cli::parse_plan_table_position_presets_options;
using featherdoc_cli::parse_table_position_horizontal_reference;
using featherdoc_cli::parse_table_position_horizontal_spec;
using featherdoc_cli::parse_table_position_options;
using featherdoc_cli::parse_table_position_target_options;
using featherdoc_cli::parse_table_position_vertical_reference;
using featherdoc_cli::parse_table_position_vertical_spec;
using featherdoc_cli::parse_template_schema_json_target;
using featherdoc_cli::parse_template_slot_kind;
using featherdoc_cli::parse_template_slot_requirement;
using featherdoc_cli::parse_uint32;
using featherdoc_cli::parse_validation_part;
using featherdoc_cli::parsed_table_position_plan_file;
using featherdoc_cli::patched_template_schema_summary;
using featherdoc_cli::patch_template_schema_options;
using featherdoc_cli::patch_numbering_catalog_options;
using featherdoc_cli::preview_template_schema_patch_options;
using featherdoc_cli::print_error_info;
using featherdoc_cli::print_parse_error;
using featherdoc_cli::print_usage;
using featherdoc_cli::report_document_error;
using featherdoc_cli::report_operation_failure;
using featherdoc_cli::resolve_body_table;
using featherdoc_cli::resolve_body_table_cell;
using featherdoc_cli::resolve_body_table_row;
using featherdoc_cli::read_numbering_catalog_patch_file;
using featherdoc_cli::read_review_mutation_plan_build_request_file;
using featherdoc_cli::read_review_mutation_plan_file;
using featherdoc_cli::read_table_position_plan_file;
using featherdoc_cli::read_template_schema_patch_file;
using featherdoc_cli::read_template_schema_file;
using featherdoc_cli::repaired_template_schema_summary;
using featherdoc_cli::repair_template_schema_result;
using featherdoc_cli::diff_template_schema_options;
using featherdoc_cli::normalize_template_schema_options;
using featherdoc_cli::repair_template_schema_options;
using featherdoc_cli::section_text_options;
using featherdoc_cli::review_mutation_plan_operation;
using featherdoc_cli::review_mutation_plan_operation_kind_name;
using featherdoc_cli::review_note_kind_name;
using featherdoc_cli::revision_kind_name;
using featherdoc_cli::row_height_rule_name;
using featherdoc_cli::run_style_refactor_command;
using featherdoc_cli::run_table_cell_style_command;
using featherdoc_cli::run_table_format_command;
using featherdoc_cli::run_table_position_command;
using featherdoc_cli::run_table_style_command;
using featherdoc_cli::run_table_structure_command;
using featherdoc_cli::run_recipe_command;
using featherdoc_cli::style_usage_hit_kind_name;
using featherdoc_cli::style_usage_part_kind_name;
using featherdoc_cli::table_alignment_name;
using featherdoc_cli::table_border_edge_name;
using featherdoc_cli::table_layout_mode_name;
using featherdoc_cli::table_overlap_name;
using featherdoc_cli::apply_table_position_plan_options;
using featherdoc_cli::plan_table_position_presets_options;
using featherdoc_cli::table_position_options;
using featherdoc_cli::table_position_preset;
using featherdoc_cli::table_position_preset_plan;
using featherdoc_cli::table_position_preset_plan_item;
using featherdoc_cli::table_position_preset_name;
using featherdoc_cli::table_position_target_options;
using featherdoc_cli::table_position_table_fingerprints_equal;
using featherdoc_cli::table_position_table_fingerprint;
using featherdoc_cli::table_position_horizontal_reference_name;
using featherdoc_cli::table_position_horizontal_spec_name;
using featherdoc_cli::table_position_vertical_reference_name;
using featherdoc_cli::table_position_vertical_spec_name;
using featherdoc_cli::table_style_cell_text_direction_name;
using featherdoc_cli::table_style_cell_vertical_alignment_name;
using featherdoc_cli::table_style_paragraph_alignment_name;
using featherdoc_cli::table_style_paragraph_line_spacing_rule_name;
using featherdoc_cli::template_slot_kind_name;
using featherdoc_cli::template_slot_source_json_name;
using featherdoc_cli::template_slot_source_new_json_name;
using featherdoc_cli::template_slot_source_text_name;
using featherdoc_cli::template_schema_diff_result;
using featherdoc_cli::template_schema_lint_issue;
using featherdoc_cli::template_schema_lint_issue_kind;
using featherdoc_cli::template_schema_lint_result;
using featherdoc_cli::template_schema_lint_issue_name;
using featherdoc_cli::built_template_schema_patch_summary;
using featherdoc_cli::build_template_schema_patch_document;
using featherdoc_cli::diff_template_schema_results;
using featherdoc_cli::has_template_schema_resolved_target_metadata;
using featherdoc_cli::merge_template_schema_result;
using featherdoc_cli::merged_template_schema_summary;
using featherdoc_cli::normalize_template_schema_result;
using featherdoc_cli::template_schema_patch_document;
using featherdoc_cli::template_schema_patch_remove_slot;
using featherdoc_cli::template_schema_patch_rename_slot;
using featherdoc_cli::template_schema_patch_target_selector;
using featherdoc_cli::template_schema_patch_update_slot;
using featherdoc_cli::make_table_row_summary;
using featherdoc_cli::numbering_catalog_level_patch;
using featherdoc_cli::numbering_catalog_override_patch;
using featherdoc_cli::numbering_catalog_patch_document;
using featherdoc_cli::numbering_catalog_patch_summary;
using featherdoc_cli::validation_part_family;
using featherdoc_cli::validation_part_name;
using featherdoc_cli::validate_template_part_selection;
using featherdoc_cli::strip_utf8_bom;
using featherdoc_cli::write_json_lines;
using featherdoc_cli::write_json_optional_bool;
using featherdoc_cli::write_json_optional_bool_value;
using featherdoc_cli::write_json_optional_double;
using featherdoc_cli::write_json_optional_size;
using featherdoc_cli::write_json_optional_string;
using featherdoc_cli::write_json_optional_u32;
using featherdoc_cli::write_json_optional_u32_value;
using featherdoc_cli::write_json_size_array;
using featherdoc_cli::write_json_optional_table_position;
using featherdoc_cli::write_table_position_text;
using featherdoc_cli::write_json_strings;
using featherdoc_cli::write_json_string;
using featherdoc_cli::write_json_command_error;
using featherdoc_cli::write_json_review_mutation_plan_document;
using featherdoc_cli::write_review_mutation_plan_file;
using featherdoc_cli::open_document;
using featherdoc_cli::path_type;
using featherdoc_cli::read_text_source;
using featherdoc_cli::run_export_pdf_command;
using featherdoc_cli::run_append_table_row_command;
using featherdoc_cli::run_append_template_table_row_command;
using featherdoc_cli::run_clear_paragraph_style_command;
using featherdoc_cli::run_clear_paragraph_list_command;
using featherdoc_cli::run_clear_default_run_properties_command;
using featherdoc_cli::run_clear_paragraph_style_properties_command;
using featherdoc_cli::run_clear_paragraph_style_numbering_command;
using featherdoc_cli::run_clear_run_font_family_command;
using featherdoc_cli::run_clear_run_language_command;
using featherdoc_cli::run_clear_run_style_command;
using featherdoc_cli::run_clear_style_run_properties_command;
using featherdoc_cli::run_inspect_default_run_properties_command;
using featherdoc_cli::run_inspect_paragraph_style_properties_command;
using featherdoc_cli::run_inspect_style_run_properties_command;
using featherdoc_cli::run_inspect_table_cells_command;
using featherdoc_cli::run_inspect_table_rows_command;
using featherdoc_cli::run_inspect_tables_command;
using featherdoc_cli::run_materialize_style_run_properties_command;
using featherdoc_cli::run_rebase_character_style_based_on_command;
using featherdoc_cli::run_rebase_paragraph_style_based_on_command;
using featherdoc_cli::run_ensure_numbering_definition_command;
using featherdoc_cli::run_ensure_style_linked_numbering_command;
using featherdoc_cli::run_insert_paragraph_after_table_command;
using featherdoc_cli::run_insert_table_column_after_command;
using featherdoc_cli::run_insert_table_column_before_command;
using featherdoc_cli::run_insert_table_row_after_command;
using featherdoc_cli::run_insert_table_row_before_command;
using featherdoc_cli::run_inspect_template_table_cells_command;
using featherdoc_cli::run_inspect_template_table_rows_command;
using featherdoc_cli::run_inspect_template_tables_command;
using featherdoc_cli::run_set_table_cell_text_command;
using featherdoc_cli::run_merge_table_cells_command;
using featherdoc_cli::run_insert_template_table_column_after_command;
using featherdoc_cli::run_insert_template_table_column_before_command;
using featherdoc_cli::run_insert_template_table_row_after_command;
using featherdoc_cli::run_insert_template_table_row_before_command;
using featherdoc_cli::run_merge_template_table_cells_command;
using featherdoc_cli::run_remove_template_table_column_command;
using featherdoc_cli::run_remove_template_table_row_command;
using featherdoc_cli::run_clear_table_row_cant_split_command;
using featherdoc_cli::run_clear_table_row_height_command;
using featherdoc_cli::run_clear_table_row_repeat_header_command;
using featherdoc_cli::run_remove_table_column_command;
using featherdoc_cli::run_remove_table_row_command;
using featherdoc_cli::run_restart_paragraph_list_command;
using featherdoc_cli::run_set_paragraph_style_command;
using featherdoc_cli::run_set_default_run_properties_command;
using featherdoc_cli::run_set_paragraph_list_command;
using featherdoc_cli::run_set_paragraph_numbering_command;
using featherdoc_cli::run_set_paragraph_style_properties_command;
using featherdoc_cli::run_set_paragraph_style_numbering_command;
using featherdoc_cli::run_set_run_font_family_command;
using featherdoc_cli::run_set_run_language_command;
using featherdoc_cli::run_set_run_style_command;
using featherdoc_cli::run_set_style_run_properties_command;
using featherdoc_cli::run_set_table_row_cant_split_command;
using featherdoc_cli::run_set_table_row_height_command;
using featherdoc_cli::run_set_table_row_repeat_header_command;
using featherdoc_cli::run_set_template_table_cell_block_texts_command;
using featherdoc_cli::run_set_template_table_cell_text_command;
using featherdoc_cli::run_set_template_table_from_json_command;
using featherdoc_cli::run_set_template_table_row_texts_command;
using featherdoc_cli::run_set_template_tables_from_json_command;
using featherdoc_cli::run_unmerge_table_cells_command;
using featherdoc_cli::run_unmerge_template_table_cells_command;
using featherdoc_cli::collect_table_row_summaries;
using featherdoc_cli::table_row_inspection_summary;
#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
using featherdoc_cli::run_import_pdf_command;
#endif
using featherdoc_cli::is_section_part_command;
using featherdoc_cli::run_image_command;
using featherdoc_cli::run_bookmark_command;
using featherdoc_cli::run_content_control_command;
using featherdoc_cli::run_document_content_command;
using featherdoc_cli::run_field_command;
using featherdoc_cli::run_numbering_catalog_command;
using featherdoc_cli::run_numbering_inspect_command;
using featherdoc_cli::run_page_setup_command;
using featherdoc_cli::run_paragraph_inspect_command;
using featherdoc_cli::run_review_command;
using featherdoc_cli::run_template_schema_command;
using featherdoc_cli::save_document;
using featherdoc_cli::run_section_part_command;
using featherdoc_cli::run_ensure_character_style_command;
using featherdoc_cli::run_ensure_paragraph_style_command;
using featherdoc_cli::run_style_inspect_command;
using featherdoc_cli::run_style_numbering_command;
using featherdoc_cli::write_json_mutation_result;
using featherdoc_cli::write_json_numbering_catalog_import_summary;
using featherdoc_cli::write_json_numbering_instance_summary;
using featherdoc_cli::write_json_numbering_level_override_summary;
using featherdoc_cli::write_json_numbering_level_definition;
using featherdoc_cli::write_json_paragraph_style_numbering_link;
using featherdoc_cli::yes_no;

enum class section_part_family {
    header,
    footer,
};

void write_json_bookmark_summary(std::ostream &stream,
                                 const featherdoc::bookmark_summary &bookmark) {
    stream << "{\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark_name);
    stream << ",\"occurrence_count\":" << bookmark.occurrence_count
           << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.kind));
    stream << ",\"is_duplicate\":" << json_bool(bookmark.is_duplicate()) << '}';
}


void print_bookmark_summary(std::ostream &stream,
                            const featherdoc::bookmark_summary &bookmark) {
    stream << "name=" << bookmark.bookmark_name
           << " occurrences=" << bookmark.occurrence_count
           << " kind=" << bookmark_kind_name(bookmark.kind)
           << " duplicate=" << yes_no(bookmark.is_duplicate());
}


} // namespace

int featherdoc_cli_main(int argc, char **argv) {
    const std::vector<std::string_view> arguments(argv + 1, argv + argc);
    if (arguments.empty()) {
        print_usage(std::cerr);
        return 2;
    }

    const auto command = arguments.front();
    featherdoc::Document doc;

    if (command == "run-recipe") {
        return run_recipe_command(command, arguments);
    }

    if (command == "export-pdf") {
        return run_export_pdf_command(command, arguments, doc);
    }

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
    if (command == "import-pdf") {
        return run_import_pdf_command(command, arguments, doc);
    }
#endif

    if (is_section_part_command(command)) {
        return run_section_part_command(command, arguments, doc);
    }

    if (is_style_inspect_command(command)) {
        return run_style_inspect_command(command, arguments, doc);
    }

    if (is_style_numbering_command(command)) {
        return run_style_numbering_command(command, arguments, doc);
    }

    if (is_table_style_command(command)) {
        return run_table_style_command(command, arguments, doc);
    }

    if (is_table_cell_style_command(command)) {
        return run_table_cell_style_command(command, arguments, doc);
    }

    if (is_table_format_command(command)) {
        return run_table_format_command(command, arguments, doc);
    }

    if (is_style_refactor_command(command)) {
        return run_style_refactor_command(command, arguments, doc);
    }

    if (is_numbering_catalog_command(command)) {
        return run_numbering_catalog_command(command, arguments, doc);
    }

    if (is_numbering_inspect_command(command)) {
        return run_numbering_inspect_command(command, arguments, doc);
    }

    if (is_paragraph_inspect_command(command)) {
        return run_paragraph_inspect_command(command, arguments);
    }

    if (command == "inspect-tables") {
        return run_inspect_tables_command(command, arguments);
    }

    if (is_table_position_command(command)) {
        return run_table_position_command(command, arguments);
    }

    if (command == "inspect-table-rows") {
        return run_inspect_table_rows_command(command, arguments);
    }

    if (command == "inspect-table-cells") {
        return run_inspect_table_cells_command(command, arguments);
    }

    if (command == "set-table-cell-text") {
        return run_set_table_cell_text_command(command, arguments);
    }

    if (command == "merge-table-cells") {
        return run_merge_table_cells_command(command, arguments);
    }

    if (command == "unmerge-table-cells") {
        return run_unmerge_table_cells_command(command, arguments);
    }

    if (is_table_structure_command(command)) {
        return run_table_structure_command(command, arguments);
    }

    if (command == "append-table-row") {
        return run_append_table_row_command(command, arguments);
    }

    if (command == "insert-table-row-before") {
        return run_insert_table_row_before_command(command, arguments);
    }

    if (command == "insert-table-row-after") {
        return run_insert_table_row_after_command(command, arguments);
    }

    if (command == "remove-table-row") {
        return run_remove_table_row_command(command, arguments);
    }

    if (command == "insert-paragraph-after-table") {
        return run_insert_paragraph_after_table_command(command, arguments);
    }

    if (command == "insert-table-column-before") {
        return run_insert_table_column_before_command(command, arguments);
    }

    if (command == "insert-table-column-after") {
        return run_insert_table_column_after_command(command, arguments);
    }

    if (command == "remove-table-column") {
        return run_remove_table_column_command(command, arguments);
    }

    if (command == "set-table-row-height") {
        return run_set_table_row_height_command(command, arguments);
    }

    if (command == "clear-table-row-height") {
        return run_clear_table_row_height_command(command, arguments);
    }

    if (command == "set-table-row-cant-split") {
        return run_set_table_row_cant_split_command(command, arguments);
    }

    if (command == "clear-table-row-cant-split") {
        return run_clear_table_row_cant_split_command(command, arguments);
    }

    if (command == "set-table-row-repeat-header") {
        return run_set_table_row_repeat_header_command(command, arguments);
    }

    if (command == "clear-table-row-repeat-header") {
        return run_clear_table_row_repeat_header_command(command, arguments);
    }

    if (command == "inspect-template-tables") {
        return run_inspect_template_tables_command(command, arguments);
    }

    if (command == "inspect-template-table-rows") {
        return run_inspect_template_table_rows_command(command, arguments);
    }

    if (command == "inspect-template-table-cells") {
        return run_inspect_template_table_cells_command(command, arguments);
    }

    if (command == "set-template-table-cell-text") {
        return run_set_template_table_cell_text_command(command, arguments);
    }

    if (command == "set-template-table-row-texts") {
        return run_set_template_table_row_texts_command(command, arguments);
    }

    if (command == "set-template-table-cell-block-texts") {
        return run_set_template_table_cell_block_texts_command(command,
                                                               arguments);
    }

    if (command == "set-template-table-from-json") {
        return run_set_template_table_from_json_command(command, arguments);
    }

    if (command == "set-template-tables-from-json") {
        return run_set_template_tables_from_json_command(command, arguments);
    }

    if (command == "insert-template-table-column-before") {
        return run_insert_template_table_column_before_command(command, arguments);
    }

    if (command == "insert-template-table-column-after") {
        return run_insert_template_table_column_after_command(command, arguments);
    }

    if (command == "remove-template-table-column") {
        return run_remove_template_table_column_command(command, arguments);
    }

    if (command == "append-template-table-row") {
        return run_append_template_table_row_command(command, arguments);
    }

    if (command == "insert-template-table-row-before") {
        return run_insert_template_table_row_before_command(command, arguments);
    }

    if (command == "insert-template-table-row-after") {
        return run_insert_template_table_row_after_command(command, arguments);
    }

    if (command == "remove-template-table-row") {
        return run_remove_template_table_row_command(command, arguments);
    }

    if (command == "merge-template-table-cells") {
        return run_merge_template_table_cells_command(command, arguments);
    }

    if (command == "unmerge-template-table-cells") {
        return run_unmerge_template_table_cells_command(command, arguments);
    }

    if (command == "set-paragraph-style") {
        return run_set_paragraph_style_command(command, arguments);
    }

    if (command == "clear-paragraph-style") {
        return run_clear_paragraph_style_command(command, arguments);
    }

    if (command == "set-run-style") {
        return run_set_run_style_command(command, arguments);
    }

    if (command == "clear-run-style") {
        return run_clear_run_style_command(command, arguments);
    }

    if (command == "set-run-font-family") {
        return run_set_run_font_family_command(command, arguments);
    }

    if (command == "clear-run-font-family") {
        return run_clear_run_font_family_command(command, arguments);
    }

    if (command == "set-run-language") {
        return run_set_run_language_command(command, arguments);
    }

    if (command == "clear-run-language") {
        return run_clear_run_language_command(command, arguments);
    }

    if (command == "inspect-default-run-properties") {
        return run_inspect_default_run_properties_command(command, arguments);
    }

    if (command == "set-default-run-properties") {
        return run_set_default_run_properties_command(command, arguments);
    }

    if (command == "clear-default-run-properties") {
        return run_clear_default_run_properties_command(command, arguments);
    }

    if (command == "inspect-style-run-properties") {
        return run_inspect_style_run_properties_command(command, arguments);
    }

    if (command == "inspect-paragraph-style-properties") {
        return run_inspect_paragraph_style_properties_command(command,
                                                              arguments);
    }

    if (command == "materialize-style-run-properties") {
        return run_materialize_style_run_properties_command(command, arguments);
    }

    if (command == "rebase-character-style-based-on") {
        return run_rebase_character_style_based_on_command(command, arguments);
    }

    if (command == "rebase-paragraph-style-based-on") {
        return run_rebase_paragraph_style_based_on_command(command, arguments);
    }

    if (command == "set-paragraph-style-properties") {
        return run_set_paragraph_style_properties_command(command, arguments);
    }

    if (command == "clear-paragraph-style-properties") {
        return run_clear_paragraph_style_properties_command(command,
                                                            arguments);
    }

    if (command == "set-style-run-properties") {
        return run_set_style_run_properties_command(command, arguments);
    }

    if (command == "clear-style-run-properties") {
        return run_clear_style_run_properties_command(command, arguments);
    }

    if (command == "ensure-style-linked-numbering") {
        return run_ensure_style_linked_numbering_command(command, arguments);
    }

    if (command == "set-paragraph-style-numbering") {
        return run_set_paragraph_style_numbering_command(command, arguments);
    }

    if (command == "clear-paragraph-style-numbering") {
        return run_clear_paragraph_style_numbering_command(command, arguments);
    }

    if (command == "ensure-numbering-definition") {
        return run_ensure_numbering_definition_command(command, arguments);
    }

    if (command == "set-paragraph-numbering") {
        return run_set_paragraph_numbering_command(command, arguments);
    }

    if (command == "set-paragraph-list") {
        return run_set_paragraph_list_command(command, arguments);
    }

    if (command == "restart-paragraph-list") {
        return run_restart_paragraph_list_command(command, arguments);
    }

    if (command == "clear-paragraph-list") {
        return run_clear_paragraph_list_command(command, arguments);
    }

    if (command == "ensure-paragraph-style") {
        return run_ensure_paragraph_style_command(command, arguments);
    }

    if (command == "ensure-character-style") {
        return run_ensure_character_style_command(command, arguments);
    }

    if (is_page_setup_command(command)) {
        return run_page_setup_command(command, arguments, doc);
    }

    if (is_field_command(command)) {
        return run_field_command(command, arguments, doc);
    }

    if (is_bookmark_command(command)) {
        return run_bookmark_command(command, arguments, doc);
    }

    if (is_content_control_command(command)) {
        return run_content_control_command(command, arguments, doc);
    }

    if (is_document_content_command(command)) {
        return run_document_content_command(command, arguments, doc);
    }

    if (is_review_command(command)) {
        return run_review_command(command, arguments, doc);
    }

    if (command == "semantic-diff") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "semantic-diff expects left and right input paths",
                              json_output);
            return 2;
        }

        semantic_diff_options options;
        std::string error_message;
        if (!parse_semantic_diff_options(arguments, 3U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::Document right_doc;
        if (!open_document(path_type(std::string(arguments[2])), right_doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto result = doc.compare_semantic(right_doc, options.diff_options);
        if (!result.has_value()) {
            report_document_error(command, "compare", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        output_semantic_diff_result(*result, options);
        return options.fail_on_diff && result->different() ? 1 : 0;
    }

    if (is_image_command(command)) {
        return run_image_command(command, arguments, doc);
    }


    if (is_template_schema_command(command)) {
        return run_template_schema_command(command, arguments, doc);
    }

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}
