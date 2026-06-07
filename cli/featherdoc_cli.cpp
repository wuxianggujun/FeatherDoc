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
#include "featherdoc_cli_paragraph_numbering_commands.hpp"
#include "featherdoc_cli_paragraph_run_commands.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
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
#include "featherdoc_cli_table_style_commands.hpp"
#include "featherdoc_cli_table_text_commands.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_table_inspect_commands.hpp"
#include "featherdoc_cli_template_table_column_commands.hpp"
#include "featherdoc_cli_template_table_json_patch_commands.hpp"
#include "featherdoc_cli_template_table_merge_commands.hpp"
#include "featherdoc_cli_template_table_row_commands.hpp"
#include "featherdoc_cli_template_table_text_commands.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_pdf_commands.hpp"
#include "featherdoc_cli_table_position_options_parse.hpp"
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
#include "featherdoc_cli_inspect_content_options_parse.hpp"
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
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

#include <zip.h>

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
using featherdoc_cli::clear_paragraph_style_options;
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
using featherdoc_cli::is_table_style_command;
using featherdoc_cli::is_template_schema_command;
using featherdoc_cli::is_word_temporary_path;
using featherdoc_cli::inspect_numbering_options;
using featherdoc_cli::inspect_options;
using featherdoc_cli::inspect_page_setup;
using featherdoc_cli::inspect_page_setup_options;
using featherdoc_cli::inspect_page_setups;
using featherdoc_cli::inspect_paragraphs_options;
using featherdoc_cli::inspect_style_inheritance_options;
using featherdoc_cli::inspect_styles_options;
using featherdoc_cli::inspect_table_cells_options;
using featherdoc_cli::inspect_table_rows_options;
using featherdoc_cli::inspect_tables_options;
using featherdoc_cli::inspect_runs_options;
using featherdoc_cli::template_inspect_paragraphs_options;
using featherdoc_cli::template_inspect_runs_options;
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
using featherdoc_cli::parse_inspect_paragraphs_options;
using featherdoc_cli::parse_inspect_style_inheritance_options;
using featherdoc_cli::parse_inspect_styles_options;
using featherdoc_cli::parse_inspect_table_cells_options;
using featherdoc_cli::parse_inspect_table_rows_options;
using featherdoc_cli::parse_inspect_tables_options;
using featherdoc_cli::parse_inspect_runs_options;
using featherdoc_cli::parse_template_part_selection_option;
using featherdoc_cli::parse_template_inspect_paragraphs_options;
using featherdoc_cli::parse_template_inspect_runs_options;
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
using featherdoc_cli::parse_clear_paragraph_style_options;
using featherdoc_cli::parse_check_template_schema_options;
using featherdoc_cli::parse_diff_template_schema_options;
using featherdoc_cli::parse_export_numbering_catalog_options;
using featherdoc_cli::parse_import_numbering_catalog_options;
using featherdoc_cli::parse_lint_template_schema_options;
using featherdoc_cli::parse_merge_template_schema_options;
using featherdoc_cli::parse_normalize_template_schema_options;
using featherdoc_cli::parse_patch_numbering_catalog_options;
using featherdoc_cli::parse_patch_template_schema_options;
using featherdoc_cli::parse_set_paragraph_style_options;
using featherdoc_cli::parse_clear_run_font_family_options;
using featherdoc_cli::parse_clear_run_language_options;
using featherdoc_cli::parse_clear_run_style_options;
using featherdoc_cli::parse_set_run_font_family_options;
using featherdoc_cli::parse_set_run_language_options;
using featherdoc_cli::parse_set_run_style_options;
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
using featherdoc_cli::set_paragraph_style_options;
using featherdoc_cli::clear_run_font_family_options;
using featherdoc_cli::clear_run_language_options;
using featherdoc_cli::clear_run_style_options;
using featherdoc_cli::set_run_font_family_options;
using featherdoc_cli::set_run_language_options;
using featherdoc_cli::set_run_style_options;
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
using featherdoc_cli::run_table_style_command;
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
using featherdoc_cli::selected_template_part;
using featherdoc_cli::select_mutable_template_part;
using featherdoc_cli::select_template_part;
using featherdoc_cli::print_selected_template_part;
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
using featherdoc_cli::write_json_selected_template_part;
using featherdoc_cli::write_json_size_array;
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








struct inspected_body_paragraph {
    std::size_t index = 0;
    std::size_t section_index = 0;
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> numbering_id;
    std::optional<std::uint32_t> numbering_level;
    bool has_section_break = false;
    std::string text;
};

struct inspected_body_run {
    std::size_t index = 0;
    std::optional<std::string> style_id;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<bool> underline;
    std::optional<double> font_size_points;
    std::optional<std::string> language;
    std::string text;
};

enum class zip_entry_read_status {
    missing,
    read_failed,
    ok,
};

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name,
                         std::string &content) -> zip_entry_read_status {
    if (zip_entry_open(archive, std::string(entry_name).c_str()) < 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    std::size_t size = 0;
    if (zip_entry_read(archive, &buffer, &size) < 0) {
        zip_entry_close(archive);
        return zip_entry_read_status::read_failed;
    }

    if (buffer != nullptr && size != 0U) {
        content.assign(static_cast<const char *>(buffer), size);
    } else {
        content.clear();
    }

    std::free(buffer);
    zip_entry_close(archive);
    return zip_entry_read_status::ok;
}

void write_text_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
}

auto resolve_body_paragraph(featherdoc::Document &doc, std::size_t paragraph_index,
                            featherdoc::Paragraph &paragraph,
                            std::string_view command, bool json_output) -> bool {
    paragraph = doc.paragraphs();
    for (std::size_t current_index = 0;
         current_index < paragraph_index && paragraph.has_next();
         ++current_index) {
        paragraph.next();
    }

    if (paragraph.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "paragraph index '" + std::to_string(paragraph_index) + "' is out of range";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "paragraph index is out of range", error_info,
                                    json_output);
}

auto resolve_body_run(featherdoc::Document &doc, std::size_t paragraph_index,
                      std::size_t run_index, featherdoc::Run &run,
                      std::string_view command, bool json_output) -> bool {
    featherdoc::Paragraph paragraph;
    if (!resolve_body_paragraph(doc, paragraph_index, paragraph, command,
                                json_output)) {
        return false;
    }

    run = paragraph.runs();
    for (std::size_t current_index = 0;
         current_index < run_index && run.has_next();
         ++current_index) {
        run.next();
    }

    if (run.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "run index '" + std::to_string(run_index) +
                        "' is out of range for paragraph index '" +
                        std::to_string(paragraph_index) + "'";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "run index is out of range", error_info,
                                    json_output);
}

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

void append_xml_paragraph_text(std::string &text, pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:t" || name == "w:delText") {
            text += child.text().get();
            continue;
        }

        if (name == "w:tab") {
            text.push_back('\t');
            continue;
        }

        if (name == "w:br" || name == "w:cr") {
            text.push_back('\n');
            continue;
        }

        append_xml_paragraph_text(text, child);
    }
}

auto collect_xml_paragraph_text(pugi::xml_node paragraph) -> std::string {
    std::string text;
    append_xml_paragraph_text(text, paragraph);
    return text;
}

auto read_optional_xml_attribute(pugi::xml_node node,
                                 const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string(value);
}

auto read_optional_run_font_family(pugi::xml_node run_fonts)
    -> std::optional<std::string> {
    if (const auto ascii = read_optional_xml_attribute(run_fonts, "w:ascii");
        ascii.has_value()) {
        return ascii;
    }

    if (const auto hansi = read_optional_xml_attribute(run_fonts, "w:hAnsi");
        hansi.has_value()) {
        return hansi;
    }

    return read_optional_xml_attribute(run_fonts, "w:cs");
}

