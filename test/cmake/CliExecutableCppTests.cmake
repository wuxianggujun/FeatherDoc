if(TARGET featherdoc_cli)
    function(featherdoc_add_cli_cpp_test target_name test_name)
        add_executable(${target_name} ${ARGN})

        target_link_libraries(
            ${target_name}
            PRIVATE FeatherDoc::FeatherDoc
        )
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /bigobj /utf-8)
        endif()
        featherdoc_copy_runtime_dlls(${target_name})

        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:featherdoc_cli>
                $<TARGET_FILE_DIR:${target_name}>
        )
        if(WIN32 AND BUILD_SHARED_LIBS)
            add_custom_command(
                TARGET ${target_name}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:featherdoc_cli>
                    $<TARGET_FILE_DIR:${target_name}>
                COMMAND_EXPAND_LISTS
            )
        endif()

        add_dependencies(${target_name} featherdoc_cli)
        add_test(
            NAME
            ${test_name}
            COMMAND
            ${target_name}
        )
    endfunction()

    featherdoc_add_cli_cpp_test(
        cli_tests
        cli
        cli_tests.cpp
    )
    featherdoc_set_test_labels(cli cli heavy)

    featherdoc_add_cli_cpp_test(
        cli_header_footer_tests
        cli_header_footer
        cli_header_footer_tests.cpp
    )
    featherdoc_set_test_labels(cli_header_footer cli header footer heavy)

    featherdoc_add_cli_cpp_test(
        cli_numbering_catalog_tests
        cli_numbering_catalog
        cli_numbering_catalog_tests.cpp
        cli_numbering_catalog_patch_lint_diff_tests.cpp
        cli_numbering_catalog_check_tests.cpp
        cli_numbering_catalog_custom_command_tests.cpp
    )
    featherdoc_set_test_labels(cli_numbering_catalog cli numbering heavy)

    featherdoc_add_cli_cpp_test(
        cli_style_numbering_tests
        cli_style_numbering
        cli_style_numbering_tests.cpp
        cli_style_numbering_linked_tests.cpp
        cli_style_numbering_error_tests.cpp
    )
    featherdoc_set_test_labels(cli_style_numbering cli styles numbering heavy)

    featherdoc_add_cli_cpp_test(
        cli_style_inheritance_tests
        cli_style_inheritance
        cli_style_inheritance_tests.cpp
    )
    featherdoc_set_test_labels(cli_style_inheritance cli styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_style_properties_tests
        cli_style_properties
        cli_style_properties_tests.cpp
    )
    featherdoc_set_test_labels(cli_style_properties cli styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_paragraph_run_tests
        cli_paragraph_run
        cli_paragraph_run_tests.cpp
    )
    featherdoc_set_test_labels(cli_paragraph_run cli paragraph run styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_run_properties_tests
        cli_run_properties
        cli_run_properties_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_run_properties cli run styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_page_setup_tests
        cli_page_setup
        cli_page_setup_tests.cpp
    )
    featherdoc_set_test_labels(cli_page_setup cli page_setup heavy)

    featherdoc_add_cli_cpp_test(
        cli_content_controls_tests
        cli_content_controls
        cli_content_controls_tests.cpp
    )
    featherdoc_set_test_labels(cli_content_controls cli content_controls heavy)

    featherdoc_add_cli_cpp_test(
        cli_semantic_diff_tests
        cli_semantic_diff
        cli_semantic_diff_tests.cpp
        cli_semantic_diff_alignment_tests.cpp
    )
    featherdoc_set_test_labels(cli_semantic_diff cli semantic_diff heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_tests
        cli_template_table
        cli_template_table_tests.cpp
    )
    featherdoc_set_test_labels(cli_template_table cli template table heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_selector_tests
        cli_template_table_selector
        cli_template_table_selector_tests.cpp
        cli_template_table_selector_text_mutation_tests.cpp
        cli_template_table_selector_structure_command_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_selector cli template table selector heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_bookmark_tests
        cli_template_table_bookmark
        cli_template_table_bookmark_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_bookmark cli template table bookmark json heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_merge_selector_tests
        cli_template_table_merge_selector
        cli_template_table_merge_selector_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_merge_selector cli template table merge selector heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_section_kind_tests
        cli_template_table_section_kind
        cli_template_table_section_kind_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_section_kind cli template table section heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_row_tests
        cli_template_table_row
        cli_template_table_row_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_row cli template table rows heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_table_column_tests
        cli_template_table_column
        cli_template_table_column_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_table_column cli template table columns heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_tests
        cli_table
        cli_table_tests.cpp
    )
    featherdoc_set_test_labels(cli_table cli table heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_position_tests
        cli_table_position
        cli_table_position_tests.cpp
        cli_table_position_command_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_position cli table position heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_cell_content_tests
        cli_table_cell_content
        cli_table_cell_content_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_cell_content cli table cells content merge heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_border_margin_tests
        cli_table_border_margin
        cli_table_border_margin_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_border_margin cli table borders margins heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_geometry_tests
        cli_table_geometry
        cli_table_geometry_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_geometry cli table geometry heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_cell_style_tests
        cli_table_cell_style
        cli_table_cell_style_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_cell_style cli table cells styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_table_row_structure_tests
        cli_table_row_structure
        cli_table_row_structure_tests.cpp
        cli_table_row_property_tests.cpp
        cli_table_row_table_structure_tests.cpp
        cli_table_column_structure_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_table_row_structure cli table rows structure heavy)

    featherdoc_add_cli_cpp_test(
        cli_review_tests
        cli_review
        cli_review_tests.cpp
    )
    featherdoc_set_test_labels(cli_review cli review heavy)

    featherdoc_add_cli_cpp_test(
        cli_review_revision_tests
        cli_review_revision
        cli_review_revision_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_review_revision cli review revision heavy)

    featherdoc_add_cli_cpp_test(
        cli_review_mutation_plan_tests
        cli_review_mutation_plan
        cli_review_mutation_plan_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_review_mutation_plan cli review mutation_plan heavy)

    featherdoc_add_cli_cpp_test(
        cli_review_comment_mutation_plan_tests
        cli_review_comment_mutation_plan
        cli_review_comment_mutation_plan_tests.cpp
        cli_review_comment_mutation_metadata_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_review_comment_mutation_plan cli review mutation_plan comments heavy)

    featherdoc_add_cli_cpp_test(
        cli_bookmark_content_tests
        cli_bookmark_content
        cli_bookmark_content_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_bookmark_content cli bookmark content_controls images heavy)

    featherdoc_add_cli_cpp_test(
        cli_bookmark_table_tests
        cli_bookmark_tables
        cli_bookmark_table_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_bookmark_tables cli bookmark tables heavy)

    featherdoc_add_cli_cpp_test(
        cli_bookmark_image_tests
        cli_bookmark_images
        cli_bookmark_image_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_bookmark_images cli bookmark images heavy)

    featherdoc_add_cli_cpp_test(
        cli_content_control_tests
        cli_content_control_mutation
        cli_content_control_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_content_control_mutation cli content_controls heavy)

    featherdoc_add_cli_cpp_test(
        cli_field_tests
        cli_fields
        cli_field_tests.cpp
    )
    featherdoc_set_test_labels(cli_fields cli fields heavy)

    featherdoc_add_cli_cpp_test(
        cli_image_tests
        cli_images
        cli_image_tests.cpp
    )
    featherdoc_set_test_labels(cli_images cli images heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_tests
        cli_template_schema
        cli_template_schema_tests.cpp
    )
    featherdoc_set_test_labels(cli_template_schema cli template schema heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_patch_build_tests
        cli_template_schema_patch_build
        cli_template_schema_patch_build_tests.cpp
        cli_template_schema_patch_build_edge_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_patch_build cli template schema patch heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_patch_tests
        cli_template_schema_patch
        cli_template_schema_patch_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_patch cli template schema patch heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_diff_check_tests
        cli_template_schema_diff_check
        cli_template_schema_diff_check_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_diff_check cli template schema diff check heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_maintenance_tests
        cli_template_schema_maintenance
        cli_template_schema_maintenance_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_maintenance cli template schema maintenance heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_export_tests
        cli_template_schema_export
        cli_template_schema_export_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_export cli template schema export heavy)

    featherdoc_add_cli_cpp_test(
        cli_template_schema_validate_schema_tests
        cli_template_schema_validate_schema
        cli_template_schema_validate_schema_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_template_schema_validate_schema cli template schema validate heavy)

    if(NOT FEATHERDOC_BUILD_PDF OR NOT TARGET FeatherDocPdf)
        featherdoc_add_cli_cpp_test(
            cli_pdf_disabled_tests
            cli_pdf_disabled
            cli_pdf_disabled_tests.cpp
        )
        featherdoc_set_test_labels(cli_pdf_disabled cli smoke pdf heavy)
    endif()

    featherdoc_add_cli_cpp_test(
        cli_styles_tests
        cli_styles
        cli_styles_tests.cpp
    )
    featherdoc_set_test_labels(cli_styles cli styles heavy)

    featherdoc_add_cli_cpp_test(
        cli_style_refactor_workflow_tests
        cli_style_refactor_workflow
        cli_style_refactor_workflow_tests.cpp
        cli_style_refactor_apply_workflow_tests.cpp
    )
    featherdoc_set_test_labels(
        cli_style_refactor_workflow cli styles heavy)
endif()
