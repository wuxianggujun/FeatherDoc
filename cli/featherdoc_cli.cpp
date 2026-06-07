#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_diff.hpp"
#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_options_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"
#include "featherdoc_cli_numbering_catalog_commands.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_bookmark_commands.hpp"
#include "featherdoc_cli_content_control_commands.hpp"
#include "featherdoc_cli_page_setup.hpp"
#include "featherdoc_cli_page_setup_parse.hpp"
#include "featherdoc_cli_paragraph_inspect_commands.hpp"
#include "featherdoc_cli_paragraph_numbering_commands.hpp"
#include "featherdoc_cli_paragraph_run_commands.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_document_content_commands.hpp"
#include "featherdoc_cli_semantic_diff.hpp"
#include "featherdoc_cli_semantic_diff_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_commands.hpp"
#include "featherdoc_cli_style_ensure_options_parse.hpp"
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
#include "featherdoc_cli_style_numbering_audit.hpp"
#include "featherdoc_cli_review_commands.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"
#include "featherdoc_cli_run_recipe_parse.hpp"
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
#include "featherdoc_cli_inspect_numbering_options_parse.hpp"
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
using featherdoc_cli::apply_review_mutation_plan_operations;
using featherdoc_cli::apply_template_schema_patch;
using featherdoc_cli::audit_style_numbering;
using featherdoc_cli::audit_style_numbering_options;
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
using featherdoc_cli::build_review_mutation_plan_operations;
using featherdoc_cli::build_template_schema_patch_options;
using featherdoc_cli::check_template_schema_options;
using featherdoc_cli::collect_applyable_style_numbering_repair_suggestions;
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
using featherdoc_cli::find_review_mutation_plan_overlap;
using featherdoc_cli::has_json_flag;
using featherdoc_cli::is_docx_path;
using featherdoc_cli::is_bookmark_command;
using featherdoc_cli::is_content_control_command;
using featherdoc_cli::is_document_content_command;
using featherdoc_cli::is_field_command;
using featherdoc_cli::is_image_command;
using featherdoc_cli::is_numbering_catalog_command;
using featherdoc_cli::is_review_command;
using featherdoc_cli::is_style_refactor_command;
using featherdoc_cli::is_table_cell_style_command;
using featherdoc_cli::is_table_format_command;
using featherdoc_cli::is_table_position_command;
using featherdoc_cli::is_table_style_command;
using featherdoc_cli::is_table_structure_command;
using featherdoc_cli::is_template_schema_command;
using featherdoc_cli::is_word_temporary_path;
using featherdoc_cli::inspect_numbering_options;
using featherdoc_cli::inspect_options;
using featherdoc_cli::inspect_page_setup;
using featherdoc_cli::inspect_page_setup_options;
using featherdoc_cli::inspect_page_setups;
using featherdoc_cli::is_paragraph_inspect_command;
using featherdoc_cli::inspect_style_inheritance_options;
using featherdoc_cli::inspect_styles_options;
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
using featherdoc_cli::ensure_character_style_options;
using featherdoc_cli::ensure_paragraph_style_options;
using featherdoc_cli::parse_ensure_character_style_options;
using featherdoc_cli::parse_ensure_paragraph_style_options;
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
using featherdoc_cli::parse_audit_style_numbering_options;
using featherdoc_cli::parse_export_template_schema_options;
using featherdoc_cli::parse_index;
using featherdoc_cli::parse_inspect_numbering_options;
using featherdoc_cli::parse_inspect_options;
using featherdoc_cli::parse_inspect_page_setup_options;
using featherdoc_cli::parse_inspect_style_inheritance_options;
using featherdoc_cli::parse_inspect_styles_options;
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
using featherdoc_cli::parse_repair_style_numbering_options;
using featherdoc_cli::parse_repair_template_schema_options;
using featherdoc_cli::parse_run_recipe_options;
using featherdoc_cli::read_run_recipe_inputs_file;
using featherdoc_cli::read_run_recipe_recipe_id;
using featherdoc_cli::parse_section_text_options;
using featherdoc_cli::parse_set_section_page_setup_options;
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
using featherdoc_cli::resolve_section_page_setup;
using featherdoc_cli::read_numbering_catalog_file;
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
using featherdoc_cli::repair_style_numbering_options;
using featherdoc_cli::repair_template_schema_options;
using featherdoc_cli::style_numbering_audit_clean;
using featherdoc_cli::style_numbering_audit_issue;
using featherdoc_cli::style_numbering_audit_issue_kind_name;
using featherdoc_cli::style_numbering_audit_result;
using featherdoc_cli::style_numbering_repair_action;
using featherdoc_cli::style_numbering_repair_action_name;
using featherdoc_cli::style_numbering_repair_result;
using featherdoc_cli::style_numbering_repair_suggestion;
using featherdoc_cli::style_numbering_repair_suggestion_applyable;
using featherdoc_cli::section_text_options;
using featherdoc_cli::set_section_page_setup_options;
using featherdoc_cli::review_mutation_plan_build_resolution;
using featherdoc_cli::review_mutation_plan_build_request_operation;
using featherdoc_cli::review_mutation_plan_operation;
using featherdoc_cli::review_mutation_plan_operation_kind;
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
using featherdoc_cli::run_recipe_options;
using featherdoc_cli::style_kind_name;
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
using featherdoc_cli::write_json_section_page_setup;
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
using featherdoc_cli::preview_review_mutation_plan_operations;
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
using featherdoc_cli::run_paragraph_inspect_command;
using featherdoc_cli::run_review_command;
using featherdoc_cli::run_template_schema_command;
using featherdoc_cli::save_document;
using featherdoc_cli::run_section_part_command;
using featherdoc_cli::write_json_mutation_result;
using featherdoc_cli::write_json_numbering_catalog_import_summary;
using featherdoc_cli::write_json_numbering_instance_summary;
using featherdoc_cli::write_json_numbering_level_override_summary;
using featherdoc_cli::write_json_numbering_level_definition;
using featherdoc_cli::write_json_paragraph_style_numbering_link;
using featherdoc_cli::write_json_style_summary;
using featherdoc_cli::write_json_style_usage_summary;
using featherdoc_cli::print_style_summary;
using featherdoc_cli::inspect_style;
using featherdoc_cli::yes_no;

enum class section_part_family {
    header,
    footer,
};

auto resolve_body_table_cell_by_grid_column(featherdoc::Document &doc,
                                            std::size_t table_index,
                                            std::size_t row_index,
                                            std::size_t grid_column,
                                            featherdoc::TableCell &cell,
                                            std::string_view command,
                                            bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row(doc, table_index, row_index, row, command,
                                json_output)) {
        return false;
    }

    auto resolved_cell = row.find_cell_by_grid_column(grid_column);
    if (resolved_cell.has_value()) {
        cell = *resolved_cell;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "grid column '" + std::to_string(grid_column) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "grid column is out of range", error_info,
                                    json_output);
}

auto load_body_table_summary(featherdoc::Document &doc,
                             std::size_t table_index,
                             featherdoc::table_inspection_summary &table,
                             std::string_view command, bool json_output,
                             std::string_view stage = "mutate") -> bool {
    const auto inspected_table = doc.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    if (inspected_table.has_value()) {
        table = *inspected_table;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "table index '" + std::to_string(table_index) + "' is out of range";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, stage,
                                    "table index is out of range", error_info,
                                    json_output);
}

