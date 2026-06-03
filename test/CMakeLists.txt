function(featherdoc_add_cpp_test target_name test_name)
    add_executable(${target_name} ${ARGN})

    target_link_libraries(
        ${target_name}
        PRIVATE FeatherDoc::FeatherDoc
    )
    featherdoc_copy_runtime_dlls(${target_name})

    add_test(
        NAME
        ${test_name}
        COMMAND
        ${target_name}
    )
endfunction()

function(featherdoc_set_test_labels test_name)
    set_tests_properties(
        ${test_name}
        PROPERTIES
        LABELS
        "${ARGN}"
    )
    if("pdf" IN_LIST ARGN)
        # PDF tests may touch font, renderer, or fixture stacks; keep CTest bounded.
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
    endif()
endfunction()

function(featherdoc_get_runtime_dir out_var target_name)
    if(NOT TARGET ${target_name})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(runtime_property_names
        IMPORTED_LOCATION_RELEASE
        IMPORTED_LOCATION_RELWITHDEBINFO
        IMPORTED_LOCATION_MINSIZEREL
        IMPORTED_LOCATION_DEBUG
        IMPORTED_LOCATION_NOCONFIG
        IMPORTED_LOCATION
        IMPORTED_IMPLIB_RELEASE
        IMPORTED_IMPLIB_RELWITHDEBINFO
        IMPORTED_IMPLIB_MINSIZEREL
        IMPORTED_IMPLIB_DEBUG
        IMPORTED_IMPLIB_NOCONFIG
        IMPORTED_IMPLIB)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(runtime_property_names
            IMPORTED_LOCATION_DEBUG
            IMPORTED_IMPLIB_DEBUG
            IMPORTED_LOCATION_RELEASE
            IMPORTED_LOCATION_RELWITHDEBINFO
            IMPORTED_LOCATION_MINSIZEREL
            IMPORTED_LOCATION_NOCONFIG
            IMPORTED_LOCATION
            IMPORTED_IMPLIB_RELEASE
            IMPORTED_IMPLIB_RELWITHDEBINFO
            IMPORTED_IMPLIB_MINSIZEREL
            IMPORTED_IMPLIB_NOCONFIG
            IMPORTED_IMPLIB)
    endif()

    set(runtime_location "")
    foreach(property_name IN LISTS runtime_property_names)
        if(runtime_location STREQUAL "")
            get_target_property(runtime_location ${target_name} ${property_name})
            if(runtime_location STREQUAL "runtime_location-NOTFOUND")
                set(runtime_location "")
            endif()
        endif()
    endforeach()

    if(runtime_location STREQUAL "")
        if(target_name STREQUAL "PNG::PNG")
            set(runtime_variable_names PNG_LIBRARY_RELEASE PNG_LIBRARY_DEBUG PNG_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names PNG_LIBRARY_DEBUG PNG_LIBRARY_RELEASE PNG_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "Freetype::Freetype")
            set(runtime_variable_names
                FREETYPE_LIBRARY_RELEASE
                FREETYPE_LIBRARY_DEBUG
                FEATHERDOC_FREETYPE_LIBRARY_RELEASE
                FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                FREETYPE_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    FREETYPE_LIBRARY_DEBUG
                    FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                    FREETYPE_LIBRARY_RELEASE
                    FEATHERDOC_FREETYPE_LIBRARY_RELEASE
                    FREETYPE_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz-subset")
            set(runtime_variable_names
                HARFBUZZ_SUBSET_LIBRARY_RELEASE
                HARFBUZZ_SUBSET_LIBRARY_DEBUG
                HARFBUZZ_SUBSET_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    HARFBUZZ_SUBSET_LIBRARY_DEBUG
                    HARFBUZZ_SUBSET_LIBRARY_RELEASE
                    HARFBUZZ_SUBSET_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz")
            set(runtime_variable_names
                HARFBUZZ_LIBRARY_RELEASE
                HARFBUZZ_LIBRARY_DEBUG
                HARFBUZZ_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    HARFBUZZ_LIBRARY_DEBUG
                    HARFBUZZ_LIBRARY_RELEASE
                    HARFBUZZ_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        endif()
    endif()

    if(runtime_location STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(runtime_ext "${runtime_location}" EXT)
    if(runtime_ext STREQUAL ".lib")
        string(REPLACE "/lib/" "/bin/" runtime_location "${runtime_location}")
        string(REPLACE "\\lib\\" "\\bin\\" runtime_location "${runtime_location}")
        string(REGEX REPLACE "\\.lib$" ".dll" runtime_location
            "${runtime_location}")
    endif()

    get_filename_component(runtime_dir "${runtime_location}" DIRECTORY)
    set(${out_var} "${runtime_dir}" PARENT_SCOPE)
endfunction()

function(featherdoc_build_pdf_test_environment out_var)
    set(runtime_dirs)

    featherdoc_get_runtime_dir(png_runtime_dir PNG::PNG)
    if(NOT png_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${png_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(freetype_runtime_dir Freetype::Freetype)
    if(NOT freetype_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${freetype_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(harfbuzz_runtime_dir harfbuzz::harfbuzz)
    if(NOT harfbuzz_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(harfbuzz_subset_runtime_dir
        harfbuzz::harfbuzz-subset)
    if(NOT harfbuzz_subset_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_subset_runtime_dir}")
    endif()

    if(DEFINED FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR AND
       NOT "${FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR}" STREQUAL "")
        list(APPEND runtime_dirs "${FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR}")
    endif()

    list(REMOVE_DUPLICATES runtime_dirs)
    if(runtime_dirs)
        string(JOIN ";" runtime_path ${runtime_dirs})
        set(${out_var} "PATH=${runtime_path};$ENV{PATH}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

set(FEATHERDOC_PDF_TEST_ENVIRONMENT "")
featherdoc_build_pdf_test_environment(FEATHERDOC_PDF_TEST_ENVIRONMENT)

featherdoc_add_cpp_test(
    unit_tests
    unit
    basic_tests.cpp
)
featherdoc_set_test_labels(unit core heavy)

if(MSVC)
    target_compile_options(unit_tests PRIVATE /bigobj /utf-8)
endif()

featherdoc_add_cpp_test(
    iterator_tests
    iterator
    iterator_tests.cpp
)
featherdoc_set_test_labels(iterator core smoke)

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

if(TARGET FeatherDocPdf AND TARGET FeatherDocPdfImport)
    featherdoc_add_cpp_test(
        pdf_unicode_font_roundtrip_tests
        pdf_unicode_font_roundtrip
        pdf_unicode_font_roundtrip_tests.cpp
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

featherdoc_add_cpp_test(
    cli_json_tests
    cli_json
    cli_json_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json.cpp
)
target_include_directories(cli_json_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_json cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_parse_tests
    cli_parse
    cli_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_parse.cpp
)
target_include_directories(cli_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_parse cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_domain_parse_tests
    cli_domain_parse
    cli_domain_parse_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_parse.cpp
)
target_include_directories(cli_domain_parse_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
featherdoc_set_test_labels(cli_domain_parse cli smoke cli_smoke)

featherdoc_add_cpp_test(
    cli_domain_name_tests
    cli_domain_name
    cli_domain_name_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_names.cpp
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
    cli_usage_tests
    cli_usage
    cli_usage_tests.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_usage.cpp
)
target_include_directories(cli_usage_tests PRIVATE ${PROJECT_SOURCE_DIR}/cli)
if(TARGET FeatherDocPdfImport)
    target_compile_definitions(
        cli_usage_tests
        PRIVATE FEATHERDOC_CLI_ENABLE_PDF_IMPORT=1
    )
endif()
featherdoc_set_test_labels(cli_usage cli smoke cli_smoke)

if(WIN32)
    find_program(FEATHERDOC_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
    if(FEATHERDOC_POWERSHELL_EXECUTABLE)
        set(FEATHERDOC_POWERSHELL_TEST_COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -NoLogo
            -NoProfile
            -NonInteractive
        )
        find_program(FEATHERDOC_PYTHON_EXECUTABLE NAMES python python3 py)

        add_test(
            NAME
            release_note_bundle_version
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_note_bundle_version_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_note_bundle_version
        )

        add_test(
            NAME
            release_note_bundle_visual_verdict_metadata
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_note_bundle_visual_verdict_metadata_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_note_bundle_visual_verdict_metadata
        )

        add_test(
            NAME
            release_blocker_action_registry
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_blocker_action_registry_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_blocker_action_registry
        )

        add_test(
            NAME
            run_reused_build_check_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/run_reused_build_check_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/run_reused_build_check_contract
        )
        set_tests_properties(run_reused_build_check_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_warning_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_warning_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_warning_contract
        )
        set_tests_properties(release_governance_warning_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            github_actions_workflow_maintenance
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/github_actions_workflow_maintenance_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(github_actions_workflow_maintenance
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_warning_helper_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_warning_helper_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_warning_helper_contract
        )
        set_tests_properties(release_governance_warning_helper_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_metrics_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_metrics_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_metrics_contract
        )
        set_tests_properties(release_governance_metrics_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_detail_rollup_static_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_detail_rollup_static_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(release_governance_detail_rollup_static_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            package_release_assets_safety
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_safety_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_safety
        )
        set_tests_properties(package_release_assets_safety
            PROPERTIES
                TIMEOUT 60
                LABELS "release;package;smoke")

        add_test(
            NAME
            package_release_assets_allow_incomplete
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_allow_incomplete_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_allow_incomplete
        )
        set_tests_properties(package_release_assets_allow_incomplete
            PROPERTIES
                TIMEOUT 60
                LABELS "release;package;smoke")

        add_test(
            NAME
            public_release_wording_regression
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/public_release_wording_regression_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )

        add_test(
            NAME
            pdf_import_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_import_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_import_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;docs;smoke")

        add_test(
            NAME
            pdf_ctest_timeout_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_timeout_contract_test.ps1
            -CTestTestfile
            ${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake
        )
        set_tests_properties(pdf_ctest_timeout_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;ctest;smoke")

        add_test(
            NAME
            pdf_ctest_label_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_label_contract_test.ps1
            -CTestTestfile
            ${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake
        )
        set_tests_properties(pdf_ctest_label_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;ctest;smoke")

        add_test(
            NAME
            pdf_bidi_line_layout_static_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_bidi_line_layout_static_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_bidi_line_layout_static_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_style_gallery_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_style_gallery_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_style_gallery_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_font_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_font_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_font_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_font_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_font_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_font_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_copy_search_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_copy_search_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_copy_search_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_embed_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_embed_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_embed_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_anchor_font_matrix_boundary_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_anchor_font_matrix_boundary_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_anchor_font_matrix_boundary_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_search_density_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_search_density_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_search_density_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_embed_wrap_mix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_embed_wrap_mix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_embed_wrap_mix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_repeated_key_boundary_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_repeated_key_boundary_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_repeated_key_boundary_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_style_overlay_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_style_overlay_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_style_overlay_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_complex_layout_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_complex_layout_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_complex_layout_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_image_wrap_stress_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_image_wrap_stress_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_image_wrap_stress_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_extreme_page_breaks_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_extreme_page_breaks_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_extreme_page_breaks_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_vertical_merge_wrap_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_vertical_merge_wrap_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_vertical_merge_wrap_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_wrap_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_wrap_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_cjk_wrap_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_vertical_merged_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_vertical_merged_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(
            pdf_document_table_cjk_vertical_merged_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_merged_repeat_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_merged_repeat_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_cjk_merged_repeat_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_vertical_merged_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_vertical_merged_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(
            pdf_document_table_vertical_merged_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_rtl_bidi_light_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_rtl_bidi_light_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_rtl_bidi_light_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_list_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_list_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_list_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_table_wrap_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_table_wrap_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_table_wrap_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_multi_anchor_table_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_multi_anchor_table_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_multi_anchor_table_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_text_decoration_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_text_decoration_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_text_decoration_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        if(TARGET pdf_unicode_font_roundtrip_tests)
            add_test(
                NAME
                pdf_unicode_font_roundtrip_visual
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/run_pdf_unicode_font_roundtrip_visual_regression.ps1
                -BuildDir
                ${CMAKE_BINARY_DIR}
                -OutputDir
                ${CMAKE_BINARY_DIR}/pdf_unicode_font_visual_regression
                -CtestExecutable
                ${CMAKE_CTEST_COMMAND}
            )
            if(FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
                set_tests_properties(pdf_unicode_font_roundtrip_visual
                    PROPERTIES
                        TIMEOUT 60
                        LABELS "pdf;smoke;regression;visual"
                        SKIP_RETURN_CODE 77)
            else()
                set_tests_properties(pdf_unicode_font_roundtrip_visual
                    PROPERTIES
                        TIMEOUT 60
                        LABELS "pdf;smoke;regression;visual"
                        SKIP_RETURN_CODE 77
                        ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
            endif()
        endif()

        add_test(
            NAME
            pdf_visual_release_gate_style_baselines
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_style_baselines_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_style_baselines
        )
        set_tests_properties(pdf_visual_release_gate_style_baselines
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_release_gate_text_shaping_baselines
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_text_shaping_baselines_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_text_shaping_baselines
        )
        set_tests_properties(pdf_visual_release_gate_text_shaping_baselines
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_dependency_inputs_check
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_dependency_inputs_check_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_dependency_inputs_check
        )
        set_tests_properties(pdf_dependency_inputs_check
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        add_test(
            NAME
            pdf_visual_release_gate_preflight
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_preflight_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_preflight
        )
        set_tests_properties(pdf_visual_release_gate_preflight
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_release_gate_preflight_governance_report
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_preflight_governance_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_preflight_governance_report
        )
        set_tests_properties(pdf_visual_release_gate_preflight_governance_report
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_release_readiness_check
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_release_readiness_check_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_release_readiness_check
        )
        set_tests_properties(pdf_release_readiness_check
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs;visual")

        add_test(
            NAME
            pdf_visual_full_gate_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_full_gate_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_full_gate_guarded
        )
        set_tests_properties(pdf_visual_full_gate_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_full_ctest_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_full_ctest_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_full_ctest_guarded
        )
        set_tests_properties(pdf_full_ctest_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            pdf_ctest_remaining_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_remaining_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_ctest_remaining_guarded
        )
        set_tests_properties(pdf_ctest_remaining_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            pdf_visual_gate_attempt_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_gate_attempt_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_gate_attempt_summary
        )
        set_tests_properties(pdf_visual_gate_attempt_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_segmented_gate_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_segmented_gate_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_segmented_gate_summary
        )
        set_tests_properties(pdf_visual_segmented_gate_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_validation_status_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_validation_status_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_visual_validation_status_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        add_test(
            NAME
            pdf_real_business_sample_manifest_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_real_business_sample_manifest_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_real_business_sample_manifest_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        set(FEATHERDOC_CONTROLLED_VISUAL_SMOKE_PYTHON_ARGS)
        if(FEATHERDOC_PYTHON_EXECUTABLE)
            list(APPEND FEATHERDOC_CONTROLLED_VISUAL_SMOKE_PYTHON_ARGS
                -PythonExecutable
                ${FEATHERDOC_PYTHON_EXECUTABLE}
            )
        endif()

        add_test(
            NAME
            pdf_controlled_visual_smoke_check
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_controlled_visual_smoke_check_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_controlled_visual_smoke_check
            ${FEATHERDOC_CONTROLLED_VISUAL_SMOKE_PYTHON_ARGS}
        )
        set_tests_properties(pdf_controlled_visual_smoke_check
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            compare_pdf_reference_branch_manifest
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/compare_pdf_reference_branch_manifest_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/compare_pdf_reference_branch_manifest
        )
        set_tests_properties(compare_pdf_reference_branch_manifest
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            stale_codex_branch_inventory_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/stale_codex_branch_inventory_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(stale_codex_branch_inventory_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;regression")

        add_test(
            NAME
            sync_github_release_notes
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_github_release_notes_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_github_release_notes
        )

        add_test(
            NAME
            sync_github_release_notes_publish_guard
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_github_release_notes_publish_guard_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_github_release_notes_publish_guard
        )

        add_test(
            NAME
            publish_github_release
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/publish_github_release_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/publish_github_release
        )

        add_test(
            NAME
            assert_release_material_safety
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/assert_release_material_safety_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/assert_release_material_safety
        )

        add_test(
            NAME
            prepare_document_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_document_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_document_review_task
        )
        set_tests_properties(prepare_document_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            prepare_word_review_task_verdict
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_word_review_task_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_word_review_task_verdict
        )
        set_tests_properties(prepare_word_review_task_verdict
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            prepare_visual_regression_bundle_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_visual_regression_bundle_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_visual_regression_bundle_review_task
        )
        set_tests_properties(prepare_visual_regression_bundle_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            refresh_readme_visual_assets
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/refresh_readme_visual_assets_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/refresh_readme_visual_assets
        )
        set_tests_properties(refresh_readme_visual_assets
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            prepare_section_page_setup_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_section_page_setup_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_section_page_setup_review_task
        )
        set_tests_properties(prepare_section_page_setup_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            prepare_page_number_fields_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_page_number_fields_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_page_number_fields_review_task
        )
        set_tests_properties(prepare_page_number_fields_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            open_latest_section_page_setup_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_section_page_setup_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_section_page_setup_review_task
        )
        set_tests_properties(open_latest_section_page_setup_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            open_latest_page_number_fields_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_page_number_fields_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_page_number_fields_review_task
        )
        set_tests_properties(open_latest_page_number_fields_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            open_latest_fixed_grid_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_fixed_grid_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_fixed_grid_review_task
        )
        set_tests_properties(open_latest_fixed_grid_review_task
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            open_latest_word_review_task_curated_source_kind
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_word_review_task_curated_source_kind_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_word_review_task_curated_source_kind
        )
        set_tests_properties(open_latest_word_review_task_curated_source_kind
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            print_latest_fixed_grid_review_prompt
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/print_latest_fixed_grid_review_prompt_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/print_latest_fixed_grid_review_prompt
        )
        set_tests_properties(print_latest_fixed_grid_review_prompt
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            latest_review_task_wrappers_cmake_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/latest_review_task_wrappers_cmake_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(latest_review_task_wrappers_cmake_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;review-task;smoke")

        add_test(
            NAME
            word_visual_release_gate_smoke_verdict
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/word_visual_release_gate_smoke_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/word_visual_release_gate_smoke_verdict
        )
        set_tests_properties(word_visual_release_gate_smoke_verdict
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-gate;smoke")

        add_test(
            NAME
            check_word_visual_release_gate_preflight
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/check_word_visual_release_gate_preflight_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/check_word_visual_release_gate_preflight
        )
        set_tests_properties(check_word_visual_release_gate_preflight
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-gate;smoke")

        add_test(
            NAME
            word_visual_release_gate_preflight_route_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/word_visual_release_gate_preflight_route_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(word_visual_release_gate_preflight_route_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-gate;smoke")

        add_test(
            NAME
            release_candidate_visual_verdict
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_visual_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_visual_verdict
        )

        add_test(
            NAME
            release_candidate_blocker_rollup
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup
        )
        featherdoc_set_test_labels(release_candidate_blocker_rollup release smoke release_smoke)

        add_test(
            NAME
            release_candidate_governance_handoff
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_governance_handoff
            -Scenario
            handoff
        )
        featherdoc_set_test_labels(release_candidate_governance_handoff release smoke release_smoke governance)

        add_test(
            NAME
            release_visual_verdict_metadata_consistency
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_visual_verdict_metadata_consistency_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_visual_verdict_metadata_consistency
        )

        add_test(
            NAME
            word_visual_review_report
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/word_visual_review_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/word_visual_review_report
        )

        add_test(
            NAME
            table_style_quality_visual_regression
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/table_style_quality_visual_regression_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -BuildDir
            ${PROJECT_BINARY_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/table_style_quality_visual_regression
        )

        add_test(
            NAME
            sync_visual_review_verdict_section_page_setup
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_visual_review_verdict_section_page_setup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_visual_review_verdict_section_page_setup
        )

        add_test(
            NAME
            sync_visual_review_verdict_page_number_fields
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_visual_review_verdict_page_number_fields_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_visual_review_verdict_page_number_fields
        )

        add_test(
            NAME
            sync_visual_review_verdict_curated_visual_bundle
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_visual_review_verdict_curated_visual_bundle_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_visual_review_verdict_curated_visual_bundle
        )

        add_test(
            NAME
            sync_latest_visual_review_verdict
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict
        )
        set_tests_properties(sync_latest_visual_review_verdict
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_uses_latest_release_summary_candidate
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_uses_latest_release_summary_candidate_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_uses_latest_release_summary_candidate
        )
        set_tests_properties(sync_latest_visual_review_verdict_uses_latest_release_summary_candidate
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate
        )
        set_tests_properties(sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_curated_visual_bundle
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_curated_visual_bundle_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_curated_visual_bundle
        )
        set_tests_properties(sync_latest_visual_review_verdict_curated_visual_bundle
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_table_style_quality
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_table_style_quality_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_table_style_quality
        )
        set_tests_properties(sync_latest_visual_review_verdict_table_style_quality
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_no_release_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_no_release_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_no_release_summary
        )
        set_tests_properties(sync_latest_visual_review_verdict_no_release_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_malformed_release_summary_candidate_only
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_malformed_release_summary_candidate_only_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_malformed_release_summary_candidate_only
        )
        set_tests_properties(sync_latest_visual_review_verdict_malformed_release_summary_candidate_only
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_conflicting_gates
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_conflicting_gates_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_conflicting_gates
        )
        set_tests_properties(sync_latest_visual_review_verdict_conflicting_gates
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_missing_source_path
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_missing_source_path_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_missing_source_path
        )
        set_tests_properties(sync_latest_visual_review_verdict_missing_source_path
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_unsupported_source_kind
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_unsupported_source_kind_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_unsupported_source_kind
        )
        set_tests_properties(sync_latest_visual_review_verdict_unsupported_source_kind
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_malformed_pointer_json
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_malformed_pointer_json_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_malformed_pointer_json
        )
        set_tests_properties(sync_latest_visual_review_verdict_malformed_pointer_json
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_malformed_release_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_malformed_release_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_malformed_release_summary
        )
        set_tests_properties(sync_latest_visual_review_verdict_malformed_release_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate
        )
        set_tests_properties(sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_missing_release_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_missing_release_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_missing_release_summary
        )
        set_tests_properties(sync_latest_visual_review_verdict_missing_release_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_explicit_gate_mismatch
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_explicit_gate_mismatch_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_explicit_gate_mismatch
        )
        set_tests_properties(sync_latest_visual_review_verdict_explicit_gate_mismatch
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            sync_latest_visual_review_verdict_cmake_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_cmake_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(sync_latest_visual_review_verdict_cmake_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "word;visual;release-verdict;smoke")

        add_test(
            NAME
            find_superseded_review_tasks
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/find_superseded_review_tasks_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/find_superseded_review_tasks
        )

        add_test(
            NAME
            find_superseded_review_tasks_curated_visual_bundle
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/find_superseded_review_tasks_curated_visual_bundle_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/find_superseded_review_tasks_curated_visual_bundle
        )

        add_test(
            NAME
            convert_render_data_to_patch_plan
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/convert_render_data_to_patch_plan_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/convert_render_data_to_patch_plan
        )

        add_test(
            NAME
            lint_render_data_mapping
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/lint_render_data_mapping_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/lint_render_data_mapping
        )

        add_test(
            NAME
            export_render_data_mapping_draft
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/export_render_data_mapping_draft_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/export_render_data_mapping_draft
        )

        add_test(
            NAME
            export_render_data_skeleton
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/export_render_data_skeleton_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/export_render_data_skeleton
        )

        add_test(
            NAME
            render_data_mapping_route_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/render_data_mapping_route_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(render_data_mapping_route_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;template")

        add_test(
            NAME
            new_project_template_smoke_onboarding_plan
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/new_project_template_smoke_onboarding_plan_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -BuildDir
            ${PROJECT_BINARY_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/new_project_template_smoke_onboarding_plan
        )
        set_tests_properties(new_project_template_smoke_onboarding_plan
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            check_project_template_smoke_manifest
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/check_project_template_smoke_manifest_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/check_project_template_smoke_manifest
        )
        set_tests_properties(check_project_template_smoke_manifest
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            register_project_template_smoke_manifest_entry
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/register_project_template_smoke_manifest_entry_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/register_project_template_smoke_manifest_entry
        )
        set_tests_properties(register_project_template_smoke_manifest_entry
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            sync_project_template_smoke_visual_verdict
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_project_template_smoke_visual_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_project_template_smoke_visual_verdict
        )
        set_tests_properties(sync_project_template_smoke_visual_verdict
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            sync_project_template_smoke_visual_verdict_failure
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_project_template_smoke_visual_verdict_failure_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_project_template_smoke_visual_verdict_failure
        )
        set_tests_properties(sync_project_template_smoke_visual_verdict_failure
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            describe_project_template_smoke_manifest
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/describe_project_template_smoke_manifest_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/describe_project_template_smoke_manifest
        )
        set_tests_properties(describe_project_template_smoke_manifest
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            discover_project_template_smoke_candidates_registered
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/discover_project_template_smoke_candidates_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/discover_project_template_smoke_candidates_registered
            -Scenario
            registered
        )
        set_tests_properties(discover_project_template_smoke_candidates_registered
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            discover_project_template_smoke_candidates_fail_on_unregistered
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/discover_project_template_smoke_candidates_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/discover_project_template_smoke_candidates_fail_on_unregistered
            -Scenario
            fail_on_unregistered
        )
        set_tests_properties(discover_project_template_smoke_candidates_fail_on_unregistered
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        add_test(
            NAME
            onboard_project_template
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/onboard_project_template_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -BuildDir
            ${PROJECT_BINARY_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/onboard_project_template
        )
        set_tests_properties(onboard_project_template
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;project_template")

        if(FEATHERDOC_PYTHON_EXECUTABLE)
            add_test(
                NAME
                docs_conf_version_sync
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/docs_conf_version_sync_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -PythonExecutable
                ${FEATHERDOC_PYTHON_EXECUTABLE}
            )
        endif()

        add_test(
            NAME
            docs_bilingual_entrypoints_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/docs_bilingual_entrypoints_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(docs_bilingual_entrypoints_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "docs;smoke;bilingual")

        if(
            TARGET featherdoc_sample_merge_right_fixed_grid
            AND TARGET featherdoc_sample_merge_down_fixed_grid
            AND TARGET featherdoc_sample_unmerge_right_fixed_grid
            AND TARGET featherdoc_sample_unmerge_down_fixed_grid
        )
            add_test(
                NAME
                fixed_grid_regression_bundle
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/fixed_grid_regression_bundle_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/fixed_grid_regression_bundle
            )
        endif()

        if(
            TARGET featherdoc_sample_section_page_setup
            AND TARGET featherdoc_cli
        )
            add_test(
                NAME
                section_page_setup_regression_bundle
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/section_page_setup_regression_bundle_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/section_page_setup_regression_bundle
            )
        endif()

        if(
            TARGET featherdoc_sample_page_number_fields
            AND TARGET featherdoc_sample_section_page_setup
            AND TARGET featherdoc_cli
        )
            add_test(
                NAME
                page_number_fields_regression_bundle
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/page_number_fields_regression_bundle_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/page_number_fields_regression_bundle
            )
        endif()

        if(
            TARGET featherdoc_sample_template_validation
            AND TARGET featherdoc_sample_part_template_validation
            AND TARGET featherdoc_cli
        )
            add_test(
                NAME
                template_validation_regression_bundle
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/template_validation_regression_bundle_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/template_validation_regression_bundle
            )
        endif()

        if(TARGET featherdoc_cli)
            add_test(
                NAME
                check_numbering_catalog_baseline
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_numbering_catalog_baseline_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_numbering_catalog_baseline
            )

            add_test(
                NAME
                check_numbering_catalog_manifest
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_numbering_catalog_manifest_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_numbering_catalog_manifest
            )

            add_test(
                NAME
                check_template_schema_baseline
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_template_schema_baseline_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_template_schema_baseline
            )

            add_test(
                NAME
                check_template_schema_manifest
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_template_schema_manifest_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_template_schema_manifest
            )

            add_test(
                NAME
                run_project_template_smoke_schema_review
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/run_project_template_smoke_schema_review_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/run_project_template_smoke_schema_review
            )

            add_test(
                NAME
                sync_project_template_schema_approval
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/sync_project_template_schema_approval_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/sync_project_template_schema_approval
            )

            add_test(
                NAME
                write_project_template_schema_approval_history
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/write_project_template_schema_approval_history_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/write_project_template_schema_approval_history
            )

            add_test(
                NAME
                build_document_skeleton_governance_report_passing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_report_passing
                -Scenario
                passing
            )

            add_test(
                NAME
                build_document_skeleton_governance_report_failing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_report_failing
                -Scenario
                failing
            )

            add_test(
                NAME
                build_document_skeleton_governance_report_fail_on_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_report_fail_on_issue
                -Scenario
                fail_on_issue
            )

            add_test(
                NAME
                build_document_skeleton_governance_rollup_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_rollup_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_document_skeleton_governance_rollup_report_empty
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_rollup_report_empty
                -Scenario
                empty
            )

            add_test(
                NAME
                build_document_skeleton_governance_rollup_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_rollup_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_document_skeleton_governance_rollup_report_failed_source_report
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_rollup_report_failed_source_report
                -Scenario
                failed_source_report
            )

            add_test(
                NAME
                build_document_skeleton_governance_rollup_report_fail_on_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_document_skeleton_governance_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_document_skeleton_governance_rollup_report_fail_on_issue
                -Scenario
                fail_on_issue
            )

            add_test(
                NAME
                build_numbering_catalog_governance_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_numbering_catalog_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_numbering_catalog_governance_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_numbering_catalog_governance_report_clean
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_numbering_catalog_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_numbering_catalog_governance_report_clean
                -Scenario
                clean
            )

            add_test(
                NAME
                build_numbering_catalog_governance_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_numbering_catalog_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_numbering_catalog_governance_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_numbering_catalog_governance_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_numbering_catalog_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_numbering_catalog_governance_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                build_table_layout_delivery_report_passing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_report_passing
                -Scenario
                passing
            )

            add_test(
                NAME
                build_table_layout_delivery_report_failing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_report_failing
                -Scenario
                failing
            )

            add_test(
                NAME
                build_table_layout_delivery_report_fail_on_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_report_fail_on_issue
                -Scenario
                fail_on_issue
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_empty
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_empty
                -Scenario
                empty
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_failed_source_report
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_failed_source_report
                -Scenario
                failed_source_report
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_fail_on_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_fail_on_issue
                -Scenario
                fail_on_issue
            )

            add_test(
                NAME
                build_table_layout_delivery_rollup_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_rollup_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                build_table_layout_delivery_governance_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_governance_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_table_layout_delivery_governance_report_ready
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_governance_report_ready
                -Scenario
                ready
            )

            add_test(
                NAME
                build_table_layout_delivery_governance_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_governance_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_table_layout_delivery_governance_report_fail_on_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_governance_report_fail_on_issue
                -Scenario
                fail_on_issue
            )
            set_tests_properties(build_table_layout_delivery_governance_report_fail_on_issue
                PROPERTIES
                    TIMEOUT 60)

            add_test(
                NAME
                build_table_layout_delivery_governance_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_table_layout_delivery_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_table_layout_delivery_governance_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                build_content_control_data_binding_governance_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_content_control_data_binding_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_content_control_data_binding_governance_report_aggregate
                -Scenario
                aggregate
            )
            set_tests_properties(build_content_control_data_binding_governance_report_aggregate
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;content-control")

            add_test(
                NAME
                build_content_control_data_binding_governance_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_content_control_data_binding_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_content_control_data_binding_governance_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )
            set_tests_properties(build_content_control_data_binding_governance_report_fail_on_blocker
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;content-control")

            add_test(
                NAME
                build_content_control_data_binding_governance_report_fail_on_warning
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_content_control_data_binding_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_content_control_data_binding_governance_report_fail_on_warning
                -Scenario
                fail_on_warning
            )
            set_tests_properties(build_content_control_data_binding_governance_report_fail_on_warning
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;content-control")

            add_test(
                NAME
                table_layout_delivery_report
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/table_layout_delivery_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/table_layout_delivery_report
            )

            add_test(
                NAME
                build_project_template_onboarding_governance_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_onboarding_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_onboarding_governance_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_project_template_onboarding_governance_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_onboarding_governance_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_onboarding_governance_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                build_project_template_delivery_readiness_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_delivery_readiness_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_delivery_readiness_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                build_project_template_delivery_readiness_report_ready
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_delivery_readiness_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_delivery_readiness_report_ready
                -Scenario
                ready
            )

            add_test(
                NAME
                build_project_template_delivery_readiness_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_delivery_readiness_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_delivery_readiness_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_project_template_delivery_readiness_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_project_template_delivery_readiness_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_project_template_delivery_readiness_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                project_template_release_readiness_checklist_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/project_template_release_readiness_checklist_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(project_template_release_readiness_checklist_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "project_template;smoke;docs;release")

            add_test(
                NAME
                check_release_metadata_docs
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_release_metadata_docs_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_release_metadata_docs
            )
            set_tests_properties(check_release_metadata_docs
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;release-metadata")

            add_test(
                NAME
                current_direction_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/current_direction_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(current_direction_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance")

            add_test(
                NAME
                document_api_mainline_status_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/document_api_mainline_status_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(document_api_mainline_status_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance")

            add_test(
                NAME
                document_api_lightweight_parser_check
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/document_api_lightweight_parser_check_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/document_api_lightweight_parser_check
            )
            set_tests_properties(document_api_lightweight_parser_check
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance")

            add_test(
                NAME
                script_task_index_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/script_task_index_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(script_task_index_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;scripts")

            add_test(
                NAME
                check_script_task_index
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${PROJECT_SOURCE_DIR}/scripts/check_script_task_index.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -SummaryJson
                ${CMAKE_CURRENT_BINARY_DIR}/check_script_task_index/summary.json
                -ReportMarkdown
                ${CMAKE_CURRENT_BINARY_DIR}/check_script_task_index/script_task_index_check.md
                -Quiet
            )
            set_tests_properties(check_script_task_index
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;scripts")

            add_test(
                NAME
                check_script_task_index_regression
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/check_script_task_index_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/check_script_task_index_regression
            )
            set_tests_properties(check_script_task_index_regression
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;scripts")

            add_test(
                NAME
                write_style_merge_suggestion_review_valid
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/write_style_merge_suggestion_review_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/write_style_merge_suggestion_review_valid
                -Scenario
                valid
            )
            set_tests_properties(write_style_merge_suggestion_review_valid
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                write_style_merge_suggestion_review_invalid_count
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/write_style_merge_suggestion_review_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/write_style_merge_suggestion_review_invalid_count
                -Scenario
                invalid_count
            )
            set_tests_properties(write_style_merge_suggestion_review_invalid_count
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                apply_reviewed_style_merge_suggestions_valid
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/apply_reviewed_style_merge_suggestions_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/apply_reviewed_style_merge_suggestions_valid
                -Scenario
                valid
            )
            set_tests_properties(apply_reviewed_style_merge_suggestions_valid
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                apply_reviewed_style_merge_suggestions_pending_review
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/apply_reviewed_style_merge_suggestions_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/apply_reviewed_style_merge_suggestions_pending_review
                -Scenario
                pending_review
            )
            set_tests_properties(apply_reviewed_style_merge_suggestions_pending_review
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                apply_reviewed_style_merge_suggestions_partial_review
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/apply_reviewed_style_merge_suggestions_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/apply_reviewed_style_merge_suggestions_partial_review
                -Scenario
                partial_review
            )
            set_tests_properties(apply_reviewed_style_merge_suggestions_partial_review
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                audit_style_merge_restore_plan_clean
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/audit_style_merge_restore_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/audit_style_merge_restore_plan_clean
                -Scenario
                clean
            )
            set_tests_properties(audit_style_merge_restore_plan_clean
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                audit_style_merge_restore_plan_issue
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/audit_style_merge_restore_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/audit_style_merge_restore_plan_issue
                -Scenario
                issue
            )
            set_tests_properties(audit_style_merge_restore_plan_issue
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                style_merge_confidence_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/style_merge_confidence_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(style_merge_confidence_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;style")

            add_test(
                NAME
                schema_patch_confidence_calibration_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/schema_patch_confidence_calibration_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(schema_patch_confidence_calibration_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;schema-calibration")

            add_test(
                NAME
                content_control_data_binding_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/content_control_data_binding_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(content_control_data_binding_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;content-control")

            add_test(
                NAME
                table_layout_delivery_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/table_layout_delivery_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(table_layout_delivery_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;table-layout")

            add_test(
                NAME
                numbering_catalog_governance_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/numbering_catalog_governance_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(numbering_catalog_governance_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;numbering")

            add_test(
                NAME
                document_skeleton_governance_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/document_skeleton_governance_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(document_skeleton_governance_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;document-skeleton")

            add_test(
                NAME
                docx_functional_smoke_readiness
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/docx_functional_smoke_readiness_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/docx_functional_smoke_readiness
                -Scenario
                ready
            )
            set_tests_properties(docx_functional_smoke_readiness
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;docx")

            add_test(
                NAME
                docx_functional_smoke_readiness_fail_on_warning
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/docx_functional_smoke_readiness_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/docx_functional_smoke_readiness_fail_on_warning
                -Scenario
                fail_on_warning
            )
            set_tests_properties(docx_functional_smoke_readiness_fail_on_warning
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;docx")

            add_test(
                NAME
                docx_functional_smoke_readiness_route_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/docx_functional_smoke_readiness_route_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(docx_functional_smoke_readiness_route_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;governance;docx")

            add_test(
                NAME
                visual_regression_entrypoints_docs_contract
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/visual_regression_entrypoints_docs_contract_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )
            set_tests_properties(visual_regression_entrypoints_docs_contract
                PROPERTIES
                    TIMEOUT 60
                    LABELS "docs;smoke;visual")

            add_test(
                NAME
                build_release_blocker_rollup_report_passing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_passing
                -Scenario
                passing
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_fail_on_warning
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_fail_on_warning
                -Scenario
                fail_on_warning
            )

            add_test(
                NAME
                build_release_governance_handoff_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_aggregate
                -Scenario
                aggregate
            )
            featherdoc_set_test_labels(build_release_governance_handoff_report_aggregate release smoke release_smoke governance)

            add_test(
                NAME
                build_release_governance_pipeline_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_aggregate
                -Scenario
                aggregate
            )
            featherdoc_set_test_labels(build_release_governance_pipeline_report_aggregate release smoke release_smoke governance)

            add_test(
                NAME
                build_release_governance_pipeline_report_markdown_counts
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_markdown_counts
                -Scenario
                markdown_counts
            )
            featherdoc_set_test_labels(build_release_governance_pipeline_report_markdown_counts release smoke release_smoke governance)

            add_test(
                NAME
                build_release_governance_pipeline_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )
            featherdoc_set_test_labels(build_release_governance_pipeline_report_fail_on_blocker release smoke release_smoke governance)

            add_test(
                NAME
                build_release_governance_pipeline_report_fail_on_warning
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_fail_on_warning
                -Scenario
                fail_on_warning
            )
            featherdoc_set_test_labels(build_release_governance_pipeline_report_fail_on_warning release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_handoff_report_missing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_missing
                -Scenario
                missing
            )

            add_test(
                NAME
                build_release_governance_handoff_report_fail_on_missing
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_fail_on_missing
                -Scenario
                fail_on_missing
            )

            add_test(
                NAME
                build_release_governance_handoff_report_fail_on_blocker
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_fail_on_blocker
                -Scenario
                fail_on_blocker
            )

            add_test(
                NAME
                build_release_governance_handoff_report_fail_on_warning
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_fail_on_warning
                -Scenario
                fail_on_warning
            )

            add_test(
                NAME
                build_release_governance_handoff_report_explicit_input
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_explicit_input
                -Scenario
                explicit_input
            )

            add_test(
                NAME
                build_release_governance_handoff_report_include_rollup
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_include_rollup
                -Scenario
                include_rollup
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_empty
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_empty
                -Scenario
                empty
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_failed_source
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_failed_source
                -Scenario
                failed_source
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_comma_input
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_comma_input
                -Scenario
                comma_input
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_malformed
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_malformed
                -Scenario
                malformed
            )

            add_test(
                NAME
                build_release_blocker_rollup_report_dedupe
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/build_release_blocker_rollup_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/build_release_blocker_rollup_report_dedupe
                -Scenario
                dedupe
            )

            add_test(
                NAME
                write_schema_patch_confidence_calibration_report_aggregate
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/write_schema_patch_confidence_calibration_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/write_schema_patch_confidence_calibration_report_aggregate
                -Scenario
                aggregate
            )

            add_test(
                NAME
                write_schema_patch_confidence_calibration_report_fail_on_pending
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/write_schema_patch_confidence_calibration_report_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/write_schema_patch_confidence_calibration_report_fail_on_pending
                -Scenario
                fail_on_pending
            )

            add_test(
                NAME
                edit_document_from_plan_table_position_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_table_position_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_table_position_plan
            )

            add_test(
                NAME
                edit_document_from_plan_numbering_catalog
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_numbering_catalog_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_numbering_catalog
            )

            add_test(
                NAME
                edit_document_from_plan_content_control_sync
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_content_control_sync_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_content_control_sync
            )

            add_test(
                NAME
                edit_document_from_plan_content_control_text_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_content_control_text_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_content_control_text_aliases
            )

            add_test(
                NAME
                edit_document_from_plan_floating_image
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_floating_image_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_floating_image
            )

            add_test(
                NAME
                edit_document_from_plan_delete_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_delete_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_delete_aliases
            )

            add_test(
                NAME
                edit_document_from_plan_body_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_body_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_body_aliases
            )

            add_test(
                NAME
                edit_document_from_plan_table_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_table_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_table_aliases
            )

            add_test(
                NAME
                edit_document_from_plan_table_structure_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_table_structure_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_table_structure_aliases
            )

            add_test(
                NAME
                edit_document_from_plan_revision_cleanup
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_revision_cleanup_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_revision_cleanup
            )

            add_test(
                NAME
                edit_document_from_plan_update_fields_aliases
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_update_fields_aliases_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan_update_fields_aliases
            )
        endif()

        if(TARGET featherdoc_cli AND TARGET featherdoc_sample_bookmark_block_visibility_visual AND TARGET featherdoc_sample_part_template_validation)
            add_test(
                NAME
                template_render_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/template_render_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/template_render_plan
            )

            add_test(
                NAME
                export_template_render_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/export_template_render_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/export_template_render_plan
            )

            add_test(
                NAME
                prepare_template_render_data_workspace
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/prepare_template_render_data_workspace_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/prepare_template_render_data_workspace
            )

            add_test(
                NAME
                render_template_document_from_workspace
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/render_template_document_from_workspace_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/render_template_document_from_workspace
            )

            add_test(
                NAME
                validate_render_data_mapping
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/validate_render_data_mapping_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/validate_render_data_mapping
            )

            add_test(
                NAME
                edit_document_from_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/edit_document_from_plan
            )

            add_test(
                NAME
                edit_document_from_plan_advanced_ops_coverage
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/edit_document_from_plan_advanced_ops_coverage_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
            )

            add_test(
                NAME
                patch_template_render_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/patch_template_render_plan_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/patch_template_render_plan
            )

            add_test(
                NAME
                render_template_document_from_patch
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/render_template_document_from_patch_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/render_template_document_from_patch
            )

            add_test(
                NAME
                render_template_document_from_data
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/render_template_document_from_data_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -BuildDir
                ${PROJECT_BINARY_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/render_template_document_from_data
            )
        endif()

    endif()
endif()

if(NOT WIN32)
    find_program(FEATHERDOC_CROSS_PLATFORM_POWERSHELL_EXECUTABLE NAMES pwsh)
    if(FEATHERDOC_CROSS_PLATFORM_POWERSHELL_EXECUTABLE)
        set(FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_EXECUTABLE}
            -NoLogo
            -NoProfile
            -NonInteractive
        )
        add_test(
            NAME
            release_candidate_blocker_rollup
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup
        )
        featherdoc_set_test_labels(release_candidate_blocker_rollup release smoke release_smoke)

        add_test(
            NAME
            release_candidate_governance_handoff
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_governance_handoff
            -Scenario
            handoff
        )
        featherdoc_set_test_labels(release_candidate_governance_handoff release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_handoff_report_aggregate
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_aggregate
            -Scenario
            aggregate
        )
        featherdoc_set_test_labels(build_release_governance_handoff_report_aggregate release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_handoff_report_fail_on_blocker
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_fail_on_blocker
            -Scenario
            fail_on_blocker
        )
        featherdoc_set_test_labels(build_release_governance_handoff_report_fail_on_blocker release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_handoff_report_fail_on_warning
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_handoff_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_handoff_report_fail_on_warning
            -Scenario
            fail_on_warning
        )
        featherdoc_set_test_labels(build_release_governance_handoff_report_fail_on_warning release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_pipeline_report_aggregate
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_aggregate
            -Scenario
            aggregate
        )
        featherdoc_set_test_labels(build_release_governance_pipeline_report_aggregate release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_pipeline_report_markdown_counts
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_markdown_counts
            -Scenario
            markdown_counts
        )
        featherdoc_set_test_labels(build_release_governance_pipeline_report_markdown_counts release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_pipeline_report_fail_on_blocker
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_fail_on_blocker
            -Scenario
            fail_on_blocker
        )
        featherdoc_set_test_labels(build_release_governance_pipeline_report_fail_on_blocker release smoke release_smoke governance)

        add_test(
            NAME
            build_release_governance_pipeline_report_fail_on_warning
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/build_release_governance_pipeline_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/build_release_governance_pipeline_report_fail_on_warning
            -Scenario
            fail_on_warning
        )
        featherdoc_set_test_labels(build_release_governance_pipeline_report_fail_on_warning release smoke release_smoke governance)
    endif()
endif()

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
    if(FEATHERDOC_BUILD_PDF AND TARGET FeatherDocPdf)
        target_compile_definitions(cli_tests PRIVATE FEATHERDOC_CLI_ENABLE_PDF=1)
    endif()
    featherdoc_set_test_labels(cli cli heavy)

    featherdoc_add_cli_cpp_test(
        cli_styles_tests
        cli_styles
        cli_styles_tests.cpp
    )
    featherdoc_set_test_labels(cli_styles cli styles heavy)
endif()

configure_file(my_test.docx ${CMAKE_CURRENT_BINARY_DIR}/my_test.docx COPYONLY)
