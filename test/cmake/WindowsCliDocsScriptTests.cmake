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
