#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_diff.hpp"
#include "featherdoc_cli_numbering_catalog_lint.hpp"
#include "featherdoc_cli_numbering_catalog_options_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_numbering_options_parse.hpp"
#include "featherdoc_cli_page_setup.hpp"
#include "featherdoc_cli_page_setup_parse.hpp"
#include "featherdoc_cli_paragraph_numbering_commands.hpp"
#include "featherdoc_cli_paragraph_run_commands.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_semantic_diff.hpp"
#include "featherdoc_cli_semantic_diff_options_parse.hpp"
#include "featherdoc_cli_run_style_properties_commands.hpp"
#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_column_commands.hpp"
#include "featherdoc_cli_table_inspect_commands.hpp"
#include "featherdoc_cli_table_merge_commands.hpp"
#include "featherdoc_cli_table_row_commands.hpp"
#include "featherdoc_cli_table_row_summary.hpp"
#include "featherdoc_cli_table_structure_validation.hpp"
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
#include "featherdoc_cli_style_refactor_plan.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"
#include "featherdoc_cli_style_refactor_options_parse.hpp"
#include "featherdoc_cli_style_refactor_rollback_parse.hpp"
#include "featherdoc_cli_style_numbering_audit.hpp"
#include "featherdoc_cli_table_style_look_options_parse.hpp"
#include "featherdoc_cli_review_mutation_plan_parse.hpp"
#include "featherdoc_cli_run_recipe_parse.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_style_options_parse.hpp"
#include "featherdoc_cli_template_schema_model.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_options_parse.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"
#include "featherdoc_cli_template_schema_patch_ops.hpp"
#include "featherdoc_cli_template_schema_patch_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_field_parse.hpp"
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
using featherdoc_cli::apply_template_schema_patch;
using featherdoc_cli::apply_table_style_quality_fixes_options;
using featherdoc_cli::append_field_options;
using featherdoc_cli::audit_style_numbering;
using featherdoc_cli::audit_style_numbering_options;
using featherdoc_cli::audit_table_style_inheritance_options;
using featherdoc_cli::audit_table_style_quality_options;
using featherdoc_cli::audit_table_style_regions_options;
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
using featherdoc_cli::check_table_style_look_options;
using featherdoc_cli::clear_paragraph_style_options;
using featherdoc_cli::content_control_form_kind_name;
using featherdoc_cli::content_control_kind_name;
using featherdoc_cli::changed_template_schema_target;
using featherdoc_cli::build_table_position_preset_plan;
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
using featherdoc_cli::drawing_image_placement_name;
using featherdoc_cli::exported_template_schema_result;
using featherdoc_cli::exported_template_schema_skipped_bookmark;
using featherdoc_cli::exported_template_schema_target;
using featherdoc_cli::export_numbering_catalog_options;
using featherdoc_cli::export_template_schema_options;
using featherdoc_cli::format_paragraph_text;
using featherdoc_cli::filter_style_refactor_plan_by_min_confidence;
using featherdoc_cli::filter_style_refactor_plan_by_style_ids;
using featherdoc_cli::style_refactor_plan_options;
using featherdoc_cli::style_merge_suggestion_options;
using featherdoc_cli::style_refactor_apply_options;
using featherdoc_cli::style_merge_restore_options;
using featherdoc_cli::floating_image_horizontal_reference_name;
using featherdoc_cli::floating_image_vertical_reference_name;
using featherdoc_cli::floating_image_wrap_mode_name;
using featherdoc_cli::filter_drawing_images;
using featherdoc_cli::has_json_flag;
using featherdoc_cli::has_drawing_image_selector;
using featherdoc_cli::has_inspect_image_filters;
using featherdoc_cli::is_docx_path;
using featherdoc_cli::is_append_field_command;
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
using featherdoc_cli::hyperlink_mutation_options;
using featherdoc_cli::omml_mutation_options;
using featherdoc_cli::review_note_mutation_options;
using featherdoc_cli::comment_mutation_options;
using featherdoc_cli::comment_metadata_mutation_options;
using featherdoc_cli::revision_authoring_options;
using featherdoc_cli::revision_metadata_mutation_options;
using featherdoc_cli::semantic_diff_options;
using featherdoc_cli::inspect_images_options;
using featherdoc_cli::cli_text_source_options;
using featherdoc_cli::bookmark_text_binding_source;
using featherdoc_cli::inspect_bookmarks_options;
using featherdoc_cli::inspect_content_controls_options;
using featherdoc_cli::replace_content_control_text_options;
using featherdoc_cli::set_content_control_form_state_options;
using featherdoc_cli::sync_content_controls_from_custom_xml_options;
using featherdoc_cli::replace_bookmark_paragraphs_options;
using featherdoc_cli::bookmark_table_replacement_options;
using featherdoc_cli::replace_content_control_paragraphs_options;
using featherdoc_cli::content_control_table_replacement_options;
using featherdoc_cli::replace_content_control_image_options;
using featherdoc_cli::remove_bookmark_block_options;
using featherdoc_cli::replace_bookmark_text_options;
using featherdoc_cli::fill_bookmarks_options;
using featherdoc_cli::set_bookmark_block_visibility_options;
using featherdoc_cli::apply_bookmark_block_visibility_options;
using featherdoc_cli::replace_image_options;
using featherdoc_cli::extract_image_options;
using featherdoc_cli::remove_image_options;
using featherdoc_cli::append_image_options;
using featherdoc_cli::describe_inspect_image_filters;
using featherdoc_cli::drawing_image_matches_filters;
using featherdoc_cli::ensure_character_style_options;
using featherdoc_cli::ensure_paragraph_style_options;
using featherdoc_cli::ensure_table_style_options;
using featherdoc_cli::parse_ensure_character_style_options;
using featherdoc_cli::parse_ensure_paragraph_style_options;
using featherdoc_cli::parse_ensure_table_style_options;
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
using featherdoc_cli::parse_append_field_options;
using featherdoc_cli::parse_apply_table_style_quality_fixes_options;
using featherdoc_cli::parse_audit_style_numbering_options;
using featherdoc_cli::parse_audit_table_style_inheritance_options;
using featherdoc_cli::parse_audit_table_style_quality_options;
using featherdoc_cli::parse_audit_table_style_regions_options;
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
using featherdoc_cli::parse_style_refactor_plan_options;
using featherdoc_cli::parse_style_merge_suggestion_options;
using featherdoc_cli::parse_style_refactor_apply_options;
using featherdoc_cli::parse_style_merge_restore_options;
using featherdoc_cli::parse_simple_document_mutation_options;
using featherdoc_cli::parse_hyperlink_mutation_options;
using featherdoc_cli::parse_omml_mutation_options;
using featherdoc_cli::parse_review_note_mutation_options;
using featherdoc_cli::parse_comment_mutation_options;
using featherdoc_cli::parse_comment_metadata_mutation_options;
using featherdoc_cli::parse_revision_authoring_options;
using featherdoc_cli::parse_revision_metadata_mutation_options;
using featherdoc_cli::parse_inspect_images_options;
using featherdoc_cli::parse_inspect_bookmarks_options;
using featherdoc_cli::parse_inspect_content_controls_options;
using featherdoc_cli::parse_replace_content_control_text_options;
using featherdoc_cli::parse_set_content_control_form_state_options;
using featherdoc_cli::parse_sync_content_controls_from_custom_xml_options;
using featherdoc_cli::parse_replace_bookmark_paragraphs_options;
using featherdoc_cli::parse_bookmark_table_replacement_options;
using featherdoc_cli::parse_content_control_table_replacement_options;
using featherdoc_cli::parse_replace_content_control_paragraphs_options;
using featherdoc_cli::parse_replace_content_control_image_options;
using featherdoc_cli::parse_remove_bookmark_block_options;
using featherdoc_cli::parse_replace_bookmark_text_options;
using featherdoc_cli::parse_fill_bookmarks_options;
using featherdoc_cli::parse_set_bookmark_block_visibility_options;
using featherdoc_cli::parse_apply_bookmark_block_visibility_options;
using featherdoc_cli::parse_replace_image_options;
using featherdoc_cli::parse_extract_image_options;
using featherdoc_cli::parse_remove_image_options;
using featherdoc_cli::parse_append_image_options;
using featherdoc_cli::print_drawing_image_summary;
using featherdoc_cli::print_inspect_image_filters;
using featherdoc_cli::parse_semantic_diff_options;
using featherdoc_cli::parse_bookmark_image_command_options;
using featherdoc_cli::parse_validate_template_options;
using featherdoc_cli::parse_validate_template_schema_options;
using featherdoc_cli::parse_int32;
using featherdoc_cli::parse_build_template_schema_patch_options;
using featherdoc_cli::parse_check_numbering_catalog_options;
using featherdoc_cli::parse_check_table_style_look_options;
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
using featherdoc_cli::parse_plan_table_style_quality_fixes_options;
using featherdoc_cli::parse_preview_template_schema_patch_options;
using featherdoc_cli::parse_repair_style_numbering_options;
using featherdoc_cli::parse_repair_table_style_look_options;
using featherdoc_cli::parse_repair_template_schema_options;
using featherdoc_cli::parse_rename_style_options;
using featherdoc_cli::parse_run_recipe_options;
using featherdoc_cli::read_run_recipe_inputs_file;
using featherdoc_cli::read_run_recipe_recipe_id;
using featherdoc_cli::parse_section_text_options;
using featherdoc_cli::parse_set_section_page_setup_options;
using featherdoc_cli::parse_set_update_fields_on_open_options;
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
using featherdoc_cli::parse_table_style_look_options;
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
using featherdoc_cli::plan_table_style_quality_fixes_options;
using featherdoc_cli::preview_template_schema_patch_options;
using featherdoc_cli::print_error_info;
using featherdoc_cli::print_parse_error;
using featherdoc_cli::print_usage;
using featherdoc_cli::report_document_error;
using featherdoc_cli::report_operation_failure;
using featherdoc_cli::resolve_selected_drawing_image;
using featherdoc_cli::resolve_section_page_setup;
using featherdoc_cli::read_numbering_catalog_file;
using featherdoc_cli::read_numbering_catalog_patch_file;
using featherdoc_cli::read_review_mutation_plan_build_request_file;
using featherdoc_cli::read_review_mutation_plan_file;
using featherdoc_cli::read_style_refactor_plan_file;
using featherdoc_cli::read_style_refactor_rollback_file;
using featherdoc_cli::read_table_position_plan_file;
using featherdoc_cli::read_template_schema_patch_file;
using featherdoc_cli::read_template_schema_file;
using featherdoc_cli::repaired_template_schema_summary;
using featherdoc_cli::repair_template_schema_result;
using featherdoc_cli::diff_template_schema_options;
using featherdoc_cli::merge_style_options;
using featherdoc_cli::normalize_template_schema_options;
using featherdoc_cli::prune_unused_styles_options;
using featherdoc_cli::rename_style_options;
using featherdoc_cli::repair_style_numbering_options;
using featherdoc_cli::repair_table_style_look_options;
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
using featherdoc_cli::table_style_look_options;
using featherdoc_cli::table_style_look_options_have_flag;
using featherdoc_cli::section_text_options;
using featherdoc_cli::set_paragraph_style_options;
using featherdoc_cli::clear_run_font_family_options;
using featherdoc_cli::clear_run_language_options;
using featherdoc_cli::clear_run_style_options;
using featherdoc_cli::set_run_font_family_options;
using featherdoc_cli::set_run_language_options;
using featherdoc_cli::set_run_style_options;
using featherdoc_cli::set_section_page_setup_options;
using featherdoc_cli::set_update_fields_on_open_options;
using featherdoc_cli::review_mutation_plan_build_request_operation;
using featherdoc_cli::review_mutation_plan_operation;
using featherdoc_cli::review_mutation_plan_operation_kind;
using featherdoc_cli::review_mutation_plan_operation_kind_name;
using featherdoc_cli::review_note_kind_name;
using featherdoc_cli::revision_kind_name;
using featherdoc_cli::row_height_rule_name;
using featherdoc_cli::run_recipe_options;
using featherdoc_cli::style_kind_name;
using featherdoc_cli::style_merge_suggestion_confidence_profile_is_valid;
using featherdoc_cli::style_merge_suggestion_confidence_profile_min_confidence;
using featherdoc_cli::style_usage_hit_kind_name;
using featherdoc_cli::style_usage_part_kind_name;
using featherdoc_cli::style_refactor_action_name;
using featherdoc_cli::style_refactor_command_template;
using featherdoc_cli::style_refactor_rollback_command_template;
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
using featherdoc_cli::template_field_name;
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
using featherdoc_cli::write_json_drawing_image_summary;
using featherdoc_cli::write_json_inspect_image_filters;
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
using featherdoc_cli::save_document;
using featherdoc_cli::write_json_mutation_result;
using featherdoc_cli::write_json_numbering_level_definition;
using featherdoc_cli::write_json_paragraph_style_numbering_link;
using featherdoc_cli::yes_no;

struct command_options {
    bool inherit_header_footer = true;
    std::optional<path_type> output_path;
    bool json_output = false;
};

struct table_style_look_repair_cli_result {
    bool apply = false;
    std::optional<path_type> output_path;
    featherdoc::table_style_look_consistency_report before;
    std::optional<featherdoc::table_style_look_consistency_report> after;
    std::size_t applicable_issue_count{0};
    std::size_t changed_table_count{0};
};

struct table_style_quality_apply_cli_result {
    std::optional<path_type> output_path;
    featherdoc::table_style_quality_fix_plan before;
    featherdoc::table_style_quality_fix_plan after;
    std::size_t changed_table_count{0};
};



enum class section_part_family {
    header,
    footer,
};






struct review_mutation_plan_preview_result {
    std::size_t index = 0U;
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::delete_paragraph_text_revision;
    bool ok = false;
    std::string message;
    std::optional<std::string> expected_text;
    std::optional<std::string> actual_text;
    std::optional<std::string> expected_comment_text;
    std::optional<std::string> actual_comment_text;
    std::optional<std::size_t> comment_index;
    std::optional<bool> expected_resolved;
    std::optional<bool> actual_resolved;
    std::optional<std::size_t> expected_parent_index;
    std::optional<std::size_t> actual_parent_index;
    std::optional<featherdoc::text_range_preview> preview;
};

struct review_mutation_plan_build_resolution {
    std::size_t index = 0U;
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::replace_text_range_revision;
    std::string find_text;
    std::size_t occurrence = 0U;
    std::optional<std::string> before_text;
    std::optional<std::string> after_text;
    bool require_unique = false;
    bool insert_after_match = false;
    std::size_t raw_matches_count = 0U;
    std::size_t matches_count = 0U;
    std::optional<std::size_t> selected_match_index;
    featherdoc::text_range_preview preview;
};


struct inspected_part_reference {
    std::size_t section_index = 0;
    std::string kind;
};

struct inspected_part_entry {
    std::string relationship_id;
    std::string entry_name;
    std::vector<inspected_part_reference> references;
    std::vector<std::string> paragraphs;
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

auto parse_options(const std::vector<std::string_view> &arguments,
                   std::size_t start_index, bool allow_no_inherit,
                   command_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (allow_no_inherit && argument == "--no-inherit") {
            options.inherit_header_footer = false;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1 >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1]));
            ++index;
            continue;
        }
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
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

auto has_section_header(featherdoc::Document &doc, std::size_t section_index,
                        featherdoc::section_reference_kind reference_kind)
    -> bool {
    return doc.section_header_paragraphs(section_index, reference_kind).has_next();
}

auto has_section_footer(featherdoc::Document &doc, std::size_t section_index,
                        featherdoc::section_reference_kind reference_kind)
    -> bool {
    return doc.section_footer_paragraphs(section_index, reference_kind).has_next();
}

auto section_part_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "header" : "footer";
}

auto bookmark_kind_to_template_slot_kind(featherdoc::bookmark_kind kind)
    -> std::optional<featherdoc::template_slot_kind> {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return featherdoc::template_slot_kind::text;
    case featherdoc::bookmark_kind::block:
        return featherdoc::template_slot_kind::block;
    case featherdoc::bookmark_kind::table_rows:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::bookmark_kind::block_range:
    case featherdoc::bookmark_kind::malformed:
    case featherdoc::bookmark_kind::mixed:
        return std::nullopt;
    }

    return std::nullopt;
}

auto content_control_kind_to_template_slot_kind(featherdoc::content_control_kind kind)
    -> featherdoc::template_slot_kind {
    switch (kind) {
    case featherdoc::content_control_kind::run:
        return featherdoc::template_slot_kind::text;
    case featherdoc::content_control_kind::table_row:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::table_cell:
    case featherdoc::content_control_kind::unknown:
        return featherdoc::template_slot_kind::block;
    }

    return featherdoc::template_slot_kind::block;
}

auto to_template_schema_part_kind(validation_part_family family)
    -> featherdoc::template_schema_part_kind {
    switch (family) {
    case validation_part_family::body:
        return featherdoc::template_schema_part_kind::body;
    case validation_part_family::header:
        return featherdoc::template_schema_part_kind::header;
    case validation_part_family::footer:
        return featherdoc::template_schema_part_kind::footer;
    case validation_part_family::section_header:
        return featherdoc::template_schema_part_kind::section_header;
    case validation_part_family::section_footer:
        return featherdoc::template_schema_part_kind::section_footer;
    }

    return featherdoc::template_schema_part_kind::body;
}

auto validation_part_name(featherdoc::template_schema_part_kind part) -> const char * {
    switch (part) {
    case featherdoc::template_schema_part_kind::body:
        return "body";
    case featherdoc::template_schema_part_kind::header:
        return "header";
    case featherdoc::template_schema_part_kind::footer:
        return "footer";
    case featherdoc::template_schema_part_kind::section_header:
        return "section-header";
    case featherdoc::template_schema_part_kind::section_footer:
        return "section-footer";
    }

    return "body";
}

auto section_reference_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "w:headerReference"
                                                 : "w:footerReference";
}

auto section_part_paragraphs(featherdoc::Document &doc, section_part_family family,
                             std::size_t index) -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.header_paragraphs(index);
    }

    return doc.footer_paragraphs(index);
}

auto collect_paragraph_text(featherdoc::Paragraph paragraph) -> std::string {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    return text;
}

auto collect_part_lines(featherdoc::Paragraph paragraph) -> std::vector<std::string> {
    std::vector<std::string> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        lines.push_back(collect_paragraph_text(paragraph));
    }
    return lines;
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

auto resolve_body_table(featherdoc::Document &doc, std::size_t table_index,
                        featherdoc::Table &table, std::string_view command,
                        bool json_output,
                        std::string_view stage = "mutate") -> bool {
    table = doc.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (!table.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "table index '" + std::to_string(table_index) + "' is out of range";
        error_info.entry_name = "word/document.xml";
        return report_operation_failure(command, stage,
                                        "table index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_body_table_row(featherdoc::Document &doc, std::size_t table_index,
                            std::size_t row_index, featherdoc::TableRow &row,
                            std::string_view command, bool json_output,
                            std::string_view stage = "mutate") -> bool {
    featherdoc::Table table;
    if (!resolve_body_table(doc, table_index, table, command, json_output,
                            stage)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next();
         ++current_index) {
        row.next();
    }

    if (!row.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'";
        error_info.entry_name = "word/document.xml";
        return report_operation_failure(command, stage,
                                        "row index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_body_table_cell(featherdoc::Document &doc, std::size_t table_index,
                              std::size_t row_index, std::size_t cell_index,
                              featherdoc::TableCell &cell,
                              std::string_view command, bool json_output) -> bool {
    featherdoc::TableRow row;
    if (!resolve_body_table_row(doc, table_index, row_index, row, command,
                                json_output)) {
        return false;
    }

    cell = row.cells();
    for (std::size_t current_index = 0;
         current_index < cell_index && cell.has_next();
         ++current_index) {
        cell.next();
    }

    if (cell.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "cell index '" + std::to_string(cell_index) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "cell index is out of range", error_info,
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

auto section_part_paragraphs(featherdoc::Document &doc, section_part_family family,
                             std::size_t section_index,
                             featherdoc::section_reference_kind reference_kind)
    -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.section_header_paragraphs(section_index, reference_kind);
    }

    return doc.section_footer_paragraphs(section_index, reference_kind);
}

auto load_inspected_part_entries(const path_type &input_path, featherdoc::Document &doc,
                                 section_part_family family,
                                 std::vector<inspected_part_entry> &parts,
                                 std::string &error_message) -> bool {
    parts.clear();

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message = "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, "word/_rels/document.xml.rels", relationships_xml_text);
    if (relationships_status == zip_entry_read_status::read_failed) {
        close_archive();
        error_message = "failed to read relationships entry 'word/_rels/document.xml.rels'";
        return false;
    }

    if (relationships_status == zip_entry_read_status::ok) {
        pugi::xml_document relationships_xml;
        const auto parse_result = relationships_xml.load_buffer(
            relationships_xml_text.data(), relationships_xml_text.size());
        if (!parse_result) {
            close_archive();
            error_message = "failed to parse relationships entry 'word/_rels/document.xml.rels'";
            return false;
        }

        const auto expected_type =
            family == section_part_family::header
                ? std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                      "header"}
                : std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                      "footer"};

        std::vector<std::string> seen_entries;
        const auto relationships = relationships_xml.child("Relationships");
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};
             relationship = relationship.next_sibling("Relationship")) {
            const auto type = std::string_view{relationship.attribute("Type").value()};
            if (type != expected_type) {
                continue;
            }

            const auto target = std::string_view{relationship.attribute("Target").value()};
            if (target.empty()) {
                continue;
            }

            const auto entry_name = normalize_word_part_entry(target);
            bool already_seen = false;
            for (const auto &seen_entry : seen_entries) {
                if (seen_entry == entry_name) {
                    already_seen = true;
                    break;
                }
            }
            if (already_seen) {
                continue;
            }

            seen_entries.push_back(entry_name);
            parts.push_back(inspected_part_entry{
                relationship.attribute("Id").value(), entry_name, {}, {}});
        }
    }

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
        error_message = "document.xml does not contain the expected w:document/w:body structure";
        return false;
    }

    const auto collect_references = [&](pugi::xml_node section_properties,
                                        std::size_t section_index) {
        for (auto reference = section_properties.child(section_reference_name(family));
             reference != pugi::xml_node{};
             reference = reference.next_sibling(section_reference_name(family))) {
            const auto relationship_id =
                std::string_view{reference.attribute("r:id").value()};
            if (relationship_id.empty()) {
                continue;
            }

            const auto kind_attribute = std::string_view{reference.attribute("w:type").value()};
            const auto kind = kind_attribute.empty() ? std::string{"default"}
                                                     : std::string{kind_attribute};

            for (auto &part : parts) {
                if (part.relationship_id == relationship_id) {
                    part.references.push_back(
                        inspected_part_reference{section_index, kind});
                    break;
                }
            }
        }
    };

    std::size_t section_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            collect_references(section_properties, section_index);
            ++section_index;
        }
    }

    if (const auto final_section_properties = body.child("w:sectPr");
        final_section_properties != pugi::xml_node{}) {
        collect_references(final_section_properties, section_index);
    }

    const auto expected_count = family == section_part_family::header ? doc.header_count()
                                                                      : doc.footer_count();
    if (parts.size() != expected_count) {
        error_message = "CLI part inspection order does not match loaded document parts";
        return false;
    }

    for (std::size_t index = 0; index < parts.size(); ++index) {
        parts[index].paragraphs =
            collect_part_lines(section_part_paragraphs(doc, family, index));
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

void write_json_section_flags(std::ostream &stream, featherdoc::Document &doc,
                              std::size_t section_index, section_part_family family) {
    const auto has_default =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::default_reference)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::default_reference);
    const auto has_first =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::first_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::first_page);
    const auto has_even =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::even_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::even_page);

    stream << "{\"default\":" << (has_default ? "true" : "false")
           << ",\"first\":" << (has_first ? "true" : "false")
           << ",\"even\":" << (has_even ? "true" : "false") << '}';
}

void write_json_part_references(std::ostream &stream,
                                const std::vector<inspected_part_reference> &references) {
    stream << '[';
    for (std::size_t index = 0; index < references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"section\":" << references[index].section_index << ",\"kind\":";
        write_json_string(stream, references[index].kind);
        stream << '}';
    }
    stream << ']';
}

void write_json_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance);

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

void write_json_style_usage_hit_reference(
    std::ostream &stream, const featherdoc::style_usage_hit_reference &reference) {
    stream << "{\"section_index\":" << reference.section_index << ",\"reference_kind\":";
    write_json_string(stream, featherdoc::to_xml_reference_type(reference.reference_kind));
    stream << '}';
}

void write_json_style_usage_breakdown(std::ostream &stream,
                                      const featherdoc::style_usage_breakdown &usage) {
    stream << "{\"paragraph_count\":" << usage.paragraph_count
           << ",\"run_count\":" << usage.run_count
           << ",\"table_count\":" << usage.table_count
           << ",\"total_count\":" << usage.total_count() << '}';
}

void write_json_style_usage_hit(std::ostream &stream, const featherdoc::style_usage_hit &hit) {
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

void write_json_style_usage_summary(std::ostream &stream,
                                    const featherdoc::style_usage_summary &usage) {
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

void write_json_optional_table_style_cell_vertical_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::cell_vertical_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_cell_vertical_alignment_name(*alignment));
}

void write_json_optional_table_style_cell_text_direction(
    std::ostream &stream,
    const std::optional<featherdoc::cell_text_direction> &direction) {
    if (!direction.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_cell_text_direction_name(*direction));
}

void write_json_optional_table_style_paragraph_alignment(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_alignment> &alignment) {
    if (!alignment.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_paragraph_alignment_name(*alignment));
}

void write_json_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    if (!rule.has_value()) {
        stream << "null";
        return;
    }

    write_json_string(stream, table_style_paragraph_line_spacing_rule_name(*rule));
}

void write_json_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    stream << "{\"before_twips\":";
    write_json_optional_u32(stream, spacing.before_twips);
    stream << ",\"after_twips\":";
    write_json_optional_u32(stream, spacing.after_twips);
    stream << ",\"line_twips\":";
    write_json_optional_u32(stream, spacing.line_twips);
    stream << ",\"line_rule\":";
    write_json_optional_table_style_paragraph_line_spacing_rule(stream,
                                                                spacing.line_rule);
    stream << '}';
}

void write_json_table_style_margins(
    std::ostream &stream, const featherdoc::table_style_margins_definition &margins) {
    stream << "{\"top_twips\":";
    write_json_optional_u32(stream, margins.top_twips);
    stream << ",\"left_twips\":";
    write_json_optional_u32(stream, margins.left_twips);
    stream << ",\"bottom_twips\":";
    write_json_optional_u32(stream, margins.bottom_twips);
    stream << ",\"right_twips\":";
    write_json_optional_u32(stream, margins.right_twips);
    stream << '}';
}

void write_json_table_style_border(
    std::ostream &stream, const featherdoc::table_style_border_summary &border) {
    stream << "{\"style\":";
    write_json_optional_string(stream, border.style);
    stream << ",\"size_eighth_points\":";
    write_json_optional_u32(stream, border.size_eighth_points);
    stream << ",\"color\":";
    write_json_optional_string(stream, border.color);
    stream << ",\"space_points\":";
    write_json_optional_u32(stream, border.space_points);
    stream << '}';
}