auto load_body_table_cell_summary(
    featherdoc::Document &doc, std::size_t table_index, std::size_t row_index,
    std::size_t cell_index, featherdoc::table_cell_inspection_summary &cell,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_body_table_summary(doc, table_index, table, command, json_output,
                                 stage)) {
        return false;
    }

    const auto inspected_cell =
        doc.inspect_table_cell(table_index, row_index, cell_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    if (inspected_cell.has_value()) {
        cell = *inspected_cell;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    if (row_index >= table.row_count) {
        error_info.detail =
            "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'";
        error_info.entry_name = "word/document.xml";
        return report_operation_failure(command, stage,
                                        "row index is out of range", error_info,
                                        json_output);
    }

    error_info.detail =
        "cell index '" + std::to_string(cell_index) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, stage,
                                    "cell index is out of range", error_info,
                                    json_output);
}

auto load_body_table_cells_summary(
    featherdoc::Document &doc, std::size_t table_index,
    std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::string_view command, bool json_output,
    std::string_view stage = "mutate") -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_body_table_summary(doc, table_index, table, command, json_output,
                                 stage)) {
        return false;
    }

    cells = doc.inspect_table_cells(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    return true;
}

void write_json_numbering_definition_summary(
    std::ostream &stream, const featherdoc::numbering_definition_summary &definition) {
    stream << "{\"definition_id\":" << definition.definition_id << ",\"name\":";
    write_json_string(stream, definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, definition.levels[index]);
    }
    stream << "],\"instance_ids\":[";
    for (std::size_t index = 0; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "],\"instances\":[";
    for (std::size_t index = 0; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream, definition.instances[index]);
    }
    stream << "]}";
}

void write_json_resolved_style_string_property(
    std::ostream &stream, const featherdoc::resolved_style_string_property &property) {
    stream << "{\"value\":";
    write_json_optional_string(stream, property.value);
    stream << ",\"source_style_id\":";
    write_json_optional_string(stream, property.source_style_id);
    stream << '}';
}

void write_json_resolved_style_bool_property(
    std::ostream &stream, const featherdoc::resolved_style_bool_property &property) {
    stream << "{\"value\":";
    write_json_optional_bool(stream, property.value);
    stream << ",\"source_style_id\":";
    write_json_optional_string(stream, property.source_style_id);
    stream << '}';
}

