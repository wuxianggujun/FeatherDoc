add_executable(
    unit_tests
    basic_tests.cpp
)

target_link_libraries(
    unit_tests
    PRIVATE FeatherDoc::FeatherDoc
)
if(MSVC)
    target_compile_options(unit_tests PRIVATE /bigobj)
endif()
featherdoc_copy_runtime_dlls(unit_tests)

add_test(
    NAME
    unit
    COMMAND
    unit_tests
)

add_executable(
    iterator_tests
    iterator_tests.cpp
)

target_link_libraries(
    iterator_tests
    PRIVATE FeatherDoc::FeatherDoc
)
featherdoc_copy_runtime_dlls(iterator_tests)

add_test(
    NAME
    iterator
    COMMAND
    iterator_tests
)

if(WIN32)
    find_program(FEATHERDOC_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
    if(FEATHERDOC_POWERSHELL_EXECUTABLE)
        add_test(
            NAME
            release_note_bundle_version
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_note_bundle_version_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_note_bundle_version
        )
    endif()
endif()

if(TARGET featherdoc_cli)
    add_executable(
        cli_tests
        cli_tests.cpp
    )

    target_link_libraries(
        cli_tests
        PRIVATE FeatherDoc::FeatherDoc
    )
    featherdoc_copy_runtime_dlls(cli_tests)

    add_dependencies(cli_tests featherdoc_cli)
    add_custom_command(
        TARGET cli_tests
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:featherdoc_cli>
            $<TARGET_FILE_DIR:cli_tests>
    )
    if(WIN32 AND BUILD_SHARED_LIBS)
        add_custom_command(
            TARGET cli_tests
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:featherdoc_cli>
                $<TARGET_FILE_DIR:cli_tests>
            COMMAND_EXPAND_LISTS
        )
    endif()

    add_test(
        NAME
        cli
        COMMAND
        cli_tests
    )
endif()

configure_file(my_test.docx ${CMAKE_CURRENT_BINARY_DIR}/my_test.docx COPYONLY)
