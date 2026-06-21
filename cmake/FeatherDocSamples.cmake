# Sample, probe, and sample-related test targets for FeatherDoc.

function(featherdoc_add_executable_with_runtime_dlls target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE ${ARGN})
    featherdoc_copy_runtime_dlls(${target_name})
endfunction()

if(BUILD_SAMPLES)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample1
        samples/sample1.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample2
        samples/sample2.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_chinese
        samples/sample_chinese.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_chinese_template
        samples/sample_chinese_template.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_visual_smoke_tables
        samples/visual_smoke_tables.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_section_inspection_visual
        samples/section_inspection_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing
        samples/sample_edit_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_paragraph_before
        samples/sample_insert_paragraph_before.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_paragraph_like_existing
        samples/sample_insert_paragraph_like_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_run_around_existing
        samples/sample_insert_run_around_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_run_like_existing
        samples/sample_insert_run_like_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_part_tables
        samples/sample_edit_existing_part_tables.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_visual
        samples/sample_template_table_cli_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_bookmark_visual
        samples/sample_template_table_cli_bookmark_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_bookmark_multiline_visual
        samples/sample_template_bookmark_multiline_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_bookmark_block_visibility_visual
        samples/sample_bookmark_block_visibility_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_bookmark_table_replacement_visual
        samples/sample_bookmark_table_replacement_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_content_control_rich_replacement_visual
        samples/sample_content_control_rich_replacement_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_content_control_image_replacement_visual
        samples/sample_content_control_image_replacement_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_semantic_diff_visual
        samples/sample_semantic_diff_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_bookmark_image_visual
        samples/sample_bookmark_image_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_bookmark_floating_image_visual
        samples/sample_bookmark_floating_image_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_fill_bookmarks_visual
        samples/sample_fill_bookmarks_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_append_image_visual
        samples/sample_append_image_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_row_visual
        samples/sample_table_row_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_paragraph_run_style_visual
        samples/sample_paragraph_run_style_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_run_font_language_visual
        samples/sample_run_font_language_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_paragraph_list_visual
        samples/sample_paragraph_list_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_paragraph_numbering_visual
        samples/sample_paragraph_numbering_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_paragraph_style_numbering_visual
        samples/sample_paragraph_style_numbering_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_ensure_style_visual
        samples/sample_ensure_style_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_merge_visual
        samples/sample_table_cell_merge_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_fill_visual
        samples/sample_table_cell_fill_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_border_visual
        samples/sample_table_cell_border_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_text_direction_visual
        samples/sample_table_cell_text_direction_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_vertical_alignment_visual
        samples/sample_table_cell_vertical_alignment_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_width_visual
        samples/sample_table_cell_width_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_cell_margin_visual
        samples/sample_table_cell_margin_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_row_height_visual
        samples/sample_table_row_height_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_row_repeat_header_visual
        samples/sample_table_row_repeat_header_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_table_row_cant_split_visual
        samples/sample_table_row_cant_split_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_bookmark_paragraphs_pagination_visual
        samples/sample_template_bookmark_paragraphs_pagination_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_section_text_multiline_visual
        samples/sample_section_text_multiline_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_section_part_refs_visual
        samples/sample_section_part_refs_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_section_order_visual
        samples/sample_section_order_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_visual
        samples/sample_template_table_cli_section_kind_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_row_visual
        samples/sample_template_table_cli_section_kind_row_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_column_visual
        samples/sample_template_table_cli_section_kind_column_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_merge_unmerge_visual
        samples/sample_template_table_cli_section_kind_merge_unmerge_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_merge_unmerge_visual
        samples/sample_template_table_cli_merge_unmerge_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_column_visual
        samples/sample_template_table_cli_column_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_direct_column_visual
        samples/sample_template_table_cli_direct_column_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_direct_visual
        samples/sample_template_table_cli_direct_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_table_cli_direct_merge_unmerge_visual
        samples/sample_template_table_cli_direct_merge_unmerge_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_part_paragraphs
        samples/sample_edit_existing_part_paragraphs.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_part_images
        samples/sample_edit_existing_part_images.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_part_append_images
        samples/sample_edit_existing_part_append_images.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_floating_images
        samples/sample_floating_images.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_floating_image_z_order_visual
        samples/sample_floating_image_z_order_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_remove_images
        samples/sample_remove_images.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_remove_bookmark_block
        samples/sample_remove_bookmark_block.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_remove_table
        samples/sample_remove_table.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_remove_table_column
        samples/sample_remove_table_column.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_table_column
        samples/sample_insert_table_column.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_unmerge_table_cells
        samples/sample_unmerge_table_cells.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_merge_right_fixed_grid
        samples/sample_merge_right_fixed_grid.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_merge_down_fixed_grid
        samples/sample_merge_down_fixed_grid.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_unmerge_right_fixed_grid
        samples/sample_unmerge_right_fixed_grid.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_unmerge_down_fixed_grid
        samples/sample_unmerge_down_fixed_grid.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_table_around_existing
        samples/sample_insert_table_around_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_table_like_existing
        samples/sample_insert_table_like_existing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_table_spacing
        samples/sample_edit_existing_table_spacing.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_table_column_widths
        samples/sample_edit_existing_table_column_widths.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_edit_existing_table_style_look
        samples/sample_edit_existing_table_style_look.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_restart_paragraph_list
        samples/sample_restart_paragraph_list.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_style_linked_numbering
        samples/sample_style_linked_numbering.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_section_page_setup
        samples/sample_section_page_setup.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_page_number_fields
        samples/sample_page_number_fields.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_generic_fields_visual
        samples/sample_generic_fields_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_hyperlinks_visual
        samples/sample_hyperlinks_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_omml_visual
        samples/sample_omml_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_extended_image_formats_visual
        samples/sample_extended_image_formats_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_review_inspection_visual
        samples/sample_review_inspection_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_review_mutation_visual
        samples/sample_review_mutation_visual.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_validation
        samples/sample_template_validation.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_part_template_validation
        samples/sample_part_template_validation.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_schema_validation
        samples/sample_template_schema_validation.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_template_schema_manifest_fixture
        samples/sample_template_schema_manifest_fixture.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_contract_template
        samples/sample_contract_template.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_tender_template
        samples/sample_tender_template.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_table_row
        samples/sample_insert_table_row.cpp
        FeatherDoc::FeatherDoc)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_sample_insert_table_row_before
        samples/sample_insert_table_row_before.cpp
        FeatherDoc::FeatherDoc)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/samples/my_test.docx
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/samples/chinese_invoice_template.docx
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_SAMPLES AND FEATHERDOC_BUILD_PDF)
    featherdoc_collect_pdf_runtime_environment(
        FEATHERDOC_PDF_RUNTIME_ENVIRONMENT FALSE)

    featherdoc_add_executable_with_runtime_dlls(featherdoc_pdfio_probe
        samples/pdfio_probe.cpp
        FeatherDoc::Pdf)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_pdf_document_probe
        samples/pdf_document_probe.cpp
        FeatherDoc::Pdf)

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
        add_test(
            NAME pdfio_generator_probe
            COMMAND featherdoc_pdfio_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-pdfio-probe.pdf"
        )
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfio_generator_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        add_test(
            NAME pdf_document_generator_probe
            COMMAND featherdoc_pdf_document_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-document-probe.pdf"
        )
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdf_document_generator_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
    endif()
