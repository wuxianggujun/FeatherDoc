if(TARGET FeatherDocPdf AND TARGET FeatherDocPdfImport)
    featherdoc_add_cpp_test(
        pdf_unicode_font_roundtrip_tests
        pdf_unicode_font_roundtrip
        pdf_unicode_font_roundtrip_tests.cpp
        pdf_unicode_font_roundtrip_cjk_tests.cpp
        pdf_unicode_font_roundtrip_shaping_tests.cpp
        pdf_unicode_font_roundtrip_adapter_failure_tests.cpp
    )
    target_link_libraries(
        pdf_unicode_font_roundtrip_tests
        PRIVATE FeatherDoc::Pdf FeatherDoc::PdfImport
    )
    featherdoc_set_test_labels(pdf_unicode_font_roundtrip core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_unicode_font_roundtrip PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_import_structure_tests
        pdf_import_structure
        pdf_import_structure_tests.cpp
    )
    target_link_libraries(
        pdf_import_structure_tests
        PRIVATE FeatherDoc::Pdf FeatherDoc::PdfImport
    )
    featherdoc_set_test_labels(pdf_import_structure core smoke pdf)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_import_structure PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_import_failure_tests
        pdf_import_failure
        pdf_import_failure_tests.cpp
    )
    target_link_libraries(
        pdf_import_failure_tests
        PRIVATE FeatherDoc::Pdf FeatherDoc::PdfImport
    )
    featherdoc_set_test_labels(pdf_import_failure core smoke pdf import-failure)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_import_failure PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()

    featherdoc_add_cpp_test(
        pdf_import_table_heuristic_tests
        pdf_import_table_heuristic
        pdf_import_table_heuristic_tests.cpp
        pdf_import_table_heuristic_parser_tests.cpp
        pdf_import_table_heuristic_parser_summary_tests.cpp
        pdf_import_table_heuristic_parser_header_tests.cpp
        pdf_import_table_heuristic_import_basic_tests.cpp
        pdf_import_table_heuristic_import_summary_tests.cpp
        pdf_import_table_heuristic_import_merged_subtotal_tests.cpp
        pdf_import_table_heuristic_import_merged_pagebreak_tests.cpp
        pdf_import_table_heuristic_import_merged_compact_tests.cpp
        pdf_import_table_heuristic_import_merged_cell_tests.cpp
        pdf_import_table_heuristic_import_merged_vertical_tests.cpp
        pdf_import_table_heuristic_import_header_flow_tests.cpp
        pdf_import_table_heuristic_import_page_flow_tests.cpp
        pdf_import_table_heuristic_import_order_cleanup_tests.cpp
        pdf_import_table_heuristic_import_continuation_tests.cpp
    )
    target_link_libraries(
        pdf_import_table_heuristic_tests
        PRIVATE FeatherDoc::Pdf FeatherDoc::PdfImport
    )
    featherdoc_set_test_labels(
        pdf_import_table_heuristic core smoke pdf import-heuristic)
    if(NOT FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
        set_tests_properties(pdf_import_table_heuristic PROPERTIES
            ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
    endif()
endif()
