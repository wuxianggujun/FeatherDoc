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

        add_test(
            NAME
            package_release_assets_allow_incomplete
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_allow_incomplete_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_allow_incomplete
        )

        add_test(
            NAME
            public_release_wording_regression
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/public_release_wording_regression_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )

        add_test(
            NAME
            sync_github_release_notes
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_document_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_document_review_task
        )

        add_test(
            NAME
            prepare_section_page_setup_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_section_page_setup_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_section_page_setup_review_task
        )

        add_test(
            NAME
            prepare_page_number_fields_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/prepare_page_number_fields_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/prepare_page_number_fields_review_task
        )

        add_test(
            NAME
            open_latest_section_page_setup_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_section_page_setup_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_section_page_setup_review_task
        )

        add_test(
            NAME
            open_latest_page_number_fields_review_task
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/open_latest_page_number_fields_review_task_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/open_latest_page_number_fields_review_task
        )

        add_test(
            NAME
            sync_visual_review_verdict_section_page_setup
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict
        )

        add_test(
            NAME
            sync_latest_visual_review_verdict_curated_visual_bundle
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/sync_latest_visual_review_verdict_curated_visual_bundle_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/sync_latest_visual_review_verdict_curated_visual_bundle
        )

        add_test(
            NAME
            find_superseded_review_tasks
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
            new_project_template_smoke_onboarding_plan
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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

        add_test(
            NAME
            onboard_project_template
            COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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

        if(
            TARGET featherdoc_sample_section_page_setup
            AND TARGET featherdoc_cli
        )
            add_test(
                NAME
                section_page_setup_regression_bundle
                COMMAND
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
        endif()

        if(TARGET featherdoc_cli AND TARGET featherdoc_sample_bookmark_block_visibility_visual AND TARGET featherdoc_sample_part_template_validation)
            add_test(
                NAME
                template_render_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                patch_template_render_plan
                COMMAND
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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
                ${FEATHERDOC_POWERSHELL_EXECUTABLE}
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

if(TARGET featherdoc_cli)
    add_executable(
        cli_tests
        cli_tests.cpp
        cli_styles_tests.cpp
    )

    target_link_libraries(
        cli_tests
        PRIVATE FeatherDoc::FeatherDoc
    )
    if(MSVC)
        target_compile_options(cli_tests PRIVATE /utf-8)
    endif()
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