auto read_optional_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return true;
    }

    return value != "0" && value != "false" && value != "off";
}

auto read_optional_underline_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return true;
    }

    return value != "0" && value != "false" && value != "none";
}

auto read_optional_font_size_points(pugi::xml_node node)
    -> std::optional<double> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return std::nullopt;
    }

    std::uint32_t half_points = 0U;
    if (!parse_uint32(value, half_points) || half_points == 0U) {
        return std::nullopt;
    }

    return static_cast<double>(half_points) / 2.0;
}

auto load_body_paragraph_summaries(const path_type &input_path,
                                   std::vector<inspected_body_paragraph> &paragraphs,
                                   std::string &error_message) -> bool {
    paragraphs.clear();

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message = "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message = "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(), document_xml_text.size());
    if (!document_parse_result) {
        close_archive();
        error_message = "failed to parse XML entry 'word/document.xml'";
        return false;
    }

    close_archive();

    const auto body = document_xml.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        error_message =
            "document.xml does not contain the expected w:document/w:body structure";
        return false;
    }

    std::size_t section_index = 0U;
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        inspected_body_paragraph paragraph_summary;
        paragraph_summary.index = paragraph_index;
        paragraph_summary.section_index = section_index;

        const auto properties = child.child("w:pPr");
        if (const auto style = properties.child("w:pStyle"); style != pugi::xml_node{}) {
            const auto style_id = std::string_view{style.attribute("w:val").value()};
            if (!style_id.empty()) {
                paragraph_summary.style_id = std::string(style_id);
            }
        }

        if (const auto numbering = properties.child("w:numPr");
            numbering != pugi::xml_node{}) {
            const auto numbering_level_text =
                std::string_view{numbering.child("w:ilvl").attribute("w:val").value()};
            if (!numbering_level_text.empty()) {
                std::uint32_t numbering_level = 0U;
                if (!parse_uint32(numbering_level_text, numbering_level)) {
                    error_message = "invalid numbering level on paragraph index " +
                                    std::to_string(paragraph_index) +
                                    " in word/document.xml";
                    return false;
                }
                paragraph_summary.numbering_level = numbering_level;
            }

            const auto numbering_id_text =
                std::string_view{numbering.child("w:numId").attribute("w:val").value()};
            if (!numbering_id_text.empty()) {
                std::uint32_t numbering_id = 0U;
                if (!parse_uint32(numbering_id_text, numbering_id)) {
                    error_message = "invalid numbering instance id on paragraph index " +
                                    std::to_string(paragraph_index) +
                                    " in word/document.xml";
                    return false;
                }
                paragraph_summary.numbering_id = numbering_id;
            }
        }

        paragraph_summary.has_section_break =
            properties.child("w:sectPr") != pugi::xml_node{};
        paragraph_summary.text = collect_xml_paragraph_text(child);
        paragraphs.push_back(std::move(paragraph_summary));

        if (paragraphs.back().has_section_break) {
            ++section_index;
        }
        ++paragraph_index;
    }

    return true;
}

auto find_body_paragraph_node(pugi::xml_node body, std::size_t paragraph_index)
    -> pugi::xml_node {
    std::size_t current_index = 0U;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (current_index == paragraph_index) {
            return paragraph;
        }
        ++current_index;
    }

    return {};
}

auto load_body_run_summaries(const path_type &input_path, std::size_t paragraph_index,
                             std::vector<inspected_body_run> &runs,
                             bool &paragraph_found,
                             std::string &error_message) -> bool {
    runs.clear();
    paragraph_found = false;

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message = "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message = "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(), document_xml_text.size());
    if (!document_parse_result) {
        close_archive();
        error_message = "failed to parse XML entry 'word/document.xml'";
        return false;
    }

    close_archive();

    const auto body = document_xml.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        error_message =
            "document.xml does not contain the expected w:document/w:body structure";
        return false;
    }

    const auto paragraph = find_body_paragraph_node(body, paragraph_index);
    if (paragraph == pugi::xml_node{}) {
        return true;
    }

    paragraph_found = true;
    std::size_t current_run_index = 0U;
    for (auto run = paragraph.child("w:r"); run != pugi::xml_node{};
         run = run.next_sibling("w:r")) {
        inspected_body_run run_summary;
        run_summary.index = current_run_index;
        const auto run_properties = run.child("w:rPr");
        const auto run_fonts = run_properties.child("w:rFonts");

        run_summary.style_id =
            read_optional_xml_attribute(run_properties.child("w:rStyle"), "w:val");
        run_summary.font_family = read_optional_run_font_family(run_fonts);
        run_summary.east_asia_font_family =
            read_optional_xml_attribute(run_fonts, "w:eastAsia");
        run_summary.text_color =
            read_optional_xml_attribute(run_properties.child("w:color"), "w:val");
        run_summary.bold = read_optional_on_off_value(run_properties.child("w:b"));
        run_summary.italic = read_optional_on_off_value(run_properties.child("w:i"));
        run_summary.underline =
            read_optional_underline_value(run_properties.child("w:u"));
        run_summary.font_size_points =
            read_optional_font_size_points(run_properties.child("w:sz"));
        run_summary.language =
            read_optional_xml_attribute(run_properties.child("w:lang"), "w:val");

        run_summary.text = collect_xml_paragraph_text(run);
        runs.push_back(std::move(run_summary));
        ++current_run_index;
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

void write_json_body_paragraph_summary(std::ostream &stream,
                                       const inspected_body_paragraph &paragraph) {
    stream << "{\"index\":" << paragraph.index
           << ",\"section_index\":" << paragraph.section_index
           << ",\"style_id\":";
    write_json_optional_string(stream, paragraph.style_id);
    stream << ",\"numbering\":";
    if (paragraph.numbering_id.has_value() || paragraph.numbering_level.has_value()) {
        stream << "{\"num_id\":";
        write_json_optional_u32(stream, paragraph.numbering_id);
        stream << ",\"level\":";
        write_json_optional_u32(stream, paragraph.numbering_level);
        stream << '}';
    } else {
        stream << "null";
    }
    stream << ",\"section_break\":" << json_bool(paragraph.has_section_break)
           << ",\"text\":";
    write_json_string(stream, paragraph.text);
    stream << '}';
}

void write_json_body_run_summary(std::ostream &stream,
                                 const inspected_body_run &run) {
    stream << "{\"index\":" << run.index << ",\"style_id\":";
    write_json_optional_string(stream, run.style_id);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, run.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, run.east_asia_font_family);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, run.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, run.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, run.italic);
    stream << ",\"underline\":";
    write_json_optional_bool(stream, run.underline);
    stream << ",\"font_size_points\":";
    write_json_optional_double(stream, run.font_size_points);
    stream << ",\"language\":";
    write_json_optional_string(stream, run.language);
    stream << ",\"text\":";
    write_json_string(stream, run.text);
    stream << '}';
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

void write_json_template_paragraph_numbering_summary(
    std::ostream &stream,
    const featherdoc::paragraph_inspection_summary::numbering_summary &numbering) {
    stream << "{\"num_id\":";
    write_json_optional_u32(stream, numbering.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, numbering.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, numbering.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, numbering.definition_name);
    stream << '}';
}

