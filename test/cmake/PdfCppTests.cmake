if(TARGET FeatherDocPdf)
    featherdoc_add_cpp_test(
        pdf_font_resolver_tests
        pdf_font_resolver
        pdf_font_resolver_tests.cpp
    )
    target_link_libraries(pdf_font_resolver_tests PRIVATE FeatherDoc::Pdf)
    featherdoc_set_test_labels(pdf_font_resolver core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_font_resolver PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_text_metrics_tests
        pdf_text_metrics
        pdf_text_metrics_tests.cpp
    )
    target_link_libraries(pdf_text_metrics_tests PRIVATE FeatherDoc::Pdf)
    featherdoc_set_test_labels(pdf_text_metrics core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_text_metrics PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_text_shaper_tests
        pdf_text_shaper
        pdf_text_shaper_tests.cpp
    )
    target_link_libraries(pdf_text_shaper_tests PRIVATE FeatherDoc::Pdf)
    featherdoc_set_test_labels(pdf_text_shaper core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_text_shaper PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_document_adapter_font_tests
        pdf_document_adapter_font
        pdf_document_adapter_font_tests.cpp
        pdf_document_adapter_font_mapping_tests.cpp
        pdf_document_adapter_table_text_layout_tests.cpp
        pdf_document_adapter_section_layout_tests.cpp
        pdf_document_adapter_header_footer_writer_tests.cpp
        pdf_document_adapter_image_tests.cpp
        pdf_document_adapter_table_layout_tests.cpp
        pdf_document_adapter_table_style_tests.cpp
        pdf_document_adapter_table_border_style_tests.cpp
        pdf_document_adapter_table_roundtrip_tests.cpp
    )
    target_link_libraries(
        pdf_document_adapter_font_tests
        PRIVATE FeatherDoc::Pdf
    )
    if(TARGET FeatherDocPdfImport)
        target_compile_definitions(
            pdf_document_adapter_font_tests
            PRIVATE FEATHERDOC_BUILD_PDF_IMPORT=1
        )
        target_link_libraries(
            pdf_document_adapter_font_tests
            PRIVATE FeatherDoc::PdfImport
        )
    endif()
    featherdoc_set_test_labels(pdf_document_adapter_font core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_document_adapter_font PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    if(TARGET FeatherDocPdfImport)
        set(FEATHERDOC_PDF_CLI_IMPORT_FIXTURE_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/pdf-fixtures")
        file(MAKE_DIRECTORY "${FEATHERDOC_PDF_CLI_IMPORT_FIXTURE_DIR}")
        foreach(pdf_cli_import_fixture IN ITEMS
            featherdoc-adapter-cjk-roundtrip.pdf
            featherdoc-adapter-table-roundtrip.pdf
        )
            set(pdf_cli_import_fixture_source
                "${PROJECT_SOURCE_DIR}/${pdf_cli_import_fixture}")
            if(EXISTS "${pdf_cli_import_fixture_source}")
                configure_file(
                    "${pdf_cli_import_fixture_source}"
                    "${FEATHERDOC_PDF_CLI_IMPORT_FIXTURE_DIR}/${pdf_cli_import_fixture}"
                    COPYONLY
                )
            endif()
        endforeach()
    endif()

    if(TARGET featherdoc_cli)
        featherdoc_add_cpp_test(
            pdf_cli_export_tests
            pdf_cli_export
            pdf_cli_export_tests.cpp
        )
        target_include_directories(
            pdf_cli_export_tests
            PRIVATE ${PROJECT_SOURCE_DIR}/cli
        )
        add_dependencies(pdf_cli_export_tests featherdoc_cli)
        if(TARGET FeatherDocPdfImport)
            target_compile_definitions(
                pdf_cli_export_tests
                PRIVATE FEATHERDOC_BUILD_PDF_IMPORT=1
            )
            target_link_libraries(
                pdf_cli_export_tests
                PRIVATE FeatherDoc::PdfImport
            )
        endif()
        featherdoc_set_test_labels(pdf_cli_export cli smoke pdf)
        if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdf_cli_export PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
        endif()
    endif()

    if(TARGET featherdoc_cli AND TARGET FeatherDocPdfImport)
        featherdoc_add_cpp_test(
            pdf_cli_import_tests
            pdf_cli_import
            pdf_cli_import_tests.cpp
            pdf_cli_import_threshold_tests.cpp
        )
        target_include_directories(
            pdf_cli_import_tests
            PRIVATE ${PROJECT_SOURCE_DIR}/cli
        )
        target_link_libraries(
            pdf_cli_import_tests
            PRIVATE FeatherDoc::Pdf
        )
        add_dependencies(pdf_cli_import_tests featherdoc_cli)
        featherdoc_set_test_labels(pdf_cli_import cli smoke pdf import)
        set_tests_properties(pdf_cli_import PROPERTIES
            TIMEOUT 120)
        if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdf_cli_import PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
        endif()
        if(TARGET pdf_document_adapter_font_tests)
            set_tests_properties(pdf_cli_import PROPERTIES
                DEPENDS pdf_document_adapter_font)
        endif()
    endif()
endif()