void write_json_resolved_style_properties_summary(
    std::ostream &stream, const featherdoc::resolved_style_properties_summary &summary) {
    stream << "{\"style_id\":";
    write_json_string(stream, summary.style_id);
    stream << ",\"type\":";
    write_json_string(stream, summary.type_name);
    stream << ",\"kind\":";
    write_json_string(stream, style_kind_name(summary.kind));
    stream << ",\"based_on\":";
    write_json_optional_string(stream, summary.based_on);
    stream << ",\"inheritance_chain\":";
    write_json_strings(stream, summary.inheritance_chain);
    stream << ",\"resolved_properties\":{\"font_family\":";
    write_json_resolved_style_string_property(stream, summary.run_font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_resolved_style_string_property(stream, summary.run_east_asia_font_family);
    stream << ",\"language\":";
    write_json_resolved_style_string_property(stream, summary.run_language);
    stream << ",\"east_asia_language\":";
    write_json_resolved_style_string_property(stream, summary.run_east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_resolved_style_string_property(stream, summary.run_bidi_language);
    stream << ",\"rtl\":";
    write_json_resolved_style_bool_property(stream, summary.run_rtl);
    stream << ",\"paragraph_bidi\":";
    write_json_resolved_style_bool_property(stream, summary.paragraph_bidi);
    stream << "}}";
}

void print_resolved_style_string_property(std::ostream &stream, std::string_view label,
                                          const featherdoc::resolved_style_string_property &property) {
    stream << label << ": ";
    if (property.value.has_value()) {
        stream << *property.value;
    } else {
        stream << "none";
    }
    stream << " (source=";
    if (property.source_style_id.has_value()) {
        stream << *property.source_style_id;
    } else {
        stream << "none";
    }
    stream << ")\n";
}

void print_resolved_style_bool_property(std::ostream &stream, std::string_view label,
                                        const featherdoc::resolved_style_bool_property &property) {
    stream << label << ": ";
    if (property.value.has_value()) {
        stream << yes_no(*property.value);
    } else {
        stream << "none";
    }
    stream << " (source=";
    if (property.source_style_id.has_value()) {
        stream << *property.source_style_id;
    } else {
        stream << "none";
    }
    stream << ")\n";
}

void inspect_resolved_style_properties(
    const featherdoc::resolved_style_properties_summary &summary, bool json_output) {
    if (json_output) {
        write_json_resolved_style_properties_summary(std::cout, summary);
        std::cout << '\n';
        return;
    }

    std::cout << "style_id: " << summary.style_id << '\n';
    std::cout << "type: " << summary.type_name << '\n';
    std::cout << "kind: " << style_kind_name(summary.kind) << '\n';
    std::cout << "based_on: ";
    if (summary.based_on.has_value()) {
        std::cout << *summary.based_on;
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    std::cout << "inheritance_chain: ";
    for (std::size_t index = 0U; index < summary.inheritance_chain.size(); ++index) {
        if (index != 0U) {
            std::cout << " -> ";
        }
        std::cout << summary.inheritance_chain[index];
    }
    std::cout << '\n';
    print_resolved_style_string_property(std::cout, "font_family", summary.run_font_family);
    print_resolved_style_string_property(std::cout, "east_asia_font_family",
                                         summary.run_east_asia_font_family);
    print_resolved_style_string_property(std::cout, "language", summary.run_language);
    print_resolved_style_string_property(std::cout, "east_asia_language",
                                         summary.run_east_asia_language);
    print_resolved_style_string_property(std::cout, "bidi_language",
                                         summary.run_bidi_language);
    print_resolved_style_bool_property(std::cout, "rtl", summary.run_rtl);
    print_resolved_style_bool_property(std::cout, "paragraph_bidi",
                                       summary.paragraph_bidi);
}

void write_json_border_inspection_summary(
    std::ostream &stream, const featherdoc::border_inspection_summary &border) {
    stream << "{\"style\":";
    write_json_string(stream, border_style_name(border.style));
    stream << ",\"size_eighth_points\":" << border.size_eighth_points
           << ",\"color\":";
    write_json_string(stream, border.color);
    stream << ",\"space_points\":" << border.space_points << '}';
}

void write_json_optional_border_inspection_summary(
    std::ostream &stream,
    const std::optional<featherdoc::border_inspection_summary> &border) {
    if (!border.has_value()) {
        stream << "null";
        return;
    }

    write_json_border_inspection_summary(stream, *border);
}

void write_json_table_borders_inspection_summary(
    std::ostream &stream,
    const featherdoc::table_borders_inspection_summary &borders) {
    stream << "{\"top\":";
    write_json_optional_border_inspection_summary(stream, borders.top);
    stream << ",\"left\":";
    write_json_optional_border_inspection_summary(stream, borders.left);
    stream << ",\"bottom\":";
    write_json_optional_border_inspection_summary(stream, borders.bottom);
    stream << ",\"right\":";
    write_json_optional_border_inspection_summary(stream, borders.right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_border_inspection_summary(stream, borders.inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_border_inspection_summary(stream, borders.inside_vertical);
    stream << '}';
}

void write_json_optional_table_borders_inspection_summary(
    std::ostream &stream,
    const std::optional<featherdoc::table_borders_inspection_summary> &borders) {
    if (!borders.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_borders_inspection_summary(stream, *borders);
}

void write_json_table_summary(std::ostream &stream,
                              const featherdoc::table_inspection_summary &table) {
    stream << "{\"index\":" << table.index << ",\"style_id\":";
    write_json_optional_string(stream, table.style_id);
    stream << ",\"width_twips\":";
    write_json_optional_u32(stream, table.width_twips);
    stream << ",\"alignment\":";
    if (table.alignment.has_value()) {
        write_json_string(stream, table_alignment_name(*table.alignment));
    } else {
        stream << "null";
    }
    stream << ",\"indent_twips\":";
    write_json_optional_u32(stream, table.indent_twips);
    stream << ",\"cell_spacing_twips\":";
    write_json_optional_u32(stream, table.cell_spacing_twips);
    stream << ",\"cell_margins\":{\"top\":";
    write_json_optional_u32(stream, table.cell_margin_top_twips);
    stream << ",\"left\":";
    write_json_optional_u32(stream, table.cell_margin_left_twips);
    stream << ",\"bottom\":";
    write_json_optional_u32(stream, table.cell_margin_bottom_twips);
    stream << ",\"right\":";
    write_json_optional_u32(stream, table.cell_margin_right_twips);
    stream << '}';
    stream << ",\"borders\":";
    write_json_optional_table_borders_inspection_summary(stream, table.borders);
    stream << ",\"position\":";
    write_json_optional_table_position(stream, table.position);
    stream << ",\"row_count\":" << table.row_count
           << ",\"column_count\":" << table.column_count
           << ",\"column_widths\":[";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_optional_u32(stream, table.column_widths[index]);
    }
    stream << "],\"text\":";
    write_json_string(stream, table.text);
    stream << '}';
}

void write_json_table_cell_summary(
    std::ostream &stream, const featherdoc::table_cell_inspection_summary &cell) {
    stream << "{\"row_index\":" << cell.row_index
           << ",\"cell_index\":" << cell.cell_index
           << ",\"column_index\":" << cell.column_index
           << ",\"column_span\":" << cell.column_span
           << ",\"paragraph_count\":" << cell.paragraph_count
           << ",\"width_twips\":";
    write_json_optional_u32(stream, cell.width_twips);
    stream << ",\"vertical_alignment\":";
    if (cell.vertical_alignment.has_value()) {
        write_json_string(stream,
                          cell_vertical_alignment_name(*cell.vertical_alignment));
    } else {
        stream << "null";
    }
    stream << ",\"text_direction\":";
    if (cell.text_direction.has_value()) {
        write_json_string(stream, cell_text_direction_name(*cell.text_direction));
    } else {
        stream << "null";
    }
    stream << ",\"text\":";
    write_json_string(stream, cell.text);
    stream << '}';
}

void write_json_table_row_summary(std::ostream &stream,
                                  const table_row_inspection_summary &row) {
    stream << "{\"row_index\":" << row.row_index
           << ",\"cell_count\":" << row.cell_count << ",\"height_twips\":";
    write_json_optional_u32(stream, row.height_twips);
    stream << ",\"height_rule\":";
    if (row.height_rule.has_value()) {
        write_json_string(stream, row_height_rule_name(*row.height_rule));
    } else {
        stream << "null";
    }
    stream << ",\"cant_split\":" << json_bool(row.cant_split)
           << ",\"repeats_header\":" << json_bool(row.repeats_header)
           << ",\"cell_texts\":[";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, row.cell_texts[index]);
    }
    stream << "]}";
}

void print_numbering_instance_ids(std::ostream &stream,
                                  const std::vector<std::uint32_t> &instance_ids) {
    if (instance_ids.empty()) {
        stream << "none";
        return;
    }

    for (std::size_t index = 0; index < instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << instance_ids[index];
    }
}

void print_numbering_definition_summary(
    std::ostream &stream, const featherdoc::numbering_definition_summary &definition) {
    stream << "id=" << definition.definition_id << " name=" << definition.name
           << " levels=" << definition.levels.size() << " instances=";
    print_numbering_instance_ids(stream, definition.instance_ids);
}

void print_numbering_level_override_summary(
    std::ostream &stream, const featherdoc::numbering_level_override_summary &level_override) {
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

void print_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance,
    std::string_view indent = {}) {
    stream << indent << "instance[" << instance.instance_id
           << "]: override_count=" << instance.level_overrides.size() << '\n';
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        stream << indent << "  override[" << index << "]: ";
        print_numbering_level_override_summary(stream, instance.level_overrides[index]);
        stream << '\n';
    }
}

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


auto filter_numbered_paragraph_styles(
    const std::vector<featherdoc::style_summary> &styles)
    -> std::vector<featherdoc::style_summary> {
    auto filtered = std::vector<featherdoc::style_summary>{};
    for (const auto &style : styles) {
        if (style.kind == featherdoc::style_kind::paragraph &&
            style.numbering.has_value()) {
            filtered.push_back(style);
        }
    }
    return filtered;
}

void inspect_styles(const std::vector<featherdoc::style_summary> &styles,
                    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << styles.size() << ",\"styles\":[";
        for (std::size_t index = 0; index < styles.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_summary(std::cout, styles[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "styles: " << styles.size() << '\n';
    for (std::size_t index = 0; index < styles.size(); ++index) {
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, styles[index]);
        std::cout << '\n';
    }
}

void write_json_style_usage_report(std::ostream &stream,
                                   const featherdoc::style_usage_report &report) {
    stream << "{\"count\":" << report.style_count
           << ",\"used_style_count\":" << report.used_style_count
           << ",\"unused_style_count\":" << report.unused_style_count
           << ",\"total_reference_count\":" << report.total_reference_count
           << ",\"styles\":[";
    for (std::size_t index = 0; index < report.entries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"style\":";
        write_json_style_summary(stream, report.entries[index].style);
        stream << ",\"usage\":";
        write_json_style_usage_summary(stream, report.entries[index].usage);
        stream << '}';
    }
    stream << "]}";
}

void inspect_style_usage_report(const featherdoc::style_usage_report &report,
                                bool json_output) {
    if (json_output) {
        write_json_style_usage_report(std::cout, report);
        std::cout << '\n';
        return;
    }

    std::cout << "styles: " << report.style_count << '\n'
              << "used_styles: " << report.used_style_count << '\n'
              << "unused_styles: " << report.unused_style_count << '\n'
              << "style_references: " << report.total_reference_count << '\n';
    for (std::size_t index = 0; index < report.entries.size(); ++index) {
        const auto &entry = report.entries[index];
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, entry.style);
        std::cout << " usage_total=" << entry.usage.total_count()
                  << " usage_body=" << entry.usage.body.total_count()
                  << " usage_header=" << entry.usage.header.total_count()
                  << " usage_footer=" << entry.usage.footer.total_count() << '\n';
    }
}

void write_json_style_numbering_audit_issue(
    std::ostream &stream, const style_numbering_audit_issue &issue) {
    stream << "{\"kind\":";
    write_json_string(stream, style_numbering_audit_issue_kind_name(issue.kind));
    stream << ",\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"based_on\":";
    write_json_optional_string(stream, issue.based_on);
    stream << ",\"num_id\":";
    write_json_optional_u32(stream, issue.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, issue.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, issue.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, issue.definition_name);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << '}';
}

void write_json_style_numbering_repair_suggestion(
    std::ostream &stream,
    const style_numbering_repair_suggestion &suggestion) {
    stream << "{\"action\":";
    write_json_string(stream,
                      style_numbering_repair_action_name(suggestion.action));
    stream << ",\"issue_kind\":";
    write_json_string(
        stream, style_numbering_audit_issue_kind_name(suggestion.issue_kind));
    stream << ",\"style_id\":";
    write_json_string(stream, suggestion.style_id);
    stream << ",\"target_definition_id\":";
    write_json_optional_u32(stream, suggestion.target_definition_id);
    stream << ",\"target_definition_name\":";
    write_json_optional_string(stream, suggestion.target_definition_name);
    stream << ",\"target_level\":";
    write_json_optional_u32(stream, suggestion.target_level);
    stream << ",\"rationale\":";
    write_json_string(stream, suggestion.rationale);
    stream << ",\"command_template\":";
    write_json_string(stream, suggestion.command_template);
    stream << ",\"applyable\":"
           << json_bool(style_numbering_repair_suggestion_applyable(
                  suggestion));
    stream << '}';
}

void print_style_numbering_repair_suggestion(
    std::ostream &stream,
    const style_numbering_repair_suggestion &suggestion) {
    stream << "action=" << style_numbering_repair_action_name(suggestion.action)
           << " issue_kind="
           << style_numbering_audit_issue_kind_name(suggestion.issue_kind)
           << " style_id=" << suggestion.style_id
           << " target_definition_id=";
    if (suggestion.target_definition_id.has_value()) {
        stream << *suggestion.target_definition_id;
    } else {
        stream << "none";
    }
    stream << " target_definition_name=";
    if (suggestion.target_definition_name.has_value()) {
        stream << *suggestion.target_definition_name;
    } else {
        stream << "none";
    }
    stream << " target_level=";
    if (suggestion.target_level.has_value()) {
        stream << *suggestion.target_level;
    } else {
        stream << "none";
    }
    stream << " applyable="
           << yes_no(style_numbering_repair_suggestion_applyable(
                  suggestion))
           << " command=" << suggestion.command_template
           << " rationale=" << suggestion.rationale;
}

void print_style_numbering_audit_issue(
    std::ostream &stream, const style_numbering_audit_issue &issue) {
    stream << "kind=" << style_numbering_audit_issue_kind_name(issue.kind)
           << " style_id=" << issue.style_id
           << " num_id=";
    if (issue.num_id.has_value()) {
        stream << *issue.num_id;
    } else {
        stream << "none";
    }
    stream << " level=";
    if (issue.level.has_value()) {
        stream << *issue.level;
    } else {
        stream << "none";
    }
    stream << " definition_id=";
    if (issue.definition_id.has_value()) {
        stream << *issue.definition_id;
    } else {
        stream << "none";
    }
    stream << " definition_name=";
    if (issue.definition_name.has_value()) {
        stream << *issue.definition_name;
    } else {
        stream << "none";
    }
    stream << " message=" << issue.message;
}

void inspect_style_numbering_audit(
    const style_numbering_audit_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"audit-style-numbering\",\"clean\":"
                  << json_bool(style_numbering_audit_clean(result))
                  << ",\"paragraph_style_count\":"
                  << result.paragraph_style_count
                  << ",\"numbered_style_count\":"
                  << result.numbered_styles.size()
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"suggestion_count\":" << result.suggestions.size()
                  << ",\"styles\":[";
        for (std::size_t index = 0; index < result.numbered_styles.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_summary(std::cout, result.numbered_styles[index]);
        }
        std::cout << "],\"issues\":[";
        for (std::size_t index = 0; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_audit_issue(std::cout,
                                                   result.issues[index]);
        }
        std::cout << "],\"suggestions\":[";
        for (std::size_t index = 0; index < result.suggestions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.suggestions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(style_numbering_audit_clean(result)) << '\n'
              << "paragraph_styles: " << result.paragraph_style_count << '\n'
              << "numbered_styles: " << result.numbered_styles.size() << '\n'
              << "issues: " << result.issues.size() << '\n'
              << "suggestions: " << result.suggestions.size() << '\n';
    for (std::size_t index = 0; index < result.numbered_styles.size(); ++index) {
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, result.numbered_styles[index]);
        std::cout << '\n';
    }
    for (std::size_t index = 0; index < result.issues.size(); ++index) {
        std::cout << "issue[" << index << "]: ";
        print_style_numbering_audit_issue(std::cout, result.issues[index]);
        std::cout << '\n';
    }
    for (std::size_t index = 0; index < result.suggestions.size(); ++index) {
        std::cout << "suggestion[" << index << "]: ";
        print_style_numbering_repair_suggestion(std::cout,
                                                result.suggestions[index]);
        std::cout << '\n';
    }
}


auto style_numbering_repair_skipped_suggestion_count(
    const style_numbering_repair_result &result) -> std::size_t {
    if (result.before.suggestions.size() < result.applyable_suggestions.size()) {
        return 0U;
    }
    return result.before.suggestions.size() - result.applyable_suggestions.size();
}

void inspect_style_numbering_repair(
    const style_numbering_repair_result &result, bool json_output) {
    const auto mode = result.apply ? std::string_view{"apply"}
                                   : std::string_view{"plan"};
    if (json_output) {
        std::cout << "{\"command\":\"repair-style-numbering\",\"mode\":";
        write_json_string(std::cout, mode);
        std::cout << ",\"ok\":true"
                  << ",\"before_clean\":"
                  << json_bool(style_numbering_audit_clean(result.before))
                  << ",\"before_issue_count\":" << result.before.issues.size()
                  << ",\"after_clean\":";
        if (result.after.has_value()) {
            std::cout << json_bool(style_numbering_audit_clean(*result.after));
        } else {
            std::cout << "null";
        }
        std::cout << ",\"after_issue_count\":";
        if (result.after.has_value()) {
            std::cout << result.after->issues.size();
        } else {
            std::cout << "null";
        }
        std::cout << ",\"suggestion_count\":"
                  << result.before.suggestions.size()
                  << ",\"applyable_count\":"
                  << result.applyable_suggestions.size()
                  << ",\"skipped_suggestion_count\":"
                  << style_numbering_repair_skipped_suggestion_count(result)
                  << ",\"applied_count\":" << result.applied_count
                  << ",\"catalog_file\":";
        if (result.catalog_path.has_value()) {
            write_json_string(std::cout, result.catalog_path->string());
        } else {
            std::cout << "null";
        }
        std::cout << ",\"catalog_import\":";
        if (result.catalog_import.has_value()) {
            std::cout << '{';
            write_json_numbering_catalog_import_summary(std::cout,
                                                        *result.catalog_import);
            std::cout << '}';
        } else {
            std::cout << "null";
        }
        std::cout << ",\"output_path\":";
        if (result.output_path.has_value()) {
            write_json_string(std::cout, result.output_path->string());
        } else {
            std::cout << "null";
        }
        std::cout << ",\"suggestions\":[";
        for (std::size_t index = 0; index < result.before.suggestions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.before.suggestions[index]);
        }
        std::cout << "],\"applyable_suggestions\":[";
        for (std::size_t index = 0;
             index < result.applyable_suggestions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_numbering_repair_suggestion(
                std::cout, result.applyable_suggestions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "mode: " << mode << '\n'
              << "before_clean: "
              << yes_no(style_numbering_audit_clean(result.before)) << '\n'
              << "before_issues: " << result.before.issues.size() << '\n'
              << "suggestions: " << result.before.suggestions.size() << '\n'
              << "applyable_suggestions: "
              << result.applyable_suggestions.size() << '\n'
              << "skipped_suggestions: "
              << style_numbering_repair_skipped_suggestion_count(result) << '\n'
              << "applied: " << result.applied_count << '\n';
    if (result.catalog_path.has_value()) {
        std::cout << "catalog_file: " << result.catalog_path->string() << '\n';
    }
    if (result.catalog_import.has_value()) {
        std::cout << "catalog_imported_definitions: "
                  << result.catalog_import->imported_definition_count << '\n'
                  << "catalog_imported_instances: "
                  << result.catalog_import->imported_instance_count << '\n';
    }
    if (result.output_path.has_value()) {
        std::cout << "output_path: " << result.output_path->string() << '\n';
    }
    if (result.after.has_value()) {
        std::cout << "after_clean: "
                  << yes_no(style_numbering_audit_clean(*result.after)) << '\n'
                  << "after_issues: " << result.after->issues.size() << '\n';
    }
    for (std::size_t index = 0; index < result.before.suggestions.size(); ++index) {
        std::cout << "suggestion[" << index << "]: ";
        print_style_numbering_repair_suggestion(
            std::cout, result.before.suggestions[index]);
        std::cout << '\n';
    }
}

void print_style_mutation_result(std::string_view command, featherdoc::Document &doc,
                                 const std::optional<path_type> &output_path,
                                 const featherdoc::style_summary &style,
                                 bool json_output) {
    if (json_output) {
        write_json_mutation_result(command, doc, output_path,
                                   [&style](std::ostream &stream) {
                                       stream << ",\"style\":";
                                       write_json_style_summary(stream, style);
                                   });
        return;
    }

    inspect_style(style, std::nullopt, false);
}

void inspect_numbering(const std::vector<featherdoc::numbering_definition_summary> &definitions,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << definitions.size() << ",\"definitions\":[";
        for (std::size_t index = 0; index < definitions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_definition_summary(std::cout, definitions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "definitions: " << definitions.size() << '\n';
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        std::cout << "definition[" << index << "]: ";
        print_numbering_definition_summary(std::cout, definitions[index]);
        std::cout << '\n';
        for (const auto &level : definitions[index].levels) {
            std::cout << "  level[" << level.level << "]: kind="
                      << list_kind_name(level.kind) << " start=" << level.start
                      << " text_pattern=" << level.text_pattern << '\n';
        }
        for (const auto &instance : definitions[index].instances) {
            print_numbering_instance_summary(std::cout, instance, "  ");
        }
    }
}

void inspect_numbering(const featherdoc::numbering_definition_summary &definition,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"definition\":";
        write_json_numbering_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    std::cout << "definition_id: " << definition.definition_id << '\n';
    std::cout << "name: " << definition.name << '\n';
    std::cout << "instances: ";
    print_numbering_instance_ids(std::cout, definition.instance_ids);
    std::cout << '\n';
    std::cout << "levels: " << definition.levels.size() << '\n';
    for (const auto &level : definition.levels) {
        std::cout << "level[" << level.level << "]: kind=" << list_kind_name(level.kind)
                  << " start=" << level.start
                  << " text_pattern=" << level.text_pattern << '\n';
    }
    std::cout << "instance_count: " << definition.instances.size() << '\n';
    for (const auto &instance : definition.instances) {
        print_numbering_instance_summary(std::cout, instance);
    }
}

void inspect_numbering(const featherdoc::numbering_instance_lookup_summary &instance_lookup,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"instance\":{\"definition_id\":"
                  << instance_lookup.definition_id
                  << ",\"definition_name\":";
        write_json_string(std::cout, instance_lookup.definition_name);
        std::cout << ",\"instance\":";
        write_json_numbering_instance_summary(std::cout, instance_lookup.instance);
        std::cout << "}}\n";
        return;
    }

    std::cout << "definition_id: " << instance_lookup.definition_id << '\n';
    std::cout << "name: " << instance_lookup.definition_name << '\n';
    print_numbering_instance_summary(std::cout, instance_lookup.instance);
}

void print_optional_table_border(
    std::ostream &stream,
    const std::optional<featherdoc::border_inspection_summary> &border) {
    if (!border.has_value()) {
        stream << "none";
        return;
    }

    stream << border_style_name(border->style) << '/' << border->size_eighth_points
           << '/' << border->color << '/' << border->space_points;
}

void print_table_summary(std::ostream &stream,
                         const featherdoc::table_inspection_summary &table) {
    stream << "table[" << table.index << "]: style_id=";
    if (table.style_id.has_value()) {
        stream << *table.style_id;
    } else {
        stream << "none";
    }
    stream << " width_twips=";
    if (table.width_twips.has_value()) {
        stream << *table.width_twips;
    } else {
        stream << "none";
    }
    stream << " rows=" << table.row_count
           << " columns=" << table.column_count
           << " alignment=";
    if (table.alignment.has_value()) {
        stream << table_alignment_name(*table.alignment);
    } else {
        stream << "none";
    }
    stream << " indent_twips=";
    if (table.indent_twips.has_value()) {
        stream << *table.indent_twips;
    } else {
        stream << "none";
    }
    stream << " cell_spacing_twips=";
    if (table.cell_spacing_twips.has_value()) {
        stream << *table.cell_spacing_twips;
    } else {
        stream << "none";
    }
    stream << " cell_margins={top:";
    if (table.cell_margin_top_twips.has_value()) {
        stream << *table.cell_margin_top_twips;
    } else {
        stream << "none";
    }
    stream << ",left:";
    if (table.cell_margin_left_twips.has_value()) {
        stream << *table.cell_margin_left_twips;
    } else {
        stream << "none";
    }
    stream << ",bottom:";
    if (table.cell_margin_bottom_twips.has_value()) {
        stream << *table.cell_margin_bottom_twips;
    } else {
        stream << "none";
    }
    stream << ",right:";
    if (table.cell_margin_right_twips.has_value()) {
        stream << *table.cell_margin_right_twips;
    } else {
        stream << "none";
    }
    stream << '}';
    stream << " borders=";
    if (table.borders.has_value()) {
        stream << "{top:";
        print_optional_table_border(stream, table.borders->top);
        stream << ",left:";
        print_optional_table_border(stream, table.borders->left);
        stream << ",bottom:";
        print_optional_table_border(stream, table.borders->bottom);
        stream << ",right:";
        print_optional_table_border(stream, table.borders->right);
        stream << ",inside_horizontal:";
        print_optional_table_border(stream, table.borders->inside_horizontal);
        stream << ",inside_vertical:";
        print_optional_table_border(stream, table.borders->inside_vertical);
        stream << '}';
    } else {
        stream << "none";
    }
    stream << " column_widths=[";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        if (table.column_widths[index].has_value()) {
            stream << *table.column_widths[index];
        } else {
            stream << "none";
        }
    }
    stream << "] position=";
    write_table_position_text(stream, table.position);
    stream << " text=" << format_paragraph_text(table.text);
}

void print_table_cell_summary(
    std::ostream &stream, const featherdoc::table_cell_inspection_summary &cell) {
    stream << "cell[row=" << cell.row_index << ", cell=" << cell.cell_index
           << "]: column=" << cell.column_index
           << " span=" << cell.column_span
           << " paragraphs=" << cell.paragraph_count
           << " width_twips=";
    if (cell.width_twips.has_value()) {
        stream << *cell.width_twips;
    } else {
        stream << "none";
    }
    stream << " vertical_alignment=";
    if (cell.vertical_alignment.has_value()) {
        stream << cell_vertical_alignment_name(*cell.vertical_alignment);
    } else {
        stream << "none";
    }
    stream << " text_direction=";
    if (cell.text_direction.has_value()) {
        stream << cell_text_direction_name(*cell.text_direction);
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(cell.text);
}

void print_table_row_summary(std::ostream &stream,
                             const table_row_inspection_summary &row) {
    stream << "row[" << row.row_index << "]: cells=" << row.cell_count
           << " height_twips=";
    if (row.height_twips.has_value()) {
        stream << *row.height_twips;
    } else {
        stream << "none";
    }
    stream << " height_rule=";
    if (row.height_rule.has_value()) {
        stream << row_height_rule_name(*row.height_rule);
    } else {
        stream << "none";
    }
    stream << " cant_split=" << yes_no(row.cant_split)
           << " repeats_header=" << yes_no(row.repeats_header)
           << " cell_texts=[";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            stream << ", ";
        }
        stream << format_paragraph_text(row.cell_texts[index]);
    }
    stream << ']';
}

void inspect_tables(const std::vector<featherdoc::table_inspection_summary> &tables,
                    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << tables.size() << ",\"tables\":[";
        for (std::size_t index = 0; index < tables.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_summary(std::cout, tables[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "tables: " << tables.size() << '\n';
    for (const auto &table : tables) {
        print_table_summary(std::cout, table);
        std::cout << '\n';
    }
}

void inspect_table(const featherdoc::table_inspection_summary &table, bool json_output) {
    if (json_output) {
        std::cout << "{\"table\":";
        write_json_table_summary(std::cout, table);
        std::cout << "}\n";
        return;
    }

    std::cout << "index: " << table.index << '\n'
              << "style_id: ";
    if (table.style_id.has_value()) {
        std::cout << *table.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "width_twips: ";
    if (table.width_twips.has_value()) {
        std::cout << *table.width_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "position: ";
    write_table_position_text(std::cout, table.position);
    std::cout << '\n' << "row_count: " << table.row_count << '\n'
              << "column_count: " << table.column_count << '\n'
              << "column_widths: [";
    for (std::size_t index = 0; index < table.column_widths.size(); ++index) {
        if (index != 0U) {
            std::cout << ',';
        }
        if (table.column_widths[index].has_value()) {
            std::cout << *table.column_widths[index];
        } else {
            std::cout << "none";
        }
    }
    std::cout << "]\n"
              << "text: " << format_paragraph_text(table.text) << '\n';
}

void inspect_table_cells(
    std::size_t table_index, std::optional<std::size_t> row_index,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index;
        if (row_index.has_value()) {
            std::cout << ",\"row_index\":" << *row_index;
        }
        std::cout << ",\"count\":" << cells.size() << ",\"cells\":[";
        for (std::size_t index = 0; index < cells.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_cell_summary(std::cout, cells[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n';
    if (row_index.has_value()) {
        std::cout << "row_index: " << *row_index << '\n';
    }
    std::cout << "cells: " << cells.size() << '\n';
    for (const auto &cell : cells) {
        print_table_cell_summary(std::cout, cell);
        std::cout << '\n';
    }
}

void inspect_table_cell(std::size_t table_index,
                        const featherdoc::table_cell_inspection_summary &cell,
                        bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index << ",\"cell\":";
        write_json_table_cell_summary(std::cout, cell);
        std::cout << "}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "row_index: " << cell.row_index << '\n'
              << "cell_index: " << cell.cell_index << '\n'
              << "column_index: " << cell.column_index << '\n'
              << "column_span: " << cell.column_span << '\n'
              << "paragraph_count: " << cell.paragraph_count << '\n'
              << "width_twips: ";
    if (cell.width_twips.has_value()) {
        std::cout << *cell.width_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "vertical_alignment: ";
    if (cell.vertical_alignment.has_value()) {
        std::cout << cell_vertical_alignment_name(*cell.vertical_alignment);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text_direction: ";
    if (cell.text_direction.has_value()) {
        std::cout << cell_text_direction_name(*cell.text_direction);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(cell.text) << '\n';
}

void inspect_table_rows(std::size_t table_index,
                        const std::vector<table_row_inspection_summary> &rows,
                        bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index
                  << ",\"count\":" << rows.size() << ",\"rows\":[";
        for (std::size_t index = 0; index < rows.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_row_summary(std::cout, rows[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "rows: " << rows.size() << '\n';
    for (const auto &row : rows) {
        print_table_row_summary(std::cout, row);
        std::cout << '\n';
    }
}

void inspect_table_row(std::size_t table_index,
                       const table_row_inspection_summary &row,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"table_index\":" << table_index << ",\"row\":";
        write_json_table_row_summary(std::cout, row);
        std::cout << "}\n";
        return;
    }

    std::cout << "table_index: " << table_index << '\n'
              << "row_index: " << row.row_index << '\n'
              << "cell_count: " << row.cell_count << '\n'
              << "height_twips: ";
    if (row.height_twips.has_value()) {
        std::cout << *row.height_twips;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "height_rule: ";
    if (row.height_rule.has_value()) {
        std::cout << row_height_rule_name(*row.height_rule);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "cant_split: " << yes_no(row.cant_split) << '\n'
              << "repeats_header: " << yes_no(row.repeats_header) << '\n'
              << "cell_texts: [";
    for (std::size_t index = 0; index < row.cell_texts.size(); ++index) {
        if (index != 0U) {
            std::cout << ", ";
        }
        std::cout << format_paragraph_text(row.cell_texts[index]);
    }
    std::cout << "]\n";
}

struct run_recipe_output_record {
    std::string id;
    std::string type;
    std::string label;
    path_type path;
};

struct run_recipe_batch_replace_result {
    std::size_t documents_count = 0U;
    std::size_t changed_count = 0U;
    std::size_t copied_count = 0U;
    std::size_t replacements_count = 0U;
    std::vector<run_recipe_output_record> outputs;
};

auto require_run_recipe_input(
    const std::unordered_map<std::string, std::string> &inputs,
    std::string_view key, std::string &value, std::string &error_message)
    -> bool {
    const auto entry = inputs.find(std::string(key));
    if (entry == inputs.end()) {
        error_message = "missing required recipe input: " + std::string(key);
        return false;
    }

    value = entry->second;
    return true;
}

auto collect_run_recipe_docx_files(const path_type &source_dir,
                                   std::vector<path_type> &docx_files,
                                   std::string &error_message) -> bool {
    docx_files.clear();

    std::error_code error_code;
    if (!std::filesystem::exists(source_dir, error_code) ||
        !std::filesystem::is_directory(source_dir, error_code)) {
        error_message = "source_dir is not a directory: " + source_dir.string();
        return false;
    }
    if (error_code) {
        error_message = "failed to inspect source_dir: " + error_code.message();
        return false;
    }

    std::filesystem::directory_iterator iterator(source_dir, error_code);
    if (error_code) {
        error_message = "failed to enumerate source_dir: " + error_code.message();
        return false;
    }

    for (const auto &entry : iterator) {
        std::error_code entry_error;
        if (!entry.is_regular_file(entry_error)) {
            if (entry_error) {
                error_message =
                    "failed to inspect source entry: " + entry_error.message();
                return false;
            }
            continue;
        }

        const auto path = entry.path();
        if (is_docx_path(path) && !is_word_temporary_path(path)) {
            docx_files.push_back(path);
        }
    }

    std::sort(docx_files.begin(), docx_files.end());
    if (docx_files.empty()) {
        error_message = "source_dir does not contain any DOCX files";
        return false;
    }

    return true;
}

auto make_run_recipe_output_path(const path_type &output_dir,
                                 const path_type &input_path) -> path_type {
    return output_dir /
           (input_path.stem().string() + std::string("_replaced") +
            input_path.extension().string());
}

auto run_recipe_document_error_message(const featherdoc::document_error_info &error)
    -> std::string {
    if (!error.detail.empty()) {
        return error.detail;
    }
    if (error.code) {
        return error.code.message();
    }
    return "document operation failed";
}

auto execute_run_recipe_batch_replace_document(
    const path_type &input_path, const path_type &output_path,
    std::string_view find_text, std::string_view replace_text,
    std::size_t &replacements_count, bool &copied_without_changes,
    std::string &error_message) -> bool {
    copied_without_changes = false;
    replacements_count = 0U;

    featherdoc::Document doc(input_path);
    if (doc.open()) {
        error_message = input_path.string() + ": " +
                        run_recipe_document_error_message(doc.last_error());
        return false;
    }

    const auto matches = doc.find_text_ranges(find_text);
    if (const auto &error = doc.last_error(); error.code) {
        error_message = input_path.string() + ": " +
                        run_recipe_document_error_message(error);
        return false;
    }

    if (matches.empty()) {
        std::error_code error_code;
        std::filesystem::copy_file(input_path, output_path,
                                   std::filesystem::copy_options::overwrite_existing,
                                   error_code);
        if (error_code) {
            error_message = "failed to copy unchanged document: " +
                            error_code.message();
            return false;
        }

        copied_without_changes = true;
        return true;
    }

    std::vector<review_mutation_plan_build_request_operation> requests;
    requests.reserve(matches.size());
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        review_mutation_plan_build_request_operation request;
        request.kind =
            review_mutation_plan_operation_kind::replace_text_range_revision;
        request.find_text = std::string(find_text);
        request.occurrence = index;
        request.text = std::string(replace_text);
        request.author = "FeatherDoc Studio";
        requests.push_back(std::move(request));
    }

    std::vector<review_mutation_plan_operation> operations;
    std::vector<review_mutation_plan_build_resolution> resolutions;
    std::size_t failed_operation_index = 0U;
    std::size_t failed_matches_count = 0U;
    std::size_t failed_raw_matches_count = 0U;
    if (!build_review_mutation_plan_operations(
            doc, requests, operations, resolutions, error_message,
            failed_operation_index, failed_matches_count,
            failed_raw_matches_count)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }

    const auto previews = preview_review_mutation_plan_operations(doc, operations);
    for (const auto &preview : previews) {
        if (!preview.ok) {
            error_message = input_path.string() + ": " + preview.message;
            return false;
        }
    }

    if (find_review_mutation_plan_overlap(operations, error_message)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }

    std::size_t applied_count = 0U;
    if (!apply_review_mutation_plan_operations(doc, operations, applied_count,
                                               error_message)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }
    static_cast<void>(doc.accept_all_revisions());

    if (doc.save_as(output_path)) {
        error_message = output_path.string() + ": " +
                        run_recipe_document_error_message(doc.last_error());
        return false;
    }

    replacements_count = applied_count;
    return true;
}

auto execute_run_recipe_batch_replace(
    const std::unordered_map<std::string, std::string> &inputs,
    const path_type &output_dir, run_recipe_batch_replace_result &result,
    std::string &error_message) -> bool {
    result = {};

    std::string source_dir_text;
    std::string find_text;
    std::string replace_text;
    if (!require_run_recipe_input(inputs, "source_dir", source_dir_text,
                                  error_message) ||
        !require_run_recipe_input(inputs, "find_text", find_text, error_message) ||
        !require_run_recipe_input(inputs, "replace_text", replace_text,
                                  error_message)) {
        return false;
    }

    if (source_dir_text.empty()) {
        error_message = "source_dir must not be empty";
        return false;
    }
    if (find_text.empty()) {
        error_message = "find_text must not be empty";
        return false;
    }
    if (replace_text.empty()) {
        error_message = "replace_text must not be empty";
        return false;
    }

    std::error_code error_code;
    std::filesystem::create_directories(output_dir, error_code);
    if (error_code) {
        error_message = "failed to create output directory: " +
                        error_code.message();
        return false;
    }

    std::vector<path_type> docx_files;
    if (!collect_run_recipe_docx_files(path_type(source_dir_text), docx_files,
                                       error_message)) {
        return false;
    }

    result.documents_count = docx_files.size();
    for (const auto &input_path : docx_files) {
        const auto output_path = make_run_recipe_output_path(output_dir, input_path);

        std::size_t document_replacements_count = 0U;
        bool copied_without_changes = false;
        if (!execute_run_recipe_batch_replace_document(
                input_path, output_path, find_text, replace_text,
                document_replacements_count, copied_without_changes,
                error_message)) {
            return false;
        }

        if (copied_without_changes) {
            ++result.copied_count;
        } else {
            ++result.changed_count;
            result.replacements_count += document_replacements_count;
        }
    }

    result.outputs.push_back(
        {"result_folder", "folder", "Output folder", output_dir});
    return true;
}

void write_json_run_recipe_outputs(
    std::ostream &stream, const std::vector<run_recipe_output_record> &outputs) {
    stream << '[';
    for (std::size_t index = 0U; index < outputs.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &output = outputs[index];
        stream << "{\"id\":";
        write_json_string(stream, output.id);
        stream << ",\"type\":";
        write_json_string(stream, output.type);
        stream << ",\"label\":";
        write_json_string(stream, output.label);
        stream << ",\"path\":";
        write_json_string(stream, output.path.string());
        stream << '}';
    }
    stream << ']';
}

void write_json_run_recipe_batch_replace_success(
    std::string_view recipe_id, const run_recipe_batch_replace_result &result) {
    std::cout << "{\"command\":\"run-recipe\",\"ok\":true,\"recipe_id\":";
    write_json_string(std::cout, recipe_id);
    std::cout << ",\"documents_count\":" << result.documents_count
              << ",\"changed_count\":" << result.changed_count
              << ",\"copied_count\":" << result.copied_count
              << ",\"replacements_count\":" << result.replacements_count
              << ",\"outputs\":";
    write_json_run_recipe_outputs(std::cout, result.outputs);
    std::cout << "}\n";
}

void print_run_recipe_batch_replace_success(
    const run_recipe_batch_replace_result &result) {
    std::cout << "run-recipe batch_replace completed: "
              << result.documents_count << " document(s), "
              << result.changed_count << " changed, "
              << result.copied_count << " copied, "
              << result.replacements_count << " replacement(s)\n";
    if (!result.outputs.empty()) {
        std::cout << "output: " << result.outputs.front().path.string() << '\n';
    }
}

auto report_run_recipe_execution_error(std::string_view stage,
                                       const std::string &message,
                                       bool json_output) -> int {
    if (json_output) {
        write_json_command_error(std::cerr, "run-recipe", stage, message);
    } else {
        std::cerr << message << '\n';
    }
    return 1;
}

auto execute_run_recipe(const run_recipe_options &options) -> int {
    std::string recipe_id;
    std::string error_message;
    if (!read_run_recipe_recipe_id(*options.recipe_path, recipe_id, error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                options.json_output);
    }

    std::unordered_map<std::string, std::string> inputs;
    if (!read_run_recipe_inputs_file(*options.inputs_path, inputs, error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                options.json_output);
    }

    if (recipe_id == "batch_replace") {
        run_recipe_batch_replace_result result;
        if (!execute_run_recipe_batch_replace(inputs, *options.output_dir, result,
                                              error_message)) {
            return report_run_recipe_execution_error("execute", error_message,
                                                    options.json_output);
        }

        if (options.json_output) {
            write_json_run_recipe_batch_replace_success(recipe_id, result);
        } else {
            print_run_recipe_batch_replace_success(result);
        }
        return 0;
    }

    return report_run_recipe_execution_error(
        "execute", "unsupported run-recipe recipe id: " + recipe_id,
        options.json_output);
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
        const auto json_output = has_json_flag(arguments);

        run_recipe_options options;
        std::string error_message;
        if (!parse_run_recipe_options(arguments, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        return execute_run_recipe(options);
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

    if (command == "inspect-styles") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-styles expects an input path",
                              json_output);
            return 2;
        }

        inspect_styles_options options;
        std::string error_message;
        if (!parse_inspect_styles_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.style_id.has_value()) {
            const auto style = doc.find_style(*options.style_id);
            if (!style.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            std::optional<featherdoc::style_usage_summary> usage;
            if (options.usage_output) {
                usage = doc.find_style_usage(*options.style_id);
                if (!usage.has_value()) {
                    report_document_error(command, "inspect", doc.last_error(),
                                          options.json_output);
                    return 1;
                }
            }

            inspect_style(*style, usage, options.json_output);
            return 0;
        }

        if (options.usage_output) {
            const auto report = doc.list_style_usage();
            if (!report.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            inspect_style_usage_report(*report, options.json_output);
            return 0;
        }

        const auto styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_styles(styles, options.json_output);
        return 0;
    }

    if (command == "inspect-style-inheritance") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "inspect-style-inheritance expects an input path and a style id",
                json_output);
            return 2;
        }

        const auto style_id = std::string(arguments[2]);
        inspect_style_inheritance_options options;
        std::string error_message;
        if (!parse_inspect_style_inheritance_options(arguments, 3U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto resolved = doc.resolve_style_properties(style_id);
        if (!resolved.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_resolved_style_properties(*resolved, options.json_output);
        return 0;
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

    if (command == "inspect-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-numbering expects an input path",
                              json_output);
            return 2;
        }

        inspect_numbering_options options;
        std::string error_message;
        if (!parse_inspect_numbering_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.instance_id.has_value()) {
            const auto instance = doc.find_numbering_instance(*options.instance_id);
            if (!instance.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            if (options.definition_id.has_value() &&
                instance->definition_id != *options.definition_id) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "numbering instance id '" + std::to_string(*options.instance_id) +
                    "' was not found under definition id '" +
                    std::to_string(*options.definition_id) + "' in word/numbering.xml";
                error_info.entry_name = "word/numbering.xml";
                report_operation_failure(command, "inspect",
                                         "numbering instance was not found", error_info,
                                         options.json_output);
                return 1;
            }

            inspect_numbering(*instance, options.json_output);
            return 0;
        }

        if (options.definition_id.has_value()) {
            const auto definition = doc.find_numbering_definition(*options.definition_id);
            if (!definition.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            inspect_numbering(*definition, options.json_output);
            return 0;
        }

        const auto definitions = doc.list_numbering_definitions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_numbering(definitions, options.json_output);
        return 0;
    }

    if (command == "inspect-style-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(
                command, "inspect-style-numbering expects an input path",
                json_output);
            return 2;
        }

        inspect_options options;
        std::string error_message;
        if (!parse_inspect_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_styles(filter_numbered_paragraph_styles(styles),
                       options.json_output);
        return 0;
    }


    if (command == "audit-style-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(
                command, "audit-style-numbering expects an input path",
                json_output);
            return 2;
        }

        audit_style_numbering_options options;
        std::string error_message;
        if (!parse_audit_style_numbering_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "audit", error_info,
                                  options.json_output);
            return 1;
        }

        const auto definitions = doc.list_numbering_definitions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "audit", error_info,
                                  options.json_output);
            return 1;
        }

        const auto result = audit_style_numbering(styles, definitions);
        inspect_style_numbering_audit(result, options.json_output);
        if (options.fail_on_issue && !style_numbering_audit_clean(result)) {
            return 1;
        }
        return 0;
    }


    if (command == "repair-style-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(
                command, "repair-style-numbering expects an input path",
                json_output);
            return 2;
        }

        repair_style_numbering_options options;
        std::string error_message;
        if (!parse_repair_style_numbering_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        auto catalog = featherdoc::numbering_catalog{};
        if (options.catalog_path.has_value() &&
            !read_numbering_catalog_file(*options.catalog_path, catalog,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }

        const auto definitions = doc.list_numbering_definitions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }

        auto result = style_numbering_repair_result{};
        result.apply = options.apply;
        result.catalog_path = options.catalog_path;
        result.output_path = options.output_path;
        result.before = audit_style_numbering(styles, definitions);
        result.applyable_suggestions =
            collect_applyable_style_numbering_repair_suggestions(
                result.before.suggestions);

        if (options.apply) {
            if (options.catalog_path.has_value()) {
                const auto import_summary = doc.import_numbering_catalog(catalog);
                if (!static_cast<bool>(import_summary)) {
                    report_document_error(command, "repair", doc.last_error(),
                                          options.json_output);
                    return 1;
                }
                result.catalog_import = import_summary;

                const auto catalog_repaired_styles = doc.list_styles();
                if (const auto &error_info = doc.last_error(); error_info.code) {
                    report_document_error(command, "repair", error_info,
                                          options.json_output);
                    return 1;
                }
                const auto catalog_repaired_definitions =
                    doc.list_numbering_definitions();
                if (const auto &error_info = doc.last_error(); error_info.code) {
                    report_document_error(command, "repair", error_info,
                                          options.json_output);
                    return 1;
                }
                const auto catalog_repaired_audit = audit_style_numbering(
                    catalog_repaired_styles, catalog_repaired_definitions);
                result.applyable_suggestions =
                    collect_applyable_style_numbering_repair_suggestions(
                        catalog_repaired_audit.suggestions);
            }

            for (const auto &suggestion : result.applyable_suggestions) {
                switch (suggestion.action) {
                case style_numbering_repair_action::clear_style_numbering:
                    if (!doc.clear_paragraph_style_numbering(suggestion.style_id)) {
                        report_document_error(command, "repair", doc.last_error(),
                                              options.json_output);
                        return 1;
                    }
                    ++result.applied_count;
                    break;
                case style_numbering_repair_action::align_with_based_on_numbering:
                case style_numbering_repair_action::relink_style_numbering:
                    if (!suggestion.target_definition_id.has_value() ||
                        !suggestion.target_level.has_value()) {
                        break;
                    }
                    if (!doc.set_paragraph_style_numbering(
                            suggestion.style_id, *suggestion.target_definition_id,
                            *suggestion.target_level)) {
                        report_document_error(command, "repair", doc.last_error(),
                                              options.json_output);
                        return 1;
                    }
                    ++result.applied_count;
                    break;
                case style_numbering_repair_action::add_numbering_level:
                case style_numbering_repair_action::rename_numbering_definition:
                case style_numbering_repair_action::manual_review:
                    break;
                }
            }

            const auto repaired_styles = doc.list_styles();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "repair", error_info,
                                      options.json_output);
                return 1;
            }
            const auto repaired_definitions = doc.list_numbering_definitions();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "repair", error_info,
                                      options.json_output);
                return 1;
            }
            result.after = audit_style_numbering(repaired_styles,
                                                 repaired_definitions);

            if (!save_document(doc, options.output_path, command,
                               options.json_output)) {
                return 1;
            }
        }

        inspect_style_numbering_repair(result, options.json_output);
        return 0;
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
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command, "ensure-paragraph-style expects an input path and a style id",
                json_output);
            return 2;
        }

        const auto style_id = arguments[2];
        ensure_paragraph_style_options options;
        std::string error_message;
        if (!parse_ensure_paragraph_style_options(arguments, 3U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.ensure_paragraph_style(style_id, options.definition)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        const auto style = doc.find_style(style_id);
        if (!style.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        print_style_mutation_result(command, doc, options.output_path, *style,
                                    options.json_output);
        return 0;
    }

    if (command == "ensure-character-style") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command, "ensure-character-style expects an input path and a style id",
                json_output);
            return 2;
        }

        const auto style_id = arguments[2];
        ensure_character_style_options options;
        std::string error_message;
        if (!parse_ensure_character_style_options(arguments, 3U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.ensure_character_style(style_id, options.definition)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        const auto style = doc.find_style(style_id);
        if (!style.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        print_style_mutation_result(command, doc, options.output_path, *style,
                                    options.json_output);
        return 0;
    }

    if (command == "inspect-page-setup") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-page-setup expects an input path",
                              json_output);
            return 2;
        }

        inspect_page_setup_options options;
        std::string error_message;
        if (!parse_inspect_page_setup_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.section_index.has_value()) {
            return inspect_page_setup(doc, *options.section_index, command,
                                      options.json_output)
                       ? 0
                       : 1;
        }

        return inspect_page_setups(doc, command, options.json_output) ? 0 : 1;
    }

    if (command == "set-section-page-setup") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-section-page-setup expects an input path and a section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0U;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        set_section_page_setup_options options;
        std::string error_message;
        if (!parse_set_section_page_setup_options(arguments, 3U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::section_page_setup setup{};
        if (!resolve_section_page_setup(doc, section_index, options, command,
                                        options.json_output, setup)) {
            return 1;
        }

        if (!doc.set_section_page_setup(section_index, setup)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [section_index, &setup](std::ostream &stream) {
                    stream << ",\"section\":" << section_index
                           << ",\"page_setup\":";
                    write_json_section_page_setup(stream, setup);
                });
        }

        return 0;
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
