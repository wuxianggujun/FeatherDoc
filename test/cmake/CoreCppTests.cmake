set(FEATHERDOC_CLI_DOMAIN_PARSE_SOURCES
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_image_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_table_border_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_table_layout_parse.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_domain_table_text_parse.cpp
)

option(
    FEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS
    "Build and register isolated allocation/OOM failure-injection tests"
    OFF
)

option(
    FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS
    "Build opt-in Windows filesystem failure-injection tests"
    OFF
)

if(WIN32 AND FEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS)
    message(FATAL_ERROR
        "FEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS is unsupported on Windows; "
        "run the isolated allocation-failure suite in Linux/WSL instead."
    )
endif()

if(FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS AND NOT WIN32)
    message(FATAL_ERROR
        "FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS is Windows-only."
    )
endif()

function(featherdoc_add_allocation_failure_cpp_test target_name test_name)
    if(NOT FEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS)
        return()
    endif()

    featherdoc_add_cpp_test_executable(${target_name} ${ARGN})
    target_compile_definitions(
        ${target_name}
        PRIVATE
        FEATHERDOC_ENABLE_ALLOCATION_FAILURE_TESTS=1
        FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY=1
    )
    add_test(
        NAME ${test_name}
        COMMAND ${target_name} --test-suite=allocation-failure
    )
    featherdoc_set_test_labels(
        ${test_name}
        core heavy security xml allocation-failure
    )
    set_tests_properties(
        ${test_name}
        PROPERTIES
        RUN_SERIAL TRUE
        TIMEOUT 180
    )
endfunction()

function(featherdoc_add_pugixml_iterative_destroy_test target_name test_name)
    add_executable(
        ${target_name}
        pugixml_iterative_destroy_tests.cpp
        ${PROJECT_SOURCE_DIR}/thirdparty/pugixml/pugixml.cpp
    )
    target_include_directories(
        ${target_name}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/thirdparty/pugixml
    )
    if(ARGN)
        target_compile_definitions(${target_name} PRIVATE ${ARGN})
    endif()
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(WIN32)
        add_test(NAME ${test_name} COMMAND ${target_name} --no-breaks=true)
    else()
        add_test(NAME ${test_name} COMMAND ${target_name})
    endif()
    featherdoc_set_test_labels(${test_name} core heavy security xml)
endfunction()

featherdoc_add_pugixml_iterative_destroy_test(
    pugixml_iterative_destroy_tests
    pugixml_iterative_destroy
)
featherdoc_add_pugixml_iterative_destroy_test(
    pugixml_iterative_destroy_compact_tests
    pugixml_iterative_destroy_compact
    PUGIXML_COMPACT
)

featherdoc_add_cpp_test(
    package_path_helpers_tests
    package_path_helpers
    package_path_helpers_tests.cpp
)
featherdoc_set_test_labels(package_path_helpers core security utf8)

featherdoc_add_cpp_test(
    xml_document_clone_tests
    xml_document_clone
    xml_document_clone_tests.cpp
)
featherdoc_set_test_labels(xml_document_clone core security xml)

featherdoc_add_cpp_test(
    xml_handle_retirement_tests
    xml_handle_retirement
    xml_handle_retirement_tests.cpp
)
featherdoc_set_test_labels(xml_handle_retirement core security xml handles)

featherdoc_add_cpp_test(
    text_mutation_transaction_tests
    text_mutation_transaction
    text_mutation_transaction_tests.cpp
)
featherdoc_set_test_labels(
    text_mutation_transaction
    core security xml handles text transactions utf8
)

featherdoc_add_cpp_test(
    bookmark_batch_transaction_tests
    bookmark_batch_transaction
    bookmark_batch_transaction_tests.cpp
)
featherdoc_set_test_labels(
    bookmark_batch_transaction
    core security xml handles templates bookmarks transactions utf8
)

featherdoc_add_cpp_test(
    revision_transaction_tests
    revision_transaction
    revision_transaction_tests.cpp
)
featherdoc_set_test_labels(
    revision_transaction
    core security xml handles reviews revisions transactions utf8
)

featherdoc_add_cpp_test(
    package_relationships_mce_tests
    package_relationships_mce
    package_relationships_mce_tests.cpp
)
featherdoc_set_test_labels(package_relationships_mce core security xml)

