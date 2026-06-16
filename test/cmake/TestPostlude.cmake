set(FEATHERDOC_CLI_JSON_PARSE_EXTRA_SOURCES
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse_members.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse_string.cpp
    ${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse_value.cpp
)
get_property(FEATHERDOC_TEST_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(featherdoc_test_target IN LISTS FEATHERDOC_TEST_TARGETS)
    get_target_property(
        featherdoc_test_sources ${featherdoc_test_target} SOURCES)
    if(featherdoc_test_sources)
        list(FIND featherdoc_test_sources
            "${PROJECT_SOURCE_DIR}/cli/featherdoc_cli_json_parse.cpp"
            featherdoc_json_parse_source_index)
        if(NOT featherdoc_json_parse_source_index EQUAL -1)
            target_sources(${featherdoc_test_target}
                PRIVATE ${FEATHERDOC_CLI_JSON_PARSE_EXTRA_SOURCES})
        endif()
    endif()
endforeach()

configure_file(my_test.docx ${CMAKE_CURRENT_BINARY_DIR}/my_test.docx COPYONLY)
