        add_test(
            NAME
            release_note_bundle_version
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
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
            release_note_bundle_visual_verdict_metadata
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_note_bundle_visual_verdict_metadata_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_note_bundle_visual_verdict_metadata
        )

        add_test(
            NAME
            release_blocker_action_registry
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_blocker_action_registry_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_blocker_action_registry
        )

        add_test(
            NAME
            run_reused_build_check_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/run_reused_build_check_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/run_reused_build_check_contract
        )
        set_tests_properties(run_reused_build_check_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_warning_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_warning_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_warning_contract
        )
        set_tests_properties(release_governance_warning_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            github_actions_workflow_maintenance
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/github_actions_workflow_maintenance_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(github_actions_workflow_maintenance
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_warning_helper_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_warning_helper_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_warning_helper_contract
        )
        set_tests_properties(release_governance_warning_helper_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_metrics_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_metrics_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/release_governance_metrics_contract
        )
        set_tests_properties(release_governance_metrics_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            release_governance_detail_rollup_static_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/release_governance_detail_rollup_static_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(release_governance_detail_rollup_static_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "release;governance;smoke")

        add_test(
            NAME
            package_release_assets_safety
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_safety_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_safety
        )
        set_tests_properties(package_release_assets_safety
            PROPERTIES
                TIMEOUT 60
                LABELS "release;package;smoke")

        add_test(
            NAME
            package_release_assets_allow_incomplete
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/package_release_assets_allow_incomplete_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
            -WorkingDir
            ${CMAKE_CURRENT_BINARY_DIR}/package_release_assets_allow_incomplete
        )
        set_tests_properties(package_release_assets_allow_incomplete
            PROPERTIES
                TIMEOUT 60
                LABELS "release;package;smoke")

        add_test(
            NAME
            public_release_wording_regression
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/public_release_wording_regression_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )

        add_test(
            NAME
            pdf_import_docs_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_import_docs_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_import_docs_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;docs;smoke")

        add_test(
            NAME
            pdf_ctest_timeout_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_timeout_contract_test.ps1
            -CTestTestfile
            ${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake
        )
        set_tests_properties(pdf_ctest_timeout_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;ctest;smoke")

        add_test(
            NAME
            pdf_ctest_label_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_ctest_label_contract_test.ps1
            -CTestTestfile
            ${CMAKE_CURRENT_BINARY_DIR}/CTestTestfile.cmake
        )
        set_tests_properties(pdf_ctest_label_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;ctest;smoke")

        add_test(
            NAME
            pdf_bidi_line_layout_static_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_bidi_line_layout_static_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_bidi_line_layout_static_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_style_gallery_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_style_gallery_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_style_gallery_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_font_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_font_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_font_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_font_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_font_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_font_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_copy_search_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_copy_search_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_copy_search_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_embed_matrix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_embed_matrix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_embed_matrix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_anchor_font_matrix_boundary_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_anchor_font_matrix_boundary_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_anchor_font_matrix_boundary_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_search_density_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_search_density_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_search_density_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_font_embed_wrap_mix_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_font_embed_wrap_mix_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_font_embed_wrap_mix_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_repeated_key_boundary_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_repeated_key_boundary_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_repeated_key_boundary_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_style_overlay_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_style_overlay_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_style_overlay_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_complex_layout_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_complex_layout_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_complex_layout_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_image_wrap_stress_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_image_wrap_stress_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_image_wrap_stress_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_extreme_page_breaks_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_extreme_page_breaks_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_extreme_page_breaks_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_vertical_merge_wrap_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_vertical_merge_wrap_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_vertical_merge_wrap_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_wrap_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_wrap_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_cjk_wrap_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_vertical_merged_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_vertical_merged_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(
            pdf_document_table_cjk_vertical_merged_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_cjk_merged_repeat_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_cjk_merged_repeat_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_document_table_cjk_merged_repeat_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_vertical_merged_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_vertical_merged_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(
            pdf_document_table_vertical_merged_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_document_table_merged_cant_split_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_document_table_merged_cant_split_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(
            pdf_document_table_merged_cant_split_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_rtl_bidi_light_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_rtl_bidi_light_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_rtl_bidi_light_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_list_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_list_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_list_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_table_wrap_page_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_table_wrap_page_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_table_wrap_page_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_cjk_multi_anchor_table_flow_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_cjk_multi_anchor_table_flow_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_cjk_multi_anchor_table_flow_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")

        add_test(
            NAME
            pdf_text_decoration_contract
            COMMAND
            ${FEATHERDOC_POWERSHELL_TEST_COMMAND}
            -ExecutionPolicy
            Bypass
            -File
            ${CMAKE_CURRENT_SOURCE_DIR}/pdf_text_decoration_contract_test.ps1
            -RepoRoot
            ${PROJECT_SOURCE_DIR}
        )
        set_tests_properties(pdf_text_decoration_contract
            PROPERTIES
                TIMEOUT 60
                LABELS "pdf;layout;smoke")