endif()

if(BUILD_SAMPLES AND FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc_add_executable_with_runtime_dlls(featherdoc_pdfium_probe
        samples/pdfium_probe.cpp
        FeatherDoc::PdfImport)
    if(DEFINED FEATHERDOC_PDFIUM_TARGET AND NOT FEATHERDOC_PDFIUM_TARGET STREQUAL "")
        target_link_libraries(featherdoc_pdfium_probe PRIVATE
            ${FEATHERDOC_PDFIUM_TARGET})
    endif()
    if(DEFINED FEATHERDOC_PDFIUM_SOURCE_DIR AND
       NOT FEATHERDOC_PDFIUM_SOURCE_DIR STREQUAL "")
        target_include_directories(featherdoc_pdfium_probe PRIVATE
            "${FEATHERDOC_PDFIUM_SOURCE_DIR}/public")
    endif()

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND TARGET featherdoc_pdfio_probe)
        add_test(
            NAME pdfium_parser_probe
            COMMAND featherdoc_pdfium_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-pdfio-probe.pdf"
        )
        featherdoc_collect_pdf_runtime_environment(
            FEATHERDOC_PDF_RUNTIME_ENVIRONMENT TRUE)
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfium_parser_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        set_tests_properties(pdfium_parser_probe
            PROPERTIES DEPENDS pdfio_generator_probe)
    endif()

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND TARGET featherdoc_pdf_document_probe)
        add_test(
            NAME pdfium_document_parser_probe
            COMMAND featherdoc_pdfium_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-document-probe.pdf"
        )
        featherdoc_collect_pdf_runtime_environment(
            FEATHERDOC_PDF_RUNTIME_ENVIRONMENT TRUE)
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfium_document_parser_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        set_tests_properties(pdfium_document_parser_probe
            PROPERTIES DEPENDS pdf_document_generator_probe)
    endif()
endif()
