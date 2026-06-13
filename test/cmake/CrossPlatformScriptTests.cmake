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
            release_candidate_blocker_rollup_fail_on_blocker
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
            release_candidate_governance_handoff_fail_on_blocker
            COMMAND
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
            ${FEATHERDOC_CROSS_PLATFORM_POWERSHELL_TEST_COMMAND}
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