void write_json_optional_table_style_border(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (!border.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_border(stream, *border);
}

void write_json_table_style_borders(
    std::ostream &stream, const featherdoc::table_style_borders_summary &borders) {
    stream << "{\"top\":";
    write_json_optional_table_style_border(stream, borders.top);
    stream << ",\"left\":";
    write_json_optional_table_style_border(stream, borders.left);
    stream << ",\"bottom\":";
    write_json_optional_table_style_border(stream, borders.bottom);
    stream << ",\"right\":";
    write_json_optional_table_style_border(stream, borders.right);
    stream << ",\"inside_horizontal\":";
    write_json_optional_table_style_border(stream, borders.inside_horizontal);
    stream << ",\"inside_vertical\":";
    write_json_optional_table_style_border(stream, borders.inside_vertical);
    stream << '}';
}

void write_json_table_style_region(
    std::ostream &stream, const featherdoc::table_style_region_summary &region) {
    stream << "{\"fill_color\":";
    write_json_optional_string(stream, region.fill_color);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, region.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, region.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, region.italic);
    stream << ",\"font_size_points\":";
    write_json_optional_u32(stream, region.font_size_points);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, region.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, region.east_asia_font_family);
    stream << ",\"cell_vertical_alignment\":";
    write_json_optional_table_style_cell_vertical_alignment(
        stream, region.cell_vertical_alignment);
    stream << ",\"cell_text_direction\":";
    write_json_optional_table_style_cell_text_direction(stream,
                                                        region.cell_text_direction);
    stream << ",\"paragraph_alignment\":";
    write_json_optional_table_style_paragraph_alignment(
        stream, region.paragraph_alignment);
    stream << ",\"paragraph_spacing\":";
    if (region.paragraph_spacing.has_value()) {
        write_json_table_style_paragraph_spacing(stream, *region.paragraph_spacing);
    } else {
        stream << "null";
    }
    stream << ",\"cell_margins\":";
    if (region.cell_margins.has_value()) {
        write_json_table_style_margins(stream, *region.cell_margins);
    } else {
        stream << "null";
    }
    stream << ",\"borders\":";
    if (region.borders.has_value()) {
        write_json_table_style_borders(stream, *region.borders);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_optional_table_style_region(
    std::ostream &stream,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (!region.has_value()) {
        stream << "null";
        return;
    }

    write_json_table_style_region(stream, *region);
}

void write_json_table_style_definition_summary(
    std::ostream &stream,
    const featherdoc::table_style_definition_summary &definition) {
    stream << "{\"style\":";
    write_json_style_summary(stream, definition.style);
    stream << ",\"regions\":{\"whole_table\":";
    write_json_optional_table_style_region(stream, definition.whole_table);
    stream << ",\"first_row\":";
    write_json_optional_table_style_region(stream, definition.first_row);
    stream << ",\"last_row\":";
    write_json_optional_table_style_region(stream, definition.last_row);
    stream << ",\"first_column\":";
    write_json_optional_table_style_region(stream, definition.first_column);
    stream << ",\"last_column\":";
    write_json_optional_table_style_region(stream, definition.last_column);
    stream << ",\"banded_rows\":";
    write_json_optional_table_style_region(stream, definition.banded_rows);
    stream << ",\"banded_columns\":";
    write_json_optional_table_style_region(stream, definition.banded_columns);
    stream << ",\"second_banded_rows\":";
    write_json_optional_table_style_region(stream, definition.second_banded_rows);
    stream << ",\"second_banded_columns\":";
    write_json_optional_table_style_region(stream, definition.second_banded_columns);
    stream << "}}";
}

void write_json_table_style_region_audit_issue(
    std::ostream &stream,
    const featherdoc::table_style_region_audit_issue &issue) {
    stream << "{\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"region\":";
    write_json_string(stream, issue.region);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"property_count\":" << issue.property_count
           << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_region_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-regions\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_style_count\":"
           << report.table_style_count << ",\"region_count\":"
           << report.region_count << ",\"issue_count\":" << report.issue_count()
           << ",\"fail_on_issue\":" << json_bool(fail_on_issue)
           << ",\"style_id\":";
    if (style_id.has_value()) {
        write_json_string(stream, *style_id);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_region_audit_issue(stream, report.issues[index]);
    }
    stream << "]}";
}

void write_json_table_style_inheritance_audit_issue(
    std::ostream &stream,
    const featherdoc::table_style_inheritance_audit_issue &issue) {
    stream << "{\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, issue.style_name);
    stream << ",\"based_on_style_id\":";
    write_json_string(stream, issue.based_on_style_id);
    stream << ",\"based_on_style_kind\":";
    write_json_string(stream, issue.based_on_style_kind);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"inheritance_chain\":";
    write_json_strings(stream, issue.inheritance_chain);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_inheritance_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-inheritance\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_style_count\":"
           << report.table_style_count << ",\"issue_count\":"
           << report.issue_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"style_id\":";
    if (style_id.has_value()) {
        write_json_string(stream, *style_id);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_inheritance_audit_issue(stream, report.issues[index]);
    }
    stream << "]}";
}

void print_style_numbering_inline(
    std::ostream &stream,
    const std::optional<featherdoc::style_summary::numbering_summary> &numbering) {
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

void write_json_numbering_level_override_summary(
    std::ostream &stream, const featherdoc::numbering_level_override_summary &level_override) {
    stream << "{\"level\":" << level_override.level << ",\"start_override\":";
    write_json_optional_u32(stream, level_override.start_override);
    stream << ",\"level_definition\":";
    if (level_override.level_definition.has_value()) {
        write_json_numbering_level_definition(stream, *level_override.level_definition);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance) {
    stream << "{\"instance_id\":" << instance.instance_id << ",\"level_overrides\":[";
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    instance.level_overrides[index]);
    }
    stream << "]}";
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

auto numbering_catalog_instance_count(
    const featherdoc::numbering_catalog &catalog) -> std::size_t {
    std::size_t count = 0U;
    for (const auto &definition : catalog.definitions) {
        count += definition.instances.size();
    }
    return count;
}

void write_json_numbering_catalog_definition(
    std::ostream &stream,
    const featherdoc::numbering_catalog_definition &definition) {
    stream << "{\"name\":";
    write_json_string(stream, definition.definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, definition.definition.levels[index]);
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

void write_json_numbering_catalog(std::ostream &stream,
                                  const featherdoc::numbering_catalog &catalog) {
    stream << "{\"definition_count\":" << catalog.definitions.size()
           << ",\"instance_count\":" << numbering_catalog_instance_count(catalog)
           << ",\"definitions\":[";
    for (std::size_t index = 0; index < catalog.definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(stream, catalog.definitions[index]);
    }
    stream << "]}";
}

void write_json_imported_numbering_definition_summary(
    std::ostream &stream,
    const featherdoc::imported_numbering_definition_summary &definition) {
    stream << "{\"name\":";
    write_json_string(stream, definition.name);
    stream << ",\"definition_id\":" << definition.definition_id
           << ",\"instance_ids\":[";
    for (std::size_t index = 0; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "]}";
}

void write_json_numbering_catalog_import_summary(
    std::ostream &stream,
    const featherdoc::numbering_catalog_import_summary &summary) {
    stream << "\"input_definition_count\":" << summary.input_definition_count
           << ",\"imported_definition_count\":" << summary.imported_definition_count
           << ",\"imported_instance_count\":" << summary.imported_instance_count
           << ",\"imported_definitions\":[";
    for (std::size_t index = 0; index < summary.definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_imported_numbering_definition_summary(stream, summary.definitions[index]);
    }
    stream << ']';
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

void write_json_table_style_look(std::ostream &stream,
                                 const featherdoc::table_style_look &style_look) {
    stream << "{\"first_row\":" << json_bool(style_look.first_row)
           << ",\"last_row\":" << json_bool(style_look.last_row)
           << ",\"first_column\":" << json_bool(style_look.first_column)
           << ",\"last_column\":" << json_bool(style_look.last_column)
           << ",\"banded_rows\":" << json_bool(style_look.banded_rows)
           << ",\"banded_columns\":" << json_bool(style_look.banded_columns)
           << '}';
}

void write_json_table_style_look_issue(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_issue &issue) {
    stream << "{\"table_index\":" << issue.table_index << ",\"style_id\":";
    write_json_string(stream, issue.style_id);
    stream << ",\"issue_type\":";
    write_json_string(stream, issue.issue_type);
    stream << ",\"region\":";
    write_json_string(stream, issue.region);
    stream << ",\"required_style_look_flag\":";
    write_json_string(stream, issue.required_style_look_flag);
    stream << ",\"expected_value\":" << json_bool(issue.expected_value)
           << ",\"actual_value\":";
    write_json_optional_bool_value(stream, issue.actual_value);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue) {
    const auto issue_count = report.issue_count();
    stream << "{\"command\":\"check-table-style-look\",\"ok\":"
           << json_bool(report.ok()) << ",\"table_count\":" << report.table_count
           << ",\"issue_count\":" << issue_count
           << ",\"fail_on_issue\":" << json_bool(fail_on_issue)
           << ",\"issues\":[";
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_look_issue(stream, report.issues[index]);
    }
    stream << "]}";
}

void write_json_table_style_quality_audit_report(
    std::ostream &stream,
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue) {
    stream << "{\"command\":\"audit-table-style-quality\",\"ok\":"
           << json_bool(report.ok()) << ",\"issue_count\":"
           << report.issue_count() << ",\"region_issue_count\":"
           << report.region_audit.issue_count()
           << ",\"inheritance_issue_count\":"
           << report.inheritance_audit.issue_count()
           << ",\"style_look_issue_count\":"
           << report.style_look.issue_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"region_audit\":";
    write_json_table_style_region_audit_report(stream, report.region_audit,
                                               std::nullopt, fail_on_issue);
    stream << ",\"inheritance_audit\":";
    write_json_table_style_inheritance_audit_report(stream, report.inheritance_audit,
                                                    std::nullopt, fail_on_issue);
    stream << ",\"style_look\":";
    write_json_table_style_look_report(stream, report.style_look, fail_on_issue);
    stream << '}';
}

void write_json_table_style_quality_fix_item(
    std::ostream &stream,
    const featherdoc::table_style_quality_fix_item &item) {
    stream << "{\"source\":";
    write_json_string(stream, item.source);
    stream << ",\"issue_type\":";
    write_json_string(stream, item.issue_type);
    stream << ",\"table_index\":";
    write_json_optional_size(stream, item.table_index);
    stream << ",\"style_id\":";
    write_json_string(stream, item.style_id);
    stream << ",\"style_name\":";
    write_json_string(stream, item.style_name);
    stream << ",\"region\":";
    write_json_string(stream, item.region);
    stream << ",\"action\":";
    write_json_string(stream, item.action);
    stream << ",\"automatic\":" << json_bool(item.automatic)
           << ",\"recommended_command\":";
    write_json_string(stream, item.command);
    stream << ",\"suggestion\":";
    write_json_string(stream, item.suggestion);
    stream << '}';
}

void write_json_table_style_quality_fix_plan(
    std::ostream &stream,
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue) {
    stream << "{\"command\":\"plan-table-style-quality-fixes\",\"ok\":"
           << json_bool(plan.ok()) << ",\"issue_count\":"
           << plan.issue_count() << ",\"plan_item_count\":"
           << plan.items.size() << ",\"automatic_fix_count\":"
           << plan.automatic_fix_count() << ",\"manual_fix_count\":"
           << plan.manual_fix_count() << ",\"fail_on_issue\":"
           << json_bool(fail_on_issue) << ",\"items\":[";
    for (std::size_t index = 0U; index < plan.items.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_quality_fix_item(stream, plan.items[index]);
    }
    stream << "],\"audit\":";
    write_json_table_style_quality_audit_report(stream, plan.audit, fail_on_issue);
    stream << '}';
}

void write_json_apply_table_style_quality_fixes_result(
    std::ostream &stream,
    const table_style_quality_apply_cli_result &result) {
    stream << "{\"command\":\"apply-table-style-quality-fixes\","
           << "\"mode\":\"look_only\",\"ok\":"
           << json_bool(result.after.ok()) << ",\"before_issue_count\":"
           << result.before.issue_count() << ",\"after_issue_count\":"
           << result.after.issue_count() << ",\"before_automatic_fix_count\":"
           << result.before.automatic_fix_count()
           << ",\"after_automatic_fix_count\":"
           << result.after.automatic_fix_count()
           << ",\"before_manual_fix_count\":"
           << result.before.manual_fix_count() << ",\"after_manual_fix_count\":"
           << result.after.manual_fix_count() << ",\"changed_table_count\":"
           << result.changed_table_count << ",\"output\":";
    if (result.output_path.has_value()) {
        write_json_string(stream, result.output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"before_plan\":";
    write_json_table_style_quality_fix_plan(stream, result.before, false);
    stream << ",\"after_plan\":";
    write_json_table_style_quality_fix_plan(stream, result.after, false);
    stream << '}';
}

auto count_applicable_table_style_look_issues(
    const featherdoc::table_style_look_consistency_report &report) -> std::size_t {
    auto count = std::size_t{0U};
    for (const auto &issue : report.issues) {
        if ((issue.issue_type == "style_look_missing" ||
             issue.issue_type == "style_look_disabled") &&
            !issue.required_style_look_flag.empty()) {
            ++count;
        }
    }
    return count;
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

void write_json_table_style_look_issue_array(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report) {
    stream << '[';
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_table_style_look_issue(stream, report.issues[index]);
    }
    stream << ']';
}

void write_json_repair_table_style_look_result(
    std::ostream &stream, const table_style_look_repair_cli_result &result) {
    const auto mode = result.apply ? std::string_view{"apply"}
                                   : std::string_view{"plan"};
    const auto after_ok = result.after.has_value()
                              ? result.after->ok()
                              : result.before.ok();
    stream << "{\"command\":\"repair-table-style-look\",\"mode\":";
    write_json_string(stream, mode);
    stream << ",\"ok\":" << json_bool(after_ok)
           << ",\"before_issue_count\":" << result.before.issue_count()
           << ",\"after_issue_count\":";
    if (result.after.has_value()) {
        stream << result.after->issue_count();
    } else {
        stream << "null";
    }
    stream << ",\"applicable_issue_count\":" << result.applicable_issue_count
           << ",\"changed_table_count\":" << result.changed_table_count
           << ",\"output\":";
    if (result.output_path.has_value()) {
        write_json_string(stream, result.output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"before_issues\":";
    write_json_table_style_look_issue_array(stream, result.before);
    stream << ",\"after_issues\":";
    if (result.after.has_value()) {
        write_json_table_style_look_issue_array(stream, *result.after);
    } else {
        stream << "null";
    }
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

void write_json_content_control_summary(
    std::ostream &stream,
    const featherdoc::content_control_summary &content_control) {
    stream << "{\"index\":" << content_control.index << ",\"kind\":";
    write_json_string(stream, content_control_kind_name(content_control.kind));
    stream << ",\"form_kind\":";
    write_json_string(
        stream, content_control_form_kind_name(content_control.form_kind));
    stream << ",\"tag\":";
    write_json_optional_string(stream, content_control.tag);
    stream << ",\"alias\":";
    write_json_optional_string(stream, content_control.alias);
    stream << ",\"id\":";
    write_json_optional_string(stream, content_control.id);
    stream << ",\"lock\":";
    write_json_optional_string(stream, content_control.lock);
    stream << ",\"data_binding_store_item_id\":";
    write_json_optional_string(stream,
                               content_control.data_binding_store_item_id);
    stream << ",\"data_binding_xpath\":";
    write_json_optional_string(stream, content_control.data_binding_xpath);
    stream << ",\"data_binding_prefix_mappings\":";
    write_json_optional_string(stream,
                               content_control.data_binding_prefix_mappings);
    stream << ",\"checked\":";
    write_json_optional_bool(stream, content_control.checked);
    stream << ",\"date_format\":";
    write_json_optional_string(stream, content_control.date_format);
    stream << ",\"date_locale\":";
    write_json_optional_string(stream, content_control.date_locale);
    stream << ",\"selected_list_item\":";
    write_json_optional_size(stream, content_control.selected_list_item);
    stream << ",\"list_items\":[";
    for (std::size_t index = 0; index < content_control.list_items.size();
         ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << "{\"display_text\":";
        write_json_string(stream,
                          content_control.list_items[index].display_text);
        stream << ",\"value\":";
        write_json_string(stream, content_control.list_items[index].value);
        stream << '}';
    }
    stream << "],\"showing_placeholder\":"
           << json_bool(content_control.showing_placeholder)
           << ",\"text\":";
    write_json_string(stream, content_control.text);
    stream << '}';
}

void write_json_hyperlink_summary(
    std::ostream &stream, const featherdoc::hyperlink_summary &hyperlink) {
    stream << "{\"index\":" << hyperlink.index << ",\"text\":";
    write_json_string(stream, hyperlink.text);
    stream << ",\"relationship_id\":";
    write_json_optional_string(stream, hyperlink.relationship_id);
    stream << ",\"target\":";
    write_json_optional_string(stream, hyperlink.target);
    stream << ",\"anchor\":";
    write_json_optional_string(stream, hyperlink.anchor);
    stream << ",\"external\":" << json_bool(hyperlink.external) << '}';
}

void write_json_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "{\"index\":" << note.index << ",\"kind\":";
    write_json_string(stream, review_note_kind_name(note.kind));
    stream << ",\"id\":";
    write_json_string(stream, note.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, note.author);
    stream << ",\"initials\":";
    write_json_optional_string(stream, note.initials);
    stream << ",\"date\":";
    write_json_optional_string(stream, note.date);
    stream << ",\"anchor_text\":";
    write_json_optional_string(stream, note.anchor_text);
    stream << ",\"resolved\":" << json_bool(note.resolved);
    stream << ",\"parent_index\":";
    write_json_optional_size(stream, note.parent_index);
    stream << ",\"parent_id\":";
    write_json_optional_string(stream, note.parent_id);
    stream << ",\"text\":";
    write_json_string(stream, note.text);
    stream << '}';
}

void write_json_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "{\"index\":" << revision.index << ",\"kind\":";
    write_json_string(stream, revision_kind_name(revision.kind));
    stream << ",\"id\":";
    write_json_string(stream, revision.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, revision.author);
    stream << ",\"date\":";
    write_json_optional_string(stream, revision.date);
    stream << ",\"part_entry_name\":";
    write_json_string(stream, revision.part_entry_name);
    stream << ",\"text\":";
    write_json_string(stream, revision.text);
    stream << '}';
}

void write_json_text_range_preview_segment(
    std::ostream &stream,
    const featherdoc::text_range_preview_segment &segment) {
    stream << "{\"paragraph_index\":" << segment.paragraph_index
           << ",\"text_offset\":" << segment.text_offset
           << ",\"text_length\":" << segment.text_length << ",\"text\":";
    write_json_string(stream, segment.text);
    stream << '}';
}

void write_json_text_range_preview(
    std::ostream &stream, const featherdoc::text_range_preview &preview) {
    stream << "{\"start_paragraph_index\":" << preview.start_paragraph_index
           << ",\"start_text_offset\":" << preview.start_text_offset
           << ",\"end_paragraph_index\":" << preview.end_paragraph_index
           << ",\"end_text_offset\":" << preview.end_text_offset
           << ",\"text_length\":" << preview.text_length
           << ",\"plain_text_runs_supported\":"
           << json_bool(preview.plain_text_runs_supported) << ",\"text\":";
    write_json_string(stream, preview.text);
    stream << ",\"segments\":[";
    for (std::size_t index = 0U; index < preview.segments.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_text_range_preview_segment(stream, preview.segments[index]);
    }
    stream << "]}";
}

void write_json_text_range_matches(
    std::ostream &stream, std::string_view command, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "{\"command\":";
    write_json_string(stream, command);
    stream << ",\"ok\":true,\"query\":";
    write_json_string(stream, query);
    stream << ",\"matches_count\":" << matches.size() << ",\"matches\":[";
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"index\":" << index << ",\"preview\":";
        write_json_text_range_preview(stream, matches[index]);
        stream << '}';
    }
    stream << "]}\n";
}

void write_json_omml_summary(std::ostream &stream,
                             const featherdoc::omml_summary &formula) {
    stream << "{\"index\":" << formula.index
           << ",\"display\":" << json_bool(formula.display)
           << ",\"text\":";
    write_json_string(stream, formula.text);
    stream << ",\"xml\":";
    write_json_string(stream, formula.xml);
    stream << '}';
}

void print_style_summary(std::ostream &stream,
                         const featherdoc::style_summary &style) {
    stream << "id=" << style.style_id << " name=" << style.name
           << " type=" << style.type_name
           << " kind=" << style_kind_name(style.kind)
           << " based_on=";
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

void print_bookmark_summary(std::ostream &stream,
                            const featherdoc::bookmark_summary &bookmark) {
    stream << "name=" << bookmark.bookmark_name
           << " occurrences=" << bookmark.occurrence_count
           << " kind=" << bookmark_kind_name(bookmark.kind)
           << " duplicate=" << yes_no(bookmark.is_duplicate());
}

void print_content_control_summary(
    std::ostream &stream,
    const featherdoc::content_control_summary &content_control) {
    stream << "index=" << content_control.index
           << " kind=" << content_control_kind_name(content_control.kind)
           << " form_kind="
           << content_control_form_kind_name(content_control.form_kind)
           << " tag=" << optional_display_value(content_control.tag)
           << " alias=" << optional_display_value(content_control.alias)
           << " id=" << optional_display_value(content_control.id)
           << " lock=" << optional_display_value(content_control.lock)
           << " data_binding_store_item_id="
           << optional_display_value(
                  content_control.data_binding_store_item_id)
           << " data_binding_xpath="
           << optional_display_value(content_control.data_binding_xpath)
           << " data_binding_prefix_mappings="
           << optional_display_value(
                  content_control.data_binding_prefix_mappings)
           << " checked="
           << (content_control.checked.has_value()
                   ? yes_no(*content_control.checked)
                   : "-")
           << " date_format=" << optional_display_value(content_control.date_format)
           << " date_locale=" << optional_display_value(content_control.date_locale)
           << " list_items=" << content_control.list_items.size()
           << " showing_placeholder="
           << yes_no(content_control.showing_placeholder) << " text=";
    write_json_string(stream, content_control.text);
}

void print_hyperlink_summary(
    std::ostream &stream, const featherdoc::hyperlink_summary &hyperlink) {
    stream << "index=" << hyperlink.index << " text=";
    write_json_string(stream, hyperlink.text);
    stream << " relationship_id="
           << optional_display_value(hyperlink.relationship_id)
           << " target=" << optional_display_value(hyperlink.target)
           << " anchor=" << optional_display_value(hyperlink.anchor)
           << " external=" << yes_no(hyperlink.external);
}

void print_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "index=" << note.index
           << " kind=" << review_note_kind_name(note.kind)
           << " id=" << note.id
           << " author=" << optional_display_value(note.author)
           << " initials=" << optional_display_value(note.initials)
           << " date=" << optional_display_value(note.date)
           << " anchor_text=" << optional_display_value(note.anchor_text)
           << " resolved=" << yes_no(note.resolved)
           << " parent_index=" << optional_size_display_value(note.parent_index)
           << " parent_id=" << optional_display_value(note.parent_id)
           << " text=";
    write_json_string(stream, note.text);
}

void print_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "index=" << revision.index
           << " kind=" << revision_kind_name(revision.kind)
           << " id=" << revision.id
           << " author=" << optional_display_value(revision.author)
           << " date=" << optional_display_value(revision.date)
           << " part_entry_name=" << revision.part_entry_name << " text=";
    write_json_string(stream, revision.text);
}

void print_text_range_preview(std::ostream &stream,
                              const featherdoc::text_range_preview &preview) {
    stream << "start_paragraph_index=" << preview.start_paragraph_index
           << " start_text_offset=" << preview.start_text_offset
           << " end_paragraph_index=" << preview.end_paragraph_index
           << " end_text_offset=" << preview.end_text_offset
           << " text_length=" << preview.text_length
           << " plain_text_runs_supported="
           << yes_no(preview.plain_text_runs_supported) << " text=";
    write_json_string(stream, preview.text);
    for (const auto &segment : preview.segments) {
        stream << '\n'
               << "segment paragraph_index=" << segment.paragraph_index
               << " text_offset=" << segment.text_offset
               << " text_length=" << segment.text_length << " text=";
        write_json_string(stream, segment.text);
    }
}

void print_text_range_matches(
    std::ostream &stream, std::string_view query,
    const std::vector<featherdoc::text_range_preview> &matches) {
    stream << "query=";
    write_json_string(stream, query);
    stream << " matches_count=" << matches.size();
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        stream << '\n' << "match index=" << index << ' ';
        print_text_range_preview(stream, matches[index]);
    }
    stream << '\n';
}

void print_omml_summary(std::ostream &stream,
                        const featherdoc::omml_summary &formula) {
    stream << "index=" << formula.index
           << " display=" << yes_no(formula.display) << " text=";
    write_json_string(stream, formula.text);
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

void write_json_style_refactor_issue(
    std::ostream &stream, const featherdoc::style_refactor_issue &issue) {
    stream << "{\"code\":";
    write_json_string(stream, issue.code);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << '}';
}

void write_json_style_refactor_suggestion(
    std::ostream &stream, const featherdoc::style_refactor_suggestion &suggestion) {
    stream << "{\"reason_code\":";
    write_json_string(stream, suggestion.reason_code);
    stream << ",\"reason\":";
    write_json_string(stream, suggestion.reason);
    stream << ",\"confidence\":" << suggestion.confidence
           << ",\"evidence\":";
    write_json_strings(stream, suggestion.evidence);
    stream << ",\"differences\":";
    write_json_strings(stream, suggestion.differences);
    stream << '}';
}

void write_json_style_refactor_suggestion_confidence_summary(
    std::ostream &stream,
    const featherdoc::style_refactor_suggestion_confidence_summary &summary) {
    stream << "{\"suggestion_count\":" << summary.suggestion_count
           << ",\"min_confidence\":";
    write_json_optional_u32(stream, summary.min_confidence);
    stream << ",\"max_confidence\":";
    write_json_optional_u32(stream, summary.max_confidence);
    stream << ",\"exact_xml_match_count\":" << summary.exact_xml_match_count
           << ",\"xml_difference_count\":" << summary.xml_difference_count
           << ",\"recommended_min_confidence\":";
    write_json_optional_u32(stream, summary.recommended_min_confidence);
    stream << ",\"recommendation\":";
    write_json_string(stream, summary.recommendation);
    stream << '}';
}

void write_json_style_refactor_operation(
    std::ostream &stream, const featherdoc::style_refactor_operation_plan &operation) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(operation.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, operation.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, operation.target_style_id);
    stream << ",\"applyable\":" << json_bool(operation.applyable)
           << ",\"source_exists\":" << json_bool(operation.source_style.has_value())
           << ",\"target_exists\":" << json_bool(operation.target_style.has_value())
           << ",\"source_kind\":";
    if (operation.source_style.has_value()) {
        write_json_string(stream, style_kind_name(operation.source_style->kind));
    } else {
        stream << "null";
    }
    stream << ",\"target_kind\":";
    if (operation.target_style.has_value()) {
        write_json_string(stream, style_kind_name(operation.target_style->kind));
    } else {
        stream << "null";
    }
    stream << ",\"source_reference_count\":";
    if (operation.source_usage.has_value()) {
        stream << operation.source_usage->total_count();
    } else {
        stream << "null";
    }
    stream << ",\"source_usage\":";
    if (operation.source_usage.has_value()) {
        write_json_style_usage_summary(stream, *operation.source_usage);
    } else {
        stream << "null";
    }
    stream << ",\"suggestion\":";
    if (operation.suggestion.has_value()) {
        write_json_style_refactor_suggestion(stream, *operation.suggestion);
    } else {
        stream << "null";
    }
    stream << ",\"issues\":[";
    for (std::size_t index = 0; index < operation.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_issue(stream, operation.issues[index]);
    }
    stream << "],\"command_template\":";
    write_json_string(stream, style_refactor_command_template(operation));
    stream << '}';
}

void write_json_style_refactor_plan_fields(
    std::ostream &stream, const featherdoc::style_refactor_plan &plan) {
    const auto confidence_summary = plan.suggestion_confidence_summary();
    stream << "\"clean\":" << json_bool(plan.clean())
           << ",\"operation_count\":" << plan.operation_count
           << ",\"applyable_count\":" << plan.applyable_count
           << ",\"issue_count\":" << plan.issue_count
           << ",\"suggestion_confidence_summary\":";
    write_json_style_refactor_suggestion_confidence_summary(stream,
                                                            confidence_summary);
    stream << ",\"operations\":[";
    for (std::size_t index = 0; index < plan.operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_operation(stream, plan.operations[index]);
    }
    stream << ']';
}

void write_json_style_refactor_rollback_entry(
    std::ostream &stream,
    const featherdoc::style_refactor_rollback_entry &entry) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(entry.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, entry.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, entry.target_style_id);
    stream << ",\"automatic\":" << json_bool(entry.automatic)
           << ",\"restorable\":" << json_bool(entry.restorable)
           << ",\"note\":";
    write_json_string(stream, entry.note);
    stream << ",\"source_reference_count\":";
    if (entry.source_usage.has_value()) {
        stream << entry.source_usage->total_count();
    } else {
        stream << "null";
    }
    stream << ",\"source_usage\":";
    if (entry.source_usage.has_value()) {
        write_json_style_usage_summary(stream, *entry.source_usage);
    } else {
        stream << "null";
    }
    stream << ",\"source_style_xml\":";
    if (!entry.source_style_xml.empty()) {
        write_json_string(stream, entry.source_style_xml);
    } else {
        stream << "null";
    }
    stream << ",\"command_template\":";
    if (entry.automatic) {
        write_json_string(stream, style_refactor_rollback_command_template(entry));
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_style_refactor_rollback_entries(
    std::ostream &stream,
    const std::vector<featherdoc::style_refactor_rollback_entry> &entries) {
    stream << '[';
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_rollback_entry(stream, entries[index]);
    }
    stream << ']';
}

void write_json_style_refactor_apply_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_apply_result &result) {
    stream << ",\"changed\":" << json_bool(result.changed)
           << ",\"requested_count\":" << result.requested_count
           << ",\"applied_count\":" << result.applied_count
           << ",\"skipped_count\":" << result.skipped_count()
           << ",\"rollback_count\":" << result.rollback_entries.size()
           << ",\"rollback_operations\":";
    write_json_style_refactor_rollback_entries(stream, result.rollback_entries);
    stream << ",\"plan\":{";
    write_json_style_refactor_plan_fields(stream, result.plan);
    stream << '}';
}

void write_json_style_refactor_restore_issue(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_issue &issue) {
    stream << "{\"code\":";
    write_json_string(stream, issue.code);
    stream << ",\"message\":";
    write_json_string(stream, issue.message);
    stream << ",\"suggestion\":";
    write_json_string(stream, issue.suggestion);
    stream << '}';
}

void write_json_style_refactor_restore_issue_summary(
    std::ostream &stream,
    const std::vector<featherdoc::style_refactor_restore_issue_summary> &summaries) {
    stream << '[';
    for (std::size_t index = 0; index < summaries.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"code\":";
        write_json_string(stream, summaries[index].code);
        stream << ",\"count\":" << summaries[index].count
               << ",\"suggestion\":";
        write_json_string(stream, summaries[index].suggestion);
        stream << '}';
    }
    stream << ']';
}

void write_json_style_refactor_restore_operation(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_operation_result &operation) {
    stream << "{\"action\":";
    write_json_string(stream, style_refactor_action_name(operation.action));
    stream << ",\"source_style_id\":";
    write_json_string(stream, operation.source_style_id);
    stream << ",\"target_style_id\":";
    write_json_string(stream, operation.target_style_id);
    stream << ",\"restorable\":" << json_bool(operation.restorable)
           << ",\"restored\":" << json_bool(operation.restored)
           << ",\"style_restored\":" << json_bool(operation.style_restored)
           << ",\"restored_reference_count\":"
           << operation.restored_reference_count << ",\"issues\":[";
    for (std::size_t index = 0; index < operation.issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_restore_issue(stream, operation.issues[index]);
    }
    stream << "]}";
}

void write_json_style_refactor_restore_selection(
    std::ostream &stream, const style_merge_restore_options &options) {
    if (!options.entry_indexes.empty()) {
        if (options.entry_indexes.size() == 1U) {
            stream << ",\"entry_index\":" << options.entry_indexes.front();
        }
        stream << ",\"entry_indexes\":[";
        for (std::size_t index = 0; index < options.entry_indexes.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            stream << options.entry_indexes[index];
        }
        stream << ']';
    }
    if (!options.source_style_ids.empty()) {
        stream << ",\"source_style_ids\":";
        write_json_strings(stream, options.source_style_ids);
    }
    if (!options.target_style_ids.empty()) {
        stream << ",\"target_style_ids\":";
        write_json_strings(stream, options.target_style_ids);
    }
}

void write_json_style_refactor_restore_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_result &result) {
    const auto issue_summary = result.issue_summary();
    stream << ",\"changed\":" << json_bool(result.changed)
           << ",\"dry_run\":" << json_bool(result.dry_run)
           << ",\"requested_count\":" << result.requested_count
           << ",\"restored_count\":" << result.restored_count
           << ",\"skipped_count\":" << result.skipped_count()
           << ",\"issue_count\":" << result.issue_count()
           << ",\"issue_summary\":";
    write_json_style_refactor_restore_issue_summary(stream, issue_summary);
    stream << ",\"restored_style_count\":" << result.restored_style_count
           << ",\"restored_reference_count\":"
           << result.restored_reference_count << ",\"operations\":[";
    for (std::size_t index = 0; index < result.operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_refactor_restore_operation(stream, result.operations[index]);
    }
    stream << ']';
}

void inspect_style_refactor_restore_result(
    const featherdoc::style_refactor_restore_result &result, bool json_output,
    const style_merge_restore_options *options = nullptr) {
    if (json_output) {
        std::cout << "{\"command\":\"restore-style-merge\",\"ok\":"
                  << json_bool(result.restored());
        write_json_style_refactor_restore_result_fields(std::cout, result);
        if (options != nullptr) {
            if (options->rollback_plan_path.has_value()) {
                std::cout << ",\"rollback_plan_file\":";
                write_json_string(std::cout, options->rollback_plan_path->string());
            }
            write_json_style_refactor_restore_selection(std::cout, *options);
        }
        std::cout << "}\n";
        return;
    }

    const auto issue_summary = result.issue_summary();
    std::cout << "restored: " << yes_no(result.restored()) << '\n'
              << "changed: " << yes_no(result.changed) << '\n'
              << "dry_run: " << yes_no(result.dry_run) << '\n'
              << "requested_operations: " << result.requested_count << '\n'
              << "restored_operations: " << result.restored_count << '\n'
              << "skipped_operations: " << result.skipped_count() << '\n'
              << "issues: " << result.issue_count() << '\n'
              << "issue_summary_entries: " << issue_summary.size() << '\n'
              << "restored_styles: " << result.restored_style_count << '\n'
              << "restored_references: " << result.restored_reference_count
              << '\n';
    for (std::size_t index = 0; index < issue_summary.size(); ++index) {
        std::cout << "issue_summary[" << index << "]: code="
                  << issue_summary[index].code
                  << " count=" << issue_summary[index].count
                  << " suggestion=" << issue_summary[index].suggestion << '\n';
    }
    for (std::size_t index = 0; index < result.operations.size(); ++index) {
        const auto &operation = result.operations[index];
        std::cout << "restore[" << index << "]: action="
                  << style_refactor_action_name(operation.action)
                  << " source=" << operation.source_style_id
                  << " target=" << operation.target_style_id
                  << " restorable=" << yes_no(operation.restorable)
                  << " restored=" << yes_no(operation.restored)
                  << " style_restored=" << yes_no(operation.style_restored)
                  << " references=" << operation.restored_reference_count
                  << " issues=" << operation.issues.size() << '\n';
        for (std::size_t issue_index = 0;
             issue_index < operation.issues.size(); ++issue_index) {
            const auto &issue = operation.issues[issue_index];
            std::cout << "restore[" << index << "].issue[" << issue_index
                      << "]: code=" << issue.code
                      << " message=" << issue.message << '\n';
            if (!issue.suggestion.empty()) {
                std::cout << "restore[" << index << "].issue[" << issue_index
                          << "].suggestion: " << issue.suggestion << '\n';
            }
        }
    }
}

void inspect_style_refactor_plan(
    const featherdoc::style_refactor_plan &plan, bool json_output,
    std::string_view command_name = "plan-style-refactor",
    std::optional<bool> fail_on_suggestion = std::nullopt);

void inspect_style_refactor_apply_result(
    const featherdoc::style_refactor_apply_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"apply-style-refactor\",\"ok\":"
                  << json_bool(result.applied());
        write_json_style_refactor_apply_result_fields(std::cout, result);
        std::cout << "}\n";
        return;
    }

    std::cout << "applied: " << yes_no(result.applied()) << '\n'
              << "changed: " << yes_no(result.changed) << '\n'
              << "requested_operations: " << result.requested_count << '\n'
              << "applied_operations: " << result.applied_count << '\n'
              << "skipped_operations: " << result.skipped_count() << '\n'
              << "rollback_operations: " << result.rollback_entries.size()
              << '\n';
    for (std::size_t index = 0; index < result.rollback_entries.size(); ++index) {
        const auto &entry = result.rollback_entries[index];
        std::cout << "rollback[" << index << "]: action="
                  << style_refactor_action_name(entry.action)
                  << " source=" << entry.source_style_id
                  << " target=" << entry.target_style_id
                  << " automatic=" << yes_no(entry.automatic)
                  << " restorable=" << yes_no(entry.restorable)
                  << " references=";
        if (entry.source_usage.has_value()) {
            std::cout << entry.source_usage->total_count();
        } else {
            std::cout << "unknown";
        }
        std::cout << " note=" << entry.note;
        if (entry.automatic) {
            std::cout << " command="
                      << style_refactor_rollback_command_template(entry);
        }
        std::cout << '\n';
    }
    inspect_style_refactor_plan(result.plan, false);
}

void inspect_style_refactor_plan(const featherdoc::style_refactor_plan &plan,
                                 bool json_output,
                                 std::string_view command_name,
                                 std::optional<bool> fail_on_suggestion) {
    const auto suggestion_gate_failed =
        fail_on_suggestion.value_or(false) && plan.operation_count != 0U;
    if (json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command_name);
        std::cout << ',';
        if (fail_on_suggestion.has_value()) {
            std::cout << "\"fail_on_suggestion\":"
                      << json_bool(*fail_on_suggestion)
                      << ",\"suggestion_gate_failed\":"
                      << json_bool(suggestion_gate_failed) << ',';
        }
        write_json_style_refactor_plan_fields(std::cout, plan);
        std::cout << "}\n";
        return;
    }

    const auto confidence_summary = plan.suggestion_confidence_summary();
    std::cout << "operations: " << plan.operation_count << '\n'
              << "applyable_operations: " << plan.applyable_count << '\n'
              << "issues: " << plan.issue_count << '\n';
    if (fail_on_suggestion.has_value()) {
        std::cout << "fail_on_suggestion: " << yes_no(*fail_on_suggestion)
                  << '\n'
                  << "suggestion_gate_failed: "
                  << yes_no(suggestion_gate_failed) << '\n';
    }
    std::cout << "suggestions: " << confidence_summary.suggestion_count << '\n'
              << "suggestion_exact_xml_matches: "
              << confidence_summary.exact_xml_match_count << '\n'
              << "suggestion_xml_differences: "
              << confidence_summary.xml_difference_count << '\n'
              << "suggestion_min_confidence: ";
    if (confidence_summary.min_confidence.has_value()) {
        std::cout << *confidence_summary.min_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "suggestion_max_confidence: ";
    if (confidence_summary.max_confidence.has_value()) {
        std::cout << *confidence_summary.max_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "suggestion_recommended_min_confidence: ";
    if (confidence_summary.recommended_min_confidence.has_value()) {
        std::cout << *confidence_summary.recommended_min_confidence;
    } else {
        std::cout << "none";
    }
    std::cout << '\n'
              << "suggestion_recommendation: "
              << confidence_summary.recommendation << '\n';
    for (std::size_t index = 0; index < plan.operations.size(); ++index) {
        const auto &operation = plan.operations[index];
        std::cout << "operation[" << index << "]: action="
                  << style_refactor_action_name(operation.action)
                  << " source=" << operation.source_style_id
                  << " target=" << operation.target_style_id
                  << " applyable=" << yes_no(operation.applyable)
                  << " references=";
        if (operation.source_usage.has_value()) {
            std::cout << operation.source_usage->total_count();
        } else {
            std::cout << "unknown";
        }
        std::cout << " issues=" << operation.issues.size();
        if (operation.suggestion.has_value()) {
            std::cout << " suggestion_confidence="
                      << operation.suggestion->confidence
                      << " suggestion_reason="
                      << operation.suggestion->reason_code
                      << " suggestion_differences="
                      << operation.suggestion->differences.size();
        }
        std::cout << " command=" << style_refactor_command_template(operation)
                  << '\n';
        for (std::size_t issue_index = 0; issue_index < operation.issues.size();
             ++issue_index) {
            std::cout << "operation[" << index << "].issue[" << issue_index
                      << "]: code=" << operation.issues[issue_index].code
                      << " message=" << operation.issues[issue_index].message << '\n';
        }
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

void inspect_style(const featherdoc::style_summary &style,
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
    std::cout << "unhide_when_used: " << yes_no(style.is_unhide_when_used) << '\n';
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
        std::cout << "usage_body_paragraphs: " << usage->body.paragraph_count << '\n';
        std::cout << "usage_body_runs: " << usage->body.run_count << '\n';
        std::cout << "usage_body_tables: " << usage->body.table_count << '\n';
        std::cout << "usage_body_total: " << usage->body.total_count() << '\n';
        std::cout << "usage_header_paragraphs: " << usage->header.paragraph_count << '\n';
        std::cout << "usage_header_runs: " << usage->header.run_count << '\n';
        std::cout << "usage_header_tables: " << usage->header.table_count << '\n';
        std::cout << "usage_header_total: " << usage->header.total_count() << '\n';
        std::cout << "usage_footer_paragraphs: " << usage->footer.paragraph_count << '\n';
        std::cout << "usage_footer_runs: " << usage->footer.run_count << '\n';
        std::cout << "usage_footer_tables: " << usage->footer.table_count << '\n';
        std::cout << "usage_footer_total: " << usage->footer.total_count() << '\n';
        std::cout << "usage_hits: " << usage->hits.size() << '\n';
        for (std::size_t index = 0; index < usage->hits.size(); ++index) {
            const auto &hit = usage->hits[index];
            std::cout << "usage_hit[" << index << "]: part="
                      << style_usage_part_kind_name(hit.part)
                      << " entry_name=" << hit.entry_name << " ordinal=" << hit.ordinal
                      << " section_index=";
            if (hit.section_index.has_value()) {
                std::cout << *hit.section_index;
            } else {
                std::cout << '-';
            }
            std::cout
                      << " kind=" << style_usage_hit_kind_name(hit.kind)
                      << " references=" << hit.references.size();
            for (std::size_t reference_index = 0; reference_index < hit.references.size();
                 ++reference_index) {
                const auto &reference = hit.references[reference_index];
                std::cout << " ref[" << reference_index
                          << "]=section:" << reference.section_index
                          << ",kind:"
                          << featherdoc::to_xml_reference_type(reference.reference_kind);
            }
            std::cout << '\n';
        }
    }
}

void print_optional_u32_field(std::ostream &stream, std::string_view name,
                              const std::optional<std::uint32_t> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_optional_string_field(std::ostream &stream, std::string_view name,
                                 const std::optional<std::string> &value) {
    stream << ' ' << name << '=';
    if (value.has_value()) {
        stream << *value;
    } else {
        stream << '-';
    }
}

void print_table_style_margins(
    std::ostream &stream, const featherdoc::table_style_margins_definition &margins) {
    print_optional_u32_field(stream, "top_twips", margins.top_twips);
    print_optional_u32_field(stream, "left_twips", margins.left_twips);
    print_optional_u32_field(stream, "bottom_twips", margins.bottom_twips);
    print_optional_u32_field(stream, "right_twips", margins.right_twips);
}

void print_optional_table_style_paragraph_line_spacing_rule(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::paragraph_line_spacing_rule> &rule) {
    stream << ' ' << name << '=';
    if (rule.has_value()) {
        stream << table_style_paragraph_line_spacing_rule_name(*rule);
    } else {
        stream << '-';
    }
}

void print_table_style_paragraph_spacing(
    std::ostream &stream,
    const featherdoc::table_style_paragraph_spacing_definition &spacing) {
    print_optional_u32_field(stream, "paragraph_spacing_before_twips",
                             spacing.before_twips);
    print_optional_u32_field(stream, "paragraph_spacing_after_twips",
                             spacing.after_twips);
    print_optional_u32_field(stream, "paragraph_spacing_line_twips",
                             spacing.line_twips);
    print_optional_table_style_paragraph_line_spacing_rule(
        stream, "paragraph_spacing_line_rule", spacing.line_rule);
}

void print_table_style_border(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_border_summary &border) {
    stream << "  border_" << name << ':';
    print_optional_string_field(stream, "style", border.style);
    print_optional_u32_field(stream, "size_eighth_points", border.size_eighth_points);
    print_optional_string_field(stream, "color", border.color);
    print_optional_u32_field(stream, "space_points", border.space_points);
    stream << '\n';
}

void print_optional_table_style_border(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_border_summary> &border) {
    if (border.has_value()) {
        print_table_style_border(stream, name, *border);
    }
}

void print_table_style_region(
    std::ostream &stream, std::string_view name,
    const featherdoc::table_style_region_summary &region) {
    stream << "region_" << name << ": fill_color=";
    if (region.fill_color.has_value()) {
        stream << *region.fill_color;
    } else {
        stream << '-';
    }
    stream << " text_color=";
    if (region.text_color.has_value()) {
        stream << *region.text_color;
    } else {
        stream << '-';
    }
    stream << " bold=";
    if (region.bold.has_value()) {
        stream << json_bool(*region.bold);
    } else {
        stream << '-';
    }
    stream << " italic=";
    if (region.italic.has_value()) {
        stream << json_bool(*region.italic);
    } else {
        stream << '-';
    }
    print_optional_u32_field(stream, "font_size_points", region.font_size_points);
    print_optional_string_field(stream, "font_family", region.font_family);
    print_optional_string_field(stream, "east_asia_font_family",
                                region.east_asia_font_family);
    stream << " cell_vertical_alignment=";
    if (region.cell_vertical_alignment.has_value()) {
        stream << table_style_cell_vertical_alignment_name(
            *region.cell_vertical_alignment);
    } else {
        stream << '-';
    }
    stream << " cell_text_direction=";
    if (region.cell_text_direction.has_value()) {
        stream << table_style_cell_text_direction_name(*region.cell_text_direction);
    } else {
        stream << '-';
    }
    stream << " paragraph_alignment=";
    if (region.paragraph_alignment.has_value()) {
        stream << table_style_paragraph_alignment_name(*region.paragraph_alignment);
    } else {
        stream << '-';
    }
    if (region.paragraph_spacing.has_value()) {
        print_table_style_paragraph_spacing(stream, *region.paragraph_spacing);
    }
    if (region.cell_margins.has_value()) {
        print_table_style_margins(stream, *region.cell_margins);
    }
    stream << '\n';

    if (region.borders.has_value()) {
        print_optional_table_style_border(stream, "top", region.borders->top);
        print_optional_table_style_border(stream, "left", region.borders->left);
        print_optional_table_style_border(stream, "bottom", region.borders->bottom);
        print_optional_table_style_border(stream, "right", region.borders->right);
        print_optional_table_style_border(stream, "inside_horizontal",
                                          region.borders->inside_horizontal);
        print_optional_table_style_border(stream, "inside_vertical",
                                          region.borders->inside_vertical);
    }
}

void print_optional_table_style_region(
    std::ostream &stream, std::string_view name,
    const std::optional<featherdoc::table_style_region_summary> &region) {
    if (region.has_value()) {
        print_table_style_region(stream, name, *region);
    } else {
        stream << "region_" << name << ": none\n";
    }
}

void inspect_table_style_definition(
    const featherdoc::table_style_definition_summary &definition, bool json_output) {
    if (json_output) {
        std::cout << "{\"table_style_definition\":";
        write_json_table_style_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    inspect_style(definition.style, std::nullopt, false);
    print_optional_table_style_region(std::cout, "whole_table", definition.whole_table);
    print_optional_table_style_region(std::cout, "first_row", definition.first_row);
    print_optional_table_style_region(std::cout, "last_row", definition.last_row);
    print_optional_table_style_region(std::cout, "first_column", definition.first_column);
    print_optional_table_style_region(std::cout, "last_column", definition.last_column);
    print_optional_table_style_region(std::cout, "banded_rows", definition.banded_rows);
    print_optional_table_style_region(std::cout, "banded_columns", definition.banded_columns);
    print_optional_table_style_region(std::cout, "second_banded_rows",
                                      definition.second_banded_rows);
    print_optional_table_style_region(std::cout, "second_banded_columns",
                                      definition.second_banded_columns);
}

void audit_table_style_regions(
    const featherdoc::table_style_region_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output) {
    if (json_output) {
        write_json_table_style_region_audit_report(std::cout, report, style_id,
                                                   fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_region_audit: " << (report.ok() ? "ok" : "issues")
              << '\n'
              << "table_style_count: " << report.table_style_count << '\n'
              << "region_count: " << report.region_count << '\n'
              << "issue_count: " << report.issue_count() << '\n';
    if (style_id.has_value()) {
        std::cout << "style_id: " << *style_id << '\n';
    }
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        std::cout << "issue[" << index << "]: style_id=" << issue.style_id
                  << " region=" << issue.region
                  << " issue_type=" << issue.issue_type
                  << " property_count=" << issue.property_count
                  << " suggestion=" << issue.suggestion << '\n';
    }
}

void audit_table_style_inheritance(
    const featherdoc::table_style_inheritance_audit_report &report,
    const std::optional<std::string> &style_id, bool fail_on_issue,
    bool json_output) {
    if (json_output) {
        write_json_table_style_inheritance_audit_report(std::cout, report, style_id,
                                                        fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_inheritance_audit: "
              << (report.ok() ? "ok" : "issues") << '\n'
              << "table_style_count: " << report.table_style_count << '\n'
              << "issue_count: " << report.issue_count() << '\n';
    if (style_id.has_value()) {
        std::cout << "style_id: " << *style_id << '\n';
    }
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        std::cout << "issue[" << index << "]: style_id=" << issue.style_id
                  << " based_on_style_id=" << issue.based_on_style_id
                  << " based_on_style_kind=" << issue.based_on_style_kind
                  << " issue_type=" << issue.issue_type << " chain=";
        for (std::size_t chain_index = 0U;
             chain_index < issue.inheritance_chain.size(); ++chain_index) {
            if (chain_index != 0U) {
                std::cout << " -> ";
            }
            std::cout << issue.inheritance_chain[chain_index];
        }
        std::cout << " suggestion=" << issue.suggestion << '\n';
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
              << "removable_styles: " << plan.removable_style_ids.size() << '\n';
    for (std::size_t index = 0U; index < plan.removable_style_ids.size(); ++index) {
        std::cout << "removable_style[" << index << "]: "
                  << plan.removable_style_ids[index] << '\n';
    }
}

void write_json_style_prune_summary(std::ostream &stream,
                                    const featherdoc::style_prune_summary &summary) {
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
    for (std::size_t index = 0U; index < summary.removed_style_ids.size(); ++index) {
        std::cout << "removed_style[" << index << "]: "
                  << summary.removed_style_ids[index] << '\n';
    }
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

void print_table_style_look_report(
    std::ostream &stream,
    const featherdoc::table_style_look_consistency_report &report) {
    stream << "table_style_look_consistency: " << (report.ok() ? "ok" : "issues")
           << '\n'
           << "table_count: " << report.table_count << '\n'
           << "issue_count: " << report.issue_count() << '\n';
    for (std::size_t index = 0U; index < report.issues.size(); ++index) {
        const auto &issue = report.issues[index];
        stream << "issue[" << index << "]: table_index=" << issue.table_index
               << " style_id=" << issue.style_id
               << " issue_type=" << issue.issue_type;
        if (!issue.region.empty()) {
            stream << " region=" << issue.region;
        }
        if (!issue.required_style_look_flag.empty()) {
            stream << " required_style_look_flag="
                   << issue.required_style_look_flag
                   << " expected=" << json_bool(issue.expected_value)
                   << " actual=";
            if (issue.actual_value.has_value()) {
                stream << json_bool(*issue.actual_value);
            } else {
                stream << "none";
            }
        }
        stream << " suggestion=" << issue.suggestion << '\n';
    }
}

void check_table_style_look(
    const featherdoc::table_style_look_consistency_report &report,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_look_report(std::cout, report, fail_on_issue);
        std::cout << '\n';
        return;
    }

    print_table_style_look_report(std::cout, report);
}

void audit_table_style_quality(
    const featherdoc::table_style_quality_audit_report &report,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_quality_audit_report(std::cout, report, fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_audit: " << (report.ok() ? "ok" : "issues")
              << '\n'
              << "issue_count: " << report.issue_count() << '\n'
              << "region_issue_count: " << report.region_audit.issue_count() << '\n'
              << "inheritance_issue_count: "
              << report.inheritance_audit.issue_count() << '\n'
              << "style_look_issue_count: " << report.style_look.issue_count()
              << '\n';
    audit_table_style_regions(report.region_audit, std::nullopt, fail_on_issue,
                              false);
    audit_table_style_inheritance(report.inheritance_audit, std::nullopt,
                                  fail_on_issue, false);
    print_table_style_look_report(std::cout, report.style_look);
}

void plan_table_style_quality_fixes(
    const featherdoc::table_style_quality_fix_plan &plan,
    bool fail_on_issue, bool json_output) {
    if (json_output) {
        write_json_table_style_quality_fix_plan(std::cout, plan, fail_on_issue);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_fix_plan: "
              << (plan.ok() ? "ok" : "issues") << '\n'
              << "issue_count: " << plan.issue_count() << '\n'
              << "plan_item_count: " << plan.items.size() << '\n'
              << "automatic_fix_count: " << plan.automatic_fix_count() << '\n'
              << "manual_fix_count: " << plan.manual_fix_count() << '\n';
    for (std::size_t index = 0U; index < plan.items.size(); ++index) {
        const auto &item = plan.items[index];
        std::cout << "item[" << index << "]: source=" << item.source
                  << " issue_type=" << item.issue_type
                  << " automatic=" << json_bool(item.automatic)
                  << " action=" << item.action;
        if (item.table_index.has_value()) {
            std::cout << " table_index=" << *item.table_index;
        }
        if (!item.style_id.empty()) {
            std::cout << " style_id=" << item.style_id;
        }
        if (!item.region.empty()) {
            std::cout << " region=" << item.region;
        }
        if (!item.command.empty()) {
            std::cout << " recommended_command=" << item.command;
        }
        std::cout << " suggestion=" << item.suggestion << '\n';
    }
}

void apply_table_style_quality_fixes(
    const table_style_quality_apply_cli_result &result, bool json_output) {
    if (json_output) {
        write_json_apply_table_style_quality_fixes_result(std::cout, result);
        std::cout << '\n';
        return;
    }

    std::cout << "table_style_quality_apply: look_only" << '\n'
              << "before_issue_count: " << result.before.issue_count() << '\n'
              << "after_issue_count: " << result.after.issue_count() << '\n'
              << "before_automatic_fix_count: "
              << result.before.automatic_fix_count() << '\n'
              << "after_automatic_fix_count: "
              << result.after.automatic_fix_count() << '\n'
              << "before_manual_fix_count: " << result.before.manual_fix_count()
              << '\n'
              << "after_manual_fix_count: " << result.after.manual_fix_count()
              << '\n'
              << "changed_table_count: " << result.changed_table_count << '\n';
    if (result.output_path.has_value()) {
        std::cout << "output: " << result.output_path->string() << '\n';
    }
}

void repair_table_style_look(const table_style_look_repair_cli_result &result,
                             bool json_output) {
    if (json_output) {
        write_json_repair_table_style_look_result(std::cout, result);
        std::cout << '\n';
        return;
    }

    const auto mode = result.apply ? std::string_view{"apply"}
                                   : std::string_view{"plan"};
    std::cout << "table_style_look_repair: " << mode << '\n'
              << "before_issue_count: " << result.before.issue_count() << '\n'
              << "applicable_issue_count: " << result.applicable_issue_count << '\n'
              << "changed_table_count: " << result.changed_table_count << '\n'
              << "after_issue_count: ";
    if (result.after.has_value()) {
        std::cout << result.after->issue_count();
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    if (result.output_path.has_value()) {
        std::cout << "output: " << result.output_path->string() << '\n';
    }
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

void inspect_bookmarks(const selected_template_part &selected,
                       const std::vector<featherdoc::bookmark_summary> &bookmarks,
                       bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        std::cout << ",\"count\":" << bookmarks.size() << ",\"bookmarks\":[";
        for (std::size_t index = 0; index < bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_bookmark_summary(std::cout, bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmarks: " << bookmarks.size() << '\n';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        std::cout << "bookmark[" << index << "]: ";
        print_bookmark_summary(std::cout, bookmarks[index]);
        std::cout << '\n';
    }
}

void inspect_bookmark(const selected_template_part &selected,
                      const featherdoc::bookmark_summary &bookmark,
                      bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        std::cout << ",\"bookmark\":";
        write_json_bookmark_summary(std::cout, bookmark);
        std::cout << "}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
}

auto filter_content_controls(
    const std::vector<featherdoc::content_control_summary> &content_controls,
    const inspect_content_controls_options &options)
    -> std::vector<featherdoc::content_control_summary> {
    auto filtered = std::vector<featherdoc::content_control_summary>{};
    filtered.reserve(content_controls.size());
    for (const auto &content_control : content_controls) {
        if (options.tag.has_value() &&
            (!content_control.tag.has_value() ||
             *content_control.tag != *options.tag)) {
            continue;
        }
        if (options.alias.has_value() &&
            (!content_control.alias.has_value() ||
             *content_control.alias != *options.alias)) {
            continue;
        }
        filtered.push_back(content_control);
    }
    return filtered;
}

void inspect_content_controls(
    const selected_template_part &selected,
    const std::vector<featherdoc::content_control_summary> &content_controls,
    const inspect_content_controls_options &options, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        if (options.tag.has_value() || options.alias.has_value()) {
            std::cout << ",\"filters\":{\"tag\":";
            write_json_optional_string(std::cout, options.tag);
            std::cout << ",\"alias\":";
            write_json_optional_string(std::cout, options.alias);
            std::cout << '}';
        }
        std::cout << ",\"count\":" << content_controls.size()
                  << ",\"content_controls\":[";
        for (std::size_t index = 0; index < content_controls.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_content_control_summary(std::cout,
                                               content_controls[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    if (options.tag.has_value()) {
        std::cout << "filter_tag: " << *options.tag << '\n';
    }
    if (options.alias.has_value()) {
        std::cout << "filter_alias: " << *options.alias << '\n';
    }
    std::cout << "content_controls: " << content_controls.size() << '\n';
    for (std::size_t index = 0; index < content_controls.size(); ++index) {
        std::cout << "content_control[" << index << "]: ";
        print_content_control_summary(std::cout, content_controls[index]);
        std::cout << '\n';
    }
}

void inspect_hyperlinks(
    const std::vector<featherdoc::hyperlink_summary> &hyperlinks,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << hyperlinks.size()
                  << ",\"hyperlinks\":[";
        for (std::size_t index = 0; index < hyperlinks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_hyperlink_summary(std::cout, hyperlinks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "hyperlinks: " << hyperlinks.size() << '\n';
    for (std::size_t index = 0; index < hyperlinks.size(); ++index) {
        std::cout << "hyperlink[" << index << "]: ";
        print_hyperlink_summary(std::cout, hyperlinks[index]);
        std::cout << '\n';
    }
}

void inspect_review(
    const std::vector<featherdoc::review_note_summary> &footnotes,
    const std::vector<featherdoc::review_note_summary> &endnotes,
    const std::vector<featherdoc::review_note_summary> &comments,
    const std::vector<featherdoc::revision_summary> &revisions, bool json_output) {
    if (json_output) {
        std::cout << "{\"footnotes_count\":" << footnotes.size()
                  << ",\"endnotes_count\":" << endnotes.size()
                  << ",\"comments_count\":" << comments.size()
                  << ",\"revisions_count\":" << revisions.size()
                  << ",\"footnotes\":[";
        for (std::size_t index = 0; index < footnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, footnotes[index]);
        }
        std::cout << "],\"endnotes\":[";
        for (std::size_t index = 0; index < endnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, endnotes[index]);
        }
        std::cout << "],\"comments\":[";
        for (std::size_t index = 0; index < comments.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, comments[index]);
        }
        std::cout << "],\"revisions\":[";
        for (std::size_t index = 0; index < revisions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_revision_summary(std::cout, revisions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "footnotes: " << footnotes.size() << '\n';
    for (std::size_t index = 0; index < footnotes.size(); ++index) {
        std::cout << "footnote[" << index << "]: ";
        print_review_note_summary(std::cout, footnotes[index]);
        std::cout << '\n';
    }
    std::cout << "endnotes: " << endnotes.size() << '\n';
    for (std::size_t index = 0; index < endnotes.size(); ++index) {
        std::cout << "endnote[" << index << "]: ";
        print_review_note_summary(std::cout, endnotes[index]);
        std::cout << '\n';
    }
    std::cout << "comments: " << comments.size() << '\n';
    for (std::size_t index = 0; index < comments.size(); ++index) {
        std::cout << "comment[" << index << "]: ";
        print_review_note_summary(std::cout, comments[index]);
        std::cout << '\n';
    }
    std::cout << "revisions: " << revisions.size() << '\n';
    for (std::size_t index = 0; index < revisions.size(); ++index) {
        std::cout << "revision[" << index << "]: ";
        print_revision_summary(std::cout, revisions[index]);
        std::cout << '\n';
    }
}

void inspect_omml(const std::vector<featherdoc::omml_summary> &formulas,
                  bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << formulas.size()
                  << ",\"formulas\":[";
        for (std::size_t index = 0; index < formulas.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_omml_summary(std::cout, formulas[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "formulas: " << formulas.size() << '\n';
    for (std::size_t index = 0; index < formulas.size(); ++index) {
        std::cout << "formula[" << index << "]: ";
        print_omml_summary(std::cout, formulas[index]);
        std::cout << '\n';
    }
}

void write_json_content_control_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_text_options &options, std::size_t replaced) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"selector\":{\"kind\":";
    write_json_string(stream, options.tag.has_value() ? "tag" : "alias");
    stream << ",\"value\":";
    write_json_string(stream, options.tag.has_value() ? *options.tag
                                                 : *options.alias);
    stream << "},\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, options.text);
}

void print_content_control_text_result(
    const selected_template_part &selected,
    const replace_content_control_text_options &options,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "selector_kind: "
              << (options.tag.has_value() ? "tag" : "alias") << '\n';
    std::cout << "selector_value: "
              << (options.tag.has_value() ? *options.tag : *options.alias)
              << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "text: " << options.text << '\n';
}

void print_simple_document_mutation_result(
    std::string_view command, const std::optional<path_type> &output_path,
    std::size_t affected) {
    std::cout << "command: " << command << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "affected: " << affected << '\n';
}

void write_json_affected_result(std::ostream &stream, std::size_t affected) {
    stream << ",\"affected\":" << affected;
}

void write_json_content_control_selector(
    std::ostream &stream, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
    stream << ",\"selector\":{\"kind\":";
    write_json_string(stream, tag.has_value() ? "tag" : "alias");
    stream << ",\"value\":";
    write_json_string(stream, tag.has_value() ? *tag : *alias);
    stream << '}';
}

void write_json_content_control_part_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    write_json_content_control_selector(stream, tag, alias);
}

void write_json_content_control_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced
           << ",\"paragraph_count\":" << paragraphs.size()
           << ",\"paragraphs\":";
    write_json_strings(stream, paragraphs);
}

void write_json_content_control_table_result(
    std::ostream &stream, const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced << ",\"row_count\":"
           << rows.size() << ",\"rows\":[";
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0U) {
            stream << ',';
        }
        stream << '[';
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                stream << ',';
            }
            write_json_string(stream, rows[row_index][cell_index]);
        }
        stream << ']';
    }
    stream << ']';
}

void write_json_content_control_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"image_path\":";
    write_json_string(stream, image_path.string());
    stream << ",\"replaced\":" << inserted_images.size() << ",\"images\":[";
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_drawing_image_summary(stream, inserted_images[index]);
    }
    stream << ']';
}

void write_json_content_control_form_state_options(
    std::ostream &stream,
    const featherdoc::content_control_form_state_options &state) {
    stream << R"({"clear_lock":)" << (state.clear_lock ? "true" : "false");
    stream << R"(,"clear_data_binding":)"
           << (state.clear_data_binding ? "true" : "false");
    if (state.lock.has_value()) {
        stream << R"(,"lock":)";
        write_json_string(stream, *state.lock);
    }
    if (state.data_binding_store_item_id.has_value()) {
        stream << R"(,"data_binding_store_item_id":)";
        write_json_string(stream, *state.data_binding_store_item_id);
    }
    if (state.data_binding_xpath.has_value()) {
        stream << R"(,"data_binding_xpath":)";
        write_json_string(stream, *state.data_binding_xpath);
    }
    if (state.data_binding_prefix_mappings.has_value()) {
        stream << R"(,"data_binding_prefix_mappings":)";
        write_json_string(stream, *state.data_binding_prefix_mappings);
    }
    if (state.checked.has_value()) {
        stream << R"(,"checked":)" << (*state.checked ? "true" : "false");
    }
    if (state.selected_list_item.has_value()) {
        stream << R"(,"selected_item":)";
        write_json_string(stream, *state.selected_list_item);
    }
    if (state.date_text.has_value()) {
        stream << R"(,"date_text":)";
        write_json_string(stream, *state.date_text);
    }
    if (state.date_format.has_value()) {
        stream << R"(,"date_format":)";
        write_json_string(stream, *state.date_format);
    }
    if (state.date_locale.has_value()) {
        stream << R"(,"date_locale":)";
        write_json_string(stream, *state.date_locale);
    }
    stream << '}';
}

void write_json_content_control_form_state_result(
    std::ostream &stream, const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << R"(,"updated":)" << updated << R"(,"form_state":)";
    write_json_content_control_form_state_options(stream, options.state);
}

void write_json_custom_xml_sync_item(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_item &item) {
    stream << R"({"part_entry_name":)";
    write_json_string(stream, item.part_entry_name);
    stream << R"(,"content_control_index":)" << item.content_control_index
           << R"(,"tag":)";
    write_json_optional_string(stream, item.tag);
    stream << R"(,"alias":)";
    write_json_optional_string(stream, item.alias);
    stream << R"(,"store_item_id":)";
    write_json_string(stream, item.store_item_id);
    stream << R"(,"xpath":)";
    write_json_string(stream, item.xpath);
    stream << R"(,"previous_text":)";
    write_json_string(stream, item.previous_text);
    stream << R"(,"value":)";
    write_json_string(stream, item.value);
    stream << '}';
}

void write_json_custom_xml_sync_issue(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_issue &issue) {
    stream << R"({"part_entry_name":)";
    write_json_string(stream, issue.part_entry_name);
    stream << R"(,"content_control_index":)";
    write_json_optional_size(stream, issue.content_control_index);
    stream << R"(,"tag":)";
    write_json_optional_string(stream, issue.tag);
    stream << R"(,"alias":)";
    write_json_optional_string(stream, issue.alias);
    stream << R"(,"store_item_id":)";
    write_json_string(stream, issue.store_item_id);
    stream << R"(,"xpath":)";
    write_json_string(stream, issue.xpath);
    stream << R"(,"reason":)";
    write_json_string(stream, issue.reason);
    stream << '}';
}

void write_json_custom_xml_sync_result(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_result &result) {
    stream << R"(,"scanned_content_controls":)" << result.scanned_content_controls
           << R"(,"bound_content_controls":)" << result.bound_content_controls
           << R"(,"synced_content_controls":)" << result.synced_content_controls
           << R"(,"issue_count":)" << result.issues.size()
           << R"(,"synced_items":[)";
    for (std::size_t index = 0; index < result.synced_items.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_custom_xml_sync_item(stream, result.synced_items[index]);
    }
    stream << R"(],"issues":[)";
    for (std::size_t index = 0; index < result.issues.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_custom_xml_sync_issue(stream, result.issues[index]);
    }
    stream << ']';
}

void print_custom_xml_sync_result(
    const std::optional<path_type> &output_path,
    const featherdoc::custom_xml_data_binding_sync_result &result) {
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "scanned_content_controls: " << result.scanned_content_controls << '\n';
    std::cout << "bound_content_controls: " << result.bound_content_controls << '\n';
    std::cout << "synced_content_controls: " << result.synced_content_controls << '\n';
    std::cout << "issue_count: " << result.issues.size() << '\n';
    for (const auto &item : result.synced_items) {
        std::cout << "synced[" << item.content_control_index << "]: "
                  << item.part_entry_name << " " << item.xpath << " = "
                  << item.value << '\n';
    }
    for (const auto &issue : result.issues) {
        std::cout << "issue";
        if (issue.content_control_index.has_value()) {
            std::cout << '[' << *issue.content_control_index << ']';
        }
        std::cout << ": " << issue.part_entry_name << " " << issue.reason
                  << " " << issue.xpath << '\n';
    }
}

void print_content_control_common_result(
    const selected_template_part &selected, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "selector_kind: " << (tag.has_value() ? "tag" : "alias")
              << '\n';
    std::cout << "selector_value: " << (tag.has_value() ? *tag : *alias)
              << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
}

void print_content_control_paragraphs_result(
    const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path, replaced);
    std::cout << "paragraph_count: " << paragraphs.size() << '\n';
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        std::cout << "paragraph[" << index << "]: " << paragraphs[index] << '\n';
    }
}

void print_content_control_table_result(
    const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path, replaced);
    std::cout << "row_count: " << rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::cout << "row[" << row_index << "]:";
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            std::cout << (cell_index == 0U ? " " : " | ")
                      << rows[row_index][cell_index];
        }
        std::cout << '\n';
    }
}

void print_content_control_image_result(
    const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path,
                                        inserted_images.size());
    std::cout << "image_path: " << image_path.string() << '\n';
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, inserted_images[index]);
        std::cout << '\n';
    }
}

void print_content_control_form_state_result(
    const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated) {
    print_selected_template_part(std::cout, selected);
    std::cout << "selector_kind: " << (options.tag.has_value() ? "tag" : "alias")
              << '\n';
    std::cout << "selector_value: "
              << (options.tag.has_value() ? *options.tag : *options.alias)
              << '\n';
    if (options.output_path.has_value()) {
        std::cout << "output_path: " << options.output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "updated: " << updated << '\n';
    const auto &state = options.state;
    if (state.lock.has_value()) {
        std::cout << "lock: " << *state.lock << '\n';
    }
    if (state.clear_lock) {
        std::cout << "clear_lock: true\n";
    }
    if (state.clear_data_binding) {
        std::cout << "clear_data_binding: true\n";
    }
    if (state.data_binding_store_item_id.has_value()) {
        std::cout << "data_binding_store_item_id: "
                  << *state.data_binding_store_item_id << "\n";
    }
    if (state.data_binding_xpath.has_value()) {
        std::cout << "data_binding_xpath: " << *state.data_binding_xpath
                  << "\n";
    }
    if (state.data_binding_prefix_mappings.has_value()) {
        std::cout << "data_binding_prefix_mappings: "
                  << *state.data_binding_prefix_mappings << "\n";
    }
    if (state.checked.has_value()) {
        std::cout << "checked: " << (*state.checked ? "true" : "false")
                  << '\n';
    }
    if (state.selected_list_item.has_value()) {
        std::cout << "selected_item: " << *state.selected_list_item << '\n';
    }
    if (state.date_text.has_value()) {
        std::cout << "date_text: " << *state.date_text << '\n';
    }
    if (state.date_format.has_value()) {
        std::cout << "date_format: " << *state.date_format << '\n';
    }
    if (state.date_locale.has_value()) {
        std::cout << "date_locale: " << *state.date_locale << '\n';
    }
}

void inspect_images(const selected_template_part &selected,
                    const std::vector<featherdoc::drawing_image_info> &images,
                    const inspect_images_options &options, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"count\":" << images.size() << ",\"images\":[";
        for (std::size_t index = 0; index < images.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_drawing_image_summary(std::cout, images[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    print_inspect_image_filters(std::cout, options);
    std::cout << "images: " << images.size() << '\n';
    for (std::size_t index = 0; index < images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, images[index]);
        std::cout << '\n';
    }
}

void inspect_image(const selected_template_part &selected,
                   const featherdoc::drawing_image_info &image,
                   const inspect_images_options &options, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, image);
        std::cout << "}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    print_inspect_image_filters(std::cout, options);
    std::cout << "image_index: " << image.index << '\n';
    std::cout << "placement: " << drawing_image_placement_name(image.placement)
              << '\n';
    std::cout << "relationship_id: " << image.relationship_id << '\n';
    std::cout << "image_entry_name: " << image.entry_name << '\n';
    std::cout << "display_name: " << image.display_name << '\n';
    std::cout << "content_type: " << image.content_type << '\n';
    std::cout << "width_px: " << image.width_px << '\n';
    std::cout << "height_px: " << image.height_px << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (!image.floating_options.has_value()) {
        return;
    }

    const auto &floating_options = *image.floating_options;
    std::cout << "horizontal_reference: "
              << floating_image_horizontal_reference_name(
                     floating_options.horizontal_reference)
              << '\n';
    std::cout << "horizontal_offset_px: " << floating_options.horizontal_offset_px
              << '\n';
    std::cout << "vertical_reference: "
              << floating_image_vertical_reference_name(
                     floating_options.vertical_reference)
              << '\n';
    std::cout << "vertical_offset_px: " << floating_options.vertical_offset_px
              << '\n';
    std::cout << "behind_text: " << yes_no(floating_options.behind_text) << '\n';
    std::cout << "allow_overlap: " << yes_no(floating_options.allow_overlap)
              << '\n';
    std::cout << "z_order: " << floating_options.z_order << '\n';
    std::cout << "wrap_mode: "
              << floating_image_wrap_mode_name(floating_options.wrap_mode)
              << '\n';
    std::cout << "wrap_distance_left_px: "
              << floating_options.wrap_distance_left_px << '\n';
    std::cout << "wrap_distance_right_px: "
              << floating_options.wrap_distance_right_px
              << '\n';
    std::cout << "wrap_distance_top_px: "
              << floating_options.wrap_distance_top_px << '\n';
    std::cout << "wrap_distance_bottom_px: "
              << floating_options.wrap_distance_bottom_px
              << '\n';
    if (!floating_options.crop.has_value()) {
        std::cout << "crop: none\n";
        return;
    }

    std::cout << "crop_left_per_mille: " << floating_options.crop->left_per_mille
              << '\n';
    std::cout << "crop_top_per_mille: " << floating_options.crop->top_per_mille
              << '\n';
    std::cout << "crop_right_per_mille: "
              << floating_options.crop->right_per_mille << '\n';
    std::cout << "crop_bottom_per_mille: "
              << floating_options.crop->bottom_per_mille
              << '\n';
}

void print_replace_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &replacement_path,
                                const std::optional<path_type> &output_path,
                                bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        std::cout << ",\"replacement_path\":";
        write_json_string(std::cout, replacement_path.string());
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, image);
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << "}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "replacement_path: " << replacement_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_append_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const path_type &image_path,
                               const std::optional<path_type> &output_path) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "image_path: " << image_path.string() << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

auto is_same_drawing_image_identity(const featherdoc::drawing_image_info &lhs,
                                    const featherdoc::drawing_image_info &rhs)
    -> bool {
    return lhs.relationship_id == rhs.relationship_id &&
           lhs.entry_name == rhs.entry_name;
}

auto collect_new_drawing_images(
    const std::vector<featherdoc::drawing_image_info> &before_images,
    const std::vector<featherdoc::drawing_image_info> &after_images)
    -> std::vector<featherdoc::drawing_image_info> {
    std::vector<bool> matched_before(before_images.size(), false);
    std::vector<featherdoc::drawing_image_info> inserted_images;
    inserted_images.reserve(after_images.size());

    for (const auto &image : after_images) {
        auto matched_it =
            std::find_if(before_images.begin(), before_images.end(),
                         [&before_images, &matched_before,
                          &image](const featherdoc::drawing_image_info &candidate) {
                             const auto candidate_index = static_cast<std::size_t>(
                                 &candidate - before_images.data());
                             return !matched_before[candidate_index] &&
                                    is_same_drawing_image_identity(candidate, image);
                         });
        if (matched_it == before_images.end()) {
            inserted_images.push_back(image);
            continue;
        }

        matched_before[static_cast<std::size_t>(matched_it - before_images.begin())] =
            true;
    }

    return inserted_images;
}

void write_json_bookmark_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"image_path\":";
    write_json_string(stream, image_path.string());
    stream << ",\"replaced\":" << inserted_images.size() << ",\"images\":[";
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_drawing_image_summary(stream, inserted_images[index]);
    }
    stream << ']';
}

void print_bookmark_image_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::optional<path_type> &output_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    std::cout << "image_path: " << image_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << inserted_images.size() << '\n';
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, inserted_images[index]);
        std::cout << '\n';
    }
}