void write_json_template_paragraph_summary(
    std::ostream &stream, const featherdoc::paragraph_inspection_summary &paragraph) {
    stream << "{\"index\":" << paragraph.index << ",\"style_id\":";
    write_json_optional_string(stream, paragraph.style_id);
    stream << ",\"bidi\":";
    write_json_optional_bool(stream, paragraph.bidi);
    stream << ",\"numbering\":";
    if (paragraph.numbering.has_value()) {
        write_json_template_paragraph_numbering_summary(stream,
                                                        *paragraph.numbering);
    } else {
        stream << "null";
    }
    stream << ",\"run_count\":" << paragraph.run_count << ",\"text\":";
    write_json_string(stream, paragraph.text);
    stream << '}';
}

void write_json_template_run_summary(
    std::ostream &stream, const featherdoc::run_inspection_summary &run) {
    stream << "{\"index\":" << run.index << ",\"style_id\":";
    write_json_optional_string(stream, run.style_id);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, run.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, run.east_asia_font_family);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, run.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, run.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, run.italic);
    stream << ",\"underline\":";
    write_json_optional_bool(stream, run.underline);
    stream << ",\"font_size_points\":";
    write_json_optional_double(stream, run.font_size_points);
    stream << ",\"language\":";
    write_json_optional_string(stream, run.language);
    stream << ",\"east_asia_language\":";
    write_json_optional_string(stream, run.east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_optional_string(stream, run.bidi_language);
    stream << ",\"rtl\":";
    write_json_optional_bool(stream, run.rtl);
    stream << ",\"text\":";
    write_json_string(stream, run.text);
    stream << '}';
}

