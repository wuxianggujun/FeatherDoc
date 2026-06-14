        if(TARGET pdf_unicode_font_roundtrip_tests)
            add_test(
                NAME
                pdf_unicode_font_roundtrip_visual
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/run_pdf_unicode_font_roundtrip_visual_regression.ps1
                -BuildDir
                ${CMAKE_BINARY_DIR}
                -OutputDir
                ${CMAKE_BINARY_DIR}/pdf_unicode_font_visual_regression
                -CtestExecutable
                ${CMAKE_CTEST_COMMAND}
            )
            if(FEATHERDOC_PDF_TEST_ENVIRONMENT STREQUAL "")
                set_tests_properties(pdf_unicode_font_roundtrip_visual
                    PROPERTIES
                        TIMEOUT 60
                        LABELS "pdf;smoke;regression;visual"
                        SKIP_RETURN_CODE 77)
            else()
                set_tests_properties(pdf_unicode_font_roundtrip_visual
                    PROPERTIES
                        TIMEOUT 60
                        LABELS "pdf;smoke;regression;visual"
                        SKIP_RETURN_CODE 77
                        ENVIRONMENT "${FEATHERDOC_PDF_TEST_ENVIRONMENT}")
            endif()
        endif()

        add_test(
            NAME
            pdf_visual_release_gate_style_baselines
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_style_baselines_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_style_baselines
        )
        set_tests_properties(pdf_visual_release_gate_style_baselines
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_release_gate_text_shaping_baselines
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_text_shaping_baselines_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_text_shaping_baselines
        )
        set_tests_properties(pdf_visual_release_gate_text_shaping_baselines
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_dependency_inputs_check
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_dependency_inputs_check_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_dependency_inputs_check
        )
        set_tests_properties(pdf_dependency_inputs_check
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        add_test(
            NAME
            pdf_visual_release_gate_preflight
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_preflight_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_preflight
            -Scenario
            synthetic
        )
        set_tests_properties(pdf_visual_release_gate_preflight
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        foreach(pdf_preflight_scenario IN ITEMS plain malformed disabled contract)
            add_test(
                NAME
                pdf_visual_release_gate_preflight_${pdf_preflight_scenario}
                COMMAND
                ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
                -ExecutionPolicy
                Bypass
                -File
                ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_preflight_test.ps1
                -RepoRoot
                ${PROJECT_SOURCE_DIR}
                -WorkingDir
                ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_preflight_${pdf_preflight_scenario}
                -Scenario
                ${pdf_preflight_scenario}
            )
            set_tests_properties(pdf_visual_release_gate_preflight_${pdf_preflight_scenario}
                PROPERTIES
                    TIMEOUT 60
                    LABELS "pdf;smoke;regression;visual")
        endforeach()

        add_test(
            NAME
            pdf_visual_release_gate_preflight_governance_report
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_release_gate_preflight_governance_report_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_release_gate_preflight_governance_report
        )
        set_tests_properties(pdf_visual_release_gate_preflight_governance_report
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_release_readiness_check
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_release_readiness_check_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_release_readiness_check
        )
        set_tests_properties(pdf_release_readiness_check
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs;visual")

        add_test(
            NAME
            pdf_visual_full_gate_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_full_gate_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_full_gate_guarded
        )
        set_tests_properties(pdf_visual_full_gate_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_full_ctest_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_full_ctest_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_full_ctest_guarded
        )
        set_tests_properties(pdf_full_ctest_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            pdf_ctest_remaining_guarded
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_remaining_guarded_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_ctest_remaining_guarded
        )
        set_tests_properties(pdf_ctest_remaining_guarded
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            pdf_ctest_bounded_subset_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_bounded_subset_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_ctest_bounded_subset_summary
        )
        set_tests_properties(pdf_ctest_bounded_subset_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression")

        add_test(
            NAME
            pdf_visual_gate_attempt_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_gate_attempt_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_gate_attempt_summary
        )
        set_tests_properties(pdf_visual_gate_attempt_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_segmented_gate_summary
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_segmented_gate_summary_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/pdf_visual_segmented_gate_summary
        )
        set_tests_properties(pdf_visual_segmented_gate_summary
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;regression;visual")

        add_test(
            NAME
            pdf_visual_validation_status_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_visual_validation_status_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_visual_validation_status_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        add_test(
            NAME
            pdf_real_business_sample_manifest_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_real_business_sample_manifest_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_real_business_sample_manifest_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;smoke;docs")

        set(FEATHERDOC_CONTROLLED_VISUAL_SMOKE_PYTHON_ARGS)