void write_json_bookmark_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced
           << ",\"paragraph_count\":" << paragraphs.size()
           << ",\"paragraphs\":[";
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, paragraphs[index]);
    }
    stream << ']';
}

void print_bookmark_paragraphs_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "paragraph_count: " << paragraphs.size() << '\n';
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        std::cout << "paragraph[" << index << "]: " << paragraphs[index] << '\n';
    }
}

void write_json_bookmark_table_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced << ",\"row_count\":" << rows.size()
           << ",\"rows\":[";
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0U) {
            stream << ',';
        }
        stream << '[';
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                stream << ',';
            }
            write_json_string(stream, rows[row_index][cell_index]);
        }
        stream << ']';
    }
    stream << ']';
}

void print_bookmark_table_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::vector<std::string>> &rows,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "row_count: " << rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::cout << "row[" << row_index << "]: ";
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                std::cout << '\t';
            }
            std::cout << rows[row_index][cell_index];
        }
        std::cout << '\n';
    }
}

void write_json_bookmark_block_removal_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::size_t removed) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"removed\":" << removed;
}

void print_bookmark_block_removal_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, std::size_t removed) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed: " << removed << '\n';
}

void write_json_bookmark_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::string_view text,
    std::size_t replaced) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, text);
}

void print_bookmark_text_result(const selected_template_part &selected,
                                const featherdoc::bookmark_summary &bookmark,
                                std::string_view text,
                                const std::optional<path_type> &output_path,
                                std::size_t replaced) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "text: " << text << '\n';
}

void write_json_bookmark_text_bindings(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_text_binding> &bindings) {
    stream << '[';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"bookmark_name\":";
        write_json_string(stream, bindings[index].bookmark_name);
        stream << ",\"text\":";
        write_json_string(stream, bindings[index].text);
        stream << '}';
    }
    stream << ']';
}

void write_json_bookmark_fill_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const featherdoc::bookmark_fill_result &result) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"complete\":" << json_bool(static_cast<bool>(result))
           << ",\"requested\":" << result.requested
           << ",\"matched\":" << result.matched
           << ",\"replaced\":" << result.replaced
           << ",\"bindings\":";
    write_json_bookmark_text_bindings(stream, bindings);
    stream << ",\"missing_bookmarks\":";
    write_json_strings(stream, result.missing_bookmarks);
}

void print_bookmark_fill_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_fill_result &result) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "complete: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "requested: " << result.requested << '\n';
    std::cout << "matched: " << result.matched << '\n';
    std::cout << "replaced: " << result.replaced << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        std::cout << "binding[" << index << "]: "
                  << bindings[index].bookmark_name << " => "
                  << bindings[index].text << '\n';
    }
    std::cout << "missing_bookmarks: ";
    if (result.missing_bookmarks.empty()) {
        std::cout << "none\n";
    } else {
        for (std::size_t index = 0; index < result.missing_bookmarks.size();
             ++index) {
            if (index != 0U) {
                std::cout << ", ";
            }
            std::cout << result.missing_bookmarks[index];
        }
        std::cout << '\n';
    }
}

void write_json_bookmark_block_visibility_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, bool visible,
    std::size_t changed) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"bookmark\":";
    write_json_bookmark_summary(stream, bookmark);
    stream << ",\"visible\":" << json_bool(visible)
           << ",\"changed\":" << changed;
}

void print_bookmark_block_visibility_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, bool visible,
    std::size_t changed) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "bookmark_kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "bookmark_occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "bookmark_duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "visible: " << yes_no(visible) << '\n';
    std::cout << "changed: " << changed << '\n';
}

void write_json_bookmark_block_visibility_bindings(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings) {
    stream << '[';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"bookmark_name\":";
        write_json_string(stream, bindings[index].bookmark_name);
        stream << ",\"visible\":" << json_bool(bindings[index].visible) << '}';
    }
    stream << ']';
}

void write_json_applied_bookmark_block_visibility_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings,
    const featherdoc::bookmark_block_visibility_result &result) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"complete\":" << json_bool(static_cast<bool>(result))
           << ",\"requested\":" << result.requested
           << ",\"matched\":" << result.matched
           << ",\"kept\":" << result.kept
           << ",\"removed\":" << result.removed
           << ",\"bindings\":";
    write_json_bookmark_block_visibility_bindings(stream, bindings);
    stream << ",\"missing_bookmarks\":";
    write_json_strings(stream, result.missing_bookmarks);
}

void print_applied_bookmark_block_visibility_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_block_visibility_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_block_visibility_result &result) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "complete: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "requested: " << result.requested << '\n';
    std::cout << "matched: " << result.matched << '\n';
    std::cout << "kept: " << result.kept << '\n';
    std::cout << "removed: " << result.removed << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        std::cout << "binding[" << index << "]: "
                  << bindings[index].bookmark_name << " => "
                  << (bindings[index].visible ? "visible" : "hidden") << '\n';
    }
    std::cout << "missing_bookmarks: ";
    if (result.missing_bookmarks.empty()) {
        std::cout << "none\n";
    } else {
        for (std::size_t index = 0; index < result.missing_bookmarks.size();
             ++index) {
            if (index != 0U) {
                std::cout << ", ";
            }
            std::cout << result.missing_bookmarks[index];
        }
        std::cout << '\n';
    }
}

void print_extract_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &output_path) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "output_path: " << output_path.string() << '\n';
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_remove_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const std::optional<path_type> &output_path) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed_image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void inspect_related_parts(const std::vector<inspected_part_entry> &parts,
                           section_part_family family, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"count\":" << parts.size() << ",\"parts\":[";
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }

            const auto &part = parts[index];
            std::cout << "{\"index\":" << index << ",\"relationship_id\":";
            write_json_string(std::cout, part.relationship_id);
            std::cout << ",\"entry\":";
            write_json_string(std::cout, part.entry_name);
            std::cout << ",\"references\":";
            write_json_part_references(std::cout, part.references);
            std::cout << ",\"paragraphs\":";
            write_json_lines(std::cout, part.paragraphs);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << section_part_name(family) << " parts: " << parts.size() << '\n';
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto &part = parts[index];
        std::cout << "part[" << index << "]: entry=" << part.entry_name
                  << " relationship=" << part.relationship_id << " references=";
        if (part.references.empty()) {
            std::cout << "none";
        } else {
            for (std::size_t reference_index = 0; reference_index < part.references.size();
                 ++reference_index) {
                if (reference_index != 0U) {
                    std::cout << ", ";
                }
                std::cout << "section[" << part.references[reference_index].section_index
                          << "]:" << part.references[reference_index].kind;
            }
        }
        std::cout << '\n';

        for (std::size_t paragraph_index = 0; paragraph_index < part.paragraphs.size();
             ++paragraph_index) {
            std::cout << "  paragraph[" << paragraph_index
                      << "]: " << part.paragraphs[paragraph_index] << '\n';
        }
    }
}

auto report_expected_revision_text_mismatch(std::string_view command,
                                            std::string_view expected_text,
                                            const featherdoc::text_range_preview &preview,
                                            bool json_output) -> bool {
    constexpr std::string_view message =
        "expected text did not match selected text";
    if (json_output) {
        std::cerr << "{\"command\":";
        write_json_string(std::cerr, command);
        std::cerr << ",\"ok\":false,\"stage\":\"validate\",\"message\":";
        write_json_string(std::cerr, message);
        std::cerr << ",\"expected_text\":";
        write_json_string(std::cerr, expected_text);
        std::cerr << ",\"actual_text\":";
        write_json_string(std::cerr, preview.text);
        std::cerr << ",\"preview\":";
        write_json_text_range_preview(std::cerr, preview);
        std::cerr << "}\n";
        return false;
    }

    std::cerr << message << '\n' << "expected_text=";
    write_json_string(std::cerr, expected_text);
    std::cerr << '\n' << "actual_text=";
    write_json_string(std::cerr, preview.text);
    std::cerr << '\n' << "selected_range=";
    print_text_range_preview(std::cerr, preview);
    std::cerr << '\n';
    return false;
}

auto validate_revision_expected_text(featherdoc::Document &doc,
                                     std::string_view command,
                                     const featherdoc_cli::revision_authoring_options
                                         &options,
                                     std::size_t start_paragraph_index,
                                     std::size_t start_text_offset,
                                     std::size_t end_paragraph_index,
                                     std::size_t end_text_offset) -> bool {
    if (!options.has_expected_text) {
        return true;
    }

    const auto preview = doc.preview_text_range(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset);
    if (!preview.has_value()) {
        return report_document_error(command, "preview", doc.last_error(),
                                     options.json_output);
    }

    if (preview->text != options.expected_text) {
        return report_expected_revision_text_mismatch(
            command, options.expected_text, *preview, options.json_output);
    }

    return true;
}

