if(NOT DEFINED CLI_SOURCE_DIR OR CLI_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "CLI_SOURCE_DIR is required")
endif()

file(
    GLOB_RECURSE cli_sources
    LIST_DIRECTORIES false
    "${CLI_SOURCE_DIR}/*.cpp"
    "${CLI_SOURCE_DIR}/*.hpp"
    "${CLI_SOURCE_DIR}/*.inc"
)

set(
    forbidden_archive_tokens
    "#include <zip.h>"
    "zip_open("
    "zip_openwitherror("
    "zip_entry_open("
    "zip_entry_read("
    "zip_close("
)

foreach(source_path IN LISTS cli_sources)
    file(READ "${source_path}" source_text)
    foreach(forbidden_token IN LISTS forbidden_archive_tokens)
        string(FIND "${source_text}" "${forbidden_token}" token_position)
        if(NOT token_position EQUAL -1)
            message(
                FATAL_ERROR
                "CLI source '${source_path}' directly accesses the DOCX ZIP "
                "archive through forbidden token '${forbidden_token}'. Use "
                "Document inspection APIs so open options, archive limits, "
                "and the loaded in-memory snapshot remain authoritative."
            )
        endif()
    endforeach()
endforeach()

message(STATUS "CLI archive boundary check passed")
