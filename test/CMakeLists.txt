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
        find_program(FEATHERDOC_PYTHON_EXECUTABLE NAMES python python3)

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

        add_test(
            NAME
            package_release_assets_safety
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_safety_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_safety
        )

        if(FEATHERDOC_PYTHON_EXECUTABLE)
            add_test(
                NAME
                docs_conf_version_sync
                COMMAND
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