void inspect_sections(featherdoc::Document &doc, bool json_output) {
    if (json_output) {
        std::cout << "{\"sections\":" << doc.section_count()
                  << ",\"headers\":" << doc.header_count()
                  << ",\"footers\":" << doc.footer_count()
                  << ",\"section_layouts\":[";
        for (std::size_t section_index = 0; section_index < doc.section_count();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"index\":" << section_index << ",\"header\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::header);
            std::cout << ",\"footer\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::footer);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "sections: " << doc.section_count() << '\n';
    std::cout << "headers: " << doc.header_count() << '\n';
    std::cout << "footers: " << doc.footer_count() << '\n';

    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        std::cout
            << "section[" << section_index << "]: header(default="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ") footer(default="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ")\n";
    }
}

auto resolve_text_sources(const std::vector<cli_text_source_options> &sources,
                          std::vector<std::string> &texts,
                          std::string &error_message) -> bool {
    texts.clear();
    texts.reserve(sources.size());

    for (const auto &source : sources) {
        std::string text;
        if (!read_text_source(source, text, error_message)) {
            return false;
        }
        texts.push_back(std::move(text));
    }

    return true;
}

auto resolve_bookmark_table_row_sources(
    const bookmark_table_replacement_options &options,
    std::vector<std::vector<std::string>> &rows, std::string &error_message)
    -> bool {
    rows.clear();
    rows.reserve(options.row_sources.size());

    for (const auto &row_sources : options.row_sources) {
        std::vector<std::string> row;
        row.reserve(row_sources.size());

        for (const auto &source : row_sources) {
            std::string text;
            if (!read_text_source(source, text, error_message)) {
                return false;
            }
            row.push_back(std::move(text));
        }

        rows.push_back(std::move(row));
    }

    return true;
}

auto resolve_fill_bookmark_bindings(
    const fill_bookmarks_options &options,
    std::vector<featherdoc::bookmark_text_binding> &bindings,
    std::string &error_message) -> bool {
    bindings.clear();
    bindings.reserve(options.binding_sources.size());

    for (const auto &binding_source : options.binding_sources) {
        std::string text;
        if (!read_text_source(binding_source.source, text, error_message)) {
            return false;
        }

        bindings.push_back({binding_source.bookmark_name, std::move(text)});
    }

    return true;
}

auto write_numbering_catalog_file(const path_type &output_path,
                                  const featherdoc::numbering_catalog &catalog,
                                  std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open numbering catalog output path: " + output_path.string();
        return false;
    }

    write_json_numbering_catalog(stream, catalog);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write numbering catalog output path: " + output_path.string();
        return false;
    }

    return true;
}

void print_exported_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"export-numbering-catalog\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"definition_count\":" << catalog.definitions.size()
                  << ",\"instance_count\":"
                  << numbering_catalog_instance_count(catalog) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "definition_count: " << catalog.definitions.size() << '\n'
              << "instance_count: " << numbering_catalog_instance_count(catalog)
              << '\n';
}

void write_json_numbering_catalog_patch_summary(
    std::ostream &stream, const numbering_catalog_patch_summary &summary) {
    stream << "\"upserted_level_count\":" << summary.upserted_level_count
           << ",\"upserted_override_count\":" << summary.upserted_override_count
           << ",\"removed_override_count\":" << summary.removed_override_count
           << ",\"missing_override_count\":" << summary.missing_override_count;
}

void print_patched_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const numbering_catalog_patch_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"patch-numbering-catalog\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"definition_count\":" << catalog.definitions.size()
                  << ",\"instance_count\":"
                  << numbering_catalog_instance_count(catalog) << ',';
        write_json_numbering_catalog_patch_summary(std::cout, summary);
        std::cout << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "definition_count: " << catalog.definitions.size() << '\n'
              << "instance_count: " << numbering_catalog_instance_count(catalog)
              << '\n'
              << "upserted_level_count: " << summary.upserted_level_count
              << '\n'
              << "upserted_override_count: " << summary.upserted_override_count
              << '\n'
              << "removed_override_count: " << summary.removed_override_count
              << '\n'
              << "missing_override_count: " << summary.missing_override_count
              << '\n';
}

void write_json_numbering_catalog_lint_issue(
    std::ostream &stream, const numbering_catalog_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, numbering_catalog_lint_issue_name(issue.kind));
    stream << ",\"definition_index\":" << issue.definition_index
           << ",\"definition_name\":";
    write_json_string(stream, issue.definition_name);
    if (issue.instance_index.has_value()) {
        stream << ",\"instance_index\":" << *issue.instance_index;
    }
    if (issue.instance_id.has_value()) {
        stream << ",\"instance_id\":" << *issue.instance_id;
    }
    if (issue.level_index.has_value()) {
        stream << ",\"level_index\":" << *issue.level_index;
    }
    if (issue.override_index.has_value()) {
        stream << ",\"override_index\":" << *issue.override_index;
    }
    if (issue.level.has_value()) {
        stream << ",\"level\":" << *issue.level;
    }
    stream << ",\"detail\":";
    write_json_string(stream, issue.detail);
    stream << '}';
}

void write_json_numbering_catalog_lint_issues(
    std::ostream &stream,
    const std::vector<numbering_catalog_lint_issue> &issues) {
    for (std::size_t index = 0U; index < issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_lint_issue(stream, issues[index]);
    }
}

void print_linted_numbering_catalog_result(
    const numbering_catalog_lint_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"lint-numbering-catalog\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"definition_count\":" << result.definition_count
                  << ",\"instance_count\":" << result.instance_count
                  << ",\"level_count\":" << result.level_count
                  << ",\"override_count\":" << result.override_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout, result.issues);
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "definition_count: " << result.definition_count << '\n'
              << "instance_count: " << result.instance_count << '\n'
              << "level_count: " << result.level_count << '\n'
              << "override_count: " << result.override_count << '\n'
              << "issue_count: " << result.issues.size() << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << numbering_catalog_lint_issue_name(issue.kind)
                  << " definition_index=" << issue.definition_index
                  << " definition_name=" << issue.definition_name;
        if (issue.instance_index.has_value()) {
            std::cout << " instance_index=" << *issue.instance_index;
        }
        if (issue.instance_id.has_value()) {
            std::cout << " instance_id=" << *issue.instance_id;
        }
        if (issue.level_index.has_value()) {
            std::cout << " level_index=" << *issue.level_index;
        }
        if (issue.override_index.has_value()) {
            std::cout << " override_index=" << *issue.override_index;
        }
        if (issue.level.has_value()) {
            std::cout << " level=" << *issue.level;
        }
        std::cout << " detail=" << issue.detail << '\n';
    }
}

void write_json_numbering_catalog_changed_level(
    std::ostream &stream, const changed_numbering_catalog_level &changed_level) {
    stream << "{\"left\":";
    write_json_numbering_level_definition(stream, changed_level.left);
    stream << ",\"right\":";
    write_json_numbering_level_definition(stream, changed_level.right);
    stream << '}';
}

void write_json_numbering_catalog_changed_override(
    std::ostream &stream, const changed_numbering_catalog_override &changed_override) {
    stream << "{\"left\":";
    write_json_numbering_level_override_summary(stream, changed_override.left);
    stream << ",\"right\":";
    write_json_numbering_level_override_summary(stream, changed_override.right);
    stream << '}';
}

void write_json_numbering_catalog_instance_diff(
    std::ostream &stream, const numbering_catalog_instance_diff_result &diff) {
    stream << "{\"instance_index\":" << diff.instance_index
           << ",\"added_override_count\":" << diff.added_overrides.size()
           << ",\"removed_override_count\":" << diff.removed_overrides.size()
           << ",\"changed_override_count\":" << diff.changed_overrides.size()
           << ",\"added_overrides\":[";
    for (std::size_t index = 0U; index < diff.added_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    diff.added_overrides[index]);
    }
    stream << "],\"removed_overrides\":[";
    for (std::size_t index = 0U; index < diff.removed_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    diff.removed_overrides[index]);
    }
    stream << "],\"changed_overrides\":[";
    for (std::size_t index = 0U; index < diff.changed_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_override(stream,
                                                      diff.changed_overrides[index]);
    }
    stream << "]}";
}

void write_json_numbering_catalog_changed_definition(
    std::ostream &stream,
    const changed_numbering_catalog_definition &definition_diff) {
    stream << "{\"name\":";
    write_json_string(stream, definition_diff.name);
    stream << ",\"added_level_count\":" << definition_diff.added_levels.size()
           << ",\"removed_level_count\":" << definition_diff.removed_levels.size()
           << ",\"changed_level_count\":" << definition_diff.changed_levels.size()
           << ",\"added_instance_count\":"
           << definition_diff.added_instances.size()
           << ",\"removed_instance_count\":"
           << definition_diff.removed_instances.size()
           << ",\"changed_instance_count\":"
           << definition_diff.changed_instances.size()
           << ",\"added_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.added_levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream,
                                              definition_diff.added_levels[index]);
    }
    stream << "],\"removed_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.removed_levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream,
                                              definition_diff.removed_levels[index]);
    }
    stream << "],\"changed_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.changed_levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_level(
            stream, definition_diff.changed_levels[index]);
    }
    stream << "],\"added_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.added_instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream,
                                              definition_diff.added_instances[index]);
    }
    stream << "],\"removed_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.removed_instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream,
                                              definition_diff.removed_instances[index]);
    }
    stream << "],\"changed_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.changed_instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_instance_diff(
            stream, definition_diff.changed_instances[index]);
    }
    stream << "]}";
}

void write_json_numbering_catalog_diff_result(
    std::ostream &stream, const numbering_catalog_diff_result &result) {
    stream << "{\"equal\":" << json_bool(result.equal())
           << ",\"added_definition_count\":" << result.added_definitions.size()
           << ",\"removed_definition_count\":"
           << result.removed_definitions.size()
           << ",\"changed_definition_count\":"
           << result.changed_definitions.size() << ",\"added_definitions\":[";
    for (std::size_t index = 0U; index < result.added_definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(stream,
                                                result.added_definitions[index]);
    }
    stream << "],\"removed_definitions\":[";
    for (std::size_t index = 0U; index < result.removed_definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(stream,
                                                result.removed_definitions[index]);
    }
    stream << "],\"changed_definitions\":[";
    for (std::size_t index = 0U; index < result.changed_definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_definition(
            stream, result.changed_definitions[index]);
    }
    stream << "]}\n";
}

void print_numbering_catalog_diff_result(
    const numbering_catalog_diff_result &result, bool json_output) {
    if (json_output) {
        write_json_numbering_catalog_diff_result(std::cout, result);
        return;
    }

    std::cout << "equal: " << yes_no(result.equal()) << '\n'
              << "added_definition_count: " << result.added_definitions.size()
              << '\n'
              << "removed_definition_count: "
              << result.removed_definitions.size() << '\n'
              << "changed_definition_count: "
              << result.changed_definitions.size() << '\n';
}

void print_checked_numbering_catalog_result(
    const path_type &catalog_path,
    const numbering_catalog_lint_result &baseline_lint,
    const numbering_catalog_lint_result &generated_lint,
    const numbering_catalog_diff_result &diff,
    const std::optional<path_type> &output_path, bool json_output) {
    const auto clean = baseline_lint.clean() && generated_lint.clean();
    if (json_output) {
        std::cout << "{\"command\":\"check-numbering-catalog\","
                  << "\"matches\":" << json_bool(diff.equal())
                  << ",\"clean\":" << json_bool(clean)
                  << ",\"catalog_file\":";
        write_json_string(std::cout, catalog_path.string());
        if (output_path.has_value()) {
            std::cout << ",\"generated_output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"baseline_issue_count\":"
                  << baseline_lint.issues.size()
                  << ",\"generated_issue_count\":"
                  << generated_lint.issues.size()
                  << ",\"added_definition_count\":"
                  << diff.added_definitions.size()
                  << ",\"removed_definition_count\":"
                  << diff.removed_definitions.size()
                  << ",\"changed_definition_count\":"
                  << diff.changed_definitions.size()
                  << ",\"baseline_issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout,
                                                 baseline_lint.issues);
        std::cout << "],\"generated_issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout,
                                                 generated_lint.issues);
        std::cout << "],\"added_definitions\":[";
        for (std::size_t index = 0U; index < diff.added_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_definition(
                std::cout, diff.added_definitions[index]);
        }
        std::cout << "],\"removed_definitions\":[";
        for (std::size_t index = 0U; index < diff.removed_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_definition(
                std::cout, diff.removed_definitions[index]);
        }
        std::cout << "],\"changed_definitions\":[";
        for (std::size_t index = 0U; index < diff.changed_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_changed_definition(
                std::cout, diff.changed_definitions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "matches: " << yes_no(diff.equal()) << '\n'
              << "clean: " << yes_no(clean) << '\n'
              << "catalog_file: " << catalog_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "generated_output_path: " << output_path->string()
                  << '\n';
    }
    std::cout << "baseline_issue_count: " << baseline_lint.issues.size()
              << '\n'
              << "generated_issue_count: " << generated_lint.issues.size()
              << '\n'
              << "added_definition_count: " << diff.added_definitions.size()
              << '\n'
              << "removed_definition_count: " << diff.removed_definitions.size()
              << '\n'
              << "changed_definition_count: " << diff.changed_definitions.size()
              << '\n';
}

auto append_validate_template_schema_file_targets(
    const path_type &schema_path,
    std::vector<validate_template_schema_target_options> &targets,
    std::string &error_message) -> bool {
    exported_template_schema_result schema_file;
    if (!read_template_schema_file(schema_path, schema_file, error_message)) {
        return false;
    }

    for (const auto &schema_target : schema_file.targets) {
        validate_template_schema_target_options target{};
        target.part = schema_target.part;
        target.part_index = schema_target.part_index;
        target.section_index = schema_target.section_index;
        target.requirements = schema_target.requirements;
        if (schema_target.reference_kind.has_value()) {
            target.reference_kind = *schema_target.reference_kind;
            target.has_kind = true;
        }
        targets.push_back(std::move(target));
    }

    return true;
}

auto show_section_text(featherdoc::Document &doc, section_part_family family,
                       std::size_t section_index,
                       featherdoc::section_reference_kind reference_kind,
                       bool json_output) -> bool {
    const auto lines = collect_part_lines(
        section_part_paragraphs(doc, family, section_index, reference_kind));

    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"section\":" << section_index << ",\"kind\":";
        write_json_string(std::cout,
                          featherdoc::to_xml_reference_type(reference_kind));
        std::cout << ",\"present\":" << (!lines.empty() ? "true" : "false")
                  << ",\"paragraphs\":";
        write_json_lines(std::cout, lines);
        std::cout << "}\n";
        return true;
    }

    if (lines.empty()) {
        std::cerr << "section " << section_part_name(family)
                  << " reference not found\n";
        return false;
    }

    for (const auto &line : lines) {
        std::cout << line << '\n';
    }

    return true;
}

auto assign_section_part(featherdoc::Document &doc, section_part_family family,
                         std::size_t section_index, std::size_t part_index,
                         featherdoc::section_reference_kind reference_kind,
                         std::string_view command, bool json_output) -> bool {
    auto &paragraphs = family == section_part_family::header
                           ? doc.assign_section_header_paragraphs(section_index, part_index,
                                                                 reference_kind)
                           : doc.assign_section_footer_paragraphs(section_index, part_index,
                                                                 reference_kind);
    if (!paragraphs.has_next()) {
        std::string message = "failed to assign section ";
        message += section_part_name(family);
        message += " reference";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto remove_section_part_reference(featherdoc::Document &doc,
                                   section_part_family family,
                                   std::size_t section_index,
                                   featherdoc::section_reference_kind reference_kind,
                                   std::string_view command, bool json_output)
    -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.remove_section_header_reference(section_index, reference_kind)
            : doc.remove_section_footer_reference(section_index, reference_kind);
    if (!success) {
        std::string message = "section ";
        message += section_part_name(family);
        message += " reference not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto remove_part(featherdoc::Document &doc, section_part_family family,
                 std::size_t part_index, std::string_view command,
                 bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.remove_header_part(part_index)
                             : doc.remove_footer_part(part_index);
    if (!success) {
        std::string message = std::string(section_part_name(family)) + " part not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto move_part(featherdoc::Document &doc, section_part_family family,
               std::size_t source_index, std::size_t target_index,
               std::string_view command, bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.move_header_part(source_index, target_index)
                             : doc.move_footer_part(source_index, target_index);
    if (!success) {
        std::string message =
            std::string(section_part_name(family)) + " part could not be reordered";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto replace_section_text(featherdoc::Document &doc, section_part_family family,
                          std::size_t section_index, std::string_view replacement_text,
                          featherdoc::section_reference_kind reference_kind,
                          std::string_view command = {}, bool json_output = false)
    -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.replace_section_header_text(section_index, replacement_text,
                                              reference_kind)
            : doc.replace_section_footer_text(section_index, replacement_text,
                                              reference_kind);

    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(), json_output);
    }

    return success;
}

void print_string_list(std::ostream &stream, std::string_view label,
                       const std::vector<std::string> &values) {
    stream << label << ": ";
    if (values.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            stream << ", ";
        }
        stream << values[index];
    }
    stream << '\n';
}

void write_json_bookmark_summaries(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_summary> &bookmarks) {
    stream << '[';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_bookmark_summary(stream, bookmarks[index]);
    }
    stream << ']';
}

void write_json_template_kind_mismatches(
    std::ostream &stream,
    const std::vector<featherdoc::template_kind_mismatch> &mismatches) {
    stream << '[';
    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &mismatch = mismatches[index];
        stream << "{\"bookmark_name\":";
        write_json_string(stream, mismatch.bookmark_name);
        stream << ",\"expected_kind\":";
        write_json_string(stream, template_slot_kind_name(mismatch.expected_kind));
        stream << ",\"actual_kind\":";
        write_json_string(stream, bookmark_kind_name(mismatch.actual_kind));
        stream << ",\"occurrence_count\":" << mismatch.occurrence_count << '}';
    }
    stream << ']';
}

void write_json_template_occurrence_mismatches(
    std::ostream &stream,
    const std::vector<featherdoc::template_occurrence_mismatch> &mismatches) {
    stream << '[';
    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &mismatch = mismatches[index];
        stream << "{\"bookmark_name\":";
        write_json_string(stream, mismatch.bookmark_name);
        stream << ",\"actual_occurrences\":" << mismatch.actual_occurrences
               << ",\"min_occurrences\":" << mismatch.min_occurrences
               << ",\"max_occurrences\":";
        if (mismatch.max_occurrences.has_value()) {
            stream << *mismatch.max_occurrences;
        } else {
            stream << "null";
        }
        stream << '}';
    }
    stream << ']';
}

void print_bookmark_summary_list(std::ostream &stream, std::string_view label,
                                 const std::vector<featherdoc::bookmark_summary> &bookmarks) {
    stream << label << ": ";
    if (bookmarks.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }
        print_bookmark_summary(stream, bookmarks[index]);
    }
    stream << '\n';
}

void print_template_kind_mismatch_list(
    std::ostream &stream, std::string_view label,
    const std::vector<featherdoc::template_kind_mismatch> &mismatches) {
    stream << label << ": ";
    if (mismatches.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }

        const auto &mismatch = mismatches[index];
        stream << mismatch.bookmark_name
               << " expected=" << template_slot_kind_name(mismatch.expected_kind)
               << " actual=" << bookmark_kind_name(mismatch.actual_kind)
               << " occurrences=" << mismatch.occurrence_count;
    }
    stream << '\n';
}

void print_template_occurrence_mismatch_list(
    std::ostream &stream, std::string_view label,
    const std::vector<featherdoc::template_occurrence_mismatch> &mismatches) {
    stream << label << ": ";
    if (mismatches.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }

        const auto &mismatch = mismatches[index];
        stream << mismatch.bookmark_name
               << " actual=" << mismatch.actual_occurrences
               << " min=" << mismatch.min_occurrences
               << " max=";
        if (mismatch.max_occurrences.has_value()) {
            stream << *mismatch.max_occurrences;
        } else {
            stream << "unbounded";
        }
    }
    stream << '\n';
}

auto append_template_part_field(featherdoc::Document &doc,
                                selected_template_part &selected,
                                std::string_view command,
                                const append_field_options &options,
                                bool json_output) -> bool {
    bool success = false;
    auto field_state = featherdoc::field_state_options{};
    field_state.dirty = options.dirty;
    field_state.locked = options.locked;
    if (command == "append-field") {
        success = selected.part.append_field(
            options.field_argument, options.result_text, field_state);
    } else if (command == "append-page-number-field") {
        success = selected.part.append_page_number_field(field_state);
    } else if (command == "append-total-pages-field") {
        success = selected.part.append_total_pages_field(field_state);
    } else if (command == "append-table-of-contents-field") {
        auto field_options = featherdoc::table_of_contents_field_options{};
        field_options.min_outline_level = options.min_outline_level;
        field_options.max_outline_level = options.max_outline_level;
        field_options.hyperlinks = options.hyperlinks;
        field_options.hide_page_numbers_in_web_layout =
            options.hide_page_numbers_in_web_layout;
        field_options.use_outline_levels = options.use_outline_levels;
        field_options.state = field_state;
        success = selected.part.append_table_of_contents_field(
            field_options, options.result_text);
    } else if (command == "append-reference-field") {
        auto field_options = featherdoc::reference_field_options{};
        field_options.hyperlink = options.hyperlink;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-page-reference-field") {
        auto field_options = featherdoc::page_reference_field_options{};
        field_options.hyperlink = options.hyperlink;
        field_options.relative_position = options.relative_position;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_page_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-style-reference-field") {
        auto field_options = featherdoc::style_reference_field_options{};
        field_options.paragraph_number = options.paragraph_number;
        field_options.relative_position = options.relative_position;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_style_reference_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-document-property-field") {
        auto field_options = featherdoc::document_property_field_options{};
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_document_property_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-date-field") {
        auto field_options = featherdoc::date_field_options{};
        field_options.format = options.date_format;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_date_field(field_options, options.result_text);
    } else if (command == "append-hyperlink-field") {
        auto field_options = featherdoc::hyperlink_field_options{};
        field_options.anchor = options.anchor;
        field_options.tooltip = options.tooltip;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_hyperlink_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-sequence-field") {
        auto field_options = featherdoc::sequence_field_options{};
        field_options.number_format = options.number_format;
        field_options.restart_value = options.restart_value;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_sequence_field(
            options.field_argument, field_options,
            options.has_result_text ? std::string_view{options.result_text}
                                    : std::string_view{"1"});
    } else if (command == "append-caption") {
        auto field_options = featherdoc::caption_field_options{};
        field_options.number_format = options.number_format;
        field_options.restart_value = options.restart_value;
        field_options.separator = options.separator;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_caption(
            options.field_argument, options.caption_text, field_options,
            options.number_result_text);
    } else if (command == "append-index-field") {
        auto field_options = featherdoc::index_field_options{};
        field_options.columns = options.columns;
        field_options.preserve_formatting = options.preserve_formatting;
        field_options.state = field_state;
        success = selected.part.append_index_field(field_options,
                                                   options.result_text);
    } else if (command == "append-index-entry-field") {
        auto field_options = featherdoc::index_entry_field_options{};
        field_options.subentry = options.subentry;
        field_options.bookmark_name = options.bookmark_name;
        field_options.cross_reference = options.cross_reference;
        field_options.bold_page_number = options.bold_page_number;
        field_options.italic_page_number = options.italic_page_number;
        field_options.state = field_state;
        success = selected.part.append_index_entry_field(
            options.field_argument, field_options, options.result_text);
    } else if (command == "append-complex-field") {
        auto field_options = featherdoc::complex_field_options{};
        field_options.state = field_state;
        if (options.instruction.has_value()) {
            success = selected.part.append_complex_field(
                *options.instruction, options.result_text, field_options);
        } else {
            success = selected.part.append_complex_field(
                {featherdoc::complex_field_text_fragment(
                     *options.instruction_before),
                 featherdoc::complex_field_nested_fragment(
                     *options.nested_instruction, options.nested_result_text),
                 featherdoc::complex_field_text_fragment(
                     *options.instruction_after)},
                options.result_text, field_options);
        }
    } else if (command == "replace-field") {
        success = selected.part.replace_field(
            *options.field_index, options.field_argument, options.result_text);
    }

    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(), json_output);
    }

    return true;
}

void print_template_validation_result(
    const selected_template_part &selected,
    const featherdoc::template_validation_result &result, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        std::cout << ",\"passed\":" << json_bool(static_cast<bool>(result))
                  << ",\"missing_required\":";
        write_json_strings(std::cout, result.missing_required);
        std::cout << ",\"duplicate_bookmarks\":";
        write_json_strings(std::cout, result.duplicate_bookmarks);
        std::cout << ",\"malformed_placeholders\":";
        write_json_strings(std::cout, result.malformed_placeholders);
        std::cout << ",\"unexpected_bookmarks\":";
        write_json_bookmark_summaries(std::cout, result.unexpected_bookmarks);
        std::cout << ",\"kind_mismatches\":";
        write_json_template_kind_mismatches(std::cout, result.kind_mismatches);
        std::cout << ",\"occurrence_mismatches\":";
        write_json_template_occurrence_mismatches(
            std::cout, result.occurrence_mismatches);
        std::cout << "}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind) << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "passed: " << yes_no(static_cast<bool>(result)) << '\n';
    print_string_list(std::cout, "missing_required", result.missing_required);
    print_string_list(std::cout, "duplicate_bookmarks", result.duplicate_bookmarks);
    print_string_list(std::cout, "malformed_placeholders",
                      result.malformed_placeholders);
    print_bookmark_summary_list(std::cout, "unexpected_bookmarks",
                                result.unexpected_bookmarks);
    print_template_kind_mismatch_list(std::cout, "kind_mismatches",
                                      result.kind_mismatches);
    print_template_occurrence_mismatch_list(std::cout, "occurrence_mismatches",
                                            result.occurrence_mismatches);
}

void write_json_template_schema_part_result(
    std::ostream &stream,
    const featherdoc::template_schema_part_validation_result &part_result) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(part_result.target.part));
    if (part_result.target.part_index.has_value()) {
        stream << ",\"part_index\":" << *part_result.target.part_index;
    }
    if (part_result.target.section_index.has_value()) {
        stream << ",\"section\":" << *part_result.target.section_index;
    }
    if (part_result.target.part ==
            featherdoc::template_schema_part_kind::section_header ||
        part_result.target.part ==
            featherdoc::template_schema_part_kind::section_footer) {
        stream << ",\"kind\":";
        write_json_string(
            stream, featherdoc::to_xml_reference_type(part_result.target.reference_kind));
    }
    stream << ",\"available\":" << json_bool(part_result.available)
           << ",\"entry_name\":";
    write_json_string(stream, part_result.entry_name);
    stream << ",\"passed\":" << json_bool(static_cast<bool>(part_result))
           << ",\"missing_required\":";
    write_json_strings(stream, part_result.validation.missing_required);
    stream << ",\"duplicate_bookmarks\":";
    write_json_strings(stream, part_result.validation.duplicate_bookmarks);
    stream << ",\"malformed_placeholders\":";
    write_json_strings(stream, part_result.validation.malformed_placeholders);
    stream << ",\"unexpected_bookmarks\":";
    write_json_bookmark_summaries(stream, part_result.validation.unexpected_bookmarks);
    stream << ",\"kind_mismatches\":";
    write_json_template_kind_mismatches(stream, part_result.validation.kind_mismatches);
    stream << ",\"occurrence_mismatches\":";
    write_json_template_occurrence_mismatches(
        stream, part_result.validation.occurrence_mismatches);
    stream << '}';
}

void print_template_schema_part_result(
    std::ostream &stream,
    const featherdoc::template_schema_part_validation_result &part_result) {
    stream << "part: " << validation_part_name(part_result.target.part) << '\n';
    if (part_result.target.part_index.has_value()) {
        stream << "part_index: " << *part_result.target.part_index << '\n';
    }
    if (part_result.target.section_index.has_value()) {
        stream << "section: " << *part_result.target.section_index << '\n';
    }
    if (part_result.target.part ==
            featherdoc::template_schema_part_kind::section_header ||
        part_result.target.part ==
            featherdoc::template_schema_part_kind::section_footer) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(part_result.target.reference_kind)
               << '\n';
    }
    stream << "available: " << yes_no(part_result.available) << '\n';
    stream << "entry_name: " << part_result.entry_name << '\n';
    stream << "passed: " << yes_no(static_cast<bool>(part_result)) << '\n';
    print_string_list(stream, "missing_required",
                      part_result.validation.missing_required);
    print_string_list(stream, "duplicate_bookmarks",
                      part_result.validation.duplicate_bookmarks);
    print_string_list(stream, "malformed_placeholders",
                      part_result.validation.malformed_placeholders);
    print_bookmark_summary_list(stream, "unexpected_bookmarks",
                                part_result.validation.unexpected_bookmarks);
    print_template_kind_mismatch_list(stream, "kind_mismatches",
                                      part_result.validation.kind_mismatches);
    print_template_occurrence_mismatch_list(
        stream, "occurrence_mismatches",
        part_result.validation.occurrence_mismatches);
}

void print_template_schema_validation_result(
    const featherdoc::template_schema_validation_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"passed\":" << json_bool(static_cast<bool>(result))
                  << ",\"part_results\":[";
        for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_part_result(std::cout,
                                                   result.part_results[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "passed: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "part_result_count: " << result.part_results.size() << '\n';
    for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
        std::cout << '\n';
        std::cout << "part_result[" << index << "]\n";
        print_template_schema_part_result(std::cout, result.part_results[index]);
    }
}

void write_json_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << "{\"" << template_slot_source_json_name(requirement.source) << "\":";
    write_json_string(stream, requirement.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, template_slot_kind_name(requirement.kind));
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << ",\"count\":" << *requirement.min_occurrences;
    } else {
        if (!requirement.required) {
            stream << ",\"required\":false";
        }
        if (requirement.min_occurrences.has_value()) {
            stream << ",\"min\":" << *requirement.min_occurrences;
        }
        if (requirement.max_occurrences.has_value()) {
            stream << ",\"max\":" << *requirement.max_occurrences;
        }
    }
    stream << '}';
}

struct previewed_template_schema_patch_summary {
    std::size_t left_slot_count = 0U;
    std::optional<path_type> output_patch_path;
    std::optional<path_type> review_json_path;
    std::optional<std::size_t> right_slot_count;
    std::optional<std::size_t> upsert_slot_count;
    std::optional<std::size_t> remove_target_count;
    std::optional<std::size_t> remove_slot_count;
    std::optional<std::size_t> rename_slot_count;
    std::optional<std::size_t> update_slot_count;
    featherdoc::template_schema_patch_summary applied{};
};

auto to_template_schema_part_selector(
    const template_schema_patch_target_selector &selector)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector target{};
    target.part = to_template_schema_part_kind(selector.part);
    target.part_index = selector.part_index;
    target.section_index = selector.section_index;
    target.reference_kind = selector.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return target;
}

auto to_template_schema_part_selector(const exported_template_schema_target &target)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector selector{};
    selector.part = to_template_schema_part_kind(target.part);
    selector.part_index = target.part_index;
    selector.section_index = target.section_index;
    selector.reference_kind = target.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return selector;
}

auto to_template_schema(const exported_template_schema_result &result)
    -> featherdoc::template_schema {
    featherdoc::template_schema schema{};
    schema.entries.reserve(result.slot_count());
    for (const auto &target : result.targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            schema.entries.push_back({selector, requirement});
        }
    }
    return schema;
}

auto to_template_schema_patch(const template_schema_patch_document &document)
    -> featherdoc::template_schema_patch {
    featherdoc::template_schema_patch patch{};

    for (const auto &target : document.upsert_targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            patch.upsert_slots.push_back({selector, requirement});
        }
    }

    patch.remove_targets.reserve(document.remove_targets.size());
    for (const auto &target : document.remove_targets) {
        patch.remove_targets.push_back(to_template_schema_part_selector(target));
    }

    patch.remove_slots.reserve(document.remove_slots.size());
    for (const auto &remove_slot : document.remove_slots) {
        featherdoc::template_schema_slot_selector slot{};
        slot.target = to_template_schema_part_selector(remove_slot.target);
        slot.source = remove_slot.source;
        slot.bookmark_name = remove_slot.bookmark_name;
        patch.remove_slots.push_back(std::move(slot));
    }

    patch.rename_slots.reserve(document.rename_slots.size());
    for (const auto &rename_slot : document.rename_slots) {
        featherdoc::template_schema_slot_rename operation{};
        operation.slot.target = to_template_schema_part_selector(rename_slot.target);
        operation.slot.source = rename_slot.source;
        operation.slot.bookmark_name = rename_slot.bookmark_name;
        operation.new_bookmark_name = rename_slot.new_bookmark_name;
        patch.rename_slots.push_back(std::move(operation));
    }

    patch.update_slots.reserve(document.update_slots.size());
    for (const auto &update_slot : document.update_slots) {
        featherdoc::template_schema_slot_patch_update operation{};
        operation.slot.target = to_template_schema_part_selector(update_slot.target);
        operation.slot.source = update_slot.source;
        operation.slot.bookmark_name = update_slot.bookmark_name;
        operation.update = update_slot.update;
        patch.update_slots.push_back(std::move(operation));
    }

    return patch;
}

void write_json_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(target.part));
    if (target.part_index.has_value()) {
        stream << ",\"index\":" << *target.part_index;
    }
    if (target.section_index.has_value()) {
        stream << ",\"section\":" << *target.section_index;
    }
    if (target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*target.reference_kind));
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *target.resolved_from_section_index;
    }
    if (target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"slots\":[";
    for (std::size_t slot_index = 0U; slot_index < target.requirements.size();
         ++slot_index) {
        if (slot_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_requirement(stream,
                                                        target.requirements[slot_index]);
    }
    stream << "]}";
}

void write_json_exported_template_schema(std::ostream &stream,
                                         const exported_template_schema_result &result) {
    stream << "{\"targets\":[";
    for (std::size_t target_index = 0U; target_index < result.targets.size();
         ++target_index) {
        if (target_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.targets[target_index]);
    }
    stream << "]}\n";
}

void write_json_template_schema_patch_selector(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(selector.part));
    if (selector.part_index.has_value()) {
        stream << ",\"index\":" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << ",\"section\":" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selector.reference_kind));
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << '}';
}

void write_json_template_schema_patch_remove_slot(
    std::ostream &stream, const template_schema_patch_remove_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_rename_slot(
    std::ostream &stream, const template_schema_patch_rename_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << ",\"" << template_slot_source_new_json_name(operation.source) << "\":";
    write_json_string(stream, operation.new_bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_update_slot(
    std::ostream &stream, const template_schema_patch_update_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    if (operation.update.kind.has_value()) {
        stream << ",\"slot_kind\":";
        write_json_string(stream, template_slot_kind_name(*operation.update.kind));
    }
    if (operation.update.required.has_value()) {
        stream << ",\"required\":" << json_bool(*operation.update.required);
    }
    if (operation.update.min_occurrences.has_value()) {
        stream << ",\"min_occurrences\":" << *operation.update.min_occurrences;
    }
    if (operation.update.max_occurrences.has_value()) {
        stream << ",\"max_occurrences\":" << *operation.update.max_occurrences;
    }
    if (operation.update.clear_min_occurrences) {
        stream << ",\"clear_min_occurrences\":true";
    }
    if (operation.update.clear_max_occurrences) {
        stream << ",\"clear_max_occurrences\":true";
    }
    stream << '}';
}

void write_json_template_schema_patch_document(
    std::ostream &stream, const template_schema_patch_document &patch) {
    bool wrote_member = false;
    stream << '{';

    const auto write_separator = [&]() {
        if (wrote_member) {
            stream << ',';
        }
        wrote_member = true;
    };

    if (!patch.remove_targets.empty()) {
        write_separator();
        stream << "\"remove_targets\":[";
        for (std::size_t index = 0U; index < patch.remove_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_selector(stream,
                                                      patch.remove_targets[index]);
        }
        stream << ']';
    }

    if (!patch.remove_slots.empty()) {
        write_separator();
        stream << "\"remove_slots\":[";
        for (std::size_t index = 0U; index < patch.remove_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_remove_slot(stream,
                                                         patch.remove_slots[index]);
        }
        stream << ']';
    }

    if (!patch.rename_slots.empty()) {
        write_separator();
        stream << "\"rename_slots\":[";
        for (std::size_t index = 0U; index < patch.rename_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_rename_slot(stream,
                                                         patch.rename_slots[index]);
        }
        stream << ']';
    }

    if (!patch.update_slots.empty()) {
        write_separator();
        stream << "\"update_slots\":[";
        for (std::size_t index = 0U; index < patch.update_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_update_slot(stream,
                                                         patch.update_slots[index]);
        }
        stream << ']';
    }

    if (!patch.upsert_targets.empty()) {
        write_separator();
        stream << "\"upsert_targets\":[";
        for (std::size_t index = 0U; index < patch.upsert_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_exported_template_schema_target(stream,
                                                       patch.upsert_targets[index]);
        }
        stream << ']';
    }

    stream << "}\n";
}

void write_json_template_schema_lint_issue(std::ostream &stream,
                                           const template_schema_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, template_schema_lint_issue_name(issue.kind));
    stream << ",\"target_index\":" << issue.target_index << ",\"target\":";
    write_json_template_schema_patch_selector(stream, issue.target);
    if (issue.previous_target_index.has_value()) {
        stream << ",\"previous_target_index\":" << *issue.previous_target_index;
    }
    if (issue.slot_index.has_value()) {
        stream << ",\"slot_index\":" << *issue.slot_index;
    }
    if (issue.previous_slot_index.has_value()) {
        stream << ",\"previous_slot_index\":" << *issue.previous_slot_index;
    }
    if (!issue.bookmark_name.empty()) {
        stream << ",\"bookmark\":";
        write_json_string(stream, issue.bookmark_name);
    }
    if (!issue.previous_bookmark_name.empty()) {
        stream << ",\"previous_bookmark\":";
        write_json_string(stream, issue.previous_bookmark_name);
    }
    if (!issue.entry_name.empty()) {
        stream << ",\"entry_name\":";
        write_json_string(stream, issue.entry_name);
    }
    stream << '}';
}

void write_json_exported_template_schema_skipped_bookmark(
    std::ostream &stream,
    const exported_template_schema_skipped_bookmark &bookmark) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(bookmark.part));
    if (bookmark.part_index.has_value()) {
        stream << ",\"part_index\":" << *bookmark.part_index;
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, bookmark.entry_name);
    if (bookmark.section_index.has_value()) {
        stream << ",\"section\":" << *bookmark.section_index;
    }
    if (bookmark.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*bookmark.reference_kind));
    }
    if (bookmark.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *bookmark.resolved_from_section_index;
    }
    if (bookmark.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.bookmark.kind));
    stream << ",\"occurrence_count\":" << bookmark.bookmark.occurrence_count
           << ",\"reason\":";
    write_json_string(stream, bookmark.reason);
    stream << '}';
}