void write_json_optional_table_overlap(
    std::ostream &stream,
    const std::optional<featherdoc::table_overlap> &overlap) {
    if (overlap.has_value()) {
        write_json_string(stream, table_overlap_name(*overlap));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_horizontal_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_horizontal_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_horizontal_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_vertical_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_vertical_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_vertical_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_table_position(std::ostream &stream,
                               const featherdoc::table_position &position) {
    stream << "{\"horizontal_reference\":";
    write_json_string(
        stream,
        table_position_horizontal_reference_name(position.horizontal_reference));
    stream << ",\"horizontal_offset_twips\":"
           << position.horizontal_offset_twips << ",\"horizontal_spec\":";
    write_json_optional_table_position_horizontal_spec(stream,
                                                       position.horizontal_spec);
    stream << ",\"vertical_reference\":";
    write_json_string(stream,
                      table_position_vertical_reference_name(
                          position.vertical_reference));
    stream << ",\"vertical_offset_twips\":" << position.vertical_offset_twips
           << ",\"vertical_spec\":";
    write_json_optional_table_position_vertical_spec(stream, position.vertical_spec);
    stream << ",\"left_from_text_twips\":";
    write_json_optional_u32_value(stream, position.left_from_text_twips);
    stream << ",\"right_from_text_twips\":";
    write_json_optional_u32_value(stream, position.right_from_text_twips);
    stream << ",\"top_from_text_twips\":";
    write_json_optional_u32_value(stream, position.top_from_text_twips);
    stream << ",\"bottom_from_text_twips\":";
    write_json_optional_u32_value(stream, position.bottom_from_text_twips);
    stream << ",\"overlap\":";
    write_json_optional_table_overlap(stream, position.overlap);
    stream << '}';
}

void write_json_optional_table_position(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (position.has_value()) {
        write_json_table_position(stream, *position);
    } else {
        stream << "null";
    }
}

void write_table_position_text(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (!position.has_value()) {
        stream << "none";
        return;
    }

    stream << table_position_horizontal_reference_name(
                  position->horizontal_reference)
           << ':' << position->horizontal_offset_twips;
    if (position->horizontal_spec.has_value()) {
        stream << ':' << table_position_horizontal_spec_name(
                              *position->horizontal_spec);
    }
    stream << ','
           << table_position_vertical_reference_name(position->vertical_reference)
           << ':' << position->vertical_offset_twips;
    if (position->vertical_spec.has_value()) {
        stream << ':' << table_position_vertical_spec_name(*position->vertical_spec);
    }
    if (position->left_from_text_twips.has_value() ||
        position->right_from_text_twips.has_value() ||
        position->top_from_text_twips.has_value() ||
        position->bottom_from_text_twips.has_value()) {
        stream << " wrap=";
        write_json_optional_u32_value(stream, position->left_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->right_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->top_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->bottom_from_text_twips);
    }
    if (position->overlap.has_value()) {
        stream << " overlap=" << table_overlap_name(*position->overlap);
    }
}

void write_json_table_position_table_fingerprint(
    std::ostream &stream, const table_position_table_fingerprint &fingerprint) {
    stream << "{\"table_index\":" << fingerprint.table_index
           << ",\"style_id\":";
    write_json_optional_string(stream, fingerprint.style_id);
    stream << ",\"width_twips\":";
    write_json_optional_u32(stream, fingerprint.width_twips);
    stream << ",\"row_count\":" << fingerprint.row_count
           << ",\"column_count\":" << fingerprint.column_count
           << ",\"column_widths\":[";
    for (std::size_t index = 0; index < fingerprint.column_widths.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_optional_u32(stream, fingerprint.column_widths[index]);
    }
    stream << "],\"text\":";
    write_json_string(stream, fingerprint.text);
    stream << '}';
}

void write_json_table_position_table_fingerprints(
    std::ostream &stream,
    const std::vector<table_position_table_fingerprint> &fingerprints) {
    stream << '[';
    for (std::size_t index = 0; index < fingerprints.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_table_position_table_fingerprint(stream, fingerprints[index]);
    }
    stream << ']';
}

void write_json_table_position_preset_plan_item(
    std::ostream &stream, const table_position_preset_plan_item &item,
    table_position_preset preset) {
    stream << "{\"table_index\":" << item.table_index << ",\"action\":";
    write_json_string(stream, item.action);
    stream << ",\"automatic\":" << json_bool(item.automatic)
           << ",\"current_position\":";
    write_json_optional_table_position(stream, item.current_position);
    stream << ",\"target_preset\":";
    write_json_string(stream, table_position_preset_name(preset));
    stream << ",\"target_position\":";
    write_json_table_position(stream, item.target_position);
    stream << ",\"fingerprint\":";
    write_json_table_position_table_fingerprint(stream, item.fingerprint);
    stream << ",\"recommended_command\":";
    write_json_string(stream, item.recommended_command);
    if (item.resolved_output_path.has_value()) {
        stream << ",\"resolved_output_path\":";
        write_json_string(stream, item.resolved_output_path->string());
    }
    if (item.resolved_recommended_command.has_value()) {
        stream << ",\"resolved_recommended_command\":";
        write_json_string(stream, *item.resolved_recommended_command);
    }
    stream << '}';
}

void write_json_table_position_preset_plan(
    std::ostream &stream, const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options) {
    stream << "{\"command\":\"plan-table-position-presets\""
           << ",\"ok\":" << json_bool(plan.items.empty());
    if (options.input_path.has_value()) {
        stream << ",\"input_path\":";
        write_json_string(stream, options.input_path->string());
    }
    stream << ",\"preset\":";
    write_json_string(stream, table_position_preset_name(plan.preset));
    stream << ",\"replace_positioned\":"
           << json_bool(options.replace_positioned)
           << ",\"fail_on_change\":" << json_bool(options.fail_on_change);
    if (options.output_path.has_value()) {
        stream << ",\"output_path\":";
        write_json_string(stream, options.output_path->string());
    }
    if (options.output_plan_path.has_value()) {
        stream << ",\"output_plan_path\":";
        write_json_string(stream, options.output_plan_path->string());
    }
    stream << ",\"table_count\":" << plan.table_count
           << ",\"positioned_count\":" << plan.positioned_count
           << ",\"unpositioned_count\":" << plan.unpositioned_count
           << ",\"already_matching_count\":" << plan.already_matching_count
           << ",\"already_matching_table_indices\":";
    write_json_size_array(stream, plan.already_matching_table_indices);
    stream << ",\"set_count\":" << plan.set_count
           << ",\"replace_count\":" << plan.replace_count
           << ",\"review_count\":" << plan.review_count
           << ",\"review_table_indices\":";
    write_json_size_array(stream, plan.review_table_indices);
    stream << ",\"automatic_count\":" << plan.automatic_table_indices.size()
           << ",\"automatic_table_indices\":";
    write_json_size_array(stream, plan.automatic_table_indices);
    stream << ",\"recommended_batch_command\":";
    write_json_optional_string(stream, plan.recommended_batch_command);
    stream << ",\"resolved_output_path\":";
    if (plan.resolved_output_path.has_value()) {
        write_json_string(stream, plan.resolved_output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"resolved_recommended_batch_command\":";
    write_json_optional_string(stream, plan.resolved_recommended_batch_command);
    stream << ",\"plan_item_count\":" << plan.items.size()
           << ",\"target_position\":";
    write_json_table_position(stream, plan.target_position);
    stream << ",\"table_fingerprints\":";
    write_json_table_position_table_fingerprints(stream, plan.table_fingerprints);
    stream << ",\"items\":[";
    for (std::size_t index = 0; index < plan.items.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_table_position_preset_plan_item(stream, plan.items[index],
                                                   plan.preset);
    }
    stream << "]}\n";
}

void write_text_table_position_preset_plan(
    const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options) {
    std::cout << "table_position_preset_plan: "
              << (plan.items.empty() ? "clean" : "changes") << '\n';
    if (options.input_path.has_value()) {
        std::cout << "input_path: " << options.input_path->string() << '\n';
    }
    std::cout << "preset: " << table_position_preset_name(plan.preset) << '\n'
              << "replace_positioned: "
              << (options.replace_positioned ? "true" : "false") << '\n'
              << "fail_on_change: " << (options.fail_on_change ? "true" : "false")
              << '\n'
              << "table_count: " << plan.table_count << '\n'
              << "positioned_count: " << plan.positioned_count << '\n'
              << "unpositioned_count: " << plan.unpositioned_count << '\n'
              << "already_matching_count: " << plan.already_matching_count << '\n'
              << "already_matching_table_indices: ";
    write_text_size_array(std::cout, plan.already_matching_table_indices);
    std::cout << '\n' << "set_count: " << plan.set_count << '\n'
              << "replace_count: " << plan.replace_count << '\n'
              << "review_count: " << plan.review_count << '\n'
              << "review_table_indices: ";
    write_text_size_array(std::cout, plan.review_table_indices);
    std::cout << '\n'
              << "automatic_count: " << plan.automatic_table_indices.size()
              << '\n'
              << "automatic_table_indices: ";
    write_text_size_array(std::cout, plan.automatic_table_indices);
    std::cout << "\nrecommended_batch_command: ";
    if (plan.recommended_batch_command.has_value()) {
        std::cout << *plan.recommended_batch_command;
    } else {
        std::cout << "none";
    }
    std::cout << "\nresolved_output_path: ";
    if (plan.resolved_output_path.has_value()) {
        std::cout << plan.resolved_output_path->string();
    } else {
        std::cout << "none";
    }
    std::cout << "\nresolved_recommended_batch_command: ";
    if (plan.resolved_recommended_batch_command.has_value()) {
        std::cout << *plan.resolved_recommended_batch_command;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "plan_item_count: " << plan.items.size() << '\n';
    if (options.output_path.has_value()) {
        std::cout << "output_path: " << options.output_path->string() << '\n';
    }
    if (options.output_plan_path.has_value()) {
        std::cout << "output_plan_path: " << options.output_plan_path->string()
                  << '\n';
    }
    for (std::size_t index = 0; index < plan.items.size(); ++index) {
        const auto &item = plan.items[index];
        std::cout << "item[" << index << "]: table=" << item.table_index
                  << " action=" << item.action
                  << " automatic=" << (item.automatic ? "true" : "false")
                  << " fingerprint_rows=" << item.fingerprint.row_count
                  << " fingerprint_columns=" << item.fingerprint.column_count
                  << " current=";
        write_table_position_text(std::cout, item.current_position);
        std::cout << " target_preset="
                  << table_position_preset_name(plan.preset)
                  << " recommended_command=\"" << item.recommended_command
                  << "\"";
        if (item.resolved_output_path.has_value()) {
            std::cout << " resolved_output_path=\""
                      << item.resolved_output_path->string() << "\"";
        }
        if (item.resolved_recommended_command.has_value()) {
            std::cout << " resolved_recommended_command=\""
                      << *item.resolved_recommended_command << "\"";
        }
        std::cout << '\n';
    }
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

auto write_table_position_preset_plan_file(
    const path_type &output_path, const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open table position plan output path: " +
                        output_path.string();
        return false;
    }

    write_json_table_position_preset_plan(stream, plan, options);
    if (!stream.good()) {
        error_message = "failed to write table position plan output path: " +
                        output_path.string();
        return false;
    }

    return true;
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

void print_body_paragraph_numbering(std::ostream &stream,
                                    const inspected_body_paragraph &paragraph) {
    if (!paragraph.numbering_id.has_value() &&
        !paragraph.numbering_level.has_value()) {
        stream << "none";
        return;
    }

    stream << "num_id=";
    if (paragraph.numbering_id.has_value()) {
        stream << *paragraph.numbering_id;
    } else {
        stream << "unknown";
    }

    stream << " level=";
    if (paragraph.numbering_level.has_value()) {
        stream << *paragraph.numbering_level;
    } else {
        stream << "unknown";
    }
}

void print_body_paragraph_summary(std::ostream &stream,
                                  const inspected_body_paragraph &paragraph) {
    stream << "paragraph[" << paragraph.index
           << "]: section_index=" << paragraph.section_index
           << " style_id=";
    if (paragraph.style_id.has_value()) {
        stream << *paragraph.style_id;
    } else {
        stream << "none";
    }

    stream << " numbering=";
    print_body_paragraph_numbering(stream, paragraph);
    stream << " section_break="
           << (paragraph.has_section_break ? "yes" : "no")
           << " text=" << format_paragraph_text(paragraph.text);
}

void print_body_run_summary(std::ostream &stream, const inspected_body_run &run) {
    stream << "run[" << run.index << "]: style_id=";
    if (run.style_id.has_value()) {
        stream << *run.style_id;
    } else {
        stream << "none";
    }
    stream << " font_family=";
    if (run.font_family.has_value()) {
        stream << *run.font_family;
    } else {
        stream << "none";
    }
    stream << " east_asia_font_family=";
    if (run.east_asia_font_family.has_value()) {
        stream << *run.east_asia_font_family;
    } else {
        stream << "none";
    }
    stream << " text_color=";
    if (run.text_color.has_value()) {
        stream << *run.text_color;
    } else {
        stream << "none";
    }
    stream << " bold=";
    if (run.bold.has_value()) {
        stream << yes_no(*run.bold);
    } else {
        stream << "none";
    }
    stream << " italic=";
    if (run.italic.has_value()) {
        stream << yes_no(*run.italic);
    } else {
        stream << "none";
    }
    stream << " underline=";
    if (run.underline.has_value()) {
        stream << yes_no(*run.underline);
    } else {
        stream << "none";
    }
    stream << " font_size_points=";
    if (run.font_size_points.has_value()) {
        stream << *run.font_size_points;
    } else {
        stream << "none";
    }
    stream << " language=";
    if (run.language.has_value()) {
        stream << *run.language;
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(run.text);
}

void print_template_paragraph_numbering(
    std::ostream &stream, const featherdoc::paragraph_inspection_summary &paragraph) {
    if (!paragraph.numbering.has_value()) {
        stream << "none";
        return;
    }

    stream << "num_id=";
    if (paragraph.numbering->num_id.has_value()) {
        stream << *paragraph.numbering->num_id;
    } else {
        stream << "none";
    }
    stream << " level=";
    if (paragraph.numbering->level.has_value()) {
        stream << *paragraph.numbering->level;
    } else {
        stream << "none";
    }
    stream << " definition_id=";
    if (paragraph.numbering->definition_id.has_value()) {
        stream << *paragraph.numbering->definition_id;
    } else {
        stream << "none";
    }
    stream << " definition_name=";
    if (paragraph.numbering->definition_name.has_value()) {
        stream << *paragraph.numbering->definition_name;
    } else {
        stream << "none";
    }
}

void print_template_paragraph_summary(
    std::ostream &stream, const featherdoc::paragraph_inspection_summary &paragraph) {
    stream << "paragraph[" << paragraph.index << "]: style_id=";
    if (paragraph.style_id.has_value()) {
        stream << *paragraph.style_id;
    } else {
        stream << "none";
    }
    stream << " bidi=";
    if (paragraph.bidi.has_value()) {
        stream << yes_no(*paragraph.bidi);
    } else {
        stream << "none";
    }
    stream << " numbering=";
    print_template_paragraph_numbering(stream, paragraph);
    stream << " runs=" << paragraph.run_count
           << " text=" << format_paragraph_text(paragraph.text);
}

void print_template_run_summary(std::ostream &stream,
                                const featherdoc::run_inspection_summary &run) {
    stream << "run[" << run.index << "]: style_id=";
    if (run.style_id.has_value()) {
        stream << *run.style_id;
    } else {
        stream << "none";
    }
    stream << " font_family=";
    if (run.font_family.has_value()) {
        stream << *run.font_family;
    } else {
        stream << "none";
    }
    stream << " east_asia_font_family=";
    if (run.east_asia_font_family.has_value()) {
        stream << *run.east_asia_font_family;
    } else {
        stream << "none";
    }
    stream << " text_color=";
    if (run.text_color.has_value()) {
        stream << *run.text_color;
    } else {
        stream << "none";
    }
    stream << " bold=";
    if (run.bold.has_value()) {
        stream << yes_no(*run.bold);
    } else {
        stream << "none";
    }
    stream << " italic=";
    if (run.italic.has_value()) {
        stream << yes_no(*run.italic);
    } else {
        stream << "none";
    }
    stream << " underline=";
    if (run.underline.has_value()) {
        stream << yes_no(*run.underline);
    } else {
        stream << "none";
    }
    stream << " font_size_points=";
    if (run.font_size_points.has_value()) {
        stream << *run.font_size_points;
    } else {
        stream << "none";
    }
    stream << " language=";
    if (run.language.has_value()) {
        stream << *run.language;
    } else {
        stream << "none";
    }
    stream << " east_asia_language=";
    if (run.east_asia_language.has_value()) {
        stream << *run.east_asia_language;
    } else {
        stream << "none";
    }
    stream << " bidi_language=";
    if (run.bidi_language.has_value()) {
        stream << *run.bidi_language;
    } else {
        stream << "none";
    }
    stream << " rtl=";
    if (run.rtl.has_value()) {
        stream << yes_no(*run.rtl);
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(run.text);
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

void inspect_paragraphs(const std::vector<inspected_body_paragraph> &paragraphs,
                        bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << paragraphs.size() << ",\"paragraphs\":[";
        for (std::size_t index = 0; index < paragraphs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_body_paragraph_summary(std::cout, paragraphs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "paragraphs: " << paragraphs.size() << '\n';
    for (const auto &paragraph : paragraphs) {
        print_body_paragraph_summary(std::cout, paragraph);
        std::cout << '\n';
    }
}

void inspect_paragraph(const inspected_body_paragraph &paragraph, bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph\":";
        write_json_body_paragraph_summary(std::cout, paragraph);
        std::cout << "}\n";
        return;
    }

    std::cout << "index: " << paragraph.index << '\n'
              << "section_index: " << paragraph.section_index << '\n'
              << "style_id: ";
    if (paragraph.style_id.has_value()) {
        std::cout << *paragraph.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "numbering: ";
    print_body_paragraph_numbering(std::cout, paragraph);
    std::cout << '\n'
              << "section_break: "
              << (paragraph.has_section_break ? "yes" : "no") << '\n'
              << "text: " << format_paragraph_text(paragraph.text) << '\n';
}

void inspect_runs(std::size_t paragraph_index,
                  const std::vector<inspected_body_run> &runs, bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph_index\":" << paragraph_index
                  << ",\"count\":" << runs.size() << ",\"runs\":[";
        for (std::size_t index = 0; index < runs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_body_run_summary(std::cout, runs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "runs: " << runs.size() << '\n';
    for (const auto &run : runs) {
        print_body_run_summary(std::cout, run);
        std::cout << '\n';
    }
}

void inspect_run(std::size_t paragraph_index, const inspected_body_run &run,
                 bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph_index\":" << paragraph_index << ",\"run\":";
        write_json_body_run_summary(std::cout, run);
        std::cout << "}\n";
        return;
    }

    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "index: " << run.index << '\n'
              << "style_id: ";
    if (run.style_id.has_value()) {
        std::cout << *run.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_family: ";
    if (run.font_family.has_value()) {
        std::cout << *run.font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "east_asia_font_family: ";
    if (run.east_asia_font_family.has_value()) {
        std::cout << *run.east_asia_font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text_color: ";
    if (run.text_color.has_value()) {
        std::cout << *run.text_color;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bold: ";
    if (run.bold.has_value()) {
        std::cout << yes_no(*run.bold);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "italic: ";
    if (run.italic.has_value()) {
        std::cout << yes_no(*run.italic);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "underline: ";
    if (run.underline.has_value()) {
        std::cout << yes_no(*run.underline);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_size_points: ";
    if (run.font_size_points.has_value()) {
        std::cout << *run.font_size_points;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "language: ";
    if (run.language.has_value()) {
        std::cout << *run.language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(run.text) << '\n';
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

void inspect_template_paragraphs(
    const selected_template_part &selected,
    const std::vector<featherdoc::paragraph_inspection_summary> &paragraphs,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"count\":" << paragraphs.size() << ",\"paragraphs\":[";
        for (std::size_t index = 0; index < paragraphs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_paragraph_summary(std::cout, paragraphs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "paragraphs: " << paragraphs.size() << '\n';
    for (const auto &paragraph : paragraphs) {
        print_template_paragraph_summary(std::cout, paragraph);
        std::cout << '\n';
    }
}

void inspect_template_paragraph(
    const selected_template_part &selected,
    const featherdoc::paragraph_inspection_summary &paragraph, bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph\":";
        write_json_template_paragraph_summary(std::cout, paragraph);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "index: " << paragraph.index << '\n'
              << "style_id: ";
    if (paragraph.style_id.has_value()) {
        std::cout << *paragraph.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bidi: ";
    if (paragraph.bidi.has_value()) {
        std::cout << yes_no(*paragraph.bidi);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "numbering: ";
    print_template_paragraph_numbering(std::cout, paragraph);
    std::cout << '\n' << "run_count: " << paragraph.run_count << '\n'
              << "text: " << format_paragraph_text(paragraph.text) << '\n';
}

void inspect_template_runs(
    const selected_template_part &selected, std::size_t paragraph_index,
    const std::vector<featherdoc::run_inspection_summary> &runs,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph_index\":" << paragraph_index
                  << ",\"count\":" << runs.size() << ",\"runs\":[";
        for (std::size_t index = 0; index < runs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_run_summary(std::cout, runs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "runs: " << runs.size() << '\n';
    for (const auto &run : runs) {
        print_template_run_summary(std::cout, run);
        std::cout << '\n';
    }
}

void inspect_template_run(const selected_template_part &selected,
                          std::size_t paragraph_index,
                          const featherdoc::run_inspection_summary &run,
                          bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph_index\":" << paragraph_index << ",\"run\":";
        write_json_template_run_summary(std::cout, run);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "index: " << run.index << '\n'
              << "style_id: ";
    if (run.style_id.has_value()) {
        std::cout << *run.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_family: ";
    if (run.font_family.has_value()) {
        std::cout << *run.font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "east_asia_font_family: ";
    if (run.east_asia_font_family.has_value()) {
        std::cout << *run.east_asia_font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text_color: ";
    if (run.text_color.has_value()) {
        std::cout << *run.text_color;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bold: ";
    if (run.bold.has_value()) {
        std::cout << yes_no(*run.bold);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "italic: ";
    if (run.italic.has_value()) {
        std::cout << yes_no(*run.italic);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "underline: ";
    if (run.underline.has_value()) {
        std::cout << yes_no(*run.underline);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_size_points: ";
    if (run.font_size_points.has_value()) {
        std::cout << *run.font_size_points;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "language: ";
    if (run.language.has_value()) {
        std::cout << *run.language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "east_asia_language: ";
    if (run.east_asia_language.has_value()) {
        std::cout << *run.east_asia_language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bidi_language: ";
    if (run.bidi_language.has_value()) {
        std::cout << *run.bidi_language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "rtl: ";
    if (run.rtl.has_value()) {
        std::cout << yes_no(*run.rtl);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(run.text) << '\n';
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


    if (command == "inspect-paragraphs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-paragraphs expects an input path",
                              json_output);
            return 2;
        }

        inspect_paragraphs_options options;
        std::string error_message;
        if (!parse_inspect_paragraphs_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto input_path = path_type(std::string(arguments[1]));
        if (!open_document(input_path, doc, command, options.json_output)) {
            return 1;
        }

        std::vector<inspected_body_paragraph> paragraphs;
        if (!load_body_paragraph_summaries(input_path, paragraphs, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "inspect",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (options.paragraph_index.has_value()) {
            if (*options.paragraph_index >= paragraphs.size()) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "paragraph index '" +
                    std::to_string(*options.paragraph_index) +
                    "' is out of range";
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "inspect",
                                         "paragraph index is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            inspect_paragraph(paragraphs[*options.paragraph_index],
                              options.json_output);
            return 0;
        }

        inspect_paragraphs(paragraphs, options.json_output);
        return 0;
    }

    if (command == "inspect-tables") {
        return run_inspect_tables_command(command, arguments);
    }

    if (command == "plan-table-position-presets") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "plan-table-position-presets expects an input path and --preset <name>",
                json_output);
            return 2;
        }

        plan_table_position_presets_options options;
        std::string error_message;
        if (!parse_plan_table_position_presets_options(arguments, 2U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        options.input_path = path_type(std::string(arguments[1]));
        if (!open_document(*options.input_path, doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto tables = doc.inspect_tables();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto plan = build_table_position_preset_plan(
            tables, *options.preset, options.replace_positioned,
            options.input_path, options.output_path);
        if (options.output_plan_path.has_value() &&
            !write_table_position_preset_plan_file(*options.output_plan_path,
                                                   plan, options,
                                                   error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write table position preset plan",
                                     error_info, options.json_output);
            return 1;
        }

        if (options.json_output) {
            write_json_table_position_preset_plan(std::cout, plan, options);
        } else {
            write_text_table_position_preset_plan(plan, options);
        }

        return options.fail_on_change && !plan.items.empty() ? 1 : 0;
    }

    if (command == "apply-table-position-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "apply-table-position-plan expects a plan path",
                              json_output);
            return 2;
        }

        apply_table_position_plan_options options;
        std::string error_message;
        if (!parse_apply_table_position_plan_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        parsed_table_position_plan_file plan;
        if (!read_table_position_plan_file(path_type(std::string(arguments[1])),
                                           plan, error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "plan",
                                     "failed to read table position plan",
                                     error_info, options.json_output);
            return 1;
        }


        const auto output_path = options.output_path.has_value()
                                     ? options.output_path
                                     : plan.resolved_output_path;
        if (!options.dry_run && !output_path.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "table position plan has no resolved output path; "
                                "rerun plan-table-position-presets with --output "
                                "or pass --output to apply-table-position-plan";
            report_operation_failure(command, "plan",
                                     "missing safe output path", error_info,
                                     options.json_output);
            return 1;
        }

        if (!open_document(plan.input_path, doc, command, options.json_output)) {
            return 1;
        }
        const auto current_tables = doc.inspect_tables();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }
        if (current_tables.size() != plan.table_count) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "table count changed since plan was generated: " +
                                std::to_string(current_tables.size()) +
                                " current, " + std::to_string(plan.table_count) +
                                " planned";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "plan",
                                     "table count mismatch", error_info,
                                     options.json_output);
            return 1;
        }

        for (const auto &expected_fingerprint : plan.table_fingerprints) {
            const auto current_fingerprint =
                make_table_position_table_fingerprint(
                    current_tables[expected_fingerprint.table_index]);
            if (!table_position_table_fingerprints_equal(
                    expected_fingerprint, current_fingerprint)) {
                featherdoc::document_error_info error_info{};
                error_info.detail =
                    describe_table_position_fingerprint_differences(
                        expected_fingerprint, current_fingerprint);
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "plan",
                                         "table fingerprint mismatch", error_info,
                                         options.json_output);
                return 1;
            }
        }

        const auto target_position = make_table_position_preset(plan.preset);
        if (options.dry_run) {
            if (options.json_output) {
                std::cout << "{\"command\":\"apply-table-position-plan\""
                          << ",\"ok\":true,\"dry_run\":true"
                          << ",\"input_path\":";
                write_json_string(std::cout, plan.input_path.string());
                std::cout << ",\"preset\":";
                write_json_string(std::cout,
                                  table_position_preset_name(plan.preset));
                std::cout << ",\"table_count\":" << plan.table_count
                          << ",\"fingerprint_checked_count\":"
                          << plan.table_fingerprints.size()
                          << ",\"would_apply_count\":"
                          << plan.automatic_table_indices.size()
                          << ",\"table_indices\":";
                write_json_size_array(std::cout, plan.automatic_table_indices);
                std::cout << ",\"resolved_output_path\":";
                if (output_path.has_value()) {
                    write_json_string(std::cout, output_path->string());
                } else {
                    std::cout << "null";
                }
                std::cout << ",\"position\":";
                write_json_table_position(std::cout, target_position);
                std::cout << "}\n";
            } else {
                std::cout << "table_position_plan_validation: ok\n"
                          << "dry_run: true\n"
                          << "input_path: " << plan.input_path.string() << '\n'
                          << "preset: "
                          << table_position_preset_name(plan.preset) << '\n'
                          << "table_count: " << plan.table_count << '\n'
                          << "fingerprint_checked_count: "
                          << plan.table_fingerprints.size() << '\n'
                          << "would_apply_count: "
                          << plan.automatic_table_indices.size() << '\n'
                          << "table_indices: ";
                write_text_size_array(std::cout, plan.automatic_table_indices);
                std::cout << "\nresolved_output_path: ";
                if (output_path.has_value()) {
                    std::cout << output_path->string();
                } else {
                    std::cout << "none";
                }
                std::cout << '\n';
            }
            return 0;
        }

        std::vector<std::size_t> mutated_table_indices;
        for (const auto table_index : plan.automatic_table_indices) {
            featherdoc::Table table;
            if (!resolve_body_table(doc, table_index, table, command,
                                    options.json_output)) {
                return 1;
            }
            if (!table.set_position(target_position)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail = "target table handle is not valid";
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "mutate",
                                         "failed to set table position",
                                         error_info, options.json_output);
                return 1;
            }
            mutated_table_indices.push_back(table_index);
        }

        if (!save_document(doc, output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, output_path,
                [&plan, &mutated_table_indices,
                 &target_position](std::ostream &stream) {
                    stream << ",\"input_path\":";
                    write_json_string(stream, plan.input_path.string());
                    stream << ",\"preset\":";
                    write_json_string(stream, table_position_preset_name(plan.preset));
                    stream << ",\"table_count\":" << plan.table_count
                           << ",\"fingerprint_checked_count\":"
                           << plan.table_fingerprints.size()
                           << ",\"applied_count\":"
                           << mutated_table_indices.size()
                           << ",\"table_indices\":";
                    write_json_size_array(stream, mutated_table_indices);
                    stream << ",\"position\":";
                    write_json_table_position(stream, target_position);
                });
        }

        return 0;
    }

    if (command == "set-table-position") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-table-position expects an input path and a table index",
                json_output);
            return 2;
        }

        const auto target_all = arguments[2] == "all";
        std::vector<std::size_t> requested_table_indices;
        if (!target_all) {
            std::size_t table_index = 0U;
            if (!parse_index(arguments[2], table_index)) {
                print_parse_error(command,
                                  "invalid table index: " +
                                      std::string(arguments[2]),
                                  json_output);
                return 2;
            }
            requested_table_indices.push_back(table_index);
        }

        table_position_options options;
        std::string error_message;
        if (!parse_table_position_options(arguments, 3U, options,
                                          error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }
        if (target_all && !options.additional_table_indices.empty()) {
            print_parse_error(command,
                              "--table cannot be combined with all table target",
                              options.json_output);
            return 2;
        }
        for (const auto additional_table_index : options.additional_table_indices) {
            if (std::find(requested_table_indices.begin(),
                          requested_table_indices.end(),
                          additional_table_index) != requested_table_indices.end()) {
                print_parse_error(command,
                                  "duplicate table target: " +
                                      std::to_string(additional_table_index),
                                  options.json_output);
                return 2;
            }
            requested_table_indices.push_back(additional_table_index);
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::vector<std::size_t> mutated_table_indices;
        if (target_all) {
            std::size_t table_index = 0U;
            for (featherdoc::Table table = doc.tables(); table.has_next();
                 table.next(), ++table_index) {
                if (!table.set_position(options.position)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code =
                        std::make_error_code(std::errc::invalid_argument);
                    error_info.detail = "target table handle is not valid";
                    error_info.entry_name = "word/document.xml";
                    report_operation_failure(command, "mutate",
                                             "failed to set table position",
                                             error_info, options.json_output);
                    return 1;
                }
                mutated_table_indices.push_back(table_index);
            }

            if (mutated_table_indices.empty()) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail = "no body tables found";
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "mutate",
                                         "no body tables found", error_info,
                                         options.json_output);
                return 1;
            }
        } else {
            for (const auto table_index : requested_table_indices) {
                featherdoc::Table table;
                if (!resolve_body_table(doc, table_index, table, command,
                                        options.json_output)) {
                    return 1;
                }

                if (!table.set_position(options.position)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code =
                        std::make_error_code(std::errc::invalid_argument);
                    error_info.detail = "target table handle is not valid";
                    error_info.entry_name = "word/document.xml";
                    report_operation_failure(command, "mutate",
                                             "failed to set table position",
                                             error_info, options.json_output);
                    return 1;
                }
                mutated_table_indices.push_back(table_index);
            }
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&mutated_table_indices, &options](std::ostream &stream) {
                    stream << ",\"table_indices\":[";
                    for (std::size_t index = 0;
                         index < mutated_table_indices.size(); ++index) {
                        if (index > 0U) {
                            stream << ',';
                        }
                        stream << mutated_table_indices[index];
                    }
                    stream << "],\"positions\":[";
                    for (std::size_t index = 0;
                         index < mutated_table_indices.size(); ++index) {
                        if (index > 0U) {
                            stream << ',';
                        }
                        write_json_table_position(stream, options.position);
                    }
                    stream << ']';
                });
        }

        return 0;
    }

    if (command == "clear-table-position") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-position expects an input path and a table index",
                json_output);
            return 2;
        }

        const auto target_all = arguments[2] == "all";
        std::vector<std::size_t> requested_table_indices;
        if (!target_all) {
            std::size_t table_index = 0U;
            if (!parse_index(arguments[2], table_index)) {
                print_parse_error(command,
                                  "invalid table index: " +
                                      std::string(arguments[2]),
                                  json_output);
                return 2;
            }
            requested_table_indices.push_back(table_index);
        }

        table_position_target_options options;
        std::string error_message;
        if (!parse_table_position_target_options(arguments, 3U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }
        if (target_all && !options.additional_table_indices.empty()) {
            print_parse_error(command,
                              "--table cannot be combined with all table target",
                              options.json_output);
            return 2;
        }
        for (const auto additional_table_index : options.additional_table_indices) {
            if (std::find(requested_table_indices.begin(),
                          requested_table_indices.end(),
                          additional_table_index) != requested_table_indices.end()) {
                print_parse_error(command,
                                  "duplicate table target: " +
                                      std::to_string(additional_table_index),
                                  options.json_output);
                return 2;
            }
            requested_table_indices.push_back(additional_table_index);
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::vector<std::size_t> mutated_table_indices;
        if (target_all) {
            std::size_t table_index = 0U;
            for (featherdoc::Table table = doc.tables(); table.has_next();
                 table.next(), ++table_index) {
                if (!table.clear_position()) {
                    featherdoc::document_error_info error_info{};
                    error_info.code =
                        std::make_error_code(std::errc::invalid_argument);
                    error_info.detail = "target table handle is not valid";
                    error_info.entry_name = "word/document.xml";
                    report_operation_failure(command, "mutate",
                                             "failed to clear table position",
                                             error_info, options.json_output);
                    return 1;
                }
                mutated_table_indices.push_back(table_index);
            }

            if (mutated_table_indices.empty()) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail = "no body tables found";
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "mutate",
                                         "no body tables found", error_info,
                                         options.json_output);
                return 1;
            }
        } else {
            for (const auto table_index : requested_table_indices) {
                featherdoc::Table table;
                if (!resolve_body_table(doc, table_index, table, command,
                                        options.json_output)) {
                    return 1;
                }

                if (!table.clear_position()) {
                    featherdoc::document_error_info error_info{};
                    error_info.code =
                        std::make_error_code(std::errc::invalid_argument);
                    error_info.detail = "target table handle is not valid";
                    error_info.entry_name = "word/document.xml";
                    report_operation_failure(command, "mutate",
                                             "failed to clear table position",
                                             error_info, options.json_output);
                    return 1;
                }
                mutated_table_indices.push_back(table_index);
            }
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&mutated_table_indices](std::ostream &stream) {
                    stream << ",\"table_indices\":[";
                    for (std::size_t index = 0;
                         index < mutated_table_indices.size(); ++index) {
                        if (index > 0U) {
                            stream << ',';
                        }
                        stream << mutated_table_indices[index];
                    }
                    stream << "],\"positions\":[";
                    for (std::size_t index = 0;
                         index < mutated_table_indices.size(); ++index) {
                        if (index > 0U) {
                            stream << ',';
                        }
                        stream << "null";
                    }
                    stream << ']';
                });
        }

        return 0;
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

    if (command == "set-table-column-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "set-table-column-width expects an input path, a table index, a column index, and a width in twips",
                json_output);
            return 2;
        }

        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t column_index = 0U;
        if (!parse_index(arguments[3], column_index)) {
            print_parse_error(command,
                              "invalid column index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::uint32_t width_twips = 0U;
        if (!parse_uint32(arguments[4], width_twips)) {
            print_parse_error(command,
                              "invalid width twips: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 5U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::table_inspection_summary inspected_table{};
        if (!load_body_table_summary(doc, table_index, inspected_table, command,
                                     options.json_output)) {
            return 1;
        }

        if (column_index >= inspected_table.column_count) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "column index '" + std::to_string(column_index) +
                "' is out of range for table index '" +
                std::to_string(table_index) + "'";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "column index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_column_width_twips(column_index, width_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table column width",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, column_index, width_twips](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"column_index\":" << column_index
                           << ",\"width_twips\":" << width_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-column-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "clear-table-column-width expects an input path, a table index, and a column index",
                json_output);
            return 2;
        }

        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t column_index = 0U;
        if (!parse_index(arguments[3], column_index)) {
            print_parse_error(command,
                              "invalid column index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 4U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::table_inspection_summary inspected_table{};
        if (!load_body_table_summary(doc, table_index, inspected_table, command,
                                     options.json_output)) {
            return 1;
        }

        if (column_index >= inspected_table.column_count) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "column index '" + std::to_string(column_index) +
                "' is out of range for table index '" +
                std::to_string(table_index) + "'";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "column index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.clear_column_width(column_index)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table column width",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, column_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"column_index\":" << column_index;
                });
        }

        return 0;
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

    if (command == "insert-table-before" || command == "insert-table-after") {
        const auto json_output = has_json_flag(arguments);
        const auto insert_before = command == "insert-table-before";
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                std::string(command) +
                    std::string{
                        " expects an input path, a table index, a row count, and a column count"},
                json_output);
            return 2;
        }

        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t row_count = 0U;
        if (!parse_index(arguments[3], row_count)) {
            print_parse_error(command,
                              "invalid row count: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }
        if (row_count == 0U) {
            print_parse_error(command, "row count must be greater than 0",
                              json_output);
            return 2;
        }

        std::size_t column_count = 0U;
        if (!parse_index(arguments[4], column_count)) {
            print_parse_error(command,
                              "invalid column count: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }
        if (column_count == 0U) {
            print_parse_error(command, "column count must be greater than 0",
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 5U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        auto inserted_table =
            insert_before ? table.insert_table_before(row_count, column_count)
                          : table.insert_table_after(row_count, column_count);
        if (!inserted_table.has_next()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to insert table", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_count, column_count,
                 insert_before](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"inserted_table_index\":"
                           << (insert_before ? table_index : table_index + 1U)
                           << ",\"row_count\":" << row_count
                           << ",\"column_count\":" << column_count;
                });
        }

        return 0;
    }

    if (command == "insert-table-like-before" ||
        command == "insert-table-like-after") {
        const auto json_output = has_json_flag(arguments);
        const auto insert_before = command == "insert-table-like-before";
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                std::string(command) +
                    std::string{" expects an input path and a table index"},
                json_output);
            return 2;
        }

        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 3U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        auto inserted_table =
            insert_before ? table.insert_table_like_before()
                          : table.insert_table_like_after();
        if (!inserted_table.has_next()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to insert table", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, insert_before](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"inserted_table_index\":"
                           << (insert_before ? table_index : table_index + 1U);
                });
        }

        return 0;
    }

    if (command == "insert-paragraph-after-table") {
        return run_insert_paragraph_after_table_command(command, arguments);
    }

    if (command == "remove-table") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "remove-table expects an input path and a table index",
                              json_output);
            return 2;
        }

        std::size_t table_index = 0U;
        if (!parse_index(arguments[2], table_index)) {
            print_parse_error(command,
                              "invalid table index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 3U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.remove()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "cannot remove table index '" + std::to_string(table_index) +
                "' because its parent would be left without block content";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to remove table", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index;
                });
        }

        return 0;
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

    if (command == "inspect-runs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "inspect-runs expects an input path and a paragraph index",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        inspect_runs_options options;
        std::string error_message;
        if (!parse_inspect_runs_options(arguments, 3U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto input_path = path_type(std::string(arguments[1]));
        if (!open_document(input_path, doc, command, options.json_output)) {
            return 1;
        }

        std::vector<inspected_body_run> runs;
        bool paragraph_found = false;
        if (!load_body_run_summaries(input_path, paragraph_index, runs,
                                     paragraph_found, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "inspect",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!paragraph_found) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" + std::to_string(paragraph_index) +
                "' is out of range";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        if (options.run_index.has_value()) {
            if (*options.run_index >= runs.size()) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "run index '" + std::to_string(*options.run_index) +
                    "' is out of range for paragraph index '" +
                    std::to_string(paragraph_index) + "'";
                error_info.entry_name = "word/document.xml";
                report_operation_failure(command, "inspect",
                                         "run index is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            inspect_run(paragraph_index, runs[*options.run_index],
                        options.json_output);
            return 0;
        }

        inspect_runs(paragraph_index, runs, options.json_output);
        return 0;
    }

    if (command == "inspect-template-paragraphs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "inspect-template-paragraphs expects an input path",
                json_output);
            return 2;
        }

        template_inspect_paragraphs_options options;
        std::string error_message;
        if (!parse_template_inspect_paragraphs_options(arguments, 2U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (options.paragraph_index.has_value()) {
            const auto paragraph =
                selected.part.inspect_paragraph(*options.paragraph_index);
            if (!paragraph.has_value()) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "paragraph index '" +
                    std::to_string(*options.paragraph_index) +
                    "' is out of range";
                error_info.entry_name = std::string(selected.part.entry_name());
                report_operation_failure(command, "inspect",
                                         "paragraph index is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            inspect_template_paragraph(selected, *paragraph, options.json_output);
            return 0;
        }

        const auto paragraphs = selected.part.inspect_paragraphs();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_template_paragraphs(selected, paragraphs, options.json_output);
        return 0;
    }

    if (command == "inspect-template-runs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "inspect-template-runs expects an input path and a paragraph index",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        template_inspect_runs_options options;
        std::string error_message;
        if (!parse_template_inspect_runs_options(arguments, 3U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto paragraph = selected.part.inspect_paragraph(paragraph_index);
        if (!paragraph.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" + std::to_string(paragraph_index) +
                "' is out of range";
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        if (options.run_index.has_value()) {
            const auto run =
                selected.part.inspect_paragraph_run(paragraph_index,
                                                    *options.run_index);
            if (!run.has_value()) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "run index '" + std::to_string(*options.run_index) +
                    "' is out of range for paragraph index '" +
                    std::to_string(paragraph_index) + "'";
                error_info.entry_name = std::string(selected.part.entry_name());
                report_operation_failure(command, "inspect",
                                         "run index is out of range", error_info,
                                         options.json_output);
                return 1;
            }

            inspect_template_run(selected, paragraph_index, *run,
                                 options.json_output);
            return 0;
        }

        const auto runs = selected.part.inspect_paragraph_runs(paragraph_index);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_template_runs(selected, paragraph_index, runs,
                              options.json_output);
        return 0;
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
