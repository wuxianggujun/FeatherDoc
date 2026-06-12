featherdoc_add_cpp_test(
    cli_json_tests
    cli_json
    cli_json_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
)
target_include_directories(cli_json_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_json cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_json_parse_tests
    cli_json_parse
    cli_json_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(cli_json_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_json_parse cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_table_position_plan_parse_tests
    cli_table_position_plan_parse
    cli_table_position_plan_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_table_position_plan_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_table_position_plan_parse cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_table_position_plan_build_tests
    cli_table_position_plan_build
    cli_table_position_plan_build_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_build.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_text.cpp
)
target_include_directories(
    cli_table_position_plan_build_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_table_position_plan_build cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_table_position_options_parse_tests
    cli_table_position_options_parse
    cli_table_position_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_build.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_set_anchor_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_set_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_set_preset_defaults.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_position_set_spacing_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_text.cpp
)
target_include_directories(
    cli_table_position_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_table_position_options_parse cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_style_refactor_plan_parse_tests
    cli_style_refactor_plan_parse
    cli_style_refactor_plan_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan_file_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan_operations_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_style_refactor_plan_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_style_refactor_plan_parse cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_style_refactor_options_parse_tests
    cli_style_refactor_options_parse
    cli_style_refactor_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_merge_restore_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_merge_suggestion_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_apply_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_pair_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_style_refactor_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_style_refactor_options_parse cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_style_refactor_plan_tests
    cli_style_refactor_plan
    cli_style_refactor_plan_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan.cpp
)
target_include_directories(
    cli_style_refactor_plan_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_style_refactor_plan cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_style_refactor_rollback_parse_tests
    cli_style_refactor_rollback_parse
    cli_style_refactor_rollback_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_rollback_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_refactor_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_style_refactor_rollback_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_style_refactor_rollback_parse cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_review_mutation_plan_parse_tests
    cli_review_mutation_plan_parse
    cli_review_mutation_plan_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_build_request_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_build_request_operation_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_build_request_operation_member_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_build_request_operation_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_build_request_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_kind_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_operation_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_review_mutation_plan_write.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_review_mutation_plan_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_review_mutation_plan_parse cli smoke cli_smoke review)

featherdoc_add_cpp_test(
    cli_parse_tests
    cli_parse
    cli_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(cli_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_parse cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_page_setup_parse_tests
    cli_page_setup_parse
    cli_page_setup_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_page_setup_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_page_setup_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_page_setup_parse cli smoke cli_smoke page_setup)

featherdoc_add_cpp_test(
    cli_section_options_parse_tests
    cli_section_options_parse
    cli_section_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_section_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_section_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_section_options_parse cli smoke cli_smoke fields sections)

featherdoc_add_cpp_test(
    cli_paragraph_list_options_parse_tests
    cli_paragraph_list_options_parse
    cli_paragraph_list_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_paragraph_list_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_paragraph_list_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_paragraph_list_options_parse cli smoke cli_smoke numbering paragraphs)

featherdoc_add_cpp_test(
    cli_inspect_style_options_parse_tests
    cli_inspect_style_options_parse
    cli_inspect_style_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_style_audit_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_style_quality_fixes_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_style_repair_numbering_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_style_options_parse.cpp
)
target_include_directories(
    cli_inspect_style_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_inspect_style_options_parse cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_style_options_parse_tests
    cli_style_options_parse
    cli_style_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_options_parse.cpp
)
target_include_directories(
    cli_style_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_style_options_parse cli smoke cli_smoke styles)

featherdoc_add_cpp_test(
    cli_run_recipe_parse_tests
    cli_run_recipe_parse
    cli_run_recipe_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_recipe_inputs_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_recipe_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_recipe_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_recipe_recipe_id_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_run_recipe_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_run_recipe_parse cli smoke cli_smoke review)

featherdoc_add_cpp_test(
    cli_pdf_parse_tests
    cli_pdf_parse
    cli_pdf_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_pdf_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(cli_pdf_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_pdf_parse cli smoke cli_smoke pdf)

featherdoc_add_cpp_test(
    cli_domain_parse_tests
    cli_domain_parse
    cli_domain_parse_tests.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(cli_domain_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_domain_parse cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_template_slot_parse_tests
    cli_template_slot_parse
    cli_template_slot_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_slot_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_template_slot_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_template_slot_parse cli smoke cli_smoke template)

featherdoc_add_cpp_test(
    cli_validation_part_parse_tests
    cli_validation_part_parse
    cli_validation_part_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_validation_part_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_validation_part_parse cli smoke cli_smoke template)

featherdoc_add_cpp_test(
    cli_field_parse_tests
    cli_field_parse
    cli_field_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_field_complex_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_field_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(cli_field_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_field_parse cli smoke cli_smoke fields)

featherdoc_add_cpp_test(
    cli_template_schema_parse_tests
    cli_template_schema_parse
    cli_template_schema_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse_target.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse_target_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_slot_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_template_schema_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_schema_parse cli smoke cli_smoke template schema)

featherdoc_add_cpp_test(
    cli_template_schema_options_parse_tests
    cli_template_schema_options_parse
    cli_template_schema_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_diff_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_export_check_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_maintenance_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_options_parse.cpp
)
target_include_directories(
    cli_template_schema_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_schema_options_parse cli smoke cli_smoke template schema)

featherdoc_add_cpp_test(
    cli_template_validation_options_parse_tests
    cli_template_validation_options_parse
    cli_template_validation_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_validation_schema_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_validation_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_slot_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_template_validation_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_validation_options_parse cli smoke cli_smoke template validation)

featherdoc_add_cpp_test(
    cli_template_schema_ops_tests
    cli_template_schema_ops
    cli_template_schema_ops_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_ops.cpp
)
target_include_directories(
    cli_template_schema_ops_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_schema_ops cli smoke cli_smoke template schema)

featherdoc_add_cpp_test(
    cli_template_schema_patch_ops_tests
    cli_template_schema_patch_ops
    cli_template_schema_patch_ops_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_ops.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_apply_ops.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_diff_ops.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_repair_ops.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_ops.cpp
)
target_include_directories(
    cli_template_schema_patch_ops_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_schema_patch_ops cli smoke cli_smoke template schema)

featherdoc_add_cpp_test(
    cli_template_schema_patch_parse_tests
    cli_template_schema_patch_parse
    cli_template_schema_patch_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_detail.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_target_selector.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_target_selectors_array.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_rename_slot_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_rename_slot_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_rename_slot_members.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_rename_slot.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_remove_slot_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_remove_slot_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_remove_slot_members.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_remove_slot.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_slot_mutation.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_update_slot_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_update_slot_finalize_detail.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_update_slot_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_patch_parse_update_slot_member_helpers.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse_target.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_parse_target_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse_member.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_schema_slot_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_slot_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_template_schema_patch_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_schema_patch_parse cli smoke cli_smoke template schema)

featherdoc_add_cpp_test(
    cli_numbering_catalog_parse_tests
    cli_numbering_catalog_parse
    cli_numbering_catalog_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_definition_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_instance_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_level_override_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_level_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_parse_support.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_numbering_catalog_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_parse cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_numbering_catalog_options_parse_tests
    cli_numbering_catalog_options_parse
    cli_numbering_catalog_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_options_parse.cpp
)
target_include_directories(
    cli_numbering_catalog_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_options_parse cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_inspect_content_options_parse_tests
    cli_inspect_content_options_parse
    cli_inspect_content_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_content_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_inspect_content_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_inspect_content_options_parse cli smoke cli_smoke paragraphs table)

featherdoc_add_cpp_test(
    cli_image_options_parse_tests
    cli_image_options_parse
    cli_image_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_crop.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_flag.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_layout.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_image_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_image_options_parse cli smoke cli_smoke images)

featherdoc_add_cpp_test(
    cli_bookmark_text_options_parse_tests
    cli_bookmark_text_options_parse
    cli_bookmark_text_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_form_state_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_form_state_options_parse_state.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_form_state_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_rich_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_sync_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_content_control_text_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_image_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_block_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_paragraph_replace_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_table_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_text_fill_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_text_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_text_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_text_replace_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_visibility_apply_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_bookmark_visibility_set_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_crop.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_flag.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_layout.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_floating_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_append_options_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_image_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_selector_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_bookmark_text_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_bookmark_text_options_parse cli smoke cli_smoke bookmarks content_controls images)

featherdoc_add_cpp_test(
    cli_paragraph_run_options_parse_tests
    cli_paragraph_run_options_parse
    cli_paragraph_run_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_paragraph_run_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_paragraph_run_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_paragraph_run_options_parse cli smoke cli_smoke paragraphs run styles)

featherdoc_add_cpp_test(
    cli_style_ensure_options_parse_tests
    cli_style_ensure_options_parse
    cli_style_ensure_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_common_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_table_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_table_region_basic_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_table_region_box_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_table_region_paragraph_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_style_ensure_table_region_parse_support.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_style_ensure_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_style_ensure_options_parse cli smoke cli_smoke styles table)

featherdoc_add_cpp_test(
    cli_table_cell_options_parse_tests
    cli_table_cell_options_parse
    cli_table_cell_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_append_table_row_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_merge_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_style_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_text_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_table_cell_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_table_cell_options_parse cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_run_properties_options_parse_tests
    cli_run_properties_options_parse
    cli_run_properties_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_paragraph_style_properties_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_properties_clear_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_properties_common_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_properties_mutation_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_run_properties_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_run_properties_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_run_properties_options_parse cli smoke cli_smoke styles run)

featherdoc_add_cpp_test(
    cli_semantic_diff_options_parse_tests
    cli_semantic_diff_options_parse
    cli_semantic_diff_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_semantic_diff_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_semantic_diff_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_semantic_diff_options_parse cli smoke cli_smoke semantic_diff)

featherdoc_add_cpp_test(
    cli_document_mutation_options_parse_tests
    cli_document_mutation_options_parse
    cli_document_mutation_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_mutation_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_revision_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_review_comment_metadata_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_review_comment_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_review_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_document_rich_content_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_document_mutation_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_document_mutation_options_parse cli smoke cli_smoke documents mutations)

featherdoc_add_cpp_test(
    cli_template_inspect_options_parse_tests
    cli_template_inspect_options_parse
    cli_template_inspect_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_inspect_table_cells_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_inspect_table_rows_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_inspect_tables_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_selector_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_template_inspect_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_inspect_options_parse cli smoke cli_smoke template)

featherdoc_add_cpp_test(
    cli_template_table_options_parse_tests
    cli_template_table_options_parse
    cli_template_table_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_batch_operation_finalize.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_batch_operation_member_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_batch_operation_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_batch_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_patch_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_patch_parse_detail.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_patch_parse_values.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_json_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_row_target_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_cell_target_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_optional_cell_target_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_target_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_selector_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
)
target_include_directories(
    cli_template_table_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_table_options_parse cli smoke cli_smoke template table)

featherdoc_add_cpp_test(
    cli_template_table_mutation_options_parse_tests
    cli_template_table_mutation_options_parse
    cli_template_table_mutation_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_merge_mutation_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_mutation_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_row_mutation_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_append_table_row_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_merge_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_style_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_cell_text_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_inspect_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_template_table_selector_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_validation_part.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_template_table_mutation_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_template_table_mutation_options_parse cli smoke cli_smoke template table)

featherdoc_add_cpp_test(
    cli_inspect_table_item_options_parse_tests
    cli_inspect_table_item_options_parse
    cli_inspect_table_item_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_table_item_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_inspect_table_item_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_inspect_table_item_options_parse cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_table_style_look_options_parse_tests
    cli_table_style_look_options_parse
    cli_table_style_look_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_table_style_look_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_table_style_look_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_table_style_look_options_parse cli smoke cli_smoke table)

featherdoc_add_cpp_test(
    cli_inspect_numbering_options_parse_tests
    cli_inspect_numbering_options_parse
    cli_inspect_numbering_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_inspect_numbering_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_inspect_numbering_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_inspect_numbering_options_parse cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_numbering_options_parse_tests
    cli_numbering_options_parse
    cli_numbering_options_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_definition_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_level_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_paragraph_numbering_options_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_paragraph_style_numbering_options_parse.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_numbering_options_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_options_parse cli smoke cli_smoke numbering styles)

featherdoc_add_cpp_test(
    cli_numbering_catalog_patch_parse_tests
    cli_numbering_catalog_patch_parse
    cli_numbering_catalog_patch_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_level_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_override_entry_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_override_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_parse_support.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_definition_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_instance_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_level_override_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_level_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_parse_support.cpp
    ${FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES}
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(
    cli_numbering_catalog_patch_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_patch_parse cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_numbering_catalog_patch_apply_tests
    cli_numbering_catalog_patch_apply
    cli_numbering_catalog_patch_apply_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_patch_apply.cpp
)
target_include_directories(
    cli_numbering_catalog_patch_apply_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_patch_apply cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_numbering_catalog_lint_tests
    cli_numbering_catalog_lint
    cli_numbering_catalog_lint_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_lint.cpp
)
target_include_directories(
    cli_numbering_catalog_lint_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_lint cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_numbering_catalog_diff_tests
    cli_numbering_catalog_diff
    cli_numbering_catalog_diff_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_numbering_catalog_diff.cpp
)
target_include_directories(
    cli_numbering_catalog_diff_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(
    cli_numbering_catalog_diff cli smoke cli_smoke numbering)

featherdoc_add_cpp_test(
    cli_domain_name_tests
    cli_domain_name
    cli_domain_name_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names_core.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names_table.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names_table_style_position.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names_template_image.cpp
)
target_include_directories(cli_domain_name_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_domain_name cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_text_tests
    cli_text
    cli_text_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_text.cpp
)
target_include_directories(cli_text_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_text cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_errors_tests
    cli_errors
    cli_errors_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_errors.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_usage.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_usage_table.cpp
)
target_include_directories(cli_errors_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_errors cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_usage_tests
    cli_usage
    cli_usage_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_usage.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_usage_table.cpp
)
target_include_directories(cli_usage_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
if(TARGET FeatherDocPdfImport)
    target_compile_definitions(
        cli_usage_tests
        PRIVATE FEATHERDOC_CLI_ENABLE_PDF_IMPORT=1
    )
endif()
featherdoc_set_test_labels(cli_usage cli smoke cli_smoke)