void print_exported_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"export-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"skipped_count\":" << result.skipped_bookmarks.size()
                  << ",\"skipped_bookmarks\":[";
        for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_skipped_bookmark(
                std::cout, result.skipped_bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "skipped_count: " << result.skipped_bookmarks.size() << '\n';
    if (result.skipped_bookmarks.empty()) {
        std::cout << "skipped_bookmarks: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
        const auto &bookmark = result.skipped_bookmarks[index];
        std::cout << "skipped_bookmarks[" << index << "]: part="
                  << validation_part_name(bookmark.part);
        if (bookmark.part_index.has_value()) {
            std::cout << " part_index=" << *bookmark.part_index;
        }
        if (bookmark.section_index.has_value()) {
            std::cout << " section=" << *bookmark.section_index;
        }
        if (bookmark.reference_kind.has_value()) {
            std::cout << " kind="
                      << featherdoc::to_xml_reference_type(*bookmark.reference_kind);
        }
        if (bookmark.resolved_from_section_index.has_value()) {
            std::cout << " resolved_from_section="
                      << *bookmark.resolved_from_section_index;
        }
        if (bookmark.linked_to_previous) {
            std::cout << " linked_to_previous=yes";
        }
        std::cout << " entry=" << bookmark.entry_name
                  << " bookmark=" << bookmark.bookmark.bookmark_name
                  << " kind=" << bookmark_kind_name(bookmark.bookmark.kind)
                  << " occurrences=" << bookmark.bookmark.occurrence_count
                  << " reason=" << bookmark.reason << '\n';
    }
}

void print_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << template_slot_source_text_name(requirement.source) << '='
           << requirement.bookmark_name << " kind="
           << template_slot_kind_name(requirement.kind);
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << " count=" << *requirement.min_occurrences;
        return;
    }

    stream << " required=" << yes_no(requirement.required);
    if (requirement.min_occurrences.has_value()) {
        stream << " min=" << *requirement.min_occurrences;
    }
    if (requirement.max_occurrences.has_value()) {
        stream << " max=" << *requirement.max_occurrences;
    }
}

void print_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "part: " << validation_part_name(target.part) << '\n';
    if (target.part_index.has_value()) {
        stream << "part_index: " << *target.part_index << '\n';
    }
    if (target.section_index.has_value()) {
        stream << "section: " << *target.section_index << '\n';
    }
    if (target.reference_kind.has_value()) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(*target.reference_kind) << '\n';
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << "resolved_from_section: " << *target.resolved_from_section_index
               << '\n';
    }
    stream << "linked_to_previous: " << yes_no(target.linked_to_previous) << '\n';
    stream << "slot_count: " << target.requirements.size() << '\n';
    for (std::size_t index = 0U; index < target.requirements.size(); ++index) {
        stream << "slot[" << index << "]: ";
        print_exported_template_schema_requirement(stream, target.requirements[index]);
        stream << '\n';
    }
}

void print_normalized_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"normalize-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count() << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n';
}

void print_template_schema_patch_selector_summary(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "part=" << validation_part_name(selector.part);
    if (selector.part_index.has_value()) {
        stream << " index=" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << " section=" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << " kind="
               << featherdoc::to_xml_reference_type(*selector.reference_kind);
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << " resolved_from_section="
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << " linked_to_previous=yes";
    }
}

void print_linted_template_schema_result(const template_schema_lint_result &result,
                                         bool json_output) {
    const auto duplicate_target_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_target_identity);
    const auto duplicate_slot_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_slot_name);
    const auto target_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::target_order);
    const auto slot_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::slot_order);
    const auto entry_name_issue_count =
        result.issue_count(template_schema_lint_issue_kind::entry_name_present);

    if (json_output) {
        std::cout << "{\"command\":\"lint-template-schema\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"target_count\":" << result.target_count
                  << ",\"slot_count\":" << result.slot_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"duplicate_target_count\":" << duplicate_target_count
                  << ",\"duplicate_slot_count\":" << duplicate_slot_count
                  << ",\"target_order_issue_count\":" << target_order_issue_count
                  << ",\"slot_order_issue_count\":" << slot_order_issue_count
                  << ",\"entry_name_issue_count\":" << entry_name_issue_count
                  << ",\"issues\":[";
        for (std::size_t index = 0U; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_lint_issue(std::cout, result.issues[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "target_count: " << result.target_count << '\n'
              << "slot_count: " << result.slot_count << '\n'
              << "issue_count: " << result.issues.size() << '\n'
              << "duplicate_target_count: " << duplicate_target_count << '\n'
              << "duplicate_slot_count: " << duplicate_slot_count << '\n'
              << "target_order_issue_count: " << target_order_issue_count << '\n'
              << "slot_order_issue_count: " << slot_order_issue_count << '\n'
              << "entry_name_issue_count: " << entry_name_issue_count << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << template_schema_lint_issue_name(issue.kind)
                  << " target_index=" << issue.target_index << ' ';
        print_template_schema_patch_selector_summary(std::cout, issue.target);
        if (issue.previous_target_index.has_value()) {
            std::cout << " previous_target_index=" << *issue.previous_target_index;
        }
        if (issue.slot_index.has_value()) {
            std::cout << " slot_index=" << *issue.slot_index;
        }
        if (issue.previous_slot_index.has_value()) {
            std::cout << " previous_slot_index=" << *issue.previous_slot_index;
        }
        if (!issue.bookmark_name.empty()) {
            std::cout << " bookmark=" << issue.bookmark_name;
        }
        if (!issue.previous_bookmark_name.empty()) {
            std::cout << " previous_bookmark=" << issue.previous_bookmark_name;
        }
        if (!issue.entry_name.empty()) {
            std::cout << " entry_name=" << issue.entry_name;
        }
        std::cout << '\n';
    }
}

void print_repaired_template_schema_summary(
    const exported_template_schema_result &result,
    const repaired_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"repair-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_target_count\":" << summary.input_target_count
                  << ",\"input_slot_count\":" << summary.input_slot_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"merged_duplicate_target_count\":"
                  << summary.merged_duplicate_target_count
                  << ",\"deduplicated_target_count\":"
                  << summary.deduplicated_target_count
                  << ",\"deduplicated_slot_count\":"
                  << summary.deduplicated_slot_count
                  << ",\"stripped_entry_name_count\":"
                  << summary.stripped_entry_name_count
                  << ",\"replaced_slot_count\":"
                  << summary.replaced_slot_count
                  << ",\"changed\":" << json_bool(summary.changed) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_target_count: " << summary.input_target_count << '\n'
              << "input_slot_count: " << summary.input_slot_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "merged_duplicate_target_count: "
              << summary.merged_duplicate_target_count << '\n'
              << "deduplicated_target_count: "
              << summary.deduplicated_target_count << '\n'
              << "deduplicated_slot_count: "
              << summary.deduplicated_slot_count << '\n'
              << "stripped_entry_name_count: "
              << summary.stripped_entry_name_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "changed: " << yes_no(summary.changed) << '\n';
}

void print_merged_template_schema_summary(
    const exported_template_schema_result &result,
    const merged_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"merge-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_count\":" << summary.input_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_count: " << summary.input_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n';
}

void print_patched_template_schema_summary(
    const exported_template_schema_result &result,
    const patched_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"patch-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"upsert_target_count\":" << summary.upsert_target_count
                  << ",\"remove_target_count\":" << summary.remove_target_count
                  << ",\"remove_slot_count\":" << summary.remove_slot_count
                  << ",\"rename_slot_count\":" << summary.rename_slot_count
                  << ",\"update_slot_count\":" << summary.update_slot_count
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << ",\"applied_remove_target_count\":"
                  << summary.applied_remove_target_count
                  << ",\"applied_remove_slot_count\":"
                  << summary.applied_remove_slot_count
                  << ",\"applied_rename_slot_count\":"
                  << summary.applied_rename_slot_count
                  << ",\"applied_update_slot_count\":"
                  << summary.applied_update_slot_count
                  << ",\"pruned_empty_target_count\":"
                  << summary.pruned_empty_target_count << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "upsert_target_count: " << summary.upsert_target_count << '\n'
              << "remove_target_count: " << summary.remove_target_count << '\n'
              << "remove_slot_count: " << summary.remove_slot_count << '\n'
              << "rename_slot_count: " << summary.rename_slot_count << '\n'
              << "update_slot_count: " << summary.update_slot_count << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "applied_remove_target_count: "
              << summary.applied_remove_target_count << '\n'
              << "applied_remove_slot_count: "
              << summary.applied_remove_slot_count << '\n'
              << "applied_rename_slot_count: "
              << summary.applied_rename_slot_count << '\n'
              << "applied_update_slot_count: "
              << summary.applied_update_slot_count << '\n'
              << "pruned_empty_target_count: "
              << summary.pruned_empty_target_count << '\n';
}

void print_previewed_template_schema_patch_summary(
    const previewed_template_schema_patch_summary &summary, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"preview-template-schema-patch\",\"ok\":true";
        if (summary.output_patch_path.has_value()) {
            std::cout << ",\"output_patch_path\":";
            write_json_string(std::cout, summary.output_patch_path->string());
        }
        if (summary.review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, summary.review_json_path->string());
        }
        std::cout << ",\"left_slot_count\":" << summary.left_slot_count;
        if (summary.right_slot_count.has_value()) {
            std::cout << ",\"right_slot_count\":" << *summary.right_slot_count;
        }
        if (summary.upsert_slot_count.has_value()) {
            std::cout << ",\"upsert_slot_count\":" << *summary.upsert_slot_count;
        }
        if (summary.remove_target_count.has_value()) {
            std::cout << ",\"remove_target_count\":"
                      << *summary.remove_target_count;
        }
        if (summary.remove_slot_count.has_value()) {
            std::cout << ",\"remove_slot_count\":" << *summary.remove_slot_count;
        }
        if (summary.rename_slot_count.has_value()) {
            std::cout << ",\"rename_slot_count\":" << *summary.rename_slot_count;
        }
        if (summary.update_slot_count.has_value()) {
            std::cout << ",\"update_slot_count\":" << *summary.update_slot_count;
        }
        std::cout << ",\"removed_targets\":" << summary.applied.removed_targets
                  << ",\"removed_slots\":" << summary.applied.removed_slots
                  << ",\"renamed_slots\":" << summary.applied.renamed_slots
                  << ",\"inserted_slots\":" << summary.applied.inserted_slots
                  << ",\"replaced_slots\":" << summary.applied.replaced_slots
                  << ",\"changed\":" << json_bool(summary.applied.changed())
                  << "}\n";
        return;
    }

    if (summary.output_patch_path.has_value()) {
        std::cout << "output_patch_path: " << summary.output_patch_path->string()
                  << '\n';
    }
    if (summary.review_json_path.has_value()) {
        std::cout << "review_json_path: " << summary.review_json_path->string()
                  << '\n';
    }
    std::cout << "left_slot_count: " << summary.left_slot_count << '\n';
    if (summary.right_slot_count.has_value()) {
        std::cout << "right_slot_count: " << *summary.right_slot_count << '\n';
    }
    if (summary.upsert_slot_count.has_value()) {
        std::cout << "upsert_slot_count: " << *summary.upsert_slot_count << '\n';
    }
    if (summary.remove_target_count.has_value()) {
        std::cout << "remove_target_count: " << *summary.remove_target_count << '\n';
    }
    if (summary.remove_slot_count.has_value()) {
        std::cout << "remove_slot_count: " << *summary.remove_slot_count << '\n';
    }
    if (summary.rename_slot_count.has_value()) {
        std::cout << "rename_slot_count: " << *summary.rename_slot_count << '\n';
    }
    if (summary.update_slot_count.has_value()) {
        std::cout << "update_slot_count: " << *summary.update_slot_count << '\n';
    }
    std::cout << "removed_targets: " << summary.applied.removed_targets << '\n'
              << "removed_slots: " << summary.applied.removed_slots << '\n'
              << "renamed_slots: " << summary.applied.renamed_slots << '\n'
              << "inserted_slots: " << summary.applied.inserted_slots << '\n'
              << "replaced_slots: " << summary.applied.replaced_slots << '\n'
              << "changed: " << yes_no(summary.applied.changed()) << '\n';
}

void print_built_template_schema_patch_summary(
    const built_template_schema_patch_summary &summary,
    const std::optional<path_type> &output_path,
    const std::optional<path_type> &review_json_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"build-template-schema-patch\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        if (review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, review_json_path->string());
        }
        std::cout << ",\"added_target_count\":" << summary.added_target_count
                  << ",\"removed_target_count\":" << summary.removed_target_count
                  << ",\"changed_target_count\":" << summary.changed_target_count
                  << ",\"generated_remove_target_count\":"
                  << summary.generated_remove_target_count
                  << ",\"generated_remove_slot_count\":"
                  << summary.generated_remove_slot_count
                  << ",\"generated_rename_slot_count\":"
                  << summary.generated_rename_slot_count
                  << ",\"generated_update_slot_count\":"
                  << summary.generated_update_slot_count
                  << ",\"generated_upsert_target_count\":"
                  << summary.generated_upsert_target_count
                  << ",\"empty_patch\":" << json_bool(summary.empty_patch()) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    if (review_json_path.has_value()) {
        std::cout << "review_json_path: " << review_json_path->string() << '\n';
    }
    std::cout << "added_target_count: " << summary.added_target_count << '\n'
              << "removed_target_count: " << summary.removed_target_count << '\n'
              << "changed_target_count: " << summary.changed_target_count << '\n'
              << "generated_remove_target_count: "
              << summary.generated_remove_target_count << '\n'
              << "generated_remove_slot_count: "
              << summary.generated_remove_slot_count << '\n'
              << "generated_rename_slot_count: "
              << summary.generated_rename_slot_count << '\n'
              << "generated_update_slot_count: "
              << summary.generated_update_slot_count << '\n'
              << "generated_upsert_target_count: "
              << summary.generated_upsert_target_count << '\n'
              << "empty_patch: " << yes_no(summary.empty_patch()) << '\n';
}

void write_json_template_schema_diff_result(
    std::ostream &stream, const template_schema_diff_result &result) {
    stream << "{\"equal\":" << json_bool(result.equal())
           << ",\"added_target_count\":" << result.added_targets.size()
           << ",\"removed_target_count\":" << result.removed_targets.size()
           << ",\"changed_target_count\":" << result.changed_targets.size()
           << ",\"added_targets\":[";
    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.added_targets[index]);
    }
    stream << "],\"removed_targets\":[";
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.removed_targets[index]);
    }
    stream << "],\"changed_targets\":[";
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"left\":";
        write_json_exported_template_schema_target(stream, result.changed_targets[index].left);
        stream << ",\"right\":";
        write_json_exported_template_schema_target(stream, result.changed_targets[index].right);
        stream << '}';
    }
    stream << "]}\n";
}

void print_template_schema_diff_result(const template_schema_diff_result &result,
                                       bool json_output) {
    if (json_output) {
        write_json_template_schema_diff_result(std::cout, result);
        return;
    }

    std::cout << "equal: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].left\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].right\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

void print_checked_template_schema_result(
    const path_type &schema_path, const template_schema_diff_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"check-template-schema\",\"matches\":"
                  << json_bool(result.equal()) << ",\"schema_file\":";
        write_json_string(std::cout, schema_path.string());
        if (output_path.has_value()) {
            std::cout << ",\"generated_output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"added_target_count\":" << result.added_targets.size()
                  << ",\"removed_target_count\":" << result.removed_targets.size()
                  << ",\"changed_target_count\":" << result.changed_targets.size()
                  << ",\"added_targets\":[";
        for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(std::cout,
                                                       result.added_targets[index]);
        }
        std::cout << "],\"removed_targets\":[";
        for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(
                std::cout, result.removed_targets[index]);
        }
        std::cout << "],\"changed_targets\":[";
        for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            std::cout << "{\"left\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].left);
            std::cout << ",\"right\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].right);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "schema_file: " << schema_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "generated_output_path: " << output_path->string() << '\n';
    }
    std::cout << "matches: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].baseline\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].generated\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

auto append_exported_template_schema_target(
    featherdoc::Document &doc, validation_part_family part_family,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    std::optional<featherdoc::section_reference_kind> reference_kind,
    std::optional<std::size_t> resolved_from_section_index,
    bool linked_to_previous,
    featherdoc::TemplatePart part,
    exported_template_schema_result &result, std::string_view command,
    bool json_output) -> bool {
    if (!part) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "target template part is not available";
        return report_operation_failure(command, "inspect",
                                        "failed to inspect template part",
                                        error_info, json_output);
    }

    const auto entry_name = std::string(part.entry_name());
    const auto bookmarks = part.list_bookmarks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    exported_template_schema_target target{};
    target.part = part_family;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    target.resolved_from_section_index = resolved_from_section_index;
    target.linked_to_previous = linked_to_previous;
    target.entry_name = entry_name;

    for (const auto &bookmark : bookmarks) {
        const auto slot_kind = bookmark_kind_to_template_slot_kind(bookmark.kind);
        if (!slot_kind.has_value()) {
            result.skipped_bookmarks.push_back(
                {part_family,
                 part_index,
                 section_index,
                 reference_kind,
                 resolved_from_section_index,
                 linked_to_previous,
                 entry_name,
                 bookmark,
                 "bookmark kind cannot be represented as a template slot"});
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = bookmark.bookmark_name;
        requirement.kind = *slot_kind;
        if (bookmark.occurrence_count > 1U) {
            requirement.min_occurrences = bookmark.occurrence_count;
            requirement.max_occurrences = bookmark.occurrence_count;
        }
        target.requirements.push_back(std::move(requirement));
    }

    const auto content_controls = part.list_content_controls();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    std::vector<featherdoc::template_slot_requirement> content_control_requirements;
    std::unordered_map<std::string, std::size_t> content_control_counts;
    for (const auto &content_control : content_controls) {
        auto source = featherdoc::template_slot_source_kind::content_control_tag;
        std::optional<std::string> selector_value = content_control.tag;
        if (!selector_value.has_value()) {
            source = featherdoc::template_slot_source_kind::content_control_alias;
            selector_value = content_control.alias;
        }
        if (!selector_value.has_value()) {
            continue;
        }

        const auto key = std::string{template_slot_source_json_name(source)} +
                         ":" + *selector_value;
        auto [count_it, inserted] = content_control_counts.emplace(key, 1U);
        if (!inserted) {
            ++count_it->second;
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = *selector_value;
        requirement.kind = content_control_kind_to_template_slot_kind(content_control.kind);
        requirement.source = source;
        content_control_requirements.push_back(std::move(requirement));
    }
    for (auto &requirement : content_control_requirements) {
        const auto key = std::string{template_slot_source_json_name(requirement.source)} +
                         ":" + requirement.bookmark_name;
        if (const auto count_it = content_control_counts.find(key);
            count_it != content_control_counts.end() && count_it->second > 1U) {
            requirement.min_occurrences = count_it->second;
            requirement.max_occurrences = count_it->second;
        }
        target.requirements.push_back(std::move(requirement));
    }

    if (!target.requirements.empty()) {
        result.targets.push_back(std::move(target));
    }

    return true;
}

auto write_exported_template_schema_file(
    const path_type &output_path, const exported_template_schema_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open schema output path: " + output_path.string();
        return false;
    }

    write_json_exported_template_schema(stream, result);
    if (!stream.good()) {
        error_message = "failed to write schema output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_file(const path_type &output_path,
                                      const template_schema_patch_document &patch,
                                      std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open patch output path: " + output_path.string();
        return false;
    }

    write_json_template_schema_patch_document(stream, patch);
    if (!stream.good()) {
        error_message = "failed to write patch output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_review_file(
    const path_type &output_path,
    const featherdoc::template_schema_patch_review_summary &summary,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open schema patch review output path: " + output_path.string();
        return false;
    }

    featherdoc::write_template_schema_patch_review_json(stream, summary);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write schema patch review output path: " + output_path.string();
        return false;
    }

    return true;
}


auto write_style_refactor_plan_file(
    const path_type &output_path, const featherdoc::style_refactor_plan &plan,
    std::string &error_message,
    std::string_view command_name = "plan-style-refactor") -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open style refactor plan output path: " + output_path.string();
        return false;
    }

    stream << "{\"command\":";
    write_json_string(stream, command_name);
    stream << ',';
    write_json_style_refactor_plan_fields(stream, plan);
    stream << "}\n";
    if (!stream.good()) {
        error_message =
            "failed to write style refactor plan output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_style_refactor_rollback_plan_file(
    const path_type &output_path,
    const featherdoc::style_refactor_apply_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open style refactor rollback output path: " +
                        output_path.string();
        return false;
    }

    stream << "{\"command\":\"apply-style-refactor\""
           << ",\"requested_count\":" << result.requested_count
           << ",\"applied_count\":" << result.applied_count
           << ",\"rollback_count\":" << result.rollback_entries.size()
           << ",\"rollback_operations\":";
    write_json_style_refactor_rollback_entries(stream, result.rollback_entries);
    stream << "}\n";
    if (!stream.good()) {
        error_message = "failed to write style refactor rollback output path: " +
                        output_path.string();
        return false;
    }

    return true;
}

auto get_review_mutation_plan_range(
    const review_mutation_plan_operation &operation,
    std::size_t &start_paragraph_index, std::size_t &start_text_offset,
    std::size_t &end_paragraph_index, std::size_t &end_text_offset,
    std::string &error_message) -> bool {
    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (operation.kind ==
                review_mutation_plan_operation_kind::
                    append_paragraph_text_comment &&
            operation.text_length >
                std::numeric_limits<std::size_t>::max() -
                    operation.text_offset) {
            error_message = "text range is out of range";
            return false;
        }
        start_paragraph_index = operation.paragraph_index;
        start_text_offset = operation.text_offset;
        end_paragraph_index = operation.paragraph_index;
        end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        append_paragraph_text_comment
                ? operation.text_offset + operation.text_length
                : operation.text_offset;
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (operation.text_length >
            std::numeric_limits<std::size_t>::max() - operation.text_offset) {
            error_message = "text range is out of range";
            return false;
        }
        start_paragraph_index = operation.paragraph_index;
        start_text_offset = operation.text_offset;
        end_paragraph_index = operation.paragraph_index;
        end_text_offset = operation.text_offset + operation.text_length;
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        start_paragraph_index = operation.start_paragraph_index;
        start_text_offset = operation.start_text_offset;
        end_paragraph_index =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? operation.end_paragraph_index
                : operation.start_paragraph_index;
        end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? operation.end_text_offset
                : operation.start_text_offset;
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        start_paragraph_index = operation.start_paragraph_index;
        start_text_offset = operation.start_text_offset;
        end_paragraph_index = operation.end_paragraph_index;
        end_text_offset = operation.end_text_offset;
        return true;
    }

    return false;
}

auto is_review_mutation_plan_insert_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind ==
               review_mutation_plan_operation_kind::insert_paragraph_text_revision ||
           kind == review_mutation_plan_operation_kind::insert_text_range_revision;
}

auto is_review_mutation_plan_text_range_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind != review_mutation_plan_operation_kind::append_comment_reply &&
           kind != review_mutation_plan_operation_kind::replace_comment &&
           kind != review_mutation_plan_operation_kind::remove_comment &&
           kind != review_mutation_plan_operation_kind::set_comment_resolved &&
           kind != review_mutation_plan_operation_kind::set_comment_metadata;
}

auto is_review_mutation_plan_comment_operation(
    review_mutation_plan_operation_kind kind) -> bool {
    return kind ==
               review_mutation_plan_operation_kind::append_paragraph_text_comment ||
           kind == review_mutation_plan_operation_kind::append_text_range_comment;
}

auto preview_review_mutation_plan_insertion_point(
    featherdoc::Document &doc, std::size_t paragraph_index,
    std::size_t text_offset, review_mutation_plan_preview_result &result)
    -> bool {
    const auto paragraph = doc.inspect_paragraph(paragraph_index);
    if (!paragraph.has_value()) {
        const auto &error_info = doc.last_error();
        if (!error_info.detail.empty()) {
            result.message = error_info.detail;
        } else if (error_info.code) {
            result.message = error_info.code.message();
        } else {
            result.message = "paragraph index is out of range";
        }
        return false;
    }
    if (text_offset > paragraph->text.size()) {
        result.message = "text offset is out of range";
        return false;
    }

    featherdoc::text_range_preview preview;
    preview.start_paragraph_index = paragraph_index;
    preview.start_text_offset = text_offset;
    preview.end_paragraph_index = paragraph_index;
    preview.end_text_offset = text_offset;
    preview.text_length = 0U;
    preview.plain_text_runs_supported = true;
    result.preview = std::move(preview);
    result.ok = true;
    result.message = "ok";
    return true;
}

auto preview_review_mutation_plan_comment_operation(
    featherdoc::Document &doc,
    const review_mutation_plan_operation &operation,
    review_mutation_plan_preview_result &result) -> bool {
    result.comment_index = operation.comment_index;
    result.expected_text = operation.expected_text;
    result.expected_comment_text = operation.expected_comment_text;
    result.expected_resolved = operation.expected_resolved;
    result.expected_parent_index = operation.expected_parent_index;

    const auto comments = doc.list_comments();
    if (operation.comment_index >= comments.size()) {
        result.ok = false;
        result.message = "comment index is out of range";
        return false;
    }

    const auto &comment = comments[operation.comment_index];
    result.actual_comment_text = comment.text;
    result.actual_resolved = comment.resolved;
    if (comment.anchor_text.has_value()) {
        result.actual_text = *comment.anchor_text;
    }
    if (comment.parent_index.has_value()) {
        result.actual_parent_index = *comment.parent_index;
    }

    if (operation.expected_text.has_value() &&
        (!comment.anchor_text.has_value() ||
         *comment.anchor_text != *operation.expected_text)) {
        result.ok = false;
        result.message = "expected text did not match comment anchor text";
        return false;
    }
    if (operation.expected_comment_text.has_value() &&
        comment.text != *operation.expected_comment_text) {
        result.ok = false;
        result.message = "expected comment text did not match comment body";
        return false;
    }
    if (operation.expected_resolved.has_value() &&
        comment.resolved != *operation.expected_resolved) {
        result.ok = false;
        result.message = "expected resolved state did not match comment state";
        return false;
    }
    if (operation.expected_parent_index.has_value() &&
        (!comment.parent_index.has_value() ||
         *comment.parent_index != *operation.expected_parent_index)) {
        result.ok = false;
        result.message =
            "expected parent index did not match comment parent";
        return false;
    }

    result.ok = true;
    result.message = "ok";
    return true;
}

struct review_mutation_plan_range {
    std::size_t operation_index = 0U;
    std::size_t start_paragraph_index = 0U;
    std::size_t start_text_offset = 0U;
    std::size_t end_paragraph_index = 0U;
    std::size_t end_text_offset = 0U;
};

auto compare_review_mutation_plan_position(std::size_t left_paragraph_index,
                                           std::size_t left_text_offset,
                                           std::size_t right_paragraph_index,
                                           std::size_t right_text_offset)
    -> int {
    if (left_paragraph_index < right_paragraph_index) {
        return -1;
    }
    if (left_paragraph_index > right_paragraph_index) {
        return 1;
    }
    if (left_text_offset < right_text_offset) {
        return -1;
    }
    if (left_text_offset > right_text_offset) {
        return 1;
    }
    return 0;
}

auto compare_review_mutation_plan_range_start(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> int {
    return compare_review_mutation_plan_position(
        left.start_paragraph_index, left.start_text_offset,
        right.start_paragraph_index, right.start_text_offset);
}

auto compare_review_mutation_plan_range_end(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> int {
    return compare_review_mutation_plan_position(
        left.end_paragraph_index, left.end_text_offset,
        right.end_paragraph_index, right.end_text_offset);
}

auto is_review_mutation_plan_range_empty(
    const review_mutation_plan_range &range) -> bool {
    return compare_review_mutation_plan_position(
               range.start_paragraph_index, range.start_text_offset,
               range.end_paragraph_index, range.end_text_offset) == 0;
}

auto review_mutation_plan_ranges_overlap(
    const review_mutation_plan_range &left,
    const review_mutation_plan_range &right) -> bool {
    if (is_review_mutation_plan_range_empty(left) ||
        is_review_mutation_plan_range_empty(right)) {
        return false;
    }
    return compare_review_mutation_plan_position(
               left.start_paragraph_index, left.start_text_offset,
               right.end_paragraph_index, right.end_text_offset) < 0 &&
           compare_review_mutation_plan_position(
               right.start_paragraph_index, right.start_text_offset,
               left.end_paragraph_index, left.end_text_offset) < 0;
}

auto collect_review_mutation_plan_ranges(
    const std::vector<review_mutation_plan_operation> &operations,
    std::vector<review_mutation_plan_range> &ranges,
    std::string &error_message) -> bool {
    ranges.clear();
    ranges.reserve(operations.size());

    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (!is_review_mutation_plan_text_range_operation(
                operations[index].kind)) {
            continue;
        }
        review_mutation_plan_range range;
        range.operation_index = index;
        if (!get_review_mutation_plan_range(
                operations[index], range.start_paragraph_index,
                range.start_text_offset, range.end_paragraph_index,
                range.end_text_offset, error_message)) {
            return false;
        }
        ranges.push_back(range);
    }

    return true;
}

auto find_review_mutation_plan_overlap(
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    std::vector<review_mutation_plan_range> ranges;
    if (!collect_review_mutation_plan_ranges(operations, ranges,
                                             error_message)) {
        return true;
    }

    for (std::size_t left_index = 0U; left_index < ranges.size();
         ++left_index) {
        for (std::size_t right_index = left_index + 1U;
             right_index < ranges.size(); ++right_index) {
            const auto &previous = ranges[left_index];
            const auto &current = ranges[right_index];
            const auto previous_is_comment =
                is_review_mutation_plan_comment_operation(
                    operations[previous.operation_index].kind);
            const auto current_is_comment =
                is_review_mutation_plan_comment_operation(
                    operations[current.operation_index].kind);
            if ((previous_is_comment && current_is_comment) ||
                !review_mutation_plan_ranges_overlap(previous, current)) {
                continue;
            }
            error_message = "review mutation plan operation ranges overlap: "
                            "operation " +
                            std::to_string(previous.operation_index) +
                            " and operation " +
                            std::to_string(current.operation_index);
            return true;
        }
    }

    return false;
}

auto build_review_mutation_plan_apply_order(
    const std::vector<review_mutation_plan_operation> &operations,
    std::vector<std::size_t> &operation_indices, std::string &error_message)
    -> bool {
    std::vector<review_mutation_plan_range> ranges;
    if (!collect_review_mutation_plan_ranges(operations, ranges,
                                             error_message)) {
        return false;
    }

    std::stable_sort(ranges.begin(), ranges.end(),
                     [&operations](const auto &left, const auto &right) {
                         const auto left_is_comment =
                             is_review_mutation_plan_comment_operation(
                                 operations[left.operation_index].kind);
                         const auto right_is_comment =
                             is_review_mutation_plan_comment_operation(
                                 operations[right.operation_index].kind);
                         const auto start_order =
                             compare_review_mutation_plan_range_start(left,
                                                                      right);
                         if (left_is_comment && right_is_comment) {
                             if (start_order != 0) {
                                 return start_order < 0;
                             }
                             return compare_review_mutation_plan_range_end(
                                        left, right) > 0;
                         }
                         if (start_order != 0) {
                             return start_order > 0;
                         }
                         return compare_review_mutation_plan_range_end(left,
                                                                       right) > 0;
                     });

    operation_indices.clear();
    operation_indices.reserve(operations.size());
    for (const auto &range : ranges) {
        operation_indices.push_back(range.operation_index);
    }
    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (!is_review_mutation_plan_text_range_operation(
                operations[index].kind)) {
            operation_indices.push_back(index);
        }
    }
    return true;
}

auto preview_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations)
    -> std::vector<review_mutation_plan_preview_result> {
    std::vector<review_mutation_plan_preview_result> results;
    results.reserve(operations.size());

    for (std::size_t index = 0U; index < operations.size(); ++index) {
        const auto &operation = operations[index];
        review_mutation_plan_preview_result result;
        result.index = index;
        result.kind = operation.kind;
        result.expected_text = operation.expected_text;

        if (operation.kind ==
                review_mutation_plan_operation_kind::append_comment_reply ||
            operation.kind ==
                review_mutation_plan_operation_kind::replace_comment ||
            operation.kind ==
                review_mutation_plan_operation_kind::remove_comment ||
            operation.kind ==
                review_mutation_plan_operation_kind::set_comment_resolved ||
            operation.kind ==
                review_mutation_plan_operation_kind::set_comment_metadata) {
            preview_review_mutation_plan_comment_operation(doc, operation,
                                                           result);
            results.push_back(std::move(result));
            continue;
        }

        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!get_review_mutation_plan_range(
                operation, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset, result.message)) {
            result.ok = false;
            results.push_back(std::move(result));
            continue;
        }

        if (is_review_mutation_plan_insert_operation(operation.kind)) {
            preview_review_mutation_plan_insertion_point(
                doc, start_paragraph_index, start_text_offset, result);
            results.push_back(std::move(result));
            continue;
        }

        auto preview = doc.preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value()) {
            result.ok = false;
            const auto &error_info = doc.last_error();
            result.message = !error_info.detail.empty()
                                 ? error_info.detail
                                 : error_info.code.message();
            results.push_back(std::move(result));
            continue;
        }

        result.preview = *preview;
        if (operation.expected_text.has_value() &&
            preview->text != *operation.expected_text) {
            result.ok = false;
            result.message = "expected text did not match selected text";
            results.push_back(std::move(result));
            continue;
        }

        result.ok = true;
        result.message = "ok";
        results.push_back(std::move(result));
    }

    return results;
}

auto apply_review_mutation_plan_operation(
    featherdoc::Document &doc,
    const review_mutation_plan_operation &operation) -> bool {
    const auto author =
        operation.author.has_value() ? std::string_view(*operation.author)
                                     : std::string_view{};
    const auto date =
        operation.date.has_value() ? std::string_view(*operation.date)
                                   : std::string_view{};
    const auto initials =
        operation.initials.has_value() ? std::string_view(*operation.initials)
                                       : std::string_view{};

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
        return doc.append_comment_reply(operation.comment_index,
                                        operation.text, author, initials,
                                        date) != 0U;
    case review_mutation_plan_operation_kind::replace_comment:
        return doc.replace_comment(operation.comment_index, operation.text);
    case review_mutation_plan_operation_kind::remove_comment:
        return doc.remove_comment(operation.comment_index);
    case review_mutation_plan_operation_kind::set_comment_resolved:
        return doc.set_comment_resolved(operation.comment_index,
                                        operation.resolved);
    case review_mutation_plan_operation_kind::set_comment_metadata: {
        featherdoc::comment_metadata_update metadata;
        metadata.author = operation.author;
        metadata.initials = operation.initials;
        metadata.date = operation.date;
        metadata.clear_author = operation.clear_author;
        metadata.clear_initials = operation.clear_initials;
        metadata.clear_date = operation.clear_date;
        return doc.set_comment_metadata(operation.comment_index, metadata);
    }
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        return doc.append_paragraph_text_comment(
                   operation.paragraph_index, operation.text_offset,
                   operation.text_length, operation.text, author, initials,
                   date) != 0U;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        return doc.insert_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset, operation.text,
            author, date);
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        return doc.delete_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, author, date);
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        return doc.replace_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, operation.text, author, date);
    case review_mutation_plan_operation_kind::append_text_range_comment:
        return doc.append_text_range_comment(
                   operation.start_paragraph_index, operation.start_text_offset,
                   operation.end_paragraph_index, operation.end_text_offset,
                   operation.text, author, initials, date) != 0U;
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        return doc.insert_text_range_revision(operation.start_paragraph_index,
                                              operation.start_text_offset,
                                              operation.text, author, date);
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        return doc.delete_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset, author,
            date);
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        return doc.replace_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset,
            operation.text, author, date);
    }

    return false;
}

auto apply_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations,
    std::size_t &applied_count, std::string &error_message) -> bool {
    std::vector<std::size_t> operation_indices;
    if (!build_review_mutation_plan_apply_order(operations, operation_indices,
                                                error_message)) {
        return false;
    }

    applied_count = 0U;
    for (const auto operation_index : operation_indices) {
        if (!apply_review_mutation_plan_operation(doc,
                                                  operations[operation_index])) {
            const auto &error_info = doc.last_error();
            error_message = "failed to apply review mutation plan operation " +
                            std::to_string(operation_index) + ": " +
                            (!error_info.detail.empty()
                                 ? error_info.detail
                                 : error_info.code.message());
            return false;
        }
        ++applied_count;
    }

    return true;
}