featherdoc_add_cpp_test(
    wordprocessingml_namespace_tests
    wordprocessingml_namespace
    wordprocessingml_namespace_tests.cpp
)
featherdoc_set_test_labels(wordprocessingml_namespace core security xml)

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
    document_core_unit_tests
    document_core_unit
    document_core_unit_tests.cpp
    document_core_open_save_tests.cpp
    document_core_run_tests.cpp
    document_core_paragraph_tests.cpp
    document_core_bookmark_section_tests.cpp
    document_move_lifetime_tests.cpp
    document_archive_utf8_tests.cpp
    document_unicode_relationship_part_tests.cpp
    document_security_tests.cpp
    document_source_archive_consistency_tests.cpp
)
featherdoc_set_test_labels(document_core_unit core heavy document)

if(FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS)
    target_compile_definitions(
        document_core_unit_tests
        PRIVATE FEATHERDOC_ENABLE_WINDOWS_FAULT_INJECTION_TESTS=1
    )
endif()

if(MSVC)
    target_compile_options(document_core_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    semantic_diff_unit_tests
    semantic_diff_unit
    semantic_diff_unit_tests.cpp
)
featherdoc_set_test_labels(semantic_diff_unit core heavy semantic_diff)

if(MSVC)
    target_compile_options(semantic_diff_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    body_image_unit_tests
    body_image_unit
    body_image_unit_tests.cpp
)
featherdoc_set_test_labels(body_image_unit core heavy images)

