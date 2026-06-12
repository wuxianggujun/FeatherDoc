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

        foreach(material_safety_shard RANGE 0 39)
            if(material_safety_shard EQUAL 0)
                set(material_safety_test_name assert_release_material_safety)
            else()
                set(material_safety_test_name assert_release_material_safety_shard_${material_safety_shard})
            endif()
            add_test(
                NAME
                ${material_safety_test_name}
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/assert_release_material_safety_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/${material_safety_test_name}
                -ShardIndex
                ${material_safety_shard}
                -ShardCount
                40
            )
            set_tests_properties(${material_safety_test_name}
                PROPERTIES
                    TIMEOUT 60
                    LABELS "release;smoke;release_smoke;material-safety")
        endforeach()

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
            -Scenario
            candidate_core
        )

        add_test(
            NAME
            release_candidate_visual_verdict_reports
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
            -Scenario
            candidate_reports
        )
        set_tests_properties(release_candidate_visual_verdict_reports
            PROPERTIES
                DEPENDS release_candidate_visual_verdict)

        add_test(
            NAME
            release_candidate_visual_verdict_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_visual_verdict_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_visual_verdict_contract
            -Scenario
            contract
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
            release_candidate_blocker_rollup_fail_on_blocker
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup_fail_on_blocker
            -Scenario
            rollup_fail_on_blocker
        )
        featherdoc_set_test_labels(
            release_candidate_blocker_rollup_fail_on_blocker release smoke release_smoke)

        add_test(
            NAME
            release_candidate_blocker_rollup_fail_on_warning
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup_fail_on_warning
            -Scenario
            rollup_fail_on_warning
        )
        featherdoc_set_test_labels(
            release_candidate_blocker_rollup_fail_on_warning release smoke release_smoke)

        add_test(
            NAME
            release_candidate_blocker_rollup_auto_discover
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup_auto_discover
            -Scenario
            rollup_auto_discover
        )
        featherdoc_set_test_labels(
            release_candidate_blocker_rollup_auto_discover release smoke release_smoke)

        add_test(
            NAME
            release_candidate_blocker_rollup_empty_auto_discover
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_blocker_rollup_empty_auto_discover
            -Scenario
            rollup_empty_auto_discover
        )
        featherdoc_set_test_labels(
            release_candidate_blocker_rollup_empty_auto_discover release smoke release_smoke)

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
            release_candidate_governance_handoff_fail_on_blocker
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_governance_handoff_fail_on_blocker
            -Scenario
            handoff_fail_on_blocker
        )
        featherdoc_set_test_labels(
            release_candidate_governance_handoff_fail_on_blocker release smoke release_smoke governance)

        add_test(
            NAME
            release_candidate_governance_handoff_fail_on_warning
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_candidate_blocker_rollup_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_candidate_governance_handoff_fail_on_warning
            -Scenario
            handoff_fail_on_warning
        )
        featherdoc_set_test_labels(
            release_candidate_governance_handoff_fail_on_warning release smoke release_smoke governance)

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