auto build_review_mutation_plan_operation_from_match(
    const review_mutation_plan_build_request_operation &request,
    const featherdoc::text_range_preview &preview,
    review_mutation_plan_operation &operation, std::string &error_message)
    -> bool {
    operation = {};
    operation.kind = request.kind;
    operation.text = request.text;
    operation.expected_text = preview.text;
    operation.author = request.author;
    operation.initials = request.initials;
    operation.date = request.date;

    switch (request.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
        error_message =
            "append_comment_reply is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::replace_comment:
        error_message =
            "replace_comment is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::remove_comment:
        error_message =
            "remove_comment is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        error_message =
            "set_comment_resolved is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        error_message =
            "set_comment_metadata is not supported by build-review-mutation-plan";
        return false;
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (preview.start_paragraph_index != preview.end_paragraph_index) {
            error_message =
                "matched text crosses paragraphs and cannot be used with paragraph text revision operation";
            return false;
        }
        operation.paragraph_index = preview.start_paragraph_index;
        operation.text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        insert_paragraph_text_revision &&
                    request.insert_after_match
                ? preview.end_text_offset
                : preview.start_text_offset;
        operation.text_length =
            operation.kind ==
                    review_mutation_plan_operation_kind::
                        append_paragraph_text_comment
                ? preview.text_length
                : 0U;
        if (operation.kind ==
            review_mutation_plan_operation_kind::insert_paragraph_text_revision) {
            operation.expected_text = std::nullopt;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (preview.start_paragraph_index != preview.end_paragraph_index) {
            error_message =
                "matched text crosses paragraphs and cannot be used with paragraph text revision operation";
            return false;
        }
        operation.paragraph_index = preview.start_paragraph_index;
        operation.text_offset = preview.start_text_offset;
        operation.text_length = preview.text_length;
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        operation.start_paragraph_index =
            operation.kind ==
                        review_mutation_plan_operation_kind::
                            insert_text_range_revision &&
                    request.insert_after_match
                ? preview.end_paragraph_index
                : preview.start_paragraph_index;
        operation.start_text_offset = request.insert_after_match
                                          ? preview.end_text_offset
                                          : preview.start_text_offset;
        operation.end_paragraph_index =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? preview.end_paragraph_index
                : operation.start_paragraph_index;
        operation.end_text_offset =
            operation.kind ==
                    review_mutation_plan_operation_kind::append_text_range_comment
                ? preview.end_text_offset
                : operation.start_text_offset;
        if (operation.kind ==
            review_mutation_plan_operation_kind::insert_text_range_revision) {
            operation.expected_text = std::nullopt;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        operation.start_paragraph_index = preview.start_paragraph_index;
        operation.start_text_offset = preview.start_text_offset;
        operation.end_paragraph_index = preview.end_paragraph_index;
        operation.end_text_offset = preview.end_text_offset;
        return true;
    }

    return false;
}

struct review_mutation_plan_build_candidate {
    featherdoc::text_range_preview preview;
    std::optional<std::size_t> selected_match_index;
};

auto map_review_mutation_context_offset(
    const featherdoc::text_range_preview &preview, std::size_t text_offset,
    bool prefer_next_segment_at_boundary, std::size_t &paragraph_index,
    std::size_t &paragraph_text_offset) -> bool {
    std::size_t global_offset = 0U;
    std::optional<featherdoc::text_range_preview_segment> previous_segment;
    for (const auto &segment : preview.segments) {
        const auto segment_start = global_offset;
        const auto segment_end = segment_start + segment.text_length;
        if (text_offset < segment_end) {
            paragraph_index = segment.paragraph_index;
            paragraph_text_offset =
                segment.text_offset + (text_offset - segment_start);
            return true;
        }
        if (text_offset == segment_end) {
            if (prefer_next_segment_at_boundary) {
                previous_segment = segment;
                global_offset = segment_end;
                continue;
            }
            paragraph_index = segment.paragraph_index;
            paragraph_text_offset = segment.text_offset + segment.text_length;
            return true;
        }
        previous_segment = segment;
        global_offset = segment_end;
    }

    if (text_offset == global_offset && previous_segment.has_value()) {
        paragraph_index = previous_segment->paragraph_index;
        paragraph_text_offset =
            previous_segment->text_offset + previous_segment->text_length;
        return true;
    }

    return false;
}

auto review_mutation_preview_has_same_range(
    const featherdoc::text_range_preview &left,
    const featherdoc::text_range_preview &right) -> bool {
    return left.start_paragraph_index == right.start_paragraph_index &&
           left.start_text_offset == right.start_text_offset &&
           left.end_paragraph_index == right.end_paragraph_index &&
           left.end_text_offset == right.end_text_offset;
}

auto find_review_mutation_raw_match_index(
    const std::vector<featherdoc::text_range_preview> &raw_matches,
    const featherdoc::text_range_preview &preview)
    -> std::optional<std::size_t> {
    for (std::size_t index = 0U; index < raw_matches.size(); ++index) {
        if (review_mutation_preview_has_same_range(raw_matches[index],
                                                   preview)) {
            return index;
        }
    }
    return std::nullopt;
}

auto build_review_mutation_plan_candidates(
    featherdoc::Document &doc,
    const review_mutation_plan_build_request_operation &request,
    const std::vector<featherdoc::text_range_preview> &raw_matches,
    std::vector<review_mutation_plan_build_candidate> &candidates,
    std::string &error_message) -> bool {
    candidates.clear();

    const auto has_context =
        request.before_text.has_value() || request.after_text.has_value();
    if (!has_context) {
        candidates.reserve(raw_matches.size());
        for (std::size_t index = 0U; index < raw_matches.size(); ++index) {
            review_mutation_plan_build_candidate candidate;
            candidate.preview = raw_matches[index];
            candidate.selected_match_index = index;
            candidates.push_back(std::move(candidate));
        }
        return true;
    }

    std::string context_text;
    if (request.before_text.has_value()) {
        context_text += *request.before_text;
    }
    const auto inner_start_offset = context_text.size();
    context_text += request.find_text;
    const auto inner_end_offset = context_text.size();
    if (request.after_text.has_value()) {
        context_text += *request.after_text;
    }

    auto context_matches = doc.find_text_ranges(context_text);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        error_message = !error_info.detail.empty() ? error_info.detail
                                                   : error_info.code.message();
        return false;
    }

    candidates.reserve(context_matches.size());
    for (const auto &context_preview : context_matches) {
        std::size_t start_paragraph_index = 0U;
        std::size_t start_text_offset = 0U;
        std::size_t end_paragraph_index = 0U;
        std::size_t end_text_offset = 0U;
        if (!map_review_mutation_context_offset(
                context_preview, inner_start_offset, true,
                start_paragraph_index, start_text_offset) ||
            !map_review_mutation_context_offset(
                context_preview, inner_end_offset, false, end_paragraph_index,
                end_text_offset)) {
            error_message =
                "failed to map context match back to requested text range";
            return false;
        }

        auto preview = doc.preview_text_range(start_paragraph_index,
                                              start_text_offset,
                                              end_paragraph_index,
                                              end_text_offset);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            error_message = !error_info.detail.empty()
                                ? error_info.detail
                                : error_info.code.message();
            return false;
        }
        if (!preview.has_value() || preview->text != request.find_text) {
            error_message =
                "context match did not resolve back to requested find_text";
            return false;
        }

        review_mutation_plan_build_candidate candidate;
        candidate.selected_match_index =
            find_review_mutation_raw_match_index(raw_matches, *preview);
        candidate.preview = std::move(*preview);
        candidates.push_back(std::move(candidate));
    }

    return true;
}

auto build_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_build_request_operation> &requests,
    std::vector<review_mutation_plan_operation> &operations,
    std::vector<review_mutation_plan_build_resolution> &resolutions,
    std::string &error_message, std::size_t &failed_operation_index,
    std::size_t &failed_matches_count,
    std::size_t &failed_raw_matches_count) -> bool {
    operations.clear();
    operations.reserve(requests.size());
    resolutions.clear();
    resolutions.reserve(requests.size());
    failed_operation_index = 0U;
    failed_matches_count = 0U;
    failed_raw_matches_count = 0U;

    for (std::size_t index = 0U; index < requests.size(); ++index) {
        const auto &request = requests[index];
        auto raw_matches = doc.find_text_ranges(request.find_text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            failed_operation_index = index;
            error_message = !error_info.detail.empty()
                                ? error_info.detail
                                : error_info.code.message();
            return false;
        }

        std::vector<review_mutation_plan_build_candidate> candidates;
        if (!build_review_mutation_plan_candidates(
                doc, request, raw_matches, candidates, error_message)) {
            failed_operation_index = index;
            failed_raw_matches_count = raw_matches.size();
            return false;
        }

        if (request.require_unique && candidates.size() != 1U) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            error_message =
                "requested text did not resolve to a unique match";
            return false;
        }

        if (request.occurrence >= candidates.size()) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            error_message = "requested text occurrence was not found";
            return false;
        }

        const auto &candidate = candidates[request.occurrence];
        review_mutation_plan_operation operation;
        if (!build_review_mutation_plan_operation_from_match(
                request, candidate.preview, operation, error_message)) {
            failed_operation_index = index;
            failed_matches_count = candidates.size();
            failed_raw_matches_count = raw_matches.size();
            return false;
        }

        review_mutation_plan_build_resolution resolution;
        resolution.index = index;
        resolution.kind = request.kind;
        resolution.find_text = request.find_text;
        resolution.occurrence = request.occurrence;
        resolution.before_text = request.before_text;
        resolution.after_text = request.after_text;
        resolution.require_unique = request.require_unique;
        resolution.insert_after_match = request.insert_after_match;
        resolution.raw_matches_count = raw_matches.size();
        resolution.matches_count = candidates.size();
        resolution.selected_match_index = candidate.selected_match_index;
        resolution.preview = candidate.preview;
        resolutions.push_back(std::move(resolution));
        operations.push_back(std::move(operation));
    }

    return true;
}

void write_json_review_mutation_plan_preview_result(
    std::ostream &stream,
    const review_mutation_plan_preview_result &result) {
    stream << "{\"index\":" << result.index << ",\"kind\":";
    write_json_string(stream, review_mutation_plan_operation_kind_name(result.kind));
    stream << ",\"ok\":" << json_bool(result.ok) << ",\"message\":";
    write_json_string(stream, result.message);
    if (result.comment_index.has_value()) {
        stream << ",\"comment_index\":" << *result.comment_index;
    }
    stream << ",\"expected_text\":";
    if (result.expected_text.has_value()) {
        write_json_string(stream, *result.expected_text);
    } else {
        stream << "null";
    }
    if (result.expected_resolved.has_value()) {
        stream << ",\"expected_resolved\":"
               << json_bool(*result.expected_resolved);
    }
    if (result.expected_comment_text.has_value()) {
        stream << ",\"expected_comment_text\":";
        write_json_string(stream, *result.expected_comment_text);
    }
    if (result.expected_parent_index.has_value()) {
        stream << ",\"expected_parent_index\":"
               << *result.expected_parent_index;
    }
    stream << ",\"actual_text\":";
    if (result.actual_text.has_value()) {
        write_json_string(stream, *result.actual_text);
    } else if (result.preview.has_value()) {
        write_json_string(stream, result.preview->text);
    } else {
        stream << "null";
    }
    if (result.actual_resolved.has_value()) {
        stream << ",\"actual_resolved\":" << json_bool(*result.actual_resolved);
    }
    if (result.actual_comment_text.has_value()) {
        stream << ",\"actual_comment_text\":";
        write_json_string(stream, *result.actual_comment_text);
    }
    if (result.comment_index.has_value() ||
        result.expected_parent_index.has_value() ||
        result.actual_parent_index.has_value()) {
        stream << ",\"actual_parent_index\":";
        if (result.actual_parent_index.has_value()) {
            stream << *result.actual_parent_index;
        } else {
            stream << "null";
        }
    }
    stream << ",\"preview\":";
    if (result.preview.has_value()) {
        write_json_text_range_preview(stream, *result.preview);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"preview-review-mutation-plan\",\"ok\":"
           << json_bool(failed_count == 0U)
           << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply(
    std::ostream &stream, featherdoc::Document &doc,
    const std::optional<path_type> &output_path,
    const std::vector<review_mutation_plan_preview_result> &results,
    std::size_t applied_count) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":true"
           << ",\"in_place\":" << json_bool(!output_path.has_value())
           << ",\"output_path\":";
    if (output_path.has_value()) {
        write_json_string(stream, output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"sections\":" << doc.section_count()
           << ",\"headers\":" << doc.header_count()
           << ",\"footers\":" << doc.footer_count()
           << ",\"operations_count\":" << results.size()
           << ",\"applied_count\":" << applied_count
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_apply_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "{\"command\":\"apply-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operations_count\":" << results.size()
           << ",\"failed_count\":" << failed_count << ",\"operations\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_preview_result(stream, results[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_resolution(
    std::ostream &stream,
    const review_mutation_plan_build_resolution &resolution) {
    stream << "{\"index\":" << resolution.index << ",\"kind\":";
    write_json_string(stream,
                      review_mutation_plan_operation_kind_name(resolution.kind));
    stream << ",\"find_text\":";
    write_json_string(stream, resolution.find_text);
    stream << ",\"before_text\":";
    if (resolution.before_text.has_value()) {
        write_json_string(stream, *resolution.before_text);
    } else {
        stream << "null";
    }
    stream << ",\"after_text\":";
    if (resolution.after_text.has_value()) {
        write_json_string(stream, *resolution.after_text);
    } else {
        stream << "null";
    }
    stream << ",\"require_unique\":" << json_bool(resolution.require_unique)
           << ",\"insert_after_match\":"
           << json_bool(resolution.insert_after_match)
           << ",\"occurrence\":" << resolution.occurrence
           << ",\"raw_matches_count\":" << resolution.raw_matches_count
           << ",\"matches_count\":" << resolution.matches_count
           << ",\"selected_match_index\":";
    if (resolution.selected_match_index.has_value()) {
        stream << *resolution.selected_match_index;
    } else {
        stream << "null";
    }
    stream << ",\"preview\":";
    write_json_text_range_preview(stream, resolution.preview);
    stream << '}';
}

void write_json_review_mutation_plan_build_result(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations,
    const std::vector<review_mutation_plan_build_resolution> &resolutions,
    const std::optional<path_type> &output_plan_path) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":true"
           << ",\"operations_count\":" << operations.size()
           << ",\"output_plan_path\":";
    if (output_plan_path.has_value()) {
        write_json_string(stream, output_plan_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"plan\":";
    write_json_review_mutation_plan_document(stream, operations);
    stream << ",\"resolutions\":[";
    for (std::size_t index = 0U; index < resolutions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_build_resolution(stream,
                                                         resolutions[index]);
    }
    stream << "]}\n";
}

void write_json_review_mutation_plan_build_failure(
    std::ostream &stream, std::string_view stage, std::string_view message,
    std::size_t operation_index, std::size_t matches_count,
    std::size_t raw_matches_count) {
    stream << "{\"command\":\"build-review-mutation-plan\",\"ok\":false"
           << ",\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);
    stream << ",\"operation_index\":" << operation_index
           << ",\"matches_count\":" << matches_count
           << ",\"raw_matches_count\":" << raw_matches_count << "}\n";
}

void print_review_mutation_plan_preview(
    std::ostream &stream,
    const std::vector<review_mutation_plan_preview_result> &results) {
    const auto failed_count =
        static_cast<std::size_t>(std::count_if(results.begin(), results.end(),
                                               [](const auto &result) {
                                                   return !result.ok;
                                               }));
    stream << "ok=" << yes_no(failed_count == 0U)
           << " operations_count=" << results.size()
           << " failed_count=" << failed_count;

    for (const auto &result : results) {
        stream << '\n'
               << "operation index=" << result.index
               << " kind="
               << review_mutation_plan_operation_kind_name(result.kind)
               << " ok=" << yes_no(result.ok) << " message=";
        write_json_string(stream, result.message);
        if (result.expected_text.has_value()) {
            stream << " expected_text=";
            write_json_string(stream, *result.expected_text);
        }
        if (result.expected_resolved.has_value()) {
            stream << " expected_resolved="
                   << json_bool(*result.expected_resolved);
        }
        if (result.expected_comment_text.has_value()) {
            stream << " expected_comment_text=";
            write_json_string(stream, *result.expected_comment_text);
        }
        if (result.expected_parent_index.has_value()) {
            stream << " expected_parent_index="
                   << *result.expected_parent_index;
        }
        if (result.actual_text.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, *result.actual_text);
        }
        if (result.actual_resolved.has_value()) {
            stream << " actual_resolved="
                   << json_bool(*result.actual_resolved);
        }
        if (result.actual_comment_text.has_value()) {
            stream << " actual_comment_text=";
            write_json_string(stream, *result.actual_comment_text);
        }
        if (result.actual_parent_index.has_value()) {
            stream << " actual_parent_index="
                   << *result.actual_parent_index;
        }
        if (result.preview.has_value()) {
            stream << " actual_text=";
            write_json_string(stream, result.preview->text);
            stream << '\n';
            print_text_range_preview(stream, *result.preview);
        }
    }
    stream << '\n';
}

auto append_exported_section_targets(featherdoc::Document &doc,
                                     exported_template_schema_result &result,
                                     std::string_view command, bool json_output)
    -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_present =
            [&](validation_part_family part_family,
                featherdoc::section_reference_kind reference_kind,
                bool present) -> bool {
            if (!present) {
                return true;
            }

            const auto template_part =
                part_family == validation_part_family::section_header
                    ? doc.section_header_template(section.index, reference_kind)
                    : doc.section_footer_template(section.index, reference_kind);
            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                std::nullopt, false, template_part, result, command, json_output);
        };

        if (!append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::default_reference,
                               section.header.has_default) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::first_page,
                               section.header.has_first) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::even_page,
                               section.header.has_even) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::default_reference,
                               section.footer.has_default) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::first_page,
                               section.footer.has_first) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::even_page,
                               section.footer.has_even)) {
            return false;
        }
    }

    return true;
}

auto find_related_template_part_by_entry_name(
    featherdoc::Document &doc, section_part_family family, std::string_view entry_name,
    std::optional<std::size_t> &part_index, featherdoc::TemplatePart &part) -> bool {
    const auto total =
        family == section_part_family::header ? doc.header_count() : doc.footer_count();
    for (std::size_t index = 0U; index < total; ++index) {
        auto current =
            family == section_part_family::header ? doc.header_template(index)
                                                  : doc.footer_template(index);
        if (current && current.entry_name() == entry_name) {
            part_index = index;
            part = current;
            return true;
        }
    }

    return false;
}

auto append_exported_resolved_section_targets(featherdoc::Document &doc,
                                              exported_template_schema_result &result,
                                              std::string_view command,
                                              bool json_output) -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_resolved =
            [&](validation_part_family part_family, section_part_family part_group,
                featherdoc::section_reference_kind reference_kind,
                const std::optional<std::string> &resolved_entry_name,
                const std::optional<std::size_t> &resolved_section_index) -> bool {
            if (!resolved_entry_name.has_value() || !resolved_section_index.has_value()) {
                return true;
            }

            std::optional<std::size_t> part_index;
            featherdoc::TemplatePart part;
            if (!find_related_template_part_by_entry_name(doc, part_group,
                                                          *resolved_entry_name,
                                                          part_index, part)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "failed to resolve related part by entry name during schema export";
                error_info.entry_name = *resolved_entry_name;
                return report_operation_failure(command, "inspect",
                                                "failed to resolve related part",
                                                error_info, json_output);
            }

            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                *resolved_section_index, *resolved_section_index != section.index, part,
                result, command, json_output);
        };

        if (!append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::default_reference,
                                section.header.resolved_default_entry_name,
                                section.header.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::first_page,
                                section.header.resolved_first_entry_name,
                                section.header.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::even_page,
                                section.header.resolved_even_entry_name,
                                section.header.resolved_even_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::default_reference,
                                section.footer.resolved_default_entry_name,
                                section.footer.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::first_page,
                                section.footer.resolved_first_entry_name,
                                section.footer.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::even_page,
                                section.footer.resolved_even_entry_name,
                                section.footer.resolved_even_section_index)) {
            return false;
        }
    }

    return true;
}

