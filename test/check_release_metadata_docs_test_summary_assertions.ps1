function Assert-PassingReleaseMetadataDocsSummary {
    param(
        [object]$Summary,
        [string]$SummaryJsonPath
    )

    $expectedSummaryJsonPath = [System.IO.Path]::GetFullPath($summaryJsonPath)
    if ($summary.summary_json_path -ne $expectedSummaryJsonPath) {
        throw "Expected JSON summary path '$expectedSummaryJsonPath', got: $($summary.summary_json_path)"
    }
    if ($summary.summary_json_relative_path -ne "docs-check-summary.json") {
        throw "Expected JSON relative summary path docs-check-summary.json, got: $($summary.summary_json_relative_path)"
    }
    if ($summary.summary_schema_version -ne 1) {
        throw "Expected JSON summary schema version 1, got: $($summary.summary_schema_version)"
    }
    Assert-SummaryAuditFields -Summary $summary
    Assert-SummaryMarkerCountsConsistent -Summary $summary
    Assert-SummaryCheckedDocumentsConsistent -Summary $summary
    if ($summary.checked_document_count -ne 7) {
        throw "Expected JSON summary checked document count 7, got: $($summary.checked_document_count)"
    }
    if ($summary.required_pipeline_marker_count -ne 143) {
        throw "Expected JSON summary pipeline marker count 143, got: $($summary.required_pipeline_marker_count)"
    }
    if ($summary.required_checklist_marker_count -ne 137) {
        throw "Expected JSON summary checklist marker count 137, got: $($summary.required_checklist_marker_count)"
    }
    if ($summary.required_document_governance_marker_count -ne 23) {
        throw "Expected JSON summary document governance marker count 23, got: $($summary.required_document_governance_marker_count)"
    }
    if ($summary.required_policy_marker_count -ne 24) {
        throw "Expected JSON summary policy marker count 24, got: $($summary.required_policy_marker_count)"
    }
    if ($summary.required_entrypoint_marker_count -ne 3) {
        throw "Expected JSON summary entrypoint marker count 3, got: $($summary.required_entrypoint_marker_count)"
    }
    if ($summary.required_marker_count -ne 330) {
        throw "Expected JSON summary total marker count 330, got: $($summary.required_marker_count)"
    }
    if ($summary.checked_documents.Count -ne 7) {
        throw "Expected JSON summary to list 7 checked documents, got: $($summary.checked_documents.Count)"
    }
    Assert-ArrayContains `
        -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
        -ExpectedValue 'docs/release_metadata_pipeline_zh.rst' `
        -Message "JSON summary should list the release metadata pipeline doc."
    Assert-ArrayContains `
        -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
        -ExpectedValue 'docs/documentation_maintenance_zh.rst' `
        -Message "JSON summary should list the documentation maintenance overview doc."
    Assert-ArrayContains `
        -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
        -ExpectedValue 'docs/pdf_release_readiness_checklist_zh.rst' `
        -Message "JSON summary should list the PDF release readiness checklist doc."
    Assert-ArrayContains `
        -Values @($summary.checked_document_labels) `
        -ExpectedValue 'PDF release readiness checklist doc' `
        -Message "JSON summary labels should list the PDF release readiness checklist doc."
    Assert-ArrayContains `
        -Values @($summary.checked_document_labels) `
        -ExpectedValue 'documentation maintenance overview doc' `
        -Message "JSON summary labels should list the documentation maintenance overview doc."
    Assert-ArrayContains `
        -Values @($summary.checked_document_relative_paths) `
        -ExpectedValue 'docs/documentation_maintenance_zh.rst' `
        -Message "JSON summary relative paths should list the documentation maintenance overview doc."
    Assert-ArrayContains `
        -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
        -ExpectedValue 'docs/document_governance_acceptance_zh.rst' `
        -Message "JSON summary should list the document governance acceptance doc."
    Assert-ArrayContains `
        -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
        -ExpectedValue 'docs/index.rst' `
        -Message "JSON summary should list the Sphinx index doc."
    Assert-ArrayContains `
        -Values @($summary.required_entrypoint_markers) `
        -ExpectedValue "release_metadata_pipeline_zh" `
        -Message "JSON summary should list release metadata pipeline entrypoint marker."
    Assert-ArrayContains `
        -Values @($summary.required_entrypoint_markers) `
        -ExpectedValue "release_metadata_maintenance_checklist_zh" `
        -Message "JSON summary should list release metadata maintenance checklist entrypoint marker."
    Assert-ArrayContains `
        -Values @($summary.required_entrypoint_markers) `
        -ExpectedValue "pdf_release_readiness_checklist_zh" `
        -Message "JSON summary should list PDF release readiness checklist entrypoint marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release_note_bundle_visual_verdict_metadata" `
        -Message "JSON summary should list required checklist markers."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "Word visual standard review metadata evidence" `
        -Message "JSON summary checklist should list Word visual metadata compact evidence marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
        -Message "JSON summary checklist should list Word visual metadata source reports marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "task_reviews=" `
        -Message "JSON summary checklist should list Word visual metadata task review compact marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "source_schema=featherdoc.release_candidate_summary" `
        -Message "JSON summary checklist should list Word visual metadata source schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release-candidate-checks" `
        -Message "JSON summary checklist should list release-candidate summary source marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release-candidate-checks-source" `
        -Message "JSON summary checklist should list source-only release candidate summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release-candidate-checks\report\summary.json" `
        -Message "JSON summary checklist should list final release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release-candidate-checks-source\summary.json" `
        -Message "JSON summary checklist should list source-only release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``output_gap_count``' `
        -Message "JSON summary should list PDF preflight output gap count marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``missing outputs``' `
        -Message "JSON summary should list PDF preflight missing outputs marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``featherdoc.word_visual_release_gate_preflight.v1``' `
        -Message "JSON summary should list Word visual release gate preflight schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``output_encoding``' `
        -Message "JSON summary should list Word visual preflight output encoding field marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue 'UTF-8 without BOM' `
        -Message "JSON summary should list Word visual preflight output encoding value marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "docx_functional_smoke_readiness" `
        -Message "JSON summary should list DOCX functional smoke readiness marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "project_template_workflow_dashboard_report" `
        -Message "JSON summary should list project-template workflow dashboard marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``featherdoc.docx_functional_smoke_readiness.v1``' `
        -Message "JSON summary should list DOCX functional smoke readiness schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``featherdoc.release_governance_local_closure.v1``' `
        -Message "JSON summary should list release governance local closure schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "local_governance_closure" `
        -Message "JSON summary should list release governance local closure marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "local_governance_closure.status" `
        -Message "JSON summary should list local closure status marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "local_governance_closure.closed" `
        -Message "JSON summary should list local closure closed marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "governance_detail_source" `
        -Message "JSON summary should list local closure governance detail source marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "pipeline_summary_json" `
        -Message "JSON summary should list local closure pipeline summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "pipeline_summary_json_display" `
        -Message "JSON summary should list local closure pipeline summary display marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "pipeline_report_markdown" `
        -Message "JSON summary should list local closure pipeline Markdown path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "pipeline_report_markdown_display" `
        -Message "JSON summary should list local closure pipeline Markdown display marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release_governance_handoff.action_items[]" `
        -Message "JSON summary should list release governance handoff action item detail marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``featherdoc.release_governance_pipeline_report.v1``' `
        -Message "JSON summary should list release governance pipeline report schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``stages[]``' `
        -Message "JSON summary should list release governance pipeline stages marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue '``source_json_display``' `
        -Message "JSON summary should list release governance source JSON display marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "blocker_source_schema_summary" `
        -Message "JSON summary should list release blocker source schema summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "action_item_source_schema_summary" `
        -Message "JSON summary should list release action item source schema summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "informational_action_item_source_schema_summary" `
        -Message "JSON summary should list informational action item source schema summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "warning_source_schema_summary" `
        -Message "JSON summary should list release warning source schema summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "final_governance_report_count" `
        -Message "JSON summary should list local closure final governance report count marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "final_governance_reports" `
        -Message "JSON summary should list local closure final governance reports marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "required_stage_count" `
        -Message "JSON summary should list local closure required stage count marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "completed_required_stage_count" `
        -Message "JSON summary should list local closure completed required stage count marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "required_stages" `
        -Message "JSON summary should list local closure required stages marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "restore_docx_functional_smoke_evidence" `
        -Message "JSON summary should list DOCX functional smoke readiness blocker action marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "Word visual standard review metadata evidence" `
        -Message "JSON summary should list Word visual metadata compact evidence marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
        -Message "JSON summary should list Word visual metadata source reports marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "task_reviews=" `
        -Message "JSON summary should list Word visual metadata task review compact marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release-candidate-checks" `
        -Message "JSON summary should list release-candidate summary source marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release-candidate-checks-source" `
        -Message "JSON summary should list source-only release candidate summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release-candidate-checks\report\summary.json" `
        -Message "JSON summary should list final release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release-candidate-checks-source\summary.json" `
        -Message "JSON summary should list source-only release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "reviewer_manifest_scoped_project_template_trace" `
        -Message "JSON summary should list reviewer manifest scoped project-template trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "manifest_signoff_entrypoints_release_trace" `
        -Message "JSON summary should list manifest signoff release trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "manifest_signoff_entrypoints_manifest_trace" `
        -Message "JSON summary should list manifest signoff packaged manifest trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "release_entry_project_template_readiness_checklist_trace" `
        -Message "JSON summary should list project-template readiness checklist release entry trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "Word visual standard review metadata evidence" `
        -Message "JSON summary policy should list Word visual metadata compact evidence marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
        -Message "JSON summary policy should list Word visual metadata source reports marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "task_reviews=" `
        -Message "JSON summary policy should list Word visual metadata task review compact marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "source_schema=featherdoc.release_candidate_summary" `
        -Message "JSON summary policy should list Word visual metadata source schema marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "release-candidate-checks" `
        -Message "JSON summary policy should list release-candidate summary source marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "release-candidate-checks-source" `
        -Message "JSON summary policy should list source-only release candidate summary marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "release-candidate-checks\report\summary.json" `
        -Message "JSON summary policy should list final release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_policy_markers) `
        -ExpectedValue "release-candidate-checks-source\summary.json" `
        -Message "JSON summary policy should list source-only release candidate summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "readme_gallery" `
        -Message "JSON summary should list README gallery metadata marker."
    Assert-ArrayContains `
        -Values @($summary.required_pipeline_markers) `
        -ExpectedValue "refresh_readme_visual_assets.ps1" `
        -Message "JSON summary should list README gallery refresh marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "check_word_visual_release_gate_preflight_test.ps1" `
        -Message "JSON summary should list Word visual preflight test marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue '``output_encoding``' `
        -Message "JSON summary checklist should list Word visual preflight output encoding field marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue 'UTF-8 without BOM' `
        -Message "JSON summary checklist should list Word visual preflight output encoding value marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "word_visual_release_gate_preflight_route_docs_contract_test.ps1" `
        -Message "JSON summary should list Word visual preflight route docs contract test marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "check_docx_functional_smoke_readiness.ps1" `
        -Message "JSON summary should list DOCX functional smoke readiness checklist marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "docx_functional_smoke_readiness_test.ps1" `
        -Message "JSON summary should list DOCX functional smoke readiness regression marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "docx_functional_smoke_readiness_route_docs_contract_test.ps1" `
        -Message "JSON summary should list DOCX functional smoke readiness route docs contract marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "local_governance_closure" `
        -Message "JSON summary checklist should list local governance closure marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "local_governance_closure.status" `
        -Message "JSON summary checklist should list local closure status marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "local_governance_closure.closed" `
        -Message "JSON summary checklist should list local closure closed marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "pipeline_summary_json" `
        -Message "JSON summary checklist should list local closure raw pipeline summary path marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "pipeline_report_markdown" `
        -Message "JSON summary checklist should list local closure raw pipeline Markdown path marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "final_governance_reports" `
        -Message "JSON summary checklist should list local closure final governance reports marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "required_stages" `
        -Message "JSON summary checklist should list local closure required stages marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "block_scoped_governance_handoff_project_template_status_trace" `
        -Message "JSON summary checklist should list project-template handoff block-scoped trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "manifest_signoff_entrypoints" `
        -Message "JSON summary checklist should list manifest signoff entrypoints marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release_entry_project_template_readiness_checklist_material_safety_audit" `
        -Message "JSON summary checklist should list project-template readiness checklist material safety audit marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "audit_script" `
        -Message "JSON summary checklist should list project-template readiness checklist audit script marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "audited_entrypoint_count" `
        -Message "JSON summary checklist should list project-template readiness checklist audited entrypoint count marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "audited_entrypoints" `
        -Message "JSON summary checklist should list project-template readiness checklist audited entrypoints marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "release_entry_project_template_readiness_checklist_trace" `
        -Message "JSON summary checklist should list project-template readiness checklist release entry trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "JSON summary checklist should list project-template readiness checklist release entry material safety trace marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "open_latest_word_review_task_curated_source_kind_test.ps1" `
        -Message "JSON summary should list curated open-latest test marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "pdf_floating_table_reviewer_focus" `
        -Message "JSON summary should list PDF floating table reviewer focus checklist marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "metadata-only tblpPr" `
        -Message "JSON summary should list metadata-only tblpPr checklist marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "pdf_floating_table_metadata_only_fields" `
        -Message "JSON summary should list PDF floating table aggregate metadata-only field marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "pdf_floating_table_review_required_fields" `
        -Message "JSON summary should list PDF floating table aggregate review-required field marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "metadata_only_fields" `
        -Message "JSON summary should list PDF floating table metadata-only field marker."
    Assert-ArrayContains `
        -Values @($summary.required_checklist_markers) `
        -ExpectedValue "review_required_fields" `
        -Message "JSON summary should list PDF floating table review-required field marker."
    Assert-ArrayContains `
        -Values @($summary.required_document_governance_markers) `
        -ExpectedValue "sync_bound_content_control" `
        -Message "JSON summary should list required document governance markers."
    Assert-ArrayContains `
        -Values @($summary.required_document_governance_markers) `
        -ExpectedValue "pdf_floating_table_support_coverage" `
        -Message "JSON summary should list PDF floating table support coverage document governance marker."
}