target_include_directories(
    body_image_unit_tests
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(body_image_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    numbering_unit_tests
    numbering_unit
    numbering_unit_tests.cpp
    numbering_catalog_edge_tests.cpp
)
featherdoc_set_test_labels(numbering_unit core heavy numbering)

if(MSVC)
    target_compile_options(numbering_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    style_management_unit_tests
    style_management_unit
    style_management_unit_tests.cpp
    style_management_numbering_tests.cpp
    style_management_catalog_usage_tests.cpp
    style_management_refactor_tests.cpp
    style_management_prune_clear_tests.cpp
)
featherdoc_set_test_labels(style_management_unit core heavy styles)

if(MSVC)
    target_compile_options(style_management_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    run_formatting_unit_tests
    run_formatting_unit
    run_formatting_unit_tests.cpp
)
featherdoc_set_test_labels(run_formatting_unit core heavy runs styles)

if(MSVC)
    target_compile_options(run_formatting_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    direction_unit_tests
    direction_unit
    direction_unit_tests.cpp
)
featherdoc_set_test_labels(direction_unit core heavy direction styles)

if(MSVC)
    target_compile_options(direction_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    style_definition_unit_tests
    style_definition_unit
    style_definition_unit_tests.cpp
    style_definition_basic_tests.cpp
    style_definition_table_tests.cpp
    style_definition_resolution_tests.cpp
    style_definition_rebase_tests.cpp
)
featherdoc_set_test_labels(style_definition_unit core heavy styles)

if(MSVC)
    target_compile_options(style_definition_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    section_page_setup_unit_tests
    section_page_setup_unit
    section_page_setup_unit_tests.cpp
)
featherdoc_set_test_labels(section_page_setup_unit core heavy section)

if(MSVC)
    target_compile_options(section_page_setup_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    inspection_unit_tests
    inspection_unit
    inspection_unit_tests.cpp
)
featherdoc_set_test_labels(inspection_unit core heavy inspection)

if(MSVC)
    target_compile_options(inspection_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    template_table_handle_unit_tests
    template_table_handle_unit
    template_table_handle_unit_tests.cpp
)
featherdoc_set_test_labels(template_table_handle_unit core heavy templates tables)

if(MSVC)
    target_compile_options(template_table_handle_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    template_part_media_unit_tests
    template_part_media_unit
    template_part_media_unit_tests.cpp
    template_part_media_bookmark_tests.cpp
    template_part_media_image_reference_tests.cpp
    template_part_media_append_tests.cpp
)
featherdoc_set_test_labels(template_part_media_unit core heavy templates images)

if(MSVC)
    target_compile_options(template_part_media_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    template_part_content_unit_tests
    template_part_content_unit
    template_part_content_unit_tests.cpp
    template_part_content_review_omml_tests.cpp
)
featherdoc_set_test_labels(template_part_content_unit core heavy templates fields)

if(MSVC)
    target_compile_options(template_part_content_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    template_schema_unit_tests
    template_schema_unit
    template_schema_unit_tests.cpp
    template_schema_patch_tests.cpp
    template_schema_validation_tests.cpp
    template_schema_scan_onboard_tests.cpp
)
featherdoc_set_test_labels(template_schema_unit core heavy templates schema)

if(MSVC)
    target_compile_options(template_schema_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    template_validation_unit_tests
    template_validation_unit
    template_validation_unit_tests.cpp
    template_validation_media_block_tests.cpp
)
featherdoc_set_test_labels(template_validation_unit core heavy templates validation)

if(MSVC)
    target_compile_options(template_validation_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    content_controls_unit_tests
    content_controls_unit
    content_controls_unit_tests.cpp
    content_controls_image_replacement_tests.cpp
)
featherdoc_set_test_labels(content_controls_unit core heavy templates content-controls)

if(MSVC)
    target_compile_options(content_controls_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_allocation_failure_cpp_test(
    xml_document_clone_allocation_failure_tests
    xml_document_clone_allocation_failure
    xml_document_clone_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    xml_handle_retirement_allocation_failure_tests
    xml_handle_retirement_allocation_failure
    xml_handle_retirement_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    text_mutation_transaction_allocation_failure_tests
    text_mutation_transaction_allocation_failure
    text_mutation_transaction_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    bookmark_batch_transaction_allocation_failure_tests
    bookmark_batch_transaction_allocation_failure
    bookmark_batch_transaction_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    revision_transaction_allocation_failure_tests
    revision_transaction_allocation_failure
    revision_transaction_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    package_relationships_mce_allocation_failure_tests
    package_relationships_mce_allocation_failure
    package_relationships_mce_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    document_core_allocation_failure_tests
    document_core_allocation_failure
    document_core_unit_tests.cpp
    document_core_open_save_tests.cpp
    document_core_bookmark_section_tests.cpp
    document_repair_allocation_tests.cpp
    document_save_clone_allocation_tests.cpp
    document_security_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    style_management_allocation_failure_tests
    style_management_allocation_failure
    style_management_unit_tests.cpp
    style_management_refactor_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    content_controls_allocation_failure_tests
    content_controls_allocation_failure
    content_controls_unit_tests.cpp
)

featherdoc_add_cpp_test(
    review_revisions_unit_tests
    review_revisions_unit
    review_revisions_unit_tests.cpp
    review_revisions_text_range_tests.cpp
)
featherdoc_set_test_labels(review_revisions_unit core heavy reviews revisions)

featherdoc_add_allocation_failure_cpp_test(
    review_revisions_allocation_failure_tests
    review_revisions_allocation_failure
    review_revisions_unit_tests.cpp
    review_comment_allocation_tests.cpp
)

if(MSVC)
    target_compile_options(review_revisions_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    section_inspection_unit_tests
    section_inspection_unit
    section_inspection_unit_tests.cpp
)
featherdoc_set_test_labels(section_inspection_unit core heavy inspection section)

if(MSVC)
    target_compile_options(section_inspection_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    section_header_footer_unit_tests
    section_header_footer_unit
    section_header_footer_unit_tests.cpp
    section_header_footer_reference_tests.cpp
    section_header_footer_part_lifecycle_tests.cpp
    section_header_footer_section_flow_tests.cpp
    section_header_footer_attachment_transaction_tests.cpp
    section_header_footer_text_allocation_tests.cpp
)
featherdoc_set_test_labels(section_header_footer_unit core heavy section headers footers)

if(MSVC)
    target_compile_options(section_header_footer_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_allocation_failure_cpp_test(
    section_header_footer_allocation_failure_tests
    section_header_footer_allocation_failure
    section_header_footer_unit_tests.cpp
    section_header_footer_section_flow_tests.cpp
    section_header_footer_text_allocation_tests.cpp
)

featherdoc_add_allocation_failure_cpp_test(
    section_header_footer_part_removal_allocation_failure_tests
    section_header_footer_part_removal_allocation_failure
    section_header_footer_part_removal_allocation_tests.cpp
)

featherdoc_add_cpp_test(
    table_structure_unit_tests
    table_structure_unit
    table_structure_unit_tests.cpp
    table_structure_cell_column_tests.cpp
    table_structure_column_removal_tests.cpp
    table_structure_row_table_tests.cpp
    table_structure_merge_tests.cpp
    table_structure_unmerge_tests.cpp
)
featherdoc_set_test_labels(table_structure_unit core heavy tables structure)

if(MSVC)
    target_compile_options(table_structure_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    table_style_unit_tests
    table_style_unit
    table_style_unit_tests.cpp
)
featherdoc_set_test_labels(table_style_unit core heavy tables styles)

if(MSVC)
    target_compile_options(table_style_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    table_layout_unit_tests
    table_layout_unit
    table_layout_unit_tests.cpp
    table_layout_properties_tests.cpp
    table_layout_fixed_grid_tests.cpp
    table_layout_fixed_grid_remove_tests.cpp
    table_layout_reopened_workflow_tests.cpp
)
featherdoc_set_test_labels(table_layout_unit core heavy tables layout)

if(MSVC)
    target_compile_options(table_layout_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    table_presentation_unit_tests
    table_presentation_unit
    table_presentation_unit_tests.cpp
)
featherdoc_set_test_labels(table_presentation_unit core heavy tables presentation)

if(MSVC)
    target_compile_options(table_presentation_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    table_properties_unit_tests
    table_properties_unit
    table_properties_unit_tests.cpp
)
featherdoc_set_test_labels(table_properties_unit core heavy tables properties)

if(MSVC)
    target_compile_options(table_properties_unit_tests PRIVATE /utf-8)
endif()

featherdoc_add_cpp_test(
    public_header_entrypoint_tests
    public_header_entrypoint
    public_header_entrypoint_tests.cpp
)
featherdoc_set_test_labels(public_header_entrypoint core smoke)

featherdoc_add_cpp_test(
    source_compat_v1_13_2_tests
    source_compat_v1_13_2
    source_compat_v1_13_2.cpp
)
featherdoc_set_test_labels(source_compat_v1_13_2 core smoke compatibility)

featherdoc_add_cpp_test(
    document_move_noallocation_tests
    document_move_noallocation
    document_move_noallocation_tests.cpp
)
featherdoc_set_test_labels(
    document_move_noallocation
    core smoke compatibility security
)
set_tests_properties(
    document_move_noallocation
    PROPERTIES SKIP_RETURN_CODE 77
)
if(MSVC)
    target_compile_options(
        document_move_noallocation_tests
        PRIVATE /W4 /permissive- /utf-8
    )
else()
    target_compile_options(
        document_move_noallocation_tests
        PRIVATE -Wall -Wextra -Wpedantic
    )
endif()

# Compile a standalone consumer against the exact v1.13.3 public headers while
# linking it to the current library. This catches ABI layout regressions that a
# source-compatibility test using current headers cannot observe.
add_executable(
    abi_compat_v1_13_3_runtime_tests
    abi_compat_v1_13_3_runtime.cpp
)
target_include_directories(
    abi_compat_v1_13_3_runtime_tests
    BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/compatibility/v1_13_3/snapshot/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compatibility/v1_13_3/snapshot/thirdparty/pugixml
)
target_link_libraries(
    abi_compat_v1_13_3_runtime_tests
    PRIVATE FeatherDoc::FeatherDoc
)
featherdoc_copy_runtime_dlls(abi_compat_v1_13_3_runtime_tests)
if(MSVC)
    target_compile_options(
        abi_compat_v1_13_3_runtime_tests
        PRIVATE /W4 /permissive- /utf-8
    )
else()
    target_compile_options(
        abi_compat_v1_13_3_runtime_tests
        PRIVATE -Wall -Wextra -Wpedantic
    )
endif()
add_test(
    NAME abi_compat_v1_13_3_runtime
    COMMAND abi_compat_v1_13_3_runtime_tests
)
featherdoc_set_test_labels(
    abi_compat_v1_13_3_runtime
    core smoke compatibility abi utf8
)

featherdoc_add_cpp_test(
    public_header_self_contained_tests
    public_header_self_contained
    public_header_self_contained_main.cpp
    public_header_self_contained_fwd_tests.cpp
    public_header_self_contained_document_core_tests.cpp
    public_header_self_contained_tables_tests.cpp
    public_header_self_contained_templates_tests.cpp
    public_header_self_contained_styles_numbering_tests.cpp
    public_header_self_contained_reviews_fields_tests.cpp
    public_header_self_contained_text_tests.cpp
    public_header_self_contained_template_part_tests.cpp
    public_header_self_contained_document_tests.cpp
    public_header_self_contained_core_tests.cpp
)
featherdoc_set_test_labels(public_header_self_contained core smoke)

featherdoc_add_cpp_test(
    iterator_tests
    iterator
    iterator_tests.cpp
)
featherdoc_set_test_labels(iterator core smoke)
