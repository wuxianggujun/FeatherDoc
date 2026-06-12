if(TARGET FeatherDocPdf AND TARGET FeatherDocPdfImport)
    set(FEATHERDOC_PDF_REGRESSION_MANIFEST
        "${CMAKE_CURRENT_SOURCE_DIR}/pdf_regression_manifest.json")
    set(FEATHERDOC_PDF_REGRESSION_SCHEMA
        "${CMAKE_CURRENT_SOURCE_DIR}/pdf_regression_manifest.schema.json")
    configure_file(
        "${FEATHERDOC_PDF_REGRESSION_MANIFEST}"
        "${CMAKE_CURRENT_BINARY_DIR}/pdf_regression_manifest.json"
        COPYONLY
    )
    configure_file(
        "${FEATHERDOC_PDF_REGRESSION_SCHEMA}"
        "${CMAKE_CURRENT_BINARY_DIR}/pdf_regression_manifest.schema.json"
        COPYONLY
    )

    featherdoc_add_cpp_test(
        pdf_regression_manifest_tests
        pdf_regression_manifest
        pdf_regression_manifest_test.cpp
    )
    featherdoc_set_test_labels(pdf_regression_manifest core smoke pdf regression)
    set(FEATHERDOC_PDF_REGRESSION_MANIFEST_RUNTIME_PATH
        "${CMAKE_CURRENT_BINARY_DIR}/pdf_regression_manifest.json")
    file(TO_CMAKE_PATH
        "${FEATHERDOC_PDF_REGRESSION_MANIFEST_RUNTIME_PATH}"
        FEATHERDOC_PDF_REGRESSION_MANIFEST_RUNTIME_PATH)
    set(FEATHERDOC_PDF_REGRESSION_MANIFEST_DEFINE
        "FEATHERDOC_PDF_REGRESSION_MANIFEST_PATH=\"${FEATHERDOC_PDF_REGRESSION_MANIFEST_RUNTIME_PATH}\"")
    target_compile_definitions(
        pdf_regression_manifest_tests
        PRIVATE "${FEATHERDOC_PDF_REGRESSION_MANIFEST_DEFINE}")

    add_executable(featherdoc_pdf_regression_sample
        ../samples/pdf_regression_sample.cpp
    )
    target_link_libraries(
        featherdoc_pdf_regression_sample
        PRIVATE FeatherDoc::Pdf FeatherDoc::PdfImport
    )
    if(DEFINED FEATHERDOC_PDFIUM_TARGET AND NOT FEATHERDOC_PDFIUM_TARGET STREQUAL "")
        target_link_libraries(
            featherdoc_pdf_regression_sample
            PRIVATE ${FEATHERDOC_PDFIUM_TARGET}
        )
    endif()
    if(DEFINED FEATHERDOC_PDFIUM_SOURCE_DIR AND NOT FEATHERDOC_PDFIUM_SOURCE_DIR STREQUAL "")
        target_include_directories(
            featherdoc_pdf_regression_sample
            PRIVATE "${FEATHERDOC_PDFIUM_SOURCE_DIR}/public"
        )
    endif()
    featherdoc_copy_runtime_dlls(featherdoc_pdf_regression_sample)

    if(MSVC)
        target_compile_options(featherdoc_pdf_regression_sample PRIVATE /utf-8)
    endif()

    file(READ "${FEATHERDOC_PDF_REGRESSION_MANIFEST}" FEATHERDOC_PDF_REGRESSION_JSON)
    string(JSON FEATHERDOC_PDF_REGRESSION_COUNT LENGTH
        "${FEATHERDOC_PDF_REGRESSION_JSON}" samples)

    math(EXPR FEATHERDOC_PDF_REGRESSION_LAST_INDEX
        "${FEATHERDOC_PDF_REGRESSION_COUNT} - 1")
    foreach(sample_index RANGE 0 ${FEATHERDOC_PDF_REGRESSION_LAST_INDEX})
        string(JSON sample_id GET "${FEATHERDOC_PDF_REGRESSION_JSON}" samples
            ${sample_index} id)
        string(JSON sample_kind GET "${FEATHERDOC_PDF_REGRESSION_JSON}" samples
            ${sample_index} kind)
        string(JSON sample_output GET "${FEATHERDOC_PDF_REGRESSION_JSON}" samples
            ${sample_index} output_file)
        string(JSON sample_expected_pages GET
            "${FEATHERDOC_PDF_REGRESSION_JSON}" samples ${sample_index}
            expected_pages)
        string(JSON sample_expected_text_count LENGTH
            "${FEATHERDOC_PDF_REGRESSION_JSON}" samples ${sample_index}
            expected_text)
        string(JSON sample_expected_image_count ERROR_VARIABLE
            sample_expected_image_count_error GET
            "${FEATHERDOC_PDF_REGRESSION_JSON}" samples ${sample_index}
            expected_image_count)

        set(sample_command
            featherdoc_pdf_regression_sample
            --scenario ${sample_kind}
            --output ${CMAKE_CURRENT_BINARY_DIR}/${sample_output}
            --expected-pages ${sample_expected_pages})
        if(sample_expected_text_count GREATER 0)
            math(EXPR sample_expected_text_last_index
                "${sample_expected_text_count} - 1")
            foreach(text_index RANGE 0 ${sample_expected_text_last_index})
                string(JSON sample_expected_text GET
                    "${FEATHERDOC_PDF_REGRESSION_JSON}" samples
                    ${sample_index} expected_text ${text_index})
                list(APPEND sample_command --expect-text "${sample_expected_text}")
            endforeach()
        endif()
        if(sample_expected_image_count_error STREQUAL "NOTFOUND")
            list(APPEND sample_command
                --expect-image-count "${sample_expected_image_count}")
        endif()
        if(sample_kind STREQUAL "cjk_text" OR
           sample_kind STREQUAL "mixed_cjk_punctuation_text" OR
           sample_kind STREQUAL "cjk_image_report_text" OR
           sample_kind STREQUAL "cjk_report_text" OR
           sample_kind STREQUAL "contract_cjk_style" OR
           sample_kind STREQUAL "document_contract_cjk_style" OR
           sample_kind STREQUAL "document_cjk_anchor_font_matrix_boundary_text" OR
           sample_kind STREQUAL "document_cjk_anchor_matrix_lite_text" OR
           sample_kind STREQUAL "document_cjk_bullet_list_text" OR
           sample_kind STREQUAL "document_cjk_bullet_overlay_lite_text" OR
           sample_kind STREQUAL "document_cjk_bullet_overlay_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_bullet_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_complex_layout_text" OR
           sample_kind STREQUAL "document_cjk_extreme_page_breaks_text" OR
           sample_kind STREQUAL "document_cjk_image_wrap_stress_text" OR
           sample_kind STREQUAL "document_cjk_copy_search_lite_text" OR
           sample_kind STREQUAL "document_cjk_copy_search_matrix_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_lite_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_matrix_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_wrap_mix_lite_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_wrap_mix_text" OR
           sample_kind STREQUAL "document_cjk_font_search_density_flow_text" OR
           sample_kind STREQUAL "document_cjk_multi_anchor_table_flow_lite_text" OR
           sample_kind STREQUAL "document_cjk_multi_anchor_table_flow_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_page_flow_lite_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_page_boundary_lite_text" OR
           sample_kind STREQUAL "document_cjk_repeated_key_boundary_flow_text" OR
           sample_kind STREQUAL "document_cjk_repeated_key_lite_text" OR
           sample_kind STREQUAL "document_cjk_search_density_lite_text" OR
           sample_kind STREQUAL "document_cjk_style_overlay_lite_text" OR
           sample_kind STREQUAL "document_cjk_style_overlay_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_table_wrap_lite_text" OR
           sample_kind STREQUAL "document_cjk_table_wrap_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_vertical_merge_wrap_cant_split_text" OR
           sample_kind STREQUAL "document_cjk_vertical_merge_lite_text" OR
           sample_kind STREQUAL "document_eastasia_style_probe" OR
           sample_kind STREQUAL "document_font_matrix_text" OR
           sample_kind STREQUAL "document_rtl_bidi_text" OR
           sample_kind STREQUAL "document_table_cjk_merged_repeat_text" OR
           sample_kind STREQUAL "document_table_cjk_wrap_flow_text" OR
           sample_kind STREQUAL "document_table_cjk_vertical_merged_cant_split_text" OR
           sample_kind STREQUAL "document_table_font_matrix_text" OR
           sample_kind STREQUAL "header_footer_rtl_text" OR
           sample_kind STREQUAL "header_footer_rtl_variants_text")
            list(APPEND sample_command --require-cjk-font)
        endif()

        set(test_name "pdf_regression_${sample_id}")
        add_test(NAME ${test_name} COMMAND ${sample_command})
        if(FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
            set_tests_properties(${test_name}
                PROPERTIES
                    TIMEOUT 60
                    SKIP_RETURN_CODE 77
                    LABELS "pdf;smoke;regression")
        else()
            set_tests_properties(${test_name}
                PROPERTIES
                    TIMEOUT 60
                    SKIP_RETURN_CODE 77
                    LABELS "pdf;smoke;regression"
                    ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
        endif()
        if(sample_kind STREQUAL "cjk_text" OR
           sample_kind STREQUAL "mixed_cjk_punctuation_text" OR
           sample_kind STREQUAL "cjk_image_report_text" OR
           sample_kind STREQUAL "cjk_report_text" OR
           sample_kind STREQUAL "contract_cjk_style" OR
           sample_kind STREQUAL "document_contract_cjk_style" OR
           sample_kind STREQUAL "document_cjk_anchor_font_matrix_boundary_text" OR
           sample_kind STREQUAL "document_cjk_anchor_matrix_lite_text" OR
           sample_kind STREQUAL "document_cjk_bullet_list_text" OR
           sample_kind STREQUAL "document_cjk_bullet_overlay_lite_text" OR
           sample_kind STREQUAL "document_cjk_bullet_overlay_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_bullet_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_complex_layout_text" OR
           sample_kind STREQUAL "document_cjk_extreme_page_breaks_text" OR
           sample_kind STREQUAL "document_cjk_image_wrap_stress_text" OR
           sample_kind STREQUAL "document_cjk_copy_search_lite_text" OR
           sample_kind STREQUAL "document_cjk_copy_search_matrix_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_lite_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_matrix_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_wrap_mix_lite_text" OR
           sample_kind STREQUAL "document_cjk_font_embed_wrap_mix_text" OR
           sample_kind STREQUAL "document_cjk_font_search_density_flow_text" OR
           sample_kind STREQUAL "document_cjk_multi_anchor_table_flow_lite_text" OR
           sample_kind STREQUAL "document_cjk_multi_anchor_table_flow_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_page_flow_lite_text" OR
           sample_kind STREQUAL "document_cjk_numbered_list_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_page_boundary_lite_text" OR
           sample_kind STREQUAL "document_cjk_repeated_key_boundary_flow_text" OR
           sample_kind STREQUAL "document_cjk_repeated_key_lite_text" OR
           sample_kind STREQUAL "document_cjk_search_density_lite_text" OR
           sample_kind STREQUAL "document_cjk_style_overlay_lite_text" OR
           sample_kind STREQUAL "document_cjk_style_overlay_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_table_wrap_lite_text" OR
           sample_kind STREQUAL "document_cjk_table_wrap_page_flow_text" OR
           sample_kind STREQUAL "document_cjk_vertical_merge_wrap_cant_split_text" OR
           sample_kind STREQUAL "document_cjk_vertical_merge_lite_text" OR
           sample_kind STREQUAL "document_eastasia_style_probe" OR
           sample_kind STREQUAL "document_font_matrix_text" OR
           sample_kind STREQUAL "document_table_cjk_merged_repeat_text" OR
           sample_kind STREQUAL "document_table_cjk_wrap_flow_text" OR
           sample_kind STREQUAL "document_table_cjk_vertical_merged_cant_split_text" OR
           sample_kind STREQUAL "document_table_font_matrix_text")
            set_tests_properties(${test_name}
                PROPERTIES
                    SKIP_RETURN_CODE 77
                    LABELS "pdf;smoke;regression;pdf-cjk")
            list(APPEND FEATHERDOC_PDF_REGRESSION_CJK_TESTS ${test_name})
        endif()
        if(sample_kind STREQUAL "document_font_matrix_text" OR
           sample_kind STREQUAL "document_rtl_bidi_text" OR
           sample_kind STREQUAL "document_table_font_matrix_text" OR
           sample_kind STREQUAL "header_footer_rtl_text" OR
           sample_kind STREQUAL "header_footer_rtl_variants_text")
            set_tests_properties(${test_name}
                PROPERTIES
                    SKIP_RETURN_CODE 77
                    LABELS "pdf;smoke;regression;pdf-rtl")
            list(APPEND FEATHERDOC_PDF_REGRESSION_RTL_TESTS ${test_name})
        endif()
    endforeach()
endif()