auto build_exported_template_schema(featherdoc::Document &doc,
                                    bool section_targets,
                                    bool resolved_section_targets,
                                    exported_template_schema_result &result,
                                    std::string_view command, bool json_output)
    -> bool {
    result = {};
    if (!append_exported_template_schema_target(
            doc, validation_part_family::body, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, false, doc.body_template(), result, command,
            json_output)) {
        return false;
    }

    if (resolved_section_targets) {
        if (!append_exported_resolved_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else if (section_targets) {
        if (!append_exported_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else {
        for (std::size_t index = 0U; index < doc.header_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::header, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.header_template(index),
                    result, command, json_output)) {
                return false;
            }
        }

        for (std::size_t index = 0U; index < doc.footer_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::footer, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.footer_template(index),
                    result, command, json_output)) {
                return false;
            }
        }
    }

    if (!result.targets.empty()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "document does not contain any exportable template bookmarks";
    return report_operation_failure(command, "inspect",
                                    "failed to export template schema", error_info,
                                    json_output);
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

    if (command == "inspect-sections") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-sections expects an input path",
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

        inspect_sections(doc, options.json_output);
        return 0;
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

    if (command == "inspect-table-style") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "inspect-table-style expects an input path and a style id",
                              json_output);
            return 2;
        }

        inspect_options options;
        std::string error_message;
        if (!parse_inspect_options(arguments, 3U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto definition = doc.find_table_style_definition(arguments[2]);
        if (!definition.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_table_style_definition(*definition, options.json_output);
        return 0;
    }

    if (command == "audit-table-style-regions") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "audit-table-style-regions expects an input path",
                              json_output);
            return 2;
        }

        audit_table_style_regions_options options;
        std::string error_message;
        if (!parse_audit_table_style_regions_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto report = doc.audit_table_style_regions(
            options.style_id.has_value()
                ? std::optional<std::string_view>{std::string_view{*options.style_id}}
                : std::optional<std::string_view>{});
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "audit", error_info,
                                  options.json_output);
            return 1;
        }

        audit_table_style_regions(report, options.style_id, options.fail_on_issue,
                                  options.json_output);
        return report.ok() || !options.fail_on_issue ? 0 : 1;
    }

    if (command == "audit-table-style-inheritance") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "audit-table-style-inheritance expects an input path",
                              json_output);
            return 2;
        }

        audit_table_style_inheritance_options options;
        std::string error_message;
        if (!parse_audit_table_style_inheritance_options(arguments, 2U, options,
                                                         error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto report = doc.audit_table_style_inheritance(
            options.style_id.has_value()
                ? std::optional<std::string_view>{std::string_view{*options.style_id}}
                : std::optional<std::string_view>{});
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "audit", error_info,
                                  options.json_output);
            return 1;
        }

        audit_table_style_inheritance(report, options.style_id,
                                      options.fail_on_issue, options.json_output);
        return report.ok() || !options.fail_on_issue ? 0 : 1;
    }

    if (command == "audit-table-style-quality") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "audit-table-style-quality expects an input path",
                              json_output);
            return 2;
        }

        audit_table_style_quality_options options;
        std::string error_message;
        if (!parse_audit_table_style_quality_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto report = doc.audit_table_style_quality();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "audit", error_info,
                                  options.json_output);
            return 1;
        }

        audit_table_style_quality(report, options.fail_on_issue,
                                  options.json_output);
        return report.ok() || !options.fail_on_issue ? 0 : 1;
    }

    if (command == "plan-table-style-quality-fixes") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "plan-table-style-quality-fixes expects an input path",
                              json_output);
            return 2;
        }

        plan_table_style_quality_fixes_options options;
        std::string error_message;
        if (!parse_plan_table_style_quality_fixes_options(arguments, 2U, options,
                                                          error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto plan = doc.plan_table_style_quality_fixes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "plan", error_info, options.json_output);
            return 1;
        }

        plan_table_style_quality_fixes(plan, options.fail_on_issue,
                                       options.json_output);
        return plan.ok() || !options.fail_on_issue ? 0 : 1;
    }

    if (command == "apply-table-style-quality-fixes") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "apply-table-style-quality-fixes expects an input path",
                              json_output);
            return 2;
        }

        apply_table_style_quality_fixes_options options;
        std::string error_message;
        if (!parse_apply_table_style_quality_fixes_options(arguments, 2U, options,
                                                           error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        auto result = table_style_quality_apply_cli_result{};
        result.output_path = options.output_path;
        result.before = doc.plan_table_style_quality_fixes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "plan", error_info, options.json_output);
            return 1;
        }

        const auto repair_report = doc.repair_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "repair", error_info,
                                  options.json_output);
            return 1;
        }
        result.changed_table_count = repair_report.changed_table_count;
        result.after = doc.plan_table_style_quality_fixes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "plan", error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        apply_table_style_quality_fixes(result, options.json_output);
        return 0;
    }

    if (command == "plan-style-refactor") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "plan-style-refactor expects an input path",
                              json_output);
            return 2;
        }

        auto options = style_refactor_plan_options{};
        std::string error_message;
        if (!parse_style_refactor_plan_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto plan = doc.plan_style_refactor(options.requests);
        if (!plan.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (options.output_plan_path.has_value() &&
            !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                            error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write style refactor plan output",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_style_refactor_plan(*plan, options.json_output);
        return plan->clean() ? 0 : 1;
    }

    if (command == "suggest-style-merges") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "suggest-style-merges expects an input path",
                              json_output);
            return 2;
        }

        auto options = style_merge_suggestion_options{};
        std::string error_message;
        if (!parse_style_merge_suggestion_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        auto plan = doc.suggest_style_merges();
        if (!plan.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        plan = filter_style_refactor_plan_by_style_ids(
            std::move(*plan), options.source_style_ids,
            options.target_style_ids);

        auto min_confidence = options.min_confidence;
        if (!min_confidence.has_value() &&
            options.confidence_profile.has_value()) {
            if (*options.confidence_profile == "recommended") {
                min_confidence =
                    plan->suggestion_confidence_summary().recommended_min_confidence;
            } else {
                min_confidence = style_merge_suggestion_confidence_profile_min_confidence(
                    *options.confidence_profile);
            }
        }
        if (min_confidence.has_value()) {
            plan = filter_style_refactor_plan_by_min_confidence(std::move(*plan),
                                                                *min_confidence);
        }

        if (options.output_plan_path.has_value() &&
            !write_style_refactor_plan_file(*options.output_plan_path, *plan,
                                            error_message, command)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write style merge suggestions",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_style_refactor_plan(*plan, options.json_output, command,
                                     options.fail_on_suggestion);
        const auto suggestion_gate_failed =
            options.fail_on_suggestion && plan->operation_count != 0U;
        return plan->clean() && !suggestion_gate_failed ? 0 : 1;
    }

    if (command == "apply-style-refactor") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "apply-style-refactor expects an input path",
                              json_output);
            return 2;
        }

        auto options = style_refactor_apply_options{};
        std::string error_message;
        if (!parse_style_refactor_apply_options(arguments, 2U, options,
                                               error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (options.plan_file_path.has_value() &&
            !read_style_refactor_plan_file(*options.plan_file_path,
                                           options.requests, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto result = doc.apply_style_refactor(options.requests);
        if (!result.has_value()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!result->applied()) {
            inspect_style_refactor_apply_result(*result, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.rollback_plan_path.has_value() &&
            !write_style_refactor_rollback_plan_file(*options.rollback_plan_path,
                                                     *result, error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(
                command, "output",
                "failed to write style refactor rollback output", error_info,
                options.json_output);
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&result, &options](std::ostream &stream) {
                    write_json_style_refactor_apply_result_fields(stream,
                                                                   *result);
                    if (options.plan_file_path.has_value()) {
                        stream << ",\"plan_file\":";
                        write_json_string(stream, options.plan_file_path->string());
                    }
                    if (options.rollback_plan_path.has_value()) {
                        stream << ",\"rollback_plan_file\":";
                        write_json_string(stream,
                                          options.rollback_plan_path->string());
                    }
                });
            return 0;
        }

        inspect_style_refactor_apply_result(*result, false);
        return 0;
    }

    if (command == "restore-style-merge") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "restore-style-merge expects an input path",
                              json_output);
            return 2;
        }

        auto options = style_merge_restore_options{};
        std::string error_message;
        if (!parse_style_merge_restore_options(arguments, 2U, options,
                                               error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        auto rollback_entries =
            std::vector<featherdoc::style_refactor_rollback_entry>{};
        if (!read_style_refactor_rollback_file(*options.rollback_plan_path,
                                               options.entry_indexes,
                                               options.source_style_ids,
                                               options.target_style_ids,
                                               rollback_entries, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto result = options.dry_run
            ? doc.plan_style_refactor_restore(rollback_entries)
            : doc.restore_style_refactor(rollback_entries);
        if (!result.has_value()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!result->restored()) {
            inspect_style_refactor_restore_result(*result, options.json_output,
                                                  &options);
            return 1;
        }

        if (options.dry_run) {
            inspect_style_refactor_restore_result(*result, options.json_output,
                                                  &options);
            return 0;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&result, &options](std::ostream &stream) {
                    write_json_style_refactor_restore_result_fields(stream, *result);
                    stream << ",\"rollback_plan_file\":";
                    write_json_string(stream, options.rollback_plan_path->string());
                    write_json_style_refactor_restore_selection(stream, options);
                });
            return 0;
        }

        inspect_style_refactor_restore_result(*result, false);
        return 0;
    }

    if (command == "rename-style") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "rename-style expects an input path, an old style id, and a new style id",
                json_output);
            return 2;
        }

        const auto old_style_id = std::string(arguments[2]);
        const auto new_style_id = std::string(arguments[3]);
        rename_style_options options;
        std::string error_message;
        if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.rename_style(old_style_id, new_style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto style = doc.find_style(new_style_id);
        if (!style.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&old_style_id, &new_style_id, &style](std::ostream &stream) {
                    stream << ",\"old_style_id\":";
                    write_json_string(stream, old_style_id);
                    stream << ",\"new_style_id\":";
                    write_json_string(stream, new_style_id);
                    stream << ",\"style\":";
                    write_json_style_summary(stream, *style);
                });
            return 0;
        }

        inspect_style(*style, std::nullopt, false);
        return 0;
    }

    if (command == "merge-style") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "merge-style expects an input path, a source style id, and a target style id",
                json_output);
            return 2;
        }

        const auto source_style_id = std::string(arguments[2]);
        const auto target_style_id = std::string(arguments[3]);
        merge_style_options options;
        std::string error_message;
        if (!parse_rename_style_options(arguments, 4U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.merge_style(source_style_id, target_style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto style = doc.find_style(target_style_id);
        if (!style.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&source_style_id, &target_style_id, &style](std::ostream &stream) {
                    stream << ",\"source_style_id\":";
                    write_json_string(stream, source_style_id);
                    stream << ",\"target_style_id\":";
                    write_json_string(stream, target_style_id);
                    stream << ",\"style\":";
                    write_json_style_summary(stream, *style);
                });
            return 0;
        }

        inspect_style(*style, std::nullopt, false);
        return 0;
    }

    if (command == "plan-prune-unused-styles") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "plan-prune-unused-styles expects an input path",
                              json_output);
            return 2;
        }

        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            if (arguments[index] == "--json") {
                continue;
            }
            print_parse_error(command, "unknown option: " + std::string(arguments[index]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto plan = doc.plan_prune_unused_styles();
        if (!plan.has_value()) {
            report_document_error(command, "inspect", doc.last_error(), json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"plan-prune-unused-styles\",\"ok\":true";
            write_json_style_prune_plan(std::cout, *plan);
            std::cout << "}\n";
            return 0;
        }

        print_style_prune_plan(*plan);
        return 0;
    }

    if (command == "prune-unused-styles") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "prune-unused-styles expects an input path",
                              json_output);
            return 2;
        }

        prune_unused_styles_options options;
        std::string error_message;
        if (!parse_rename_style_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto summary = doc.prune_unused_styles();
        if (!summary.has_value()) {
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
                [&summary](std::ostream &stream) {
                    write_json_style_prune_summary(stream, *summary);
                });
            return 0;
        }

        print_style_prune_summary(*summary);
        return 0;
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

    if (command == "export-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "export-numbering-catalog expects an input path",
                              json_output);
            return 2;
        }

        export_numbering_catalog_options options;
        std::string error_message;
        if (!parse_export_numbering_catalog_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto catalog = doc.export_numbering_catalog();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "export", error_info,
                                  options.json_output);
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_numbering_catalog(std::cout, catalog);
            std::cout << '\n';
            return 0;
        }

        if (!write_numbering_catalog_file(*options.output_path, catalog,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write numbering catalog output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_numbering_catalog_summary(catalog, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "import-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "import-numbering-catalog expects an input path and --catalog-file <catalog.json>",
                json_output);
            return 2;
        }

        import_numbering_catalog_options options;
        std::string error_message;
        if (!parse_import_numbering_catalog_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(*options.catalog_path, catalog,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto summary = doc.import_numbering_catalog(catalog);
        if (!static_cast<bool>(summary)) {
            report_document_error(command, "import", doc.last_error(),
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
                [&summary, &options](std::ostream &stream) {
                    stream << ",\"catalog_file\":";
                    write_json_string(stream, options.catalog_path->string());
                    stream << ',';
                    write_json_numbering_catalog_import_summary(stream, summary);
                });
        }

        return 0;
    }

    if (command == "patch-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "patch-numbering-catalog expects a catalog path and "
                              "--patch-file <patch.json>",
                              json_output);
            return 2;
        }

        patch_numbering_catalog_options options;
        std::string error_message;
        if (!parse_patch_numbering_catalog_options(arguments, 2U, options,
                                                   error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])),
                                         catalog, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        numbering_catalog_patch_document patch;
        if (!read_numbering_catalog_patch_file(*options.patch_path, patch,
                                               error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        numbering_catalog_patch_summary summary;
        if (!apply_numbering_catalog_patch(catalog, patch, summary,
                                           error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "patch",
                                     "failed to patch numbering catalog",
                                     error_info, options.json_output);
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_numbering_catalog(std::cout, catalog);
            std::cout << '\n';
            return 0;
        }

        if (!write_numbering_catalog_file(*options.output_path, catalog,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write patched numbering catalog output",
                                     error_info, options.json_output);
            return 1;
        }

        print_patched_numbering_catalog_summary(catalog, summary,
                                                options.output_path,
                                                options.json_output);
        return 0;
    }

    if (command == "lint-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "lint-numbering-catalog expects a catalog path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog catalog;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])),
                                         catalog, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_numbering_catalog(catalog);
        print_linted_numbering_catalog_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "diff-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--") ||
            arguments[2].starts_with("--")) {
            print_parse_error(
                command,
                "diff-numbering-catalog expects left and right catalog paths",
                json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog left;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[1])), left,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        featherdoc::numbering_catalog right;
        if (!read_numbering_catalog_file(path_type(std::string(arguments[2])), right,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto result = diff_numbering_catalogs(left, right);
        print_numbering_catalog_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-numbering-catalog") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(
                command,
                "check-numbering-catalog expects an input path and --catalog-file <catalog.json>",
                json_output);
            return 2;
        }

        check_numbering_catalog_options options;
        std::string error_message;
        if (!parse_check_numbering_catalog_options(arguments, 2U, options,
                                                   error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        featherdoc::numbering_catalog baseline;
        if (!read_numbering_catalog_file(*options.catalog_path, baseline,
                                         error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        const auto baseline_lint = lint_numbering_catalog(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto generated = doc.export_numbering_catalog();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "export", error_info,
                                  options.json_output);
            return 1;
        }
        const auto generated_lint = lint_numbering_catalog(generated);

        if (options.output_path.has_value() &&
            !write_numbering_catalog_file(*options.output_path, generated,
                                          error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(
                command, "output",
                "failed to write generated numbering catalog output",
                error_info, options.json_output);
            return 1;
        }

        const auto result = diff_numbering_catalogs(baseline, generated);
        print_checked_numbering_catalog_result(
            *options.catalog_path, baseline_lint, generated_lint, result,
            options.output_path, options.json_output);
        if (!baseline_lint.clean() || !generated_lint.clean() ||
            !result.equal()) {
            return 1;
        }
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

    if (command == "check-table-style-look") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "check-table-style-look expects an input path",
                              json_output);
            return 2;
        }

        check_table_style_look_options options;
        std::string error_message;
        if (!parse_check_table_style_look_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto input_path = path_type(std::string(arguments[1]));
        if (!open_document(input_path, doc, command, options.json_output)) {
            return 1;
        }

        const auto report = doc.check_table_style_look_consistency();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "check", error_info,
                                  options.json_output);
            return 1;
        }

        check_table_style_look(report, options.fail_on_issue, options.json_output);
        return report.ok() || !options.fail_on_issue ? 0 : 1;
    }

    if (command == "repair-table-style-look") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "repair-table-style-look expects an input path",
                              json_output);
            return 2;
        }

        repair_table_style_look_options options;
        std::string error_message;
        if (!parse_repair_table_style_look_options(arguments, 2U, options,
                                                   error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto input_path = path_type(std::string(arguments[1]));
        if (!open_document(input_path, doc, command, options.json_output)) {
            return 1;
        }

        auto result = table_style_look_repair_cli_result{};
        result.apply = options.apply;
        result.output_path = options.output_path;
        if (options.apply) {
            const auto repair_report = doc.repair_table_style_look_consistency();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "repair", error_info,
                                      options.json_output);
                return 1;
            }
            result.before = repair_report.before;
            result.after = repair_report.after;
            result.changed_table_count = repair_report.changed_table_count;
            result.applicable_issue_count =
                count_applicable_table_style_look_issues(result.before);
            if (!save_document(doc, options.output_path, command,
                               options.json_output)) {
                return 1;
            }
        } else {
            result.before = doc.check_table_style_look_consistency();
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "check", error_info,
                                      options.json_output);
                return 1;
            }
            result.applicable_issue_count =
                count_applicable_table_style_look_issues(result.before);
        }

        repair_table_style_look(result, options.json_output);
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

    if (command == "set-table-cell-fill") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "set-table-cell-fill expects an input path, a table index, a row index, a cell index, and a fill color",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        const auto fill_color = arguments[5];
        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.set_fill_color(fill_color)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = fill_color.empty()
                                    ? "table cell fill color must not be empty"
                                    : "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell fill color",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, fill_color](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"fill_color\":";
                    write_json_string(stream, fill_color);
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-fill") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "clear-table-cell-fill expects an input path, a table index, a row index, and a cell index",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
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

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_fill_color()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell fill color",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index;
                });
        }

        return 0;
    }

    if (command == "set-table-cell-vertical-alignment") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "set-table-cell-vertical-alignment expects an input path, a table index, a row index, a cell index, and an alignment",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_vertical_alignment alignment =
            featherdoc::cell_vertical_alignment::top;
        if (!parse_cell_vertical_alignment_text(arguments[5], alignment)) {
            print_parse_error(command,
                              "invalid vertical alignment: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.set_vertical_alignment(alignment)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell vertical alignment",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, alignment](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"vertical_alignment\":";
                    write_json_string(stream,
                                      cell_vertical_alignment_name(alignment));
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-vertical-alignment") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "clear-table-cell-vertical-alignment expects an input path, a table index, a row index, and a cell index",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
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

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_vertical_alignment()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell vertical alignment",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index;
                });
        }

        return 0;
    }

    if (command == "set-table-cell-text-direction") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "set-table-cell-text-direction expects an input path, a table index, a row index, a cell index, and a direction",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_text_direction direction =
            featherdoc::cell_text_direction::left_to_right_top_to_bottom;
        if (!parse_cell_text_direction_text(arguments[5], direction)) {
            print_parse_error(command,
                              "invalid text direction: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.set_text_direction(direction)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell text direction",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, direction](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"text_direction\":";
                    write_json_string(stream, cell_text_direction_name(direction));
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-text-direction") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "clear-table-cell-text-direction expects an input path, a table index, a row index, and a cell index",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
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

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_text_direction()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell text direction",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index;
                });
        }

        return 0;
    }

    if (command == "set-table-cell-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "set-table-cell-width expects an input path, a table index, a row index, a cell index, and a width in twips",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        std::uint32_t width_twips = 0U;
        if (!parse_uint32(arguments[5], width_twips)) {
            print_parse_error(command,
                              "invalid width twips: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.set_width_twips(width_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell width",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, width_twips](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"width_twips\":" << width_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "clear-table-cell-width expects an input path, a table index, a row index, and a cell index",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
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

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_width()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell width",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index;
                });
        }

        return 0;
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

    if (command == "set-table-style-id") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-style-id expects an input path, a table index, and a style id",
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

        const auto style_id = std::string(arguments[3]);
        if (style_id.empty()) {
            print_parse_error(command, "style id must not be empty", json_output);
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_style_id(style_id)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table style id", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, &style_id](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"style_id\":";
                    write_json_string(stream, style_id);
                });
        }

        return 0;
    }

    if (command == "clear-table-style-id") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-style-id expects an input path and a table index",
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

        if (!table.clear_style_id()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table style id", error_info,
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

    if (command == "set-table-style-look") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-table-style-look expects an input path, a table index, "
                "and at least one style-look flag",
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

        table_style_look_options options;
        std::string error_message;
        if (!parse_table_style_look_options(arguments, 3U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }
        if (!table_style_look_options_have_flag(options)) {
            print_parse_error(command,
                              "set-table-style-look requires at least one "
                              "style-look flag",
                              json_output);
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

        auto style_look =
            table.style_look().value_or(featherdoc::table_style_look{});
        if (options.first_row.has_value()) {
            style_look.first_row = *options.first_row;
        }
        if (options.last_row.has_value()) {
            style_look.last_row = *options.last_row;
        }
        if (options.first_column.has_value()) {
            style_look.first_column = *options.first_column;
        }
        if (options.last_column.has_value()) {
            style_look.last_column = *options.last_column;
        }
        if (options.banded_rows.has_value()) {
            style_look.banded_rows = *options.banded_rows;
        }
        if (options.banded_columns.has_value()) {
            style_look.banded_columns = *options.banded_columns;
        }

        if (!table.set_style_look(style_look)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table style look",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, &style_look](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"style_look\":";
                    write_json_table_style_look(stream, style_look);
                });
        }

        return 0;
    }

    if (command == "clear-table-style-look") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-style-look expects an input path and a table index",
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

        if (!table.clear_style_look()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table style look",
                                     error_info, options.json_output);
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

    if (command == "set-table-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-width expects an input path, a table index, and a width in twips",
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

        std::uint32_t width_twips = 0U;
        if (!parse_uint32(arguments[3], width_twips)) {
            print_parse_error(command,
                              "invalid width twips: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_width_twips(width_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table width", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, width_twips](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"width_twips\":" << width_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-width") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-width expects an input path and a table index",
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

        if (!table.clear_width()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table width", error_info,
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

    if (command == "set-table-layout-mode") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-layout-mode expects an input path, a table index, and a layout mode",
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

        featherdoc::table_layout_mode layout_mode =
            featherdoc::table_layout_mode::autofit;
        if (!parse_table_layout_mode_text(arguments[3], layout_mode)) {
            print_parse_error(command,
                              "invalid table layout mode: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_layout_mode(layout_mode)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table layout mode",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, layout_mode](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"layout_mode\":";
                    write_json_string(stream, table_layout_mode_name(layout_mode));
                });
        }

        return 0;
    }

    if (command == "clear-table-layout-mode") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-layout-mode expects an input path and a table index",
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

        if (!table.clear_layout_mode()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table layout mode",
                                     error_info, options.json_output);
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

    if (command == "set-table-alignment") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-alignment expects an input path, a table index, and an alignment",
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

        featherdoc::table_alignment alignment = featherdoc::table_alignment::left;
        if (!parse_table_alignment_text(arguments[3], alignment)) {
            print_parse_error(command,
                              "invalid table alignment: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_alignment(alignment)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table alignment",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, alignment](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"alignment\":";
                    write_json_string(stream, table_alignment_name(alignment));
                });
        }

        return 0;
    }

    if (command == "clear-table-alignment") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-alignment expects an input path and a table index",
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

        if (!table.clear_alignment()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table alignment",
                                     error_info, options.json_output);
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

    if (command == "set-table-indent") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-indent expects an input path, a table index, and an indent in twips",
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

        std::uint32_t indent_twips = 0U;
        if (!parse_uint32(arguments[3], indent_twips)) {
            print_parse_error(command,
                              "invalid indent twips: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_indent_twips(indent_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table indent", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, indent_twips](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"indent_twips\":" << indent_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-indent") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "clear-table-indent expects an input path and a table index",
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

        if (!table.clear_indent()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table indent", error_info,
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

    if (command == "set-table-cell-spacing") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-cell-spacing expects an input path, a table index, and spacing in twips",
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

        std::uint32_t spacing_twips = 0U;
        if (!parse_uint32(arguments[3], spacing_twips)) {
            print_parse_error(command,
                              "invalid cell spacing twips: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_cell_spacing_twips(spacing_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell spacing",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, spacing_twips](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"cell_spacing_twips\":" << spacing_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-spacing") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-table-cell-spacing expects an input path and a table index",
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

        if (!table.clear_cell_spacing()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell spacing",
                                     error_info, options.json_output);
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

    if (command == "set-table-default-cell-margin") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 5U) {
            print_parse_error(
                command,
                "set-table-default-cell-margin expects an input path, a table index, a margin edge, and a margin in twips",
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

        featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
        if (!parse_cell_margin_edge_text(arguments[3], edge)) {
            print_parse_error(command,
                              "invalid margin edge: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::uint32_t margin_twips = 0U;
        if (!parse_uint32(arguments[4], margin_twips)) {
            print_parse_error(command,
                              "invalid margin twips: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.set_cell_margin_twips(edge, margin_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table default cell margin",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, edge, margin_twips](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_margin_edge_name(edge));
                    stream << ",\"margin_twips\":" << margin_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-default-cell-margin") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "clear-table-default-cell-margin expects an input path, a table index, and a margin edge",
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

        featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
        if (!parse_cell_margin_edge_text(arguments[3], edge)) {
            print_parse_error(command,
                              "invalid margin edge: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.clear_cell_margin(edge)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table default cell margin",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, edge](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_margin_edge_name(edge));
                });
        }

        return 0;
    }

    if (command == "set-table-border") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "set-table-border expects an input path, a table index, and a border edge",
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

        featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
        if (!parse_table_border_edge_text(arguments[3], edge)) {
            print_parse_error(command,
                              "invalid table border edge: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        table_cell_border_options options;
        std::string error_message;
        if (!parse_table_cell_border_options(arguments, 4U, options,
                                             error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!options.style.has_value()) {
            print_parse_error(command, "missing --style option", json_output);
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

        featherdoc::border_definition border{};
        border.style = *options.style;
        if (options.size_eighth_points.has_value()) {
            border.size_eighth_points = *options.size_eighth_points;
        }
        if (options.color.has_value()) {
            border.color = *options.color;
        }
        if (options.space_points.has_value()) {
            border.space_points = *options.space_points;
        }

        if (!table.set_border(edge, border)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table border",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, edge, border](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"edge\":";
                    write_json_string(stream, table_border_edge_name(edge));
                    stream << ",\"style\":";
                    write_json_string(stream, border_style_name(border.style));
                    stream << ",\"size_eighth_points\":"
                           << border.size_eighth_points
                           << ",\"color\":";
                    write_json_string(stream, border.color);
                    stream << ",\"space_points\":" << border.space_points;
                });
        }

        return 0;
    }

    if (command == "clear-table-border") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "clear-table-border expects an input path, a table index, and a border edge",
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

        featherdoc::table_border_edge edge = featherdoc::table_border_edge::top;
        if (!parse_table_border_edge_text(arguments[3], edge)) {
            print_parse_error(command,
                              "invalid table border edge: " +
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

        featherdoc::Table table;
        if (!resolve_body_table(doc, table_index, table, command,
                                options.json_output)) {
            return 1;
        }

        if (!table.clear_border(edge)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table border",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, edge](std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"edge\":";
                    write_json_string(stream, table_border_edge_name(edge));
                });
        }

        return 0;
    }

    if (command == "set-table-cell-margin") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 7U) {
            print_parse_error(
                command,
                "set-table-cell-margin expects an input path, a table index, a row index, a cell index, a margin edge, and a margin in twips",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
        if (!parse_cell_margin_edge_text(arguments[5], edge)) {
            print_parse_error(command,
                              "invalid margin edge: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        std::uint32_t margin_twips = 0U;
        if (!parse_uint32(arguments[6], margin_twips)) {
            print_parse_error(command,
                              "invalid margin twips: " +
                                  std::string(arguments[6]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 7U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.set_margin_twips(edge, margin_twips)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell margin",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, edge, margin_twips](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_margin_edge_name(edge));
                    stream << ",\"margin_twips\":" << margin_twips;
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-margin") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "clear-table-cell-margin expects an input path, a table index, a row index, a cell index, and a margin edge",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_margin_edge edge = featherdoc::cell_margin_edge::top;
        if (!parse_cell_margin_edge_text(arguments[5], edge)) {
            print_parse_error(command,
                              "invalid margin edge: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_margin(edge)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell margin",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, edge](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_margin_edge_name(edge));
                });
        }

        return 0;
    }

    if (command == "set-table-cell-border") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "set-table-cell-border expects an input path, a table index, a row index, a cell index, and a border edge",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_border_edge edge = featherdoc::cell_border_edge::top;
        if (!parse_cell_border_edge_text(arguments[5], edge)) {
            print_parse_error(command,
                              "invalid border edge: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_border_options options;
        std::string error_message;
        if (!parse_table_cell_border_options(arguments, 6U, options,
                                             error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!options.style.has_value()) {
            print_parse_error(command, "missing --style option", json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        featherdoc::border_definition border{};
        border.style = *options.style;
        if (options.size_eighth_points.has_value()) {
            border.size_eighth_points = *options.size_eighth_points;
        }
        if (options.color.has_value()) {
            border.color = *options.color;
        }
        if (options.space_points.has_value()) {
            border.space_points = *options.space_points;
        }

        if (!cell.set_border(edge, border)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to set table cell border",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, edge, border](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_border_edge_name(edge));
                    stream << ",\"style\":";
                    write_json_string(stream, border_style_name(border.style));
                    stream << ",\"size_eighth_points\":"
                           << border.size_eighth_points
                           << ",\"color\":";
                    write_json_string(stream, border.color);
                    stream << ",\"space_points\":" << border.space_points;
                });
        }

        return 0;
    }

    if (command == "clear-table-cell-border") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "clear-table-cell-border expects an input path, a table index, a row index, a cell index, and a border edge",
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

        std::size_t row_index = 0U;
        if (!parse_index(arguments[3], row_index)) {
            print_parse_error(command,
                              "invalid row index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t cell_index = 0U;
        if (!parse_index(arguments[4], cell_index)) {
            print_parse_error(command,
                              "invalid cell index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        featherdoc::cell_border_edge edge = featherdoc::cell_border_edge::top;
        if (!parse_cell_border_edge_text(arguments[5], edge)) {
            print_parse_error(command,
                              "invalid border edge: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        table_cell_style_options options;
        std::string error_message;
        if (!parse_table_cell_style_options(arguments, 6U, options,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::TableCell cell;
        if (!resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                     command, options.json_output)) {
            return 1;
        }

        if (!cell.clear_border(edge)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail = "target table cell handle is not valid";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "mutate",
                                     "failed to clear table cell border",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [table_index, row_index, cell_index, edge](
                    std::ostream &stream) {
                    stream << ",\"table_index\":" << table_index
                           << ",\"row_index\":" << row_index
                           << ",\"cell_index\":" << cell_index
                           << ",\"edge\":";
                    write_json_string(stream, cell_border_edge_name(edge));
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

    if (command == "ensure-table-style") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "ensure-table-style expects an input path and a style id",
                              json_output);
            return 2;
        }

        const auto style_id = arguments[2];
        ensure_table_style_options options;
        std::string error_message;
        if (!parse_ensure_table_style_options(arguments, 3U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.ensure_table_style(style_id, options.definition)) {
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

    if (command == "inspect-update-fields-on-open") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "inspect-update-fields-on-open expects an input path",
                              json_output);
            return 2;
        }
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            if (arguments[index] != "--json") {
                print_parse_error(command,
                                  "unknown option: " + std::string(arguments[index]),
                                  json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto enabled = doc.update_fields_on_open_enabled();
        if (!enabled.has_value()) {
            report_document_error(command, "inspect", doc.last_error(), json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"inspect-update-fields-on-open\","
                      << "\"ok\":true,\"update_fields_on_open\":"
                      << json_bool(*enabled) << "}\n";
        } else {
            std::cout << "update_fields_on_open: " << (*enabled ? "true" : "false")
                      << '\n';
        }
        return 0;
    }

    if (command == "set-update-fields-on-open") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "set-update-fields-on-open expects an input path",
                              json_output);
            return 2;
        }

        set_update_fields_on_open_options options;
        std::string error_message;
        if (!parse_set_update_fields_on_open_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto mutated = *options.enabled ? doc.enable_update_fields_on_open()
                                             : doc.clear_update_fields_on_open();
        if (!mutated) {
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
                [&options](std::ostream &stream) {
                    stream << ",\"update_fields_on_open\":"
                           << json_bool(*options.enabled);
                });
        }
        return 0;
    }

    if (command == "inspect-bookmarks") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-bookmarks expects an input path",
                              json_output);
            return 2;
        }

        inspect_bookmarks_options options;
        std::string error_message;
        if (!parse_inspect_bookmarks_options(arguments, 2U, options, error_message)) {
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

        if (options.bookmark_name.has_value()) {
            const auto bookmark = selected.part.find_bookmark(*options.bookmark_name);
            if (!bookmark.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            inspect_bookmark(selected, *bookmark, options.json_output);
            return 0;
        }

        const auto bookmarks = selected.part.list_bookmarks();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_bookmarks(selected, bookmarks, options.json_output);
        return 0;
    }

    if (command == "inspect-content-controls") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "inspect-content-controls expects an input path",
                json_output);
            return 2;
        }

        inspect_content_controls_options options;
        std::string error_message;
        if (!parse_inspect_content_controls_options(arguments, 2U, options,
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

        const auto content_controls = selected.part.list_content_controls();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto filtered_content_controls =
            filter_content_controls(content_controls, options);
        inspect_content_controls(selected, filtered_content_controls, options,
                                 options.json_output);
        return 0;
    }

    if (command == "inspect-hyperlinks") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "inspect-hyperlinks expects an input path",
                              json_output);
            return 2;
        }
        if (arguments.size() > 3U ||
            (arguments.size() == 3U && arguments[2] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[2]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto hyperlinks = doc.list_hyperlinks();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }

        inspect_hyperlinks(hyperlinks, json_output);
        return 0;
    }

    if (command == "inspect-review") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-review expects an input path",
                              json_output);
            return 2;
        }
        if (arguments.size() > 3U ||
            (arguments.size() == 3U && arguments[2] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[2]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto footnotes = doc.list_footnotes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto endnotes = doc.list_endnotes();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto comments = doc.list_comments();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }
        const auto revisions = doc.list_revisions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }

        inspect_review(footnotes, endnotes, comments, revisions, json_output);
        return 0;
    }

    if (command == "find-text-ranges") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "find-text-ranges expects an input path",
                              json_output);
            return 2;
        }

        std::optional<std::string> query_text;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--text") {
                if (query_text.has_value()) {
                    print_parse_error(command, "duplicate --text option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing value after --text",
                                      json_output);
                    return 2;
                }
                query_text = std::string(arguments[index + 1U]);
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!query_text.has_value()) {
            print_parse_error(command, "missing --text <text>", json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto matches = doc.find_text_ranges(*query_text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "find", error_info, json_output);
            return 1;
        }

        if (json_output) {
            write_json_text_range_matches(std::cout, command, *query_text,
                                          matches);
        } else {
            print_text_range_matches(std::cout, *query_text, matches);
        }
        return 0;
    }

    if (command == "build-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "build-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> request_file_path;
        std::optional<path_type> output_plan_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--request-file") {
                if (request_file_path.has_value()) {
                    print_parse_error(command, "duplicate --request-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --request-file",
                                      json_output);
                    return 2;
                }
                request_file_path =
                    path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output-plan") {
                if (output_plan_path.has_value()) {
                    print_parse_error(command, "duplicate --output-plan option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command,
                                      "missing path after --output-plan",
                                      json_output);
                    return 2;
                }
                output_plan_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!request_file_path.has_value()) {
            print_parse_error(command, "missing --request-file <request.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_build_request_operation> requests;
        std::string error_message;
        if (!read_review_mutation_plan_build_request_file(
                *request_file_path, requests, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
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
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "resolve", error_message, failed_operation_index,
                    failed_matches_count, failed_raw_matches_count);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (output_plan_path.has_value() &&
            !write_review_mutation_plan_file(*output_plan_path, operations,
                                             error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_build_failure(
                    std::cout, "write", error_message, 0U, 0U, 0U);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_build_result(
                std::cout, operations, resolutions, output_plan_path);
        } else if (output_plan_path.has_value()) {
            std::cout << "command: " << command << '\n'
                      << "output_plan_path: " << output_plan_path->string()
                      << '\n'
                      << "operations_count: " << operations.size() << '\n';
        } else {
            write_json_review_mutation_plan_document(std::cout, operations);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "preview-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "preview-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                results.begin(), results.end(),
                [](const auto &result) { return !result.ok; }));

        if (json_output) {
            write_json_review_mutation_plan_preview(std::cout, results);
        } else {
            print_review_mutation_plan_preview(std::cout, results);
        }
        return failed_count == 0U ? 0 : 1;
    }

    if (command == "apply-review-mutation-plan") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "apply-review-mutation-plan expects an input path",
                              json_output);
            return 2;
        }

        std::optional<path_type> plan_file_path;
        std::optional<path_type> output_path;
        for (std::size_t index = 2U; index < arguments.size(); ++index) {
            const auto argument = arguments[index];
            if (argument == "--plan-file") {
                if (plan_file_path.has_value()) {
                    print_parse_error(command, "duplicate --plan-file option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --plan-file",
                                      json_output);
                    return 2;
                }
                plan_file_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--output") {
                if (output_path.has_value()) {
                    print_parse_error(command, "duplicate --output option",
                                      json_output);
                    return 2;
                }
                if (index + 1U >= arguments.size()) {
                    print_parse_error(command, "missing path after --output",
                                      json_output);
                    return 2;
                }
                output_path = path_type(std::string(arguments[index + 1U]));
                ++index;
                continue;
            }
            if (argument == "--json") {
                continue;
            }

            print_parse_error(command, "unknown option: " + std::string(argument),
                              json_output);
            return 2;
        }

        if (!plan_file_path.has_value()) {
            print_parse_error(command, "missing --plan-file <plan.json>",
                              json_output);
            return 2;
        }

        std::vector<review_mutation_plan_operation> operations;
        std::string error_message;
        if (!read_review_mutation_plan_file(*plan_file_path, operations,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview_results =
            preview_review_mutation_plan_operations(doc, operations);
        const auto failed_count =
            static_cast<std::size_t>(std::count_if(
                preview_results.begin(), preview_results.end(),
                [](const auto &result) { return !result.ok; }));
        if (failed_count != 0U) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "preflight",
                    "review mutation plan preflight failed", preview_results);
            } else {
                std::cerr << "review mutation plan preflight failed\n";
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        if (find_review_mutation_plan_overlap(operations, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "validate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
                print_review_mutation_plan_preview(std::cerr, preview_results);
            }
            return 1;
        }

        std::size_t applied_count = 0U;
        if (!apply_review_mutation_plan_operations(
                doc, operations, applied_count, error_message)) {
            if (json_output) {
                write_json_review_mutation_plan_apply_failure(
                    std::cout, "mutate", error_message, preview_results);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!save_document(doc, output_path, command, json_output)) {
            return 1;
        }

        if (json_output) {
            write_json_review_mutation_plan_apply(
                std::cout, doc, output_path, preview_results, applied_count);
        } else {
            print_simple_document_mutation_result(command, output_path,
                                                  applied_count);
        }
        return 0;
    }

    if (command == "preview-text-range") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "preview-text-range expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }
        if (arguments.size() > 7U ||
            (arguments.size() == 7U && arguments[6] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[6]),
                              json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t end_paragraph_index = 0U;
        if (!parse_index(arguments[4], end_paragraph_index)) {
            print_parse_error(command,
                              "invalid end paragraph index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        std::size_t end_text_offset = 0U;
        if (!parse_index(arguments[5], end_text_offset)) {
            print_parse_error(command,
                              "invalid end text offset: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview = doc.preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value()) {
            report_document_error(command, "preview", doc.last_error(),
                                  json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"preview-text-range\",\"ok\":true,"
                      << "\"preview\":";
            write_json_text_range_preview(std::cout, *preview);
            std::cout << "}\n";
        } else {
            print_text_range_preview(std::cout, *preview);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "inspect-omml") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-omml expects an input path",
                              json_output);
            return 2;
        }
        if (arguments.size() > 3U ||
            (arguments.size() == 3U && arguments[2] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[2]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto formulas = doc.list_omml();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, json_output);
            return 1;
        }

        inspect_omml(formulas, json_output);
        return 0;
    }

    if (command == "append-hyperlink" || command == "replace-hyperlink" ||
        command == "remove-hyperlink") {
        const auto json_output = has_json_flag(arguments);
        const bool append = command == "append-hyperlink";
        const bool remove = command == "remove-hyperlink";
        const std::size_t min_argument_count = append ? 2U : 3U;
        if (arguments.size() < min_argument_count) {
            print_parse_error(command,
                              append ? "append-hyperlink expects an input path"
                                     : std::string(command) +
                                           " expects an input path and hyperlink index",
                              json_output);
            return 2;
        }

        std::size_t hyperlink_index = 0U;
        if (!append && !parse_index(arguments[2], hyperlink_index)) {
            print_parse_error(command,
                              "invalid hyperlink index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        hyperlink_mutation_options options;
        std::string error_message;
        if (!parse_hyperlink_mutation_options(
                arguments, append ? 2U : 3U, options, !remove,
                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (append) {
            affected = doc.append_hyperlink(options.text, options.target);
        } else if (remove) {
            affected = doc.remove_hyperlink(hyperlink_index) ? 1U : 0U;
        } else {
            affected = doc.replace_hyperlink(hyperlink_index, options.text,
                                             options.target)
                           ? 1U
                           : 0U;
        }
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "append-omml" || command == "replace-omml" ||
        command == "remove-omml") {
        const auto json_output = has_json_flag(arguments);
        const bool append = command == "append-omml";
        const bool remove = command == "remove-omml";
        const std::size_t min_argument_count = append ? 2U : 3U;
        if (arguments.size() < min_argument_count) {
            print_parse_error(command,
                              append ? "append-omml expects an input path"
                                     : std::string(command) +
                                           " expects an input path and formula index",
                              json_output);
            return 2;
        }

        std::size_t formula_index = 0U;
        if (!append && !parse_index(arguments[2], formula_index)) {
            print_parse_error(command,
                              "invalid formula index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        omml_mutation_options options;
        std::string error_message;
        if (!parse_omml_mutation_options(arguments, append ? 2U : 3U,
                                         options, !remove, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (append) {
            affected = doc.append_omml(options.xml) ? 1U : 0U;
        } else if (remove) {
            affected = doc.remove_omml(formula_index) ? 1U : 0U;
        } else {
            affected = doc.replace_omml(formula_index, options.xml) ? 1U : 0U;
        }
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "append-insertion-revision" ||
        command == "append-deletion-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        if (!parse_revision_authoring_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const bool insertion = command == "append-insertion-revision";
        const auto affected = insertion
                                  ? doc.append_insertion_revision(
                                        options.text, options.author, options.date)
                                  : doc.append_deletion_revision(
                                        options.text, options.author, options.date);
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "insert-run-revision-after" ||
        command == "delete-run-revision" ||
        command == "replace-run-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, paragraph index, and run index",
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

        std::size_t run_index = 0U;
        if (!parse_index(arguments[3], run_index)) {
            print_parse_error(command,
                              "invalid run index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-run-revision";
        if (!parse_revision_authoring_options(arguments, 4U, options,
                                              error_message, require_text)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        bool ok = false;
        if (command == "insert-run-revision-after") {
            ok = doc.insert_run_revision_after(
                paragraph_index, run_index, options.text, options.author,
                options.date);
        } else if (command == "delete-run-revision") {
            ok = doc.delete_run_revision(paragraph_index, run_index,
                                         options.author, options.date);
        } else {
            ok = doc.replace_run_revision(paragraph_index, run_index,
                                          options.text, options.author,
                                          options.date);
        }
        if (!ok) {
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
                [paragraph_index, run_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"run_index\":" << run_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-paragraph-text-revision" ||
        command == "delete-paragraph-text-revision" ||
        command == "replace-paragraph-text-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-paragraph-text-revision";
        if (arguments.size() < (insert_revision ? 4U : 5U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, paragraph index, text offset, and text length",
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

        std::size_t text_offset = 0U;
        if (!parse_index(arguments[3], text_offset)) {
            print_parse_error(command,
                              "invalid text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        if (!insert_revision && !parse_index(arguments[4], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-paragraph-text-revision";
        const auto options_start = insert_revision ? 4U : 5U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-paragraph-text-revision does not accept --text",
                !insert_revision,
                "insert-paragraph-text-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision) {
            if (text_length >
                std::numeric_limits<std::size_t>::max() - text_offset) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.entry_name = "word/document.xml";
                error_info.detail = "text range is out of range";
                report_operation_failure(command, "validate",
                                         "text range is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            if (!validate_revision_expected_text(
                    doc, command, options, paragraph_index, text_offset,
                    paragraph_index, text_offset + text_length)) {
                return 1;
            }
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_paragraph_text_revision(
                paragraph_index, text_offset, options.text, options.author,
                options.date);
        } else if (command == "delete-paragraph-text-revision") {
            ok = doc.delete_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.author,
                options.date);
        } else {
            ok = doc.replace_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.text,
                options.author, options.date);
        }
        if (!ok) {
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
                [insert_revision, paragraph_index, text_offset,
                 text_length](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"text_offset\":" << text_offset;
                    if (!insert_revision) {
                        stream << ",\"text_length\":" << text_length;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-text-range-revision" ||
        command == "delete-text-range-revision" ||
        command == "replace-text-range-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-text-range-revision";
        if (arguments.size() < (insert_revision ? 4U : 6U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (!insert_revision) {
            if (!parse_index(arguments[4], end_paragraph_index)) {
                print_parse_error(command,
                                  "invalid end paragraph index: " +
                                      std::string(arguments[4]),
                                  json_output);
                return 2;
            }
            if (!parse_index(arguments[5], end_text_offset)) {
                print_parse_error(command,
                                  "invalid end text offset: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-text-range-revision";
        const auto options_start = insert_revision ? 4U : 6U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-text-range-revision does not accept --text",
                !insert_revision,
                "insert-text-range-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision &&
            !validate_revision_expected_text(
                doc, command, options, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset)) {
            return 1;
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_text_range_revision(
                start_paragraph_index, start_text_offset, options.text,
                options.author, options.date);
        } else if (command == "delete-text-range-revision") {
            ok = doc.delete_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.author, options.date);
        } else {
            ok = doc.replace_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.text, options.author, options.date);
        }
        if (!ok) {
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
                [insert_revision, start_paragraph_index, start_text_offset,
                 end_paragraph_index, end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (!insert_revision) {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "set-revision-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-revision-metadata expects an input path and revision index",
                json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (!parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        revision_metadata_mutation_options options;
        std::string error_message;
        if (!parse_revision_metadata_mutation_options(arguments, 3U, options,
                                                      error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_revision_metadata(revision_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
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
                [revision_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"revision_index\":" << revision_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "accept-revision" || command == "reject-revision" ||
        command == "accept-all-revisions" || command == "reject-all-revisions") {
        const auto json_output = has_json_flag(arguments);
        const bool accept_one = command == "accept-revision";
        const bool reject_one = command == "reject-revision";
        const bool one_revision = accept_one || reject_one;
        if (arguments.size() < (one_revision ? 3U : 2U)) {
            print_parse_error(command,
                              one_revision ? std::string(command) +
                                                 " expects an input path and revision index"
                                           : std::string(command) +
                                                 " expects an input path",
                              json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (one_revision && !parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(
                arguments, one_revision ? 3U : 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (accept_one) {
            affected = doc.accept_revision(revision_index) ? 1U : 0U;
        } else if (reject_one) {
            affected = doc.reject_revision(revision_index) ? 1U : 0U;
        } else if (command == "accept-all-revisions") {
            affected = doc.accept_all_revisions();
        } else {
            affected = doc.reject_all_revisions();
        }
        if (one_revision && affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "append-footnote" || command == "replace-footnote" ||
        command == "remove-footnote" || command == "append-endnote" ||
        command == "replace-endnote" || command == "remove-endnote") {
        const auto json_output = has_json_flag(arguments);
        const bool footnote = command.find("footnote") != std::string::npos;
        const bool append = command.find("append") == 0U;
        const bool replace = command.find("replace") == 0U;
        if (arguments.size() < (append ? 2U : 3U)) {
            print_parse_error(command,
                              append ? std::string(command) +
                                           " expects an input path"
                                     : std::string(command) +
                                           " expects an input path and note index",
                              json_output);
            return 2;
        }

        std::size_t note_index = 0U;
        if (!append && !parse_index(arguments[2], note_index)) {
            print_parse_error(command,
                              "invalid note index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        review_note_mutation_options options;
        std::string error_message;
        if (!parse_review_note_mutation_options(
                arguments, append ? 2U : 3U, options, append, replace || append,
                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (append) {
            affected = footnote ? doc.append_footnote(options.reference_text,
                                                      options.note_text)
                                : doc.append_endnote(options.reference_text,
                                                     options.note_text);
        } else if (replace) {
            affected = footnote ? (doc.replace_footnote(note_index,
                                                        options.note_text) ? 1U : 0U)
                                : (doc.replace_endnote(note_index,
                                                       options.note_text) ? 1U : 0U);
        } else {
            affected = footnote ? (doc.remove_footnote(note_index) ? 1U : 0U)
                                : (doc.remove_endnote(note_index) ? 1U : 0U);
        }
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "append-paragraph-text-comment" ||
        command == "append-text-range-comment") {
        const auto json_output = has_json_flag(arguments);
        const bool paragraph_range = command == "append-paragraph-text-comment";
        if (arguments.size() < (paragraph_range ? 5U : 6U)) {
            print_parse_error(
                command,
                paragraph_range
                    ? std::string(command) +
                          " expects an input path, paragraph index, text offset, and text length"
                    : std::string(command) +
                          " expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (paragraph_range) {
            if (!parse_index(arguments[4], text_length)) {
                print_parse_error(command,
                                  "invalid text length: " +
                                      std::string(arguments[4]),
                                  json_output);
                return 2;
            }
        } else {
            if (!parse_index(arguments[4], end_paragraph_index)) {
                print_parse_error(command,
                                  "invalid end paragraph index: " +
                                      std::string(arguments[4]),
                                  json_output);
                return 2;
            }
            if (!parse_index(arguments[5], end_text_offset)) {
                print_parse_error(command,
                                  "invalid end text offset: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
        }

        comment_mutation_options options;
        std::string error_message;
        if (!parse_comment_mutation_options(
                arguments, paragraph_range ? 5U : 6U, options, false, true,
                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (paragraph_range) {
            affected = doc.append_paragraph_text_comment(
                start_paragraph_index, start_text_offset, text_length,
                options.comment_text, options.author, options.initials,
                options.date);
        } else {
            affected = doc.append_text_range_comment(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.comment_text, options.author,
                options.initials, options.date);
        }
        if (affected == 0U) {
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
                [affected, paragraph_range, start_paragraph_index,
                 start_text_offset, text_length, end_paragraph_index,
                 end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                    stream << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (paragraph_range) {
                        stream << ",\"text_length\":" << text_length;
                    } else {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "set-paragraph-text-comment-range" ||
        command == "set-text-range-comment-range") {
        const auto json_output = has_json_flag(arguments);
        const bool paragraph_range =
            command == "set-paragraph-text-comment-range";
        if (arguments.size() < (paragraph_range ? 6U : 7U)) {
            print_parse_error(
                command,
                paragraph_range
                    ? std::string(command) +
                          " expects an input path, comment index, paragraph index, text offset, and text length"
                    : std::string(command) +
                          " expects an input path, comment index, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }

        std::size_t comment_index = 0U;
        if (!parse_index(arguments[2], comment_index)) {
            print_parse_error(command,
                              "invalid comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[3], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[4], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (paragraph_range) {
            if (!parse_index(arguments[5], text_length)) {
                print_parse_error(command,
                                  "invalid text length: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
        } else {
            if (!parse_index(arguments[5], end_paragraph_index)) {
                print_parse_error(command,
                                  "invalid end paragraph index: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
            if (!parse_index(arguments[6], end_text_offset)) {
                print_parse_error(command,
                                  "invalid end text offset: " +
                                      std::string(arguments[6]),
                                  json_output);
                return 2;
            }
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(
                arguments, paragraph_range ? 6U : 7U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const bool ok =
            paragraph_range
                ? doc.set_paragraph_text_comment_range(
                      comment_index, start_paragraph_index, start_text_offset,
                      text_length)
                : doc.set_text_range_comment_range(
                      comment_index, start_paragraph_index, start_text_offset,
                      end_paragraph_index, end_text_offset);
        if (!ok) {
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
                [paragraph_range, comment_index, start_paragraph_index,
                 start_text_offset, text_length, end_paragraph_index,
                 end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"comment_index\":" << comment_index
                           << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (paragraph_range) {
                        stream << ",\"text_length\":" << text_length;
                    } else {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "append-comment-reply") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "append-comment-reply expects an input path and parent comment index",
                json_output);
            return 2;
        }

        std::size_t parent_comment_index = 0U;
        if (!parse_index(arguments[2], parent_comment_index)) {
            print_parse_error(command,
                              "invalid parent comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        comment_mutation_options options;
        std::string error_message;
        if (!parse_comment_mutation_options(arguments, 3U, options, false, true,
                                            error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto affected = doc.append_comment_reply(
            parent_comment_index, options.comment_text, options.author,
            options.initials, options.date);
        if (affected == 0U) {
            report_document_error(command, "mutate", doc.last_error(),
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
                [affected, parent_comment_index](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                    stream << ",\"parent_comment_index\":"
                           << parent_comment_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "set-comment-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-comment-metadata expects an input path and comment index",
                json_output);
            return 2;
        }

        std::size_t comment_index = 0U;
        if (!parse_index(arguments[2], comment_index)) {
            print_parse_error(command,
                              "invalid comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        comment_metadata_mutation_options options;
        std::string error_message;
        if (!parse_comment_metadata_mutation_options(arguments, 3U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_comment_metadata(comment_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
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
                [comment_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"comment_index\":" << comment_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "set-comment-resolved") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, comment index, and resolved value",
                json_output);
            return 2;
        }

        std::size_t comment_index = 0U;
        if (!parse_index(arguments[2], comment_index)) {
            print_parse_error(command,
                              "invalid comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        bool resolved = false;
        if (!parse_bool(arguments[3], resolved)) {
            print_parse_error(command,
                              "invalid resolved value: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(arguments, 4U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_comment_resolved(comment_index, resolved)) {
            report_document_error(command, "mutate", doc.last_error(),
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
                [comment_index, resolved](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"comment_index\":" << comment_index
                           << ",\"resolved\":" << json_bool(resolved);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "append-comment" || command == "replace-comment" ||
        command == "remove-comment") {
        const auto json_output = has_json_flag(arguments);
        const bool append = command == "append-comment";
        const bool replace = command == "replace-comment";
        if (arguments.size() < (append ? 2U : 3U)) {
            print_parse_error(command,
                              append ? "append-comment expects an input path"
                                     : std::string(command) +
                                           " expects an input path and comment index",
                              json_output);
            return 2;
        }

        std::size_t comment_index = 0U;
        if (!append && !parse_index(arguments[2], comment_index)) {
            print_parse_error(command,
                              "invalid comment index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        comment_mutation_options options;
        std::string error_message;
        if (!parse_comment_mutation_options(
                arguments, append ? 2U : 3U, options, append, append || replace,
                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (append) {
            affected = doc.append_comment(options.selected_text,
                                          options.comment_text, options.author,
                                          options.initials, options.date);
        } else if (replace) {
            affected = doc.replace_comment(comment_index, options.comment_text) ? 1U
                                                                                : 0U;
        } else {
            affected = doc.remove_comment(comment_index) ? 1U : 0U;
        }
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "replace-content-control-text") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "replace-content-control-text expects an input path",
                json_output);
            return 2;
        }

        replace_content_control_text_options options;
        std::string error_message;
        if (!parse_replace_content_control_text_options(arguments, 2U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto replaced = options.tag.has_value()
                                  ? selected.part.replace_content_control_text_by_tag(
                                        *options.tag, options.text)
                                  : selected.part.replace_content_control_text_by_alias(
                                        *options.alias, options.text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }
        if (replaced == 0U) {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, replaced](std::ostream &stream) {
                    write_json_content_control_text_result(stream, selected,
                                                           options, replaced);
                });
        } else {
            print_content_control_text_result(selected, options,
                                              options.output_path, replaced);
        }

        return 0;
    }

    if (command == "set-content-control-form-state") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "set-content-control-form-state expects an input path",
                json_output);
            return 2;
        }

        set_content_control_form_state_options options;
        std::string error_message;
        if (!parse_set_content_control_form_state_options(arguments, 2U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto updated =
            options.tag.has_value()
                ? selected.part.set_content_control_form_state_by_tag(
                      *options.tag, options.state)
                : selected.part.set_content_control_form_state_by_alias(
                      *options.alias, options.state);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }
        if (updated == 0U) {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, updated](std::ostream &stream) {
                    write_json_content_control_form_state_result(
                        stream, selected, options, updated);
                });
        } else {
            print_content_control_form_state_result(selected, options, updated);
        }

        return 0;
    }

    if (command == "sync-content-controls-from-custom-xml") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "sync-content-controls-from-custom-xml expects an input path",
                json_output);
            return 2;
        }

        sync_content_controls_from_custom_xml_options options;
        std::string error_message;
        if (!parse_sync_content_controls_from_custom_xml_options(
                arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto sync_result = doc.sync_content_controls_from_custom_xml();
        if (!sync_result.has_value()) {
            report_document_error(command, "sync", doc.last_error(),
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
                [&sync_result](std::ostream &stream) {
                    write_json_custom_xml_sync_result(stream, *sync_result);
                });
        } else {
            print_custom_xml_sync_result(options.output_path, *sync_result);
        }

        return 0;
    }

    if (command == "replace-content-control-paragraphs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "replace-content-control-paragraphs expects an input path",
                json_output);
            return 2;
        }

        replace_content_control_paragraphs_options options;
        std::string error_message;
        if (!parse_replace_content_control_paragraphs_options(
                arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::string> paragraphs;
        if (!resolve_text_sources(options.paragraph_sources, paragraphs,
                                  error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto replaced =
            options.tag.has_value()
                ? selected.part.replace_content_control_with_paragraphs_by_tag(
                      *options.tag, paragraphs)
                : selected.part.replace_content_control_with_paragraphs_by_alias(
                      *options.alias, paragraphs);
        if (replaced == 0U) {
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "mutate", error_info,
                                      options.json_output);
            } else {
                report_operation_failure(command, "mutate",
                                         "matching content control not found",
                                         doc.last_error(), options.json_output);
            }
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, &paragraphs,
                 replaced](std::ostream &stream) {
                    write_json_content_control_paragraphs_result(
                        stream, selected, options, paragraphs, replaced);
                });
        } else {
            print_content_control_paragraphs_result(selected, options,
                                                    paragraphs, replaced);
        }

        return 0;
    }

    if (command == "replace-content-control-table" ||
        command == "replace-content-control-table-rows") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        content_control_table_replacement_options options;
        std::string error_message;
        const bool allow_empty_rows =
            command == "replace-content-control-table-rows";
        if (!parse_content_control_table_replacement_options(
                arguments, 2U, options, allow_empty_rows, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::vector<std::string>> rows;
        if (!resolve_bookmark_table_row_sources(options, rows, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto replaced =
            command == "replace-content-control-table"
                ? (options.tag.has_value()
                       ? selected.part.replace_content_control_with_table_by_tag(
                             *options.tag, rows)
                       : selected.part.replace_content_control_with_table_by_alias(
                             *options.alias, rows))
                : (options.tag.has_value()
                       ? selected.part.replace_content_control_with_table_rows_by_tag(
                             *options.tag, rows)
                       : selected.part.replace_content_control_with_table_rows_by_alias(
                             *options.alias, rows));
        if (replaced == 0U) {
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "mutate", error_info,
                                      options.json_output);
            } else {
                report_operation_failure(command, "mutate",
                                         "matching content control not found",
                                         doc.last_error(), options.json_output);
            }
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, &rows,
                 replaced](std::ostream &stream) {
                    write_json_content_control_table_result(
                        stream, selected, options, rows, replaced);
                });
        } else {
            print_content_control_table_result(selected, options, rows, replaced);
        }

        return 0;
    }

    if (command == "replace-content-control-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "replace-content-control-image expects an input path and image path",
                json_output);
            return 2;
        }

        const auto image_path = path_type(std::string(arguments[2]));
        replace_content_control_image_options options;
        std::string error_message;
        if (!parse_replace_content_control_image_options(arguments, 3U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto existing_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto replaced =
            options.width_px.has_value()
                ? (options.tag.has_value()
                       ? selected.part.replace_content_control_with_image_by_tag(
                             *options.tag, image_path, *options.width_px,
                             *options.height_px)
                       : selected.part.replace_content_control_with_image_by_alias(
                             *options.alias, image_path, *options.width_px,
                             *options.height_px))
                : (options.tag.has_value()
                       ? selected.part.replace_content_control_with_image_by_tag(
                             *options.tag, image_path)
                       : selected.part.replace_content_control_with_image_by_alias(
                             *options.alias, image_path));
        if (replaced == 0U) {
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "mutate", error_info,
                                      options.json_output);
            } else {
                report_operation_failure(command, "mutate",
                                         "matching content control not found",
                                         doc.last_error(), options.json_output);
            }
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        const auto inserted_images =
            collect_new_drawing_images(existing_images, updated_images);
        if (inserted_images.size() != replaced) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail =
                "expected " + std::to_string(replaced) +
                " replaced drawing image(s) in " +
                std::string(selected.part.entry_name()) + ", found " +
                std::to_string(inserted_images.size());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "replaced drawing images not found",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, &image_path,
                 &inserted_images](std::ostream &stream) {
                    write_json_content_control_image_result(
                        stream, selected, options, image_path, inserted_images);
                });
        } else {
            print_content_control_image_result(selected, options, image_path,
                                               inserted_images);
        }

        return 0;
    }

    if (command == "replace-bookmark-paragraphs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "replace-bookmark-paragraphs expects an input path and bookmark name",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        replace_bookmark_paragraphs_options options;
        std::string error_message;
        if (!parse_replace_bookmark_paragraphs_options(arguments, 3U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::string> paragraphs;
        if (!resolve_text_sources(options.paragraph_sources, paragraphs,
                                  error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto replaced = selected.part.replace_bookmark_with_paragraphs(
            bookmark_name, paragraphs);
        if (replaced == 0U) {
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
                [&selected, &bookmark_summary, replaced,
                 &paragraphs](std::ostream &stream) {
                    write_json_bookmark_paragraphs_result(
                        stream, selected, bookmark_summary, paragraphs, replaced);
                });
        } else {
            print_bookmark_paragraphs_result(selected, bookmark_summary,
                                             paragraphs, options.output_path,
                                             replaced);
        }

        return 0;
    }

    if (command == "replace-bookmark-table" ||
        command == "replace-bookmark-table-rows") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                std::string(command) + " expects an input path and bookmark name",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        bookmark_table_replacement_options options;
        std::string error_message;
        const bool allow_empty_rows = command == "replace-bookmark-table-rows";
        if (!parse_bookmark_table_replacement_options(arguments, 3U, options,
                                                     allow_empty_rows,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::vector<std::string>> rows;
        if (!resolve_bookmark_table_row_sources(options, rows, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto replaced =
            command == "replace-bookmark-table"
                ? selected.part.replace_bookmark_with_table(bookmark_name, rows)
                : selected.part.replace_bookmark_with_table_rows(bookmark_name,
                                                                 rows);
        if (replaced == 0U) {
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
                [&selected, &bookmark_summary, &rows,
                 replaced](std::ostream &stream) {
                    write_json_bookmark_table_result(
                        stream, selected, bookmark_summary, rows, replaced);
                });
        } else {
            print_bookmark_table_result(selected, bookmark_summary, rows,
                                        options.output_path, replaced);
        }

        return 0;
    }

    if (command == "remove-bookmark-block") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "remove-bookmark-block expects an input path and bookmark name",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        remove_bookmark_block_options options;
        std::string error_message;
        if (!parse_remove_bookmark_block_options(arguments, 3U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto removed = selected.part.remove_bookmark_block(bookmark_name);
        if (removed == 0U) {
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
                [&selected, &bookmark_summary,
                 removed](std::ostream &stream) {
                    write_json_bookmark_block_removal_result(
                        stream, selected, bookmark_summary, removed);
                });
        } else {
            print_bookmark_block_removal_result(selected, bookmark_summary,
                                                options.output_path, removed);
        }

        return 0;
    }

    if (command == "replace-bookmark-text") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "replace-bookmark-text expects an input path and bookmark name",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        replace_bookmark_text_options options;
        std::string error_message;
        if (!parse_replace_bookmark_text_options(arguments, 3U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto replaced =
            selected.part.replace_bookmark_text(bookmark_name, options.text);
        if (replaced == 0U) {
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
                [&selected, &bookmark_summary, &options,
                 replaced](std::ostream &stream) {
                    write_json_bookmark_text_result(
                        stream, selected, bookmark_summary, options.text,
                        replaced);
                });
        } else {
            print_bookmark_text_result(selected, bookmark_summary, options.text,
                                       options.output_path, replaced);
        }

        return 0;
    }

    if (command == "fill-bookmarks") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "fill-bookmarks expects an input path",
                              json_output);
            return 2;
        }

        fill_bookmarks_options options;
        std::string error_message;
        if (!parse_fill_bookmarks_options(arguments, 2U, options,
                                          error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<featherdoc::bookmark_text_binding> bindings;
        if (!resolve_fill_bookmark_bindings(options, bindings, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto result = selected.part.fill_bookmarks(bindings);
        if (doc.last_error().code) {
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
                [&selected, &bindings, &result](std::ostream &stream) {
                    write_json_bookmark_fill_result(stream, selected, bindings,
                                                    result);
                });
        } else {
            print_bookmark_fill_result(selected, bindings, options.output_path,
                                       result);
        }

        return 0;
    }

    if (command == "set-bookmark-block-visibility") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-bookmark-block-visibility expects an input path and bookmark name",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        set_bookmark_block_visibility_options options;
        std::string error_message;
        if (!parse_set_bookmark_block_visibility_options(arguments, 3U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto changed = selected.part.set_bookmark_block_visibility(
            bookmark_name, options.visible);
        if (changed == 0U) {
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
                [&selected, &bookmark_summary, &options,
                 changed](std::ostream &stream) {
                    write_json_bookmark_block_visibility_result(
                        stream, selected, bookmark_summary, options.visible,
                        changed);
                });
        } else {
            print_bookmark_block_visibility_result(
                selected, bookmark_summary, options.output_path, options.visible,
                changed);
        }

        return 0;
    }

    if (command == "apply-bookmark-block-visibility") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "apply-bookmark-block-visibility expects an input path and at least one visibility binding",
                json_output);
            return 2;
        }

        apply_bookmark_block_visibility_options options;
        std::string error_message;
        if (!parse_apply_bookmark_block_visibility_options(arguments, 2U, options,
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto result =
            selected.part.apply_bookmark_block_visibility(options.bindings);
        if (doc.last_error().code) {
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
                [&selected, &options, &result](std::ostream &stream) {
                    write_json_applied_bookmark_block_visibility_result(
                        stream, selected, options.bindings, result);
                });
        } else {
            print_applied_bookmark_block_visibility_result(
                selected, options.bindings, options.output_path, result);
        }

        return 0;
    }

    if (command == "replace-bookmark-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "replace-bookmark-image expects an input path, bookmark name, and image path",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        append_image_options options;
        std::string error_message;
        if (!parse_bookmark_image_command_options(arguments, 4U, command, false,
                                                  options, error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto existing_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto image_path = path_type(std::string(arguments[3]));
        std::size_t replaced = 0U;
        if (options.width_px.has_value()) {
            replaced = selected.part.replace_bookmark_with_image(
                bookmark_name, image_path, *options.width_px, *options.height_px);
        } else {
            replaced = selected.part.replace_bookmark_with_image(bookmark_name,
                                                                 image_path);
        }
        if (replaced == 0U) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        const auto inserted_images =
            collect_new_drawing_images(existing_images, updated_images);
        if (inserted_images.size() != replaced) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail =
                "expected " + std::to_string(replaced) +
                " replaced drawing image(s) in " +
                std::string(selected.part.entry_name()) + ", found " +
                std::to_string(inserted_images.size());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "replaced drawing images not found",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &bookmark_summary, &image_path,
                 &inserted_images](std::ostream &stream) {
                    write_json_bookmark_image_result(
                        stream, selected, bookmark_summary, image_path,
                        inserted_images);
                });
        } else {
            print_bookmark_image_result(selected, bookmark_summary, image_path,
                                        options.output_path, inserted_images);
        }

        return 0;
    }

    if (command == "replace-bookmark-floating-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "replace-bookmark-floating-image expects an input path, bookmark name, and image path",
                json_output);
            return 2;
        }

        const auto bookmark_name = std::string(arguments[2]);
        if (bookmark_name.empty()) {
            print_parse_error(command, "bookmark name must not be empty",
                              json_output);
            return 2;
        }

        append_image_options options;
        std::string error_message;
        if (!parse_bookmark_image_command_options(arguments, 4U, command, true,
                                                  options, error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto bookmark = selected.part.find_bookmark(bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }
        const auto bookmark_summary = *bookmark;

        const auto existing_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto image_path = path_type(std::string(arguments[3]));
        std::size_t replaced = 0U;
        if (options.width_px.has_value()) {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, *options.width_px, *options.height_px,
                options.floating_options);
        } else {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, options.floating_options);
        }
        if (replaced == 0U) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        const auto inserted_images =
            collect_new_drawing_images(existing_images, updated_images);
        if (inserted_images.size() != replaced) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail =
                "expected " + std::to_string(replaced) +
                " replaced drawing image(s) in " +
                std::string(selected.part.entry_name()) + ", found " +
                std::to_string(inserted_images.size());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "replaced drawing images not found",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &bookmark_summary, &image_path,
                 &inserted_images](std::ostream &stream) {
                    write_json_bookmark_image_result(
                        stream, selected, bookmark_summary, image_path,
                        inserted_images);
                });
        } else {
            print_bookmark_image_result(selected, bookmark_summary, image_path,
                                        options.output_path, inserted_images);
        }

        return 0;
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

    if (command == "inspect-images") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-images expects an input path",
                              json_output);
            return 2;
        }

        inspect_images_options options;
        std::string error_message;
        if (!parse_inspect_images_options(arguments, 2U, options, error_message)) {
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

        const auto images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        if (options.image_index.has_value()) {
            const auto image_it = std::find_if(
                images.begin(), images.end(),
                [&options](const featherdoc::drawing_image_info &image) {
                    return image.index == *options.image_index;
                });
            if (image_it == images.end() ||
                !drawing_image_matches_filters(*image_it, options)) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::result_out_of_range);
                error_info.detail = "drawing image index " +
                                    std::to_string(*options.image_index) +
                                    " was not found in " +
                                    std::string(selected.part.entry_name());
                if (has_inspect_image_filters(options)) {
                    error_info.detail += " for " +
                                         describe_inspect_image_filters(options);
                }
                error_info.entry_name = std::string(selected.part.entry_name());
                report_operation_failure(command, "inspect", "drawing image not found",
                                         error_info, options.json_output);
                return 1;
            }

            inspect_image(selected, *image_it, options, options.json_output);
            return 0;
        }

        inspect_images(selected, filter_drawing_images(images, options), options,
                       options.json_output);
        return 0;
    }

    if (command == "extract-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "extract-image expects an input path and an output path",
                              json_output);
            return 2;
        }

        extract_image_options options;
        std::string error_message;
        if (!parse_extract_image_options(arguments, 3U, options, error_message)) {
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

        const auto images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        featherdoc::drawing_image_info target_image{};
        featherdoc::document_error_info selection_error{};
        if (!resolve_selected_drawing_image(images, options,
                                            selected.part.entry_name(), target_image,
                                            selection_error)) {
            report_operation_failure(command, "input", "drawing image not found",
                                     selection_error, options.json_output);
            return 1;
        }

        const auto output_path = path_type(std::string(arguments[2]));
        if (!selected.part.extract_drawing_image(target_image.index, output_path)) {
            report_document_error(command, "extract", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (options.json_output) {
            std::cout << "{\"command\":";
            write_json_string(std::cout, command);
            std::cout << ",\"ok\":true"
                      << ",\"part\":";
            write_json_string(std::cout, validation_part_name(selected.family));
            if (selected.part_index.has_value()) {
                std::cout << ",\"part_index\":" << *selected.part_index;
            }
            if (selected.section_index.has_value()) {
                std::cout << ",\"section\":" << *selected.section_index;
            }
            if (selected.reference_kind.has_value()) {
                std::cout << ",\"kind\":";
                write_json_string(
                    std::cout,
                    featherdoc::to_xml_reference_type(*selected.reference_kind));
            }
            std::cout << ",\"entry_name\":";
            write_json_string(std::cout, std::string(selected.part.entry_name()));
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path.string());
            write_json_inspect_image_filters(std::cout, options);
            std::cout << ",\"image\":";
            write_json_drawing_image_summary(std::cout, target_image);
            std::cout << "}\n";
        } else {
            print_extract_image_result(selected, target_image, output_path);
        }

        return 0;
    }

    if (command == "replace-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "replace-image expects an input path and an image path",
                              json_output);
            return 2;
        }

        replace_image_options options;
        std::string error_message;
        if (!parse_replace_image_options(arguments, 3U, options, error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        featherdoc::drawing_image_info target_image{};
        featherdoc::document_error_info selection_error{};
        if (!resolve_selected_drawing_image(images, options,
                                            selected.part.entry_name(), target_image,
                                            selection_error)) {
            report_operation_failure(command, "input", "drawing image not found",
                                     selection_error, options.json_output);
            return 1;
        }

        const auto replacement_path = path_type(std::string(arguments[2]));
        if (!selected.part.replace_drawing_image(target_image.index, replacement_path)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        const auto updated_image_it = std::find_if(
            updated_images.begin(), updated_images.end(),
            [&target_image](const featherdoc::drawing_image_info &image) {
                return image.index == target_image.index;
            });
        if (updated_image_it == updated_images.end()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail = "updated drawing image index " +
                                std::to_string(target_image.index) +
                                " was not found in " +
                                std::string(selected.part.entry_name());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "updated drawing image not found", error_info,
                                     options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &updated_image_it, &replacement_path,
                 &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, validation_part_name(selected.family));
                    if (selected.part_index.has_value()) {
                        stream << ",\"part_index\":" << *selected.part_index;
                    }
                    if (selected.section_index.has_value()) {
                        stream << ",\"section\":" << *selected.section_index;
                    }
                    if (selected.reference_kind.has_value()) {
                        stream << ",\"kind\":";
                        write_json_string(
                            stream,
                            featherdoc::to_xml_reference_type(*selected.reference_kind));
                    }
                    stream << ",\"entry_name\":";
                    write_json_string(stream, std::string(selected.part.entry_name()));
                    stream << ",\"replacement_path\":";
                    write_json_string(stream, replacement_path.string());
                    write_json_inspect_image_filters(stream, options);
                    stream << ",\"image\":";
                    write_json_drawing_image_summary(stream, *updated_image_it);
                });
        } else {
            print_replace_image_result(selected, *updated_image_it, replacement_path,
                                       options.output_path, options.json_output);
        }

        return 0;
    }

    if (command == "remove-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "remove-image expects an input path and image selectors",
                              json_output);
            return 2;
        }

        remove_image_options options;
        std::string error_message;
        if (!parse_remove_image_options(arguments, 2U, options, error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        featherdoc::drawing_image_info target_image{};
        featherdoc::document_error_info selection_error{};
        if (!resolve_selected_drawing_image(images, options,
                                            selected.part.entry_name(), target_image,
                                            selection_error)) {
            report_operation_failure(command, "input", "drawing image not found",
                                     selection_error, options.json_output);
            return 1;
        }

        if (!selected.part.remove_drawing_image(target_image.index)) {
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
                [&selected, &target_image, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, validation_part_name(selected.family));
                    if (selected.part_index.has_value()) {
                        stream << ",\"part_index\":" << *selected.part_index;
                    }
                    if (selected.section_index.has_value()) {
                        stream << ",\"section\":" << *selected.section_index;
                    }
                    if (selected.reference_kind.has_value()) {
                        stream << ",\"kind\":";
                        write_json_string(
                            stream,
                            featherdoc::to_xml_reference_type(*selected.reference_kind));
                    }
                    stream << ",\"entry_name\":";
                    write_json_string(stream, std::string(selected.part.entry_name()));
                    write_json_inspect_image_filters(stream, options);
                    stream << ",\"image\":";
                    write_json_drawing_image_summary(stream, target_image);
                });
        } else {
            print_remove_image_result(selected, target_image, options.output_path);
        }

        return 0;
    }

    if (command == "append-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "append-image expects an input path and an image path",
                              json_output);
            return 2;
        }

        append_image_options options;
        std::string error_message;
        if (!parse_append_image_options(arguments, 3U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_mutable_template_part(doc, options.part, options.part_index,
                                          options.section_index,
                                          options.reference_kind, selected,
                                          error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto existing_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto image_path = path_type(std::string(arguments[2]));
        bool success = false;
        if (options.floating) {
            if (options.width_px.has_value()) {
                success = selected.part.append_floating_image(
                    image_path, *options.width_px, *options.height_px,
                    options.floating_options);
            } else {
                success = selected.part.append_floating_image(image_path,
                                                              options.floating_options);
            }
        } else if (options.width_px.has_value()) {
            success = selected.part.append_image(image_path, *options.width_px,
                                                 *options.height_px);
        } else {
            success = selected.part.append_image(image_path);
        }

        if (!success) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        if (updated_images.size() <= existing_images.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail =
                "appended drawing image was not found in " +
                std::string(selected.part.entry_name());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "appended drawing image not found", error_info,
                                     options.json_output);
            return 1;
        }

        const auto &appended_image = updated_images.back();
        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &appended_image, &image_path](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, validation_part_name(selected.family));
                    if (selected.part_index.has_value()) {
                        stream << ",\"part_index\":" << *selected.part_index;
                    }
                    if (selected.section_index.has_value()) {
                        stream << ",\"section\":" << *selected.section_index;
                    }
                    if (selected.reference_kind.has_value()) {
                        stream << ",\"kind\":";
                        write_json_string(
                            stream,
                            featherdoc::to_xml_reference_type(*selected.reference_kind));
                    }
                    stream << ",\"entry_name\":";
                    write_json_string(stream, std::string(selected.part.entry_name()));
                    stream << ",\"image_path\":";
                    write_json_string(stream, image_path.string());
                    stream << ",\"floating\":"
                           << json_bool(appended_image.floating_options.has_value());
                    stream << ",\"image\":";
                    write_json_drawing_image_summary(stream, appended_image);
                });
        } else {
            print_append_image_result(selected, appended_image, image_path,
                                      options.output_path);
        }

        return 0;
    }

    if (command == "inspect-header-parts" || command == "inspect-footer-parts") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-header/footer-parts expects an input path",
                              json_output);
            return 2;
        }

        inspect_options options;
        std::string error_message;
        if (!parse_inspect_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "inspect-header-parts"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::vector<inspected_part_entry> parts;
        if (!load_inspected_part_entries(path_type(std::string(arguments[1])), doc, family,
                                         parts, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "inspect",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        inspect_related_parts(parts, family, options.json_output);
        return 0;
    }

    if (command == "insert-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "insert-section expects an input path and a section index",
                              json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, true, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.insert_section(section_index, options.inherit_header_footer)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            const auto inserted_section = section_index + 1U;
            write_json_mutation_result(
                command, doc, options.output_path,
                [inserted_section, &options](std::ostream &stream) {
                    stream << ",\"section\":" << inserted_section
                           << ",\"inherit_header_footer\":"
                           << json_bool(options.inherit_header_footer);
                });
        }

        return 0;
    }

    if (command == "remove-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "remove-section expects an input path and a section index",
                              json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.remove_section(section_index)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(command, doc, options.output_path,
                                       [section_index](std::ostream &stream) {
                                           stream << ",\"section\":" << section_index;
                                       });
        }

        return 0;
    }

    if (command == "move-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "move-section expects an input path, a source index, and a "
                "target index",
                json_output);
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error(command,
                              "invalid source index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error(command,
                              "invalid target index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.move_section(source_index, target_index)) {
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
                [source_index, target_index](std::ostream &stream) {
                    stream << ",\"source\":" << source_index
                           << ",\"target\":" << target_index;
                });
        }

        return 0;
    }

    if (command == "copy-section-layout") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "copy-section-layout expects an input path, a source index, "
                "and a target index",
                json_output);
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error(command,
                              "invalid source index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error(command,
                              "invalid target index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.copy_section_header_references(source_index, target_index) ||
            !doc.copy_section_footer_references(source_index, target_index)) {
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
                [source_index, target_index](std::ostream &stream) {
                    stream << ",\"source\":" << source_index
                           << ",\"target\":" << target_index;
                });
        }

        return 0;
    }

    if (command == "assign-section-header" || command == "assign-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "assign-section-header/footer expects an input path, a section index, "
                "and a part index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        std::size_t part_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], part_index)) {
            print_parse_error(command,
                              "invalid part index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 4U, false, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "assign-section-header"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!assign_section_part(doc, family, section_index, part_index,
                                 options.reference_kind, command,
                                 options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, section_index, part_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"section\":" << section_index
                           << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                    stream << ",\"part_index\":" << part_index;
                });
        }

        return 0;
    }

    if (command == "remove-section-header" || command == "remove-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "remove-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, false, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "remove-section-header"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!remove_section_part_reference(doc, family, section_index,
                                           options.reference_kind, command,
                                           options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, section_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"section\":" << section_index
                           << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                });
        }

        return 0;
    }

    if (command == "remove-header-part" || command == "remove-footer-part") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "remove-header/footer-part expects an input path and a "
                              "part index",
                              json_output);
            return 2;
        }

        std::size_t part_index = 0;
        if (!parse_index(arguments[2], part_index)) {
            print_parse_error(command,
                              "invalid part index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "remove-header-part"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!remove_part(doc, family, part_index, command, options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, part_index](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"part_index\":" << part_index;
                });
        }

        return 0;
    }

    if (command == "move-header-part" || command == "move-footer-part") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "move-header/footer-part expects an input path, a source index, "
                "and a target index",
                json_output);
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error(command,
                              "invalid source index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error(command,
                              "invalid target index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "move-header-part"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!move_part(doc, family, source_index, target_index, command,
                       options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, source_index, target_index](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"source\":" << source_index
                           << ",\"target\":" << target_index;
                });
        }

        return 0;
    }

    if (command == "show-section-header" || command == "show-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "show-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, false, false, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        return show_section_text(
                   doc,
                   command == "show-section-header"
                       ? section_part_family::header
                       : section_part_family::footer,
                   section_index, options.reference_kind, options.json_output)
                   ? 0
                   : 1;
    }

    if (command == "set-section-header" || command == "set-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, true, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::string replacement_text;
        if (!read_text_source(options, replacement_text, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!replace_section_text(
                doc,
                command == "set-section-header" ? section_part_family::header
                                                : section_part_family::footer,
                section_index, replacement_text, options.reference_kind, command,
                options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [command, section_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(
                        stream, command == "set-section-header" ? "header" : "footer");
                    stream << ",\"section\":" << section_index << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                });
        }

        return 0;
    }

    if (is_append_field_command(command)) {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                std::string(command) + " expects an input path and field target options",
                json_output);
            return 2;
        }

        append_field_options options;
        std::string error_message;
        if (!parse_append_field_options(command, arguments, 2U, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_mutable_template_part(doc, options.part, options.part_index,
                                          options.section_index,
                                          options.reference_kind, selected,
                                          error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!append_template_part_field(doc, selected, command, options,
                                        options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, command](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, validation_part_name(selected.family));
                    if (selected.part_index.has_value()) {
                        stream << ",\"part_index\":" << *selected.part_index;
                    }
                    if (selected.section_index.has_value()) {
                        stream << ",\"section\":" << *selected.section_index;
                    }
                    if (selected.reference_kind.has_value()) {
                        stream << ",\"kind\":";
                        write_json_string(
                            stream,
                            featherdoc::to_xml_reference_type(*selected.reference_kind));
                    }
                    stream << ",\"entry_name\":";
                    write_json_string(stream, std::string(selected.part.entry_name()));
                    stream << ",\"field\":";
                    write_json_string(stream, template_field_name(command));
                    if (options.field_index.has_value()) {
                        stream << ",\"field_index\":" << *options.field_index;
                    }
                    if (options.has_field_argument) {
                        stream << ",\"field_argument\":";
                        write_json_string(stream, options.field_argument);
                    }
                    if (options.has_result_text) {
                        stream << ",\"result_text\":";
                        write_json_string(stream, options.result_text);
                    }
                    if (command == "append-table-of-contents-field") {
                        stream << ",\"min_outline_level\":"
                               << options.min_outline_level;
                        stream << ",\"max_outline_level\":"
                               << options.max_outline_level;
                        stream << ",\"hyperlinks\":"
                               << json_bool(options.hyperlinks);
                        stream << ",\"hide_page_numbers_in_web_layout\":"
                               << json_bool(
                                      options.hide_page_numbers_in_web_layout);
                        stream << ",\"use_outline_levels\":"
                               << json_bool(options.use_outline_levels);
                    }
                    if (options.has_caption_text) {
                        stream << ",\"caption_text\":";
                        write_json_string(stream, options.caption_text);
                    }
                    if (options.has_number_result_text) {
                        stream << ",\"number_result_text\":";
                        write_json_string(stream, options.number_result_text);
                    }
                    if (options.columns.has_value()) {
                        stream << ",\"columns\":" << *options.columns;
                    }
                    if (options.restart_value.has_value()) {
                        stream << ",\"restart\":" << *options.restart_value;
                    }
                    if (options.subentry.has_value()) {
                        stream << ",\"subentry\":";
                        write_json_string(stream, *options.subentry);
                    }
                    if (options.bookmark_name.has_value()) {
                        stream << ",\"bookmark\":";
                        write_json_string(stream, *options.bookmark_name);
                    }
                    if (options.cross_reference.has_value()) {
                        stream << ",\"cross_reference\":";
                        write_json_string(stream, *options.cross_reference);
                    }
                    if (options.instruction.has_value()) {
                        stream << ",\"instruction\":";
                        write_json_string(stream, *options.instruction);
                    }
                    if (options.instruction_before.has_value()) {
                        stream << ",\"instruction_before\":";
                        write_json_string(stream, *options.instruction_before);
                    }
                    if (options.nested_instruction.has_value()) {
                        stream << ",\"nested_instruction\":";
                        write_json_string(stream, *options.nested_instruction);
                    }
                    if (!options.nested_result_text.empty()) {
                        stream << ",\"nested_result_text\":";
                        write_json_string(stream, options.nested_result_text);
                    }
                    if (options.instruction_after.has_value()) {
                        stream << ",\"instruction_after\":";
                        write_json_string(stream, *options.instruction_after);
                    }
                });
        }

        return 0;
    }

    if (command == "validate-template") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template expects an input path and validation options",
                json_output);
            return 2;
        }

        validate_template_options options;
        std::string error_message;
        if (!parse_validate_template_options(arguments, 2U, options, error_message)) {
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

        const auto result = selected.part.validate_template(options.requirements);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_validation_result(selected, result, options.json_output);
        return 0;
    }

    if (command == "validate-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template-schema expects an input path and schema validation options",
                json_output);
            return 2;
        }

        validate_template_schema_options options;
        std::string error_message;
        if (!parse_validate_template_schema_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        for (const auto &schema_file : options.schema_files) {
            if (!append_validate_template_schema_file_targets(
                    schema_file, options.targets, error_message)) {
                print_parse_error(command, error_message, json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::template_schema schema;
        std::size_t entry_count = 0U;
        for (const auto &target : options.targets) {
            entry_count += target.requirements.size();
        }
        schema.entries.reserve(entry_count);

        for (const auto &target : options.targets) {
            featherdoc::template_schema_part_selector selector{};
            selector.part = to_template_schema_part_kind(target.part);
            selector.part_index = target.part_index;
            selector.section_index = target.section_index;
            selector.reference_kind = target.reference_kind;

            for (const auto &requirement : target.requirements) {
                schema.entries.push_back({selector, requirement});
            }
        }

        const auto result = doc.validate_template_schema(schema);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_schema_validation_result(result, options.json_output);
        return 0;
    }

    if (command == "export-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "export-template-schema expects an input path",
                              json_output);
            return 2;
        }

        export_template_schema_options options;
        std::string error_message;
        if (!parse_export_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result result;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets, result,
                command, options.json_output)) {
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write template schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_template_schema_summary(result, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "normalize-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "normalize-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        normalize_template_schema_options options;
        std::string error_message;
        if (!parse_normalize_template_schema_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write normalized schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_normalized_template_schema_summary(result, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "lint-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "lint-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_template_schema(result);
        print_linted_template_schema_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "repair-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "repair-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        repair_template_schema_options options;
        std::string error_message;
        if (!parse_repair_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result input;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), input,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result result;
        repaired_template_schema_summary summary{};
        repair_template_schema_result(input, result, summary);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write repaired schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_repaired_template_schema_summary(result, summary, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "merge-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "merge-template-schema expects at least two schema paths",
                              json_output);
            return 2;
        }

        merge_template_schema_options options;
        std::string error_message;
        if (!parse_merge_template_schema_options(arguments, 1U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        merged_template_schema_summary summary;
        for (const auto &schema_path : options.schema_paths) {
            exported_template_schema_result input;
            if (!read_template_schema_file(schema_path, input, error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            merge_template_schema_result(result, input, summary);
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write merged schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_merged_template_schema_summary(result, summary, options.output_path,
                                             options.json_output);
        return 0;
    }

    if (command == "patch-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "patch-template-schema expects a schema path and "
                              "--patch-file <path>",
                              json_output);
            return 2;
        }

        patch_template_schema_options options;
        std::string error_message;
        if (!parse_patch_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        template_schema_patch_document patch;
        if (!read_template_schema_patch_file(*options.patch_path, patch,
                                             error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        patched_template_schema_summary summary;
        apply_template_schema_patch(result, patch, summary);
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write patched schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_patched_template_schema_summary(result, summary, options.output_path,
                                              options.json_output);
        return 0;
    }

    if (command == "preview-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "preview-template-schema-patch expects a schema path "
                              "and --patch-file <path> or <right-schema.json>",
                              json_output);
            return 2;
        }

        preview_template_schema_patch_options options;
        std::string error_message;
        if (!parse_preview_template_schema_patch_options(arguments, 2U, options,
                                                         error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left_result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])),
                                       left_result, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(left_result);
        if (has_template_schema_resolved_target_metadata(left_result)) {
            print_parse_error(command,
                              "preview-template-schema-patch does not support "
                              "schemas with resolved-section target metadata",
                              options.json_output);
            return 2;
        }

        previewed_template_schema_patch_summary summary{};
        summary.left_slot_count = left_result.slot_count();
        summary.review_json_path = options.review_json_path;
        const auto left_schema = to_template_schema(left_result);

        if (options.patch_path.has_value()) {
            template_schema_patch_document patch_document;
            if (!read_template_schema_patch_file(*options.patch_path, patch_document,
                                                 error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            if (has_template_schema_resolved_target_metadata(patch_document)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "patches with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                const auto review =
                    featherdoc::make_template_schema_patch_review_summary(
                        left_schema, patch);
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        } else {
            exported_template_schema_result right_result;
            if (!read_template_schema_file(*options.right_schema_path, right_result,
                                           error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            normalize_template_schema_result(right_result);
            if (has_template_schema_resolved_target_metadata(right_result)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "schemas with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            summary.right_slot_count = right_result.slot_count();
            const auto diff = diff_template_schema_results(left_result, right_result);
            built_template_schema_patch_summary built_summary{};
            const auto patch_document =
                build_template_schema_patch_document(diff, built_summary);
            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                auto review = featherdoc::make_template_schema_patch_review_summary(
                    left_schema, patch);
                review.generated_slot_count = right_result.slot_count();
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed generated patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        }

        summary.output_patch_path = options.output_patch_path;
        print_previewed_template_schema_patch_summary(summary, options.json_output);
        return 0;
    }

    if (command == "build-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--") ||
            arguments[2].starts_with("--")) {
            print_parse_error(
                command,
                "build-template-schema-patch expects left and right schema paths",
                json_output);
            return 2;
        }

        build_template_schema_patch_options options;
        std::string error_message;
        if (!parse_build_template_schema_patch_options(arguments, 3U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto diff = diff_template_schema_results(left, right);
        built_template_schema_patch_summary summary{};
        const auto patch = build_template_schema_patch_document(diff, summary);

        if (options.review_json_path.has_value()) {
            const auto left_schema = to_template_schema(left);
            const auto patch_model = to_template_schema_patch(patch);
            auto review = featherdoc::make_template_schema_patch_review_summary(
                left_schema, patch_model);
            review.generated_slot_count = right.slot_count();
            if (!write_template_schema_patch_review_file(
                    *options.review_json_path, review, error_message)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::io_error);
                error_info.detail = std::move(error_message);
                report_operation_failure(
                    command, "review-json",
                    "failed to write schema patch review output", error_info,
                    options.json_output);
                return 1;
            }
        }

        if (!options.output_path.has_value()) {
            write_json_template_schema_patch_document(std::cout, patch);
            return 0;
        }

        if (!write_template_schema_patch_file(*options.output_path, patch,
                                              error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated patch output",
                                     error_info, options.json_output);
            return 1;
        }

        print_built_template_schema_patch_summary(
            summary, options.output_path, options.review_json_path,
            options.json_output);
        return 0;
    }

    if (command == "diff-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "diff-template-schema expects left and right schema paths",
                              json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto result = diff_template_schema_results(left, right);
        print_template_schema_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "check-template-schema expects an input path and --schema-file <path>",
                json_output);
            return 2;
        }

        check_template_schema_options options;
        std::string error_message;
        if (!parse_check_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result baseline;
        if (!read_template_schema_file(*options.schema_path, baseline, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result generated;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets,
                generated, command, options.json_output)) {
            return 1;
        }
        normalize_template_schema_result(generated);

        if (options.output_path.has_value() &&
            !write_exported_template_schema_file(*options.output_path, generated,
                                                error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated schema output",
                                     error_info, options.json_output);
            return 1;
        }

        const auto result = diff_template_schema_results(baseline, generated);
        print_checked_template_schema_result(*options.schema_path, result,
                                             options.output_path,
                                             options.json_output);
        if (!result.equal()) {
            return 1;
        }
        return 0;
    }

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}
