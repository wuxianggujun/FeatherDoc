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
            foreach(section_page_setup_regression_case IN ITEMS api-sample cli-rewrite)
                add_test(
                    NAME
                    section_page_setup_regression_bundle_${section_page_setup_regression_case}
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
                    ${CMAKE_CURRENT_BINARY_DIR}/section_page_setup_regression_bundle_${section_page_setup_regression_case}
                    -CaseId
                    ${section_page_setup_regression_case}
                )
            endforeach()
        endif()

        if(
            TARGET featherdoc_sample_page_number_fields
            AND TARGET featherdoc_sample_section_page_setup
            AND TARGET featherdoc_cli
        )
            foreach(page_number_fields_regression_case IN ITEMS api-sample cli-append)
                add_test(
                    NAME
                    page_number_fields_regression_bundle_${page_number_fields_regression_case}
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
                    ${CMAKE_CURRENT_BINARY_DIR}/page_number_fields_regression_bundle_${page_number_fields_regression_case}
                    -CaseId
                    ${page_number_fields_regression_case}
                )
            endforeach()
        endif()

        if(
            TARGET featherdoc_sample_template_validation
            AND TARGET featherdoc_sample_part_template_validation
            AND TARGET featherdoc_cli
        )
            foreach(template_validation_regression_case IN ITEMS body-template part-template)
                add_test(
                    NAME
                    template_validation_regression_bundle_${template_validation_regression_case}
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
                    ${CMAKE_CURRENT_BINARY_DIR}/template_validation_regression_bundle_${template_validation_regression_case}
                    -CaseId
                    ${template_validation_regression_case}
                )
            endforeach()
        endif()
