if (Test-Scenario -Name "passing") {
    $outputDir = Join-Path $resolvedWorkingDir "passing-report"
    $passingInputRoot = Join-Path $resolvedWorkingDir "passing-input"
    Write-PassingInputRoot -Root $passingInputRoot
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $passingInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should succeed without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_blocker_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Rollup should write Markdown report."
    Assert-NoUtf8Bom -Path $summaryPath `
        -Message "Rollup summary JSON should be UTF-8 without BOM."
    Assert-NoUtf8Bom -Path $markdownPath `
        -Message "Rollup Markdown report should be UTF-8 without BOM."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Summary should expose rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Rollup should be blocked when blockers exist."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 8 `
        -Message "Rollup should aggregate all blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 6 `
        -Message "Rollup should aggregate action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 3 `
        -Message "Rollup should aggregate warning items."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 9 `
        -Message "Rollup should keep source report count."
    $projectTemplateReadinessSourceReport = ($summary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.project_template_delivery_readiness_report.v1" } |
        Select-Object -First 1)
    Assert-True -Condition ($null -ne $projectTemplateReadinessSourceReport) `
        -Message "Rollup should keep project-template delivery readiness as a source report."
    Assert-Equal -Actual ([string]$projectTemplateReadinessSourceReport.business_document_type_summary[0].document_type) -Expected "invoice" `
        -Message "Rollup source report should preserve project-template business document type summary."
    Assert-Equal -Actual ([int]$projectTemplateReadinessSourceReport.business_document_type_summary[0].count) -Expected 1 `
        -Message "Rollup source report should preserve project-template business document type counts."
    Assert-Equal -Actual ([string]$projectTemplateReadinessSourceReport.business_document_type_summary[1].document_type) -Expected "policy" `
        -Message "Rollup source report should preserve all project-template business document types."
    Assert-Equal -Actual ([string]$projectTemplateReadinessSourceReport.corpus_role_summary[0].corpus_role) -Expected "planned-business-template" `
        -Message "Rollup source report should preserve project-template corpus role summary."
    Assert-Equal -Actual ([string]$projectTemplateReadinessSourceReport.corpus_role_summary[1].corpus_role) -Expected "registered-business-template" `
        -Message "Rollup source report should preserve all project-template corpus roles."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Rollup should aggregate report-level governance metrics."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true
    Assert-GovernanceTraceMetadata -Items @($summary.informational_action_items) -CollectionName "informational_action_items" `
        -ExpectOpenCommandProperty $true
    Assert-SummaryGroupCount -Groups @($summary.blocker_source_schema_summary) `
        -PropertyName "source_schema" `
        -Name "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -ExpectedCount 1 `
        -Message "Rollup should summarize blockers by source schema for reviewer filtering."
    Assert-SummaryGroupCount -Groups @($summary.action_item_source_schema_summary) `
        -PropertyName "source_schema" `
        -Name "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -ExpectedCount 1 `
        -Message "Rollup should summarize action items by source schema for reviewer filtering."
    Assert-SummaryGroupCount -Groups @($summary.action_item_source_schema_summary) `
        -PropertyName "source_schema" `
        -Name "featherdoc.project_template_onboarding_governance_report.v1" `
        -ExpectedCount 1 `
        -Message "Rollup should summarize project-template action items by source schema."
    Assert-SummaryGroupCount -Groups @($summary.warning_source_schema_summary) `
        -PropertyName "source_schema" `
        -Name "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -ExpectedCount 1 `
        -Message "Rollup should summarize warnings by source schema for reviewer filtering."
    Assert-SummaryGroupCount -Groups @($summary.warning_source_schema_summary) `
        -PropertyName "source_schema" `
        -Name "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
        -ExpectedCount 1 `
        -Message "Rollup should summarize PDF preflight warnings by source schema."
    Assert-Equal -Actual (@($summary.informational_action_item_source_schema_summary).Count) -Expected 0 `
        -Message "Rollup should expose an empty informational action source-schema summary when no informational actions are present."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    Assert-ContainsText -Text $metricText -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Rollup should preserve numbering real-corpus confidence metric."
    $metricContractText = ($summary.governance_metrics | ForEach-Object { "$($_.id):$($_.report_id):$($_.source_schema)" }) -join "`n"
    Assert-ContainsText -Text $metricContractText -ExpectedText "numbering_catalog_governance.real_corpus_confidence:numbering_catalog_governance:featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Rollup should preserve numbering real-corpus confidence contract."
    if ($metricText -match "experimental") {
        throw "Rollup should not treat style governance real_corpus_confidence as numbering confidence."
    }
    Assert-ContainsText -Text $metricText -ExpectedText "delivery_quality:blocked:0" `
        -Message "Rollup should preserve table layout delivery quality metric."
    $numberingMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingMetric.source_json) -ExpectedText "numbering-governance\summary.json" `
        -Message "Rollup should preserve numbering confidence raw source JSON."
    Assert-ContainsText -Text ([string]$numberingMetric.source_json_display) -ExpectedText "numbering-governance\summary.json" `
        -Message "Rollup should preserve numbering confidence source JSON display."
    Assert-Equal -Actual ([int]$numberingMetric.details.catalog_coverage_percent) -Expected 100 `
        -Message "Rollup should preserve numbering confidence detail fields."
    Assert-Equal -Actual ([int]$numberingMetric.details.matched_document_count) -Expected 2 `
        -Message "Rollup should preserve numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text (($numberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Rollup should preserve numbering catalog document keys."
    Assert-ContainsText -Text (($numberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Rollup should preserve numbering baseline document keys."
    Assert-ContainsText -Text (($numberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Rollup should preserve numbering matched document keys."
    Assert-ContainsText -Text (($numberingMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "style_numbering_issues" `
        -Message "Rollup should preserve numbering confidence penalty summary."
    $tableMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableMetric.source_json) -ExpectedText "table-layout\summary.json" `
        -Message "Rollup should preserve table delivery raw source JSON."
    Assert-ContainsText -Text ([string]$tableMetric.source_json_display) -ExpectedText "table-layout\summary.json" `
        -Message "Rollup should preserve table delivery source JSON display."
    Assert-Equal -Actual ([int]$tableMetric.details.unresolved_item_count) -Expected 10 `
        -Message "Rollup should preserve table layout delivery quality detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_automatic_count) -Expected 2 `
        -Message "Rollup should preserve automatic floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_review_count) -Expected 1 `
        -Message "Rollup should preserve review floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_tracked_geometry_count) -Expected 9 `
        -Message "Rollup should preserve tracked PDF floating table geometry detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Rollup should preserve supported PDF floating table geometry percentage detail fields."
    Assert-ContainsText -Text (($tableMetric.details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "Rollup should preserve generic PDF floating table metadata-only fields."
    Assert-ContainsText -Text (($tableMetric.details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "table overlap avoidance and collision resolution" `
        -Message "Rollup should preserve generic PDF floating table review-required fields."
    Assert-ContainsText -Text (($tableMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "floating_table_plans_pending" `
        -Message "Rollup should preserve table layout delivery penalty summary."
    $tableStyleBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "table_layout.manual_table_style_quality_work" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$tableStyleBlocker.repair_strategy) -Expected "review_source_table_style_quality_plan" `
        -Message "Rollup should preserve table layout style blocker repair strategy."
    Assert-ContainsText -Text ([string]$tableStyleBlocker.repair_hint) -ExpectedText "style quality findings" `
        -Message "Rollup should preserve table layout style blocker repair hint."
    Assert-ContainsText -Text ([string]$tableStyleBlocker.command_template) -ExpectedText "inspect-table-style" `
        -Message "Rollup should preserve table layout style blocker command template."
    $tablePositionBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "table_layout.positioned_tables_need_review" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$tablePositionBlocker.repair_strategy) -Expected "review_source_table_position_plan" `
        -Message "Rollup should preserve table layout position blocker repair strategy."
    Assert-ContainsText -Text ([string]$tablePositionBlocker.command_template) -ExpectedText "apply-table-position-plan" `
        -Message "Rollup should preserve table layout position blocker command template."
    $tablePositionAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_table_position_preset" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$tablePositionAction.repair_strategy) -Expected "dry_run_table_position_plan" `
        -Message "Rollup should preserve table layout action repair strategy."
    Assert-ContainsText -Text ([string]$tablePositionAction.command_template) -ExpectedText "--dry-run" `
        -Message "Rollup should preserve table layout action command template."
    $skeletonBlocker = @(
        $summary.release_blockers |
            Where-Object { [string]$_.id -eq "document_skeleton.style_numbering_issues" } |
            Select-Object -First 1
    )
    Assert-True -Condition ($null -ne $skeletonBlocker) `
        -Message "Rollup should include the document skeleton blocker."
    Assert-True -Condition ([string]$skeletonBlocker.composite_id -match '^source\d+\.blocker\d+\.document_skeleton\.style_numbering_issues$') `
        -Message "Rollup should generate composite blocker ids."
    Assert-Equal -Actual ([string]$skeletonBlocker.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Rollup should preserve document skeleton source schema."
    Assert-Equal -Actual ([string]$skeletonBlocker.action) -Expected "review_style_numbering_audit" `
        -Message "Rollup should preserve blocker action."
    Assert-ContainsText -Text ([string]$skeletonBlocker.message) -ExpectedText "Style numbering audit" `
        -Message "Rollup should preserve blocker message."
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_json) -ExpectedText "style-numbering-audit.json" `
        -Message "Rollup should preserve blocker raw source JSON."
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_json_display) -ExpectedText "style-numbering-audit.json" `
        -Message "Rollup should preserve blocker source JSON display."
    Assert-ContainsText -Text ([string]$skeletonBlocker.origin_source_report_display) -ExpectedText "document-skeleton-governance-rollup\summary.json" `
        -Message "Rollup should preserve blocker origin source report display."
    $skeletonAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "preview_style_numbering_repair" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$skeletonAction.open_command) -ExpectedText "repair-style-numbering" `
        -Message "Rollup should expose action item open command."
    Assert-Equal -Actual ([string]$skeletonAction.action) -Expected "preview_style_numbering_repair" `
        -Message "Rollup should fall back to action item id when action is absent."
    $contentControlBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlBlocker.repair_strategy) -Expected "sync_bound_content_control" `
        -Message "Rollup should preserve content-control repair strategy."
    Assert-ContainsText -Text ([string]$contentControlBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
        -Message "Rollup should preserve content-control blocker command template."
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx) -Expected "samples/invoice.docx" `
        -Message "Rollup should preserve content-control blocker input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Rollup should preserve content-control blocker input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.template_name) -Expected "invoice-template" `
        -Message "Rollup should preserve content-control blocker template_name provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.schema_target) -Expected "invoice" `
        -Message "Rollup should preserve content-control blocker schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.target_mode) -Expected "resolved-section-targets" `
        -Message "Rollup should preserve content-control blocker target_mode provenance."
    $contentControlAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_content_control_lock_strategy" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlAction.repair_strategy) -Expected "review_lock_state" `
        -Message "Rollup should preserve content-control action repair strategy."
    Assert-ContainsText -Text ([string]$contentControlAction.command_template) -ExpectedText "--clear-lock" `
        -Message "Rollup should preserve content-control action command template."
    Assert-Equal -Actual ([string]$contentControlAction.input_docx) -Expected "samples/invoice.docx" `
        -Message "Rollup should preserve content-control action input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlAction.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Rollup should preserve content-control action input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlAction.template_name) -Expected "invoice-template" `
        -Message "Rollup should preserve content-control action template_name provenance."
    Assert-Equal -Actual ([string]$contentControlAction.schema_target) -Expected "invoice" `
        -Message "Rollup should preserve content-control action schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlAction.target_mode) -Expected "resolved-section-targets" `
        -Message "Rollup should preserve content-control action target_mode provenance."
    $pdfPreflightBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "pdf_visual_release_gate_preflight.build_outputs_missing" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([int]$pdfPreflightBlocker.output_gap_count) -Expected 3 `
        -Message "Rollup should preserve PDF preflight blocker output gap count."
    Assert-Equal -Actual ([int]$pdfPreflightBlocker.missing_output_count) -Expected 87 `
        -Message "Rollup should preserve PDF preflight blocker missing output count."
    Assert-Equal -Actual ([int]$pdfPreflightBlocker.blocking_summary.missing_visual_baseline_pdf_count) -Expected 42 `
        -Message "Rollup should preserve PDF preflight blocker blocking summary details."
    Assert-Equal -Actual (@($pdfPreflightBlocker.build_dir_auto_candidates).Count) -Expected 2 `
        -Message "Rollup should preserve PDF preflight blocker build auto candidates."
    $pdfPreflightBuildCandidate = @($pdfPreflightBlocker.build_dir_auto_candidates |
            Where-Object { [string]$_.relative_path -eq "build" } |
            Select-Object -First 1)
    Assert-Equal -Actual ([bool]$pdfPreflightBuildCandidate.cmake_cache_exists) -Expected $true `
        -Message "Rollup should preserve PDF preflight build candidate CMake cache state."
    Assert-Equal -Actual ([bool]$pdfPreflightBuildCandidate.ctest_manifest_exists) -Expected $true `
        -Message "Rollup should preserve PDF preflight build candidate CTest manifest state."
    Assert-Equal -Actual ([bool]$pdfPreflightBuildCandidate.pdf_build_options_enabled) -Expected $false `
        -Message "Rollup should preserve PDF preflight build candidate PDF option readiness."
    Assert-ContainsText -Text (($pdfPreflightBuildCandidate.pdf_build_options | ForEach-Object { "$($_.name)=$($_.value):$($_.enabled)" }) -join "`n") `
        -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT=OFF:False" `
        -Message "Rollup should preserve PDF preflight build candidate PDF option snapshots."
    Assert-Equal -Actual ([string]$pdfPreflightBlocker.pdf_dependency_inputs.status) -Expected "not_ready" `
        -Message "Rollup should preserve PDF preflight dependency input readiness."
    Assert-Equal -Actual ([string]$pdfPreflightBlocker.pdf_dependency_inputs.selected_pdfium_provider) -Expected "unresolved" `
        -Message "Rollup should preserve PDF preflight selected PDFium provider."
    Assert-Equal -Actual ([int]$pdfPreflightBlocker.pdf_dependency_inputs.missing_input_count) -Expected 3 `
        -Message "Rollup should preserve PDF preflight dependency missing input count."
    Assert-Equal -Actual ([int]$pdfPreflightBlocker.readiness_action_evidence_count) -Expected 2 `
        -Message "Rollup should preserve PDF preflight blocker readiness action evidence count."
    Assert-ContainsText -Text (($pdfPreflightBlocker.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.issue_key)|$($_.item)|$($_.source_json_display)" }) -join "`n") `
        -ExpectedText "provide_pdf_dependency_input|pdf_dependency_inputs_ready|PDFio source header" `
        -Message "Rollup should preserve PDF dependency input readiness evidence."
    Assert-ContainsText -Text (($pdfPreflightBlocker.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.issue_key)|$($_.item)|$($_.source_json_display)" }) -join "`n") `
        -ExpectedText "enable_pdf_build_option|pdf_build_options_enabled|FEATHERDOC_BUILD_PDF_IMPORT|.\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
        -Message "Rollup should preserve disabled PDF build option readiness evidence."
    Assert-ContainsText -Text (($pdfPreflightBlocker.output_gap_summary | ForEach-Object { [string]$_.check }) -join "`n") `
        -ExpectedText "visual_baseline_manifest_pdfs_exist" `
        -Message "Rollup should preserve PDF preflight blocker output gap summary."
    $pdfPreflightAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "prepare_pdf_visual_release_gate_build_outputs" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([int]$pdfPreflightAction.output_gap_count) -Expected 3 `
        -Message "Rollup should preserve PDF preflight action output gap count."
    Assert-Equal -Actual ([int]$pdfPreflightAction.blocking_summary.missing_cjk_text_layer_pdf_count) -Expected 43 `
        -Message "Rollup should preserve PDF preflight action blocking summary details."
    Assert-Equal -Actual (@($pdfPreflightAction.build_dir_auto_candidates).Count) -Expected 2 `
        -Message "Rollup should preserve PDF preflight action build auto candidates."
    $pdfPreflightActionBuildCandidate = @($pdfPreflightAction.build_dir_auto_candidates |
            Where-Object { [string]$_.relative_path -eq "build" } |
            Select-Object -First 1)
    Assert-Equal -Actual ([bool]$pdfPreflightActionBuildCandidate.pdf_build_options_enabled) -Expected $false `
        -Message "Rollup should preserve PDF preflight action item build candidate PDF option readiness."
    Assert-Equal -Actual ([string]$pdfPreflightAction.pdf_dependency_inputs.status) -Expected "not_ready" `
        -Message "Rollup should preserve PDF preflight action item dependency input readiness."
    Assert-Equal -Actual ([int]$pdfPreflightAction.readiness_action_evidence_count) -Expected 2 `
        -Message "Rollup should preserve PDF preflight action readiness action evidence count."
    Assert-ContainsText -Text (($pdfPreflightAction.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.issue_key)|$($_.item)|$($_.source_json)" }) -join "`n") `
        -ExpectedText "enable_pdf_build_option|pdf_build_options_enabled|FEATHERDOC_BUILD_PDF_IMPORT|" `
        -Message "Rollup should preserve PDF preflight action readiness evidence raw source JSON."
    Assert-ContainsText -Text (($pdfPreflightAction.readiness_action_evidence | ForEach-Object { [string]$_.source_json }) -join "`n") `
        -ExpectedText "pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
        -Message "Rollup should resolve PDF preflight action readiness evidence raw source JSON paths."
    $projectTemplateSourceReport = ($summary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.project_template_delivery_readiness_report.v1" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateSourceReport.latest_schema_approval_gate_status) -Expected "pending" `
        -Message "Rollup should preserve project-template latest schema approval gate status."
    Assert-ContainsText -Text (($projectTemplateSourceReport.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" }) -join "`n") `
        -ExpectedText "pending_review=1" `
        -Message "Rollup should preserve project-template schema approval status summary."
    $projectTemplateRollupBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "project_template_onboarding.schema_approval" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([bool]$projectTemplateRollupBlocker.requires_reviewer_action) -Expected $true `
        -Message "Rollup should preserve project-template reviewer-action requirement on blockers."
    Assert-Equal -Actual ([string]$projectTemplateRollupBlocker.reviewer_action_summary) -Expected "review_schema_update_candidate" `
        -Message "Rollup should preserve project-template reviewer action summary on blockers."
    Assert-ContainsText -Text ([string]$projectTemplateRollupBlocker.reviewer_action_reason) -ExpectedText "latest_review_state=pending" `
        -Message "Rollup should preserve project-template reviewer action reason on blockers."
    Assert-True -Condition (@($projectTemplateRollupBlocker.reviewer_actions) -contains "review_schema_update_candidate") `
        -Message "Rollup should preserve project-template reviewer actions on blockers."
    $projectTemplateRollupAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_project_template_schema_approval" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([bool]$projectTemplateRollupAction.requires_reviewer_action) -Expected $true `
        -Message "Rollup should preserve project-template reviewer-action requirement on action items."
    Assert-Equal -Actual ([string]$projectTemplateRollupAction.reviewer_action_summary) -Expected "review_schema_update_candidate" `
        -Message "Rollup should preserve project-template reviewer action summary on action items."
    $pdfPreflightSourceReport = ($summary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([bool]$pdfPreflightSourceReport.preflight_ready) -Expected $false `
        -Message "Rollup should preserve PDF preflight readiness on the source report contract."
    Assert-Equal -Actual ([bool]$pdfPreflightSourceReport.full_visual_gate_required) -Expected $true `
        -Message "Rollup should preserve that PDF preflight still requires the full visual gate."
    Assert-Equal -Actual ([string]$pdfPreflightSourceReport.full_visual_gate_status) -Expected "not_run_by_preflight_governance" `
        -Message "Rollup should preserve the full visual gate status from PDF preflight governance."
    Assert-Equal -Actual ([bool]$pdfPreflightSourceReport.controlled_visual_smoke_available) -Expected $true `
        -Message "Rollup should preserve controlled visual smoke availability."
    Assert-Equal -Actual ([string]$pdfPreflightSourceReport.controlled_visual_smoke_status) -Expected "pass" `
        -Message "Rollup should preserve controlled visual smoke status."
    Assert-Equal -Actual ([bool]$pdfPreflightSourceReport.controlled_visual_smoke_passed) -Expected $true `
        -Message "Rollup should preserve controlled visual smoke pass result."
    Assert-Equal -Actual ([int]$pdfPreflightSourceReport.controlled_visual_smoke_case_count) -Expected 2 `
        -Message "Rollup should preserve controlled visual smoke case count."
    Assert-ContainsText -Text ([string]$pdfPreflightSourceReport.controlled_visual_smoke_json_display) `
        -ExpectedText "controlled-visual-smoke-check-latest.json" `
        -Message "Rollup should preserve controlled visual smoke JSON display."
    $releaseCandidateSourceReport = ($summary.source_reports |
        Where-Object { [string]$_.path_display -match [regex]::Escape("release-candidate") } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_status) -Expected "loaded" `
        -Message "Rollup should preserve loaded PDF visual gate evidence status from release candidate summaries."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.full_visual_gate_status) -Expected "pass" `
        -Message "Rollup should normalize full PDF visual gate status from release candidate summaries."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_verdict) -Expected "pass" `
        -Message "Rollup should preserve PDF visual gate verdict from release candidate summaries."
    Assert-Equal -Actual ([bool]$releaseCandidateSourceReport.pdf_visual_gate_finalizable) -Expected $true `
        -Message "Rollup should preserve PDF visual gate finalizable status from release candidate summaries."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_visual_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\summary.json" `
        -Message "Rollup should preserve reviewer-openable PDF visual gate summary display path."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_visual_gate_aggregate_contact_sheet_display) `
        -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Rollup should preserve reviewer-openable PDF visual gate contact-sheet display path."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_cjk_manifest_count) -Expected 43 `
        -Message "Rollup should preserve PDF visual gate CJK manifest count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_cjk_copy_search_count) -Expected 43 `
        -Message "Rollup should preserve PDF visual gate CJK copy/search count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_cjk_missing_text_count) -Expected 0 `
        -Message "Rollup should preserve PDF visual gate CJK missing text count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_visual_baseline_manifest_count) -Expected 42 `
        -Message "Rollup should preserve PDF visual gate visual baseline manifest count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_visual_baseline_count) -Expected 44 `
        -Message "Rollup should preserve PDF visual gate rendered visual baseline count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_bounded_ctest_summary_count) -Expected 7 `
        -Message "Rollup should preserve PDF bounded CTest summary count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_bounded_ctest_pass_count) -Expected 7 `
        -Message "Rollup should preserve PDF bounded CTest pass count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_bounded_ctest_selected_test_count) -Expected 70 `
        -Message "Rollup should preserve PDF bounded CTest selected test count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_bounded_ctest_skipped_test_count) -Expected 0 `
        -Message "Rollup should preserve PDF bounded CTest skipped test count."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.pdf_bounded_ctest_subsets) -join ",") `
        -ExpectedText "regression-business-samples" `
        -Message "Rollup should preserve PDF bounded CTest subset names."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.pdf_bounded_ctest_summary_json_display) -join ",") `
        -ExpectedText "pdf-ctest-bounded-regression-table-layout-current\summary.json" `
        -Message "Rollup should preserve PDF bounded CTest summary display paths."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_status) -Expected "pass" `
        -Message "Rollup should preserve PDF full CTest readiness status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_verdict) -Expected "pass_with_warnings" `
        -Message "Rollup should preserve PDF full CTest readiness verdict."
    Assert-Equal -Actual ([bool]$releaseCandidateSourceReport.pdf_full_ctest_readiness_release_ready) -Expected $true `
        -Message "Rollup should preserve PDF full CTest readiness release_ready."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_summary_json_display) `
        -ExpectedText "pdf-release-readiness-current\summary.json" `
        -Message "Rollup should preserve PDF release readiness summary display path."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_full_ctest_summary_json_display) `
        -ExpectedText "pdf-ctest-full-current\summary.json" `
        -Message "Rollup should preserve PDF full CTest summary display path."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_full_ctest_status) -Expected "timeout" `
        -Message "Rollup should preserve PDF full CTest observed status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_full_ctest_verdict) -Expected "not_complete" `
        -Message "Rollup should preserve PDF full CTest verdict."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_outer_guard_status) -Expected "timed_out" `
        -Message "Rollup should preserve PDF full CTest guard status."
    Assert-Equal -Actual ([bool]$releaseCandidateSourceReport.pdf_full_ctest_readiness_outer_guard_timed_out) -Expected $true `
        -Message "Rollup should preserve PDF full CTest guard timeout flag."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_full_ctest_readiness_selected_test_count) -Expected 139 `
        -Message "Rollup should preserve PDF full CTest selected count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_full_ctest_readiness_completed_test_count) -Expected 133 `
        -Message "Rollup should preserve PDF full CTest completed count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_full_ctest_readiness_failed_test_count) -Expected 0 `
        -Message "Rollup should preserve PDF full CTest observed failure count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_full_ctest_readiness_not_run_test_count) -Expected 6 `
        -Message "Rollup should preserve PDF full CTest not-run count."
    Assert-Equal -Actual ([double]$releaseCandidateSourceReport.pdf_full_ctest_readiness_completion_percent) -Expected 95.7 `
        -Message "Rollup should preserve PDF full CTest completion percent."
    Assert-Equal -Actual ([bool]$releaseCandidateSourceReport.pdf_full_ctest_readiness_zero_failed_tests_observed) -Expected $true `
        -Message "Rollup should preserve PDF full CTest zero-failure observation."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_full_ctest_readiness_marker) -Expected "pdf_full_ctest_readiness_trace" `
        -Message "Rollup should preserve PDF full CTest readiness marker."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Rollup should preserve PDF visual gate attempt status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Rollup should preserve PDF visual gate attempt verdict."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_full_visual_gate_status) -Expected "not_complete" `
        -Message "Rollup should keep partial attempt separate from full visual gate pass."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_evidence_scope) -Expected "bounded_attempt_auxiliary_only" `
        -Message "Rollup should preserve PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\attempt-summary.json" `
        -Message "Rollup should preserve reviewer-openable PDF visual gate attempt summary display path."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_stage_count) -Expected 6 `
        -Message "Rollup should preserve PDF visual gate attempt stage count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_passed_stage_count) -Expected 4 `
        -Message "Rollup should preserve PDF visual gate attempt passed stage count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_incomplete_stage_count) -Expected 2 `
        -Message "Rollup should preserve PDF visual gate attempt incomplete stage count."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_status) -Expected "timed_out" `
        -Message "Rollup should preserve PDF visual gate attempt outer guard status."
    Assert-Equal -Actual ([bool]$releaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timed_out) -Expected $true `
        -Message "Rollup should preserve PDF visual gate attempt outer guard timeout flag."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timeout_seconds) -Expected 60 `
        -Message "Rollup should preserve PDF visual gate attempt outer guard timeout seconds."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_status) -Expected "pass" `
        -Message "Rollup should preserve PDF visual gate attempt pdf_regression status."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_selected_test_count) -Expected 91 `
        -Message "Rollup should preserve PDF visual gate attempt pdf_regression selected count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_failed_test_count) -Expected 0 `
        -Message "Rollup should preserve PDF visual gate attempt pdf_regression failed count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Rollup should preserve PDF visual gate attempt pdf_regression skipped count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_cjk_copy_search_count) -Expected 43 `
        -Message "Rollup should preserve PDF visual gate attempt CJK copy/search count."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_visual_baseline_render_status) -Expected "partial" `
        -Message "Rollup should preserve PDF visual gate attempt render status."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Rollup should preserve PDF visual gate attempt fresh render count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_gate_attempt_expected_visual_render_count) -Expected 44 `
        -Message "Rollup should preserve PDF visual gate attempt expected render count."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_gate_attempt_aggregate_contact_sheet_status) -Expected "stale" `
        -Message "Rollup should preserve PDF visual gate attempt contact sheet status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_status) -Expected "pass" `
        -Message "Rollup should preserve segmented PDF visual gate status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_verdict) -Expected "pass" `
        -Message "Rollup should preserve segmented PDF visual gate verdict."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
        -Message "Rollup should keep segmented evidence separate from the full visual gate status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
        -Message "Rollup should preserve segmented PDF visual gate evidence scope."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_boundary) -Expected "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Rollup should preserve segmented PDF visual gate boundary."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\segmented-summary.json" `
        -Message "Rollup should preserve reviewer-openable segmented PDF visual gate summary display path."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_segmented_gate_slice_summary_count) -Expected 4 `
        -Message "Rollup should preserve segmented PDF visual gate slice count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_segmented_gate_slice_pass_count) -Expected 4 `
        -Message "Rollup should preserve segmented PDF visual gate pass count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_segmented_gate_covered_baseline_count) -Expected 44 `
        -Message "Rollup should preserve segmented PDF visual gate covered baseline count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_segmented_gate_expected_visual_render_count) -Expected 44 `
        -Message "Rollup should preserve segmented PDF visual gate expected baseline count."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.pdf_visual_segmented_gate_attempt_passed_stage_count) -Expected 6 `
        -Message "Rollup should preserve segmented PDF visual gate attempt stage count."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_visual_baseline_render_status) -Expected "pass" `
        -Message "Rollup should preserve segmented PDF visual gate render status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_aggregate_contact_sheet_status) -Expected "pass" `
        -Message "Rollup should preserve segmented PDF visual gate contact-sheet status."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_aggregate_contact_sheet_display) `
        -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Rollup should preserve segmented PDF visual gate contact-sheet display path."
    Assert-Equal -Actual ([int64]$releaseCandidateSourceReport.pdf_visual_segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
        -Message "Rollup should preserve segmented PDF visual gate contact-sheet byte size."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.pdf_visual_segmented_gate_aggregate_rebuild_status) -Expected "pass" `
        -Message "Rollup should preserve segmented PDF visual gate aggregate rebuild status."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.manifest_signoff_entrypoints_status) -Expected "declared" `
        -Message "Rollup should preserve manifest signoff status from release candidate summaries."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.manifest_signoff_entrypoints_release_assets_manifest_display) `
        -ExpectedText "release_assets_manifest.json" `
        -Message "Rollup should preserve reviewer-openable release assets manifest display path."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.manifest_signoff_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Rollup should preserve manifest signoff required entrypoint count."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "start_here" `
        -Message "Rollup should preserve START_HERE manifest signoff entrypoint."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "artifact_guide" `
        -Message "Rollup should preserve artifact guide manifest signoff entrypoint."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Rollup should preserve reviewer checklist manifest signoff entrypoint."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.manifest_signoff_entrypoints_required_contracts) -join "`n") `
        -ExpectedText "project_template_onboarding_governance_contract" `
        -Message "Rollup should preserve manifest signoff required governance contracts."
    $manifestSignoffRequiredFieldsText = @($releaseCandidateSourceReport.manifest_signoff_entrypoints_required_fields) -join "`n"
    foreach ($requiredField in @(
        "status",
        "release_ready",
        "release_blocker_count",
        "warning_count",
        "schema_approval_status_summary",
        "onboarding_governance_next_action",
        "onboarding_governance_next_action_summary",
        "onboarding_governance_next_action_group_count",
        "next_action",
        "next_action_summary",
        "next_action_group_count",
        "requires_reviewer_action",
        "reviewer_action_summary",
        "reviewer_action_reason",
        "reviewer_actions",
        "business_document_type_summary",
        "corpus_role_summary",
        "source_report_display",
        "source_json_display"
    )) {
        Assert-ContainsText -Text $manifestSignoffRequiredFieldsText `
            -ExpectedText $requiredField `
            -Message "Rollup should preserve manifest signoff required field '$requiredField'."
    }
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.manifest_signoff_entrypoints_checklist_marker) -Expected "reviewer_manifest_scoped_project_template_trace" `
        -Message "Rollup should preserve manifest signoff reviewer checklist marker."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_status) -Expected "declared" `
        -Message "Rollup should preserve project-template checklist entrypoint status from release candidate summaries."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_checklist_label) -Expected "Project template release readiness checklist" `
        -Message "Rollup should preserve project-template checklist label."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Rollup should preserve project-template checklist path."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Rollup should preserve project-template checklist required entrypoint count."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Rollup should preserve reviewer checklist project-template readiness entrypoint."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_checklist_marker) -Expected "release_entry_project_template_readiness_checklist_trace" `
        -Message "Rollup should preserve project-template readiness checklist marker."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_status) -Expected "passed" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety audit status."
    Assert-ContainsText -Text ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_script) `
        -ExpectedText "assert_release_material_safety.ps1" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety audit script."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count) -Expected 3 `
        -Message "Rollup should preserve packaged release-entry checklist material-safety audited entrypoint count."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join "`n") `
        -ExpectedText "artifact_guide" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety audited entrypoint ids."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label) -Expected "Project-template readiness checklist handoff evidence" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety compact evidence label."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field) -Expected "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety compact evidence field."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety compact evidence source schema."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety checklist path."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker) -Expected "release_entry_project_template_readiness_checklist_trace" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety checklist marker."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker) -Expected "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Rollup should preserve packaged release-entry checklist material-safety audit marker."
    Assert-Equal -Actual ([int]$releaseCandidateSourceReport.word_visual_standard_review_metadata_count) -Expected 4 `
        -Message "Rollup should preserve Word visual standard review metadata count."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.word_visual_standard_review_status_summary) -Expected "reviewed=4" `
        -Message "Rollup should summarize Word visual standard review statuses."
    Assert-Equal -Actual ([string]$releaseCandidateSourceReport.word_visual_standard_review_verdict_summary) -Expected "pass=4" `
        -Message "Rollup should summarize Word visual standard review verdicts."
    Assert-ContainsText -Text (@($releaseCandidateSourceReport.word_visual_standard_review_task_keys) -join "`n") `
        -ExpectedText "page_number_fields" `
        -Message "Rollup should preserve Word visual standard review task keys."
    $wordVisualMetadata = @($releaseCandidateSourceReport.word_visual_standard_review_metadata)
    Assert-Equal -Actual $wordVisualMetadata.Count -Expected 4 `
        -Message "Rollup should preserve four Word visual standard review metadata entries."
    $wordVisualMetadataByTask = @{}
    foreach ($entry in $wordVisualMetadata) {
        $wordVisualMetadataByTask[[string]$entry.task_key] = $entry
        Assert-True -Condition ($null -eq $entry.PSObject.Properties["review_note"]) `
            -Message "Rollup should not expose Word visual standard review notes."
    }
    foreach ($expectedTaskKey in @("smoke", "fixed_grid", "section_page_setup", "page_number_fields")) {
        Assert-True -Condition $wordVisualMetadataByTask.ContainsKey($expectedTaskKey) `
            -Message "Rollup should preserve Word visual standard review metadata task '$expectedTaskKey'."
    }
    $smokeWordVisualMetadata = $wordVisualMetadataByTask["smoke"]
    Assert-Equal -Actual ([string]$smokeWordVisualMetadata.review_task_key) -Expected "document" `
        -Message "Rollup should preserve the smoke Word visual review task key."
    Assert-Equal -Actual ([string]$smokeWordVisualMetadata.label) -Expected "Word visual smoke" `
        -Message "Rollup should preserve the smoke Word visual review label."
    Assert-Equal -Actual ([string]$smokeWordVisualMetadata.verdict) -Expected "pass" `
        -Message "Rollup should preserve the smoke Word visual review verdict."
    Assert-Equal -Actual ([string]$smokeWordVisualMetadata.review_status) -Expected "reviewed" `
        -Message "Rollup should preserve the smoke Word visual review status."
    Assert-Equal -Actual $smokeWordVisualMetadata.reviewed_at -Expected "2026-04-12T12:10:00" `
        -Message "Rollup should preserve the smoke Word visual reviewed timestamp."
    Assert-Equal -Actual ([string]$smokeWordVisualMetadata.review_method) -Expected "operator_supplied" `
        -Message "Rollup should preserve the smoke Word visual review method."
    Assert-ContainsText -Text ([string]$smokeWordVisualMetadata.review_result_path) `
        -ExpectedText "word-visual-release-gate\review-tasks\document\report\review_result.json" `
        -Message "Rollup should preserve the smoke Word visual review result path."
    Assert-ContainsText -Text ([string]$wordVisualMetadataByTask["page_number_fields"].final_review_path) `
        -ExpectedText "word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md" `
        -Message "Rollup should preserve the page-number-fields Word visual final review path."
    $skeletonWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "document_skeleton.exemplar_catalog_missing" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$skeletonWarning.action) -Expected "open_document_skeleton_rollup" `
        -Message "Rollup should preserve warning action."
    Assert-ContainsText -Text ([string]$skeletonWarning.message) -ExpectedText "exemplar catalog" `
        -Message "Rollup should preserve warning message."
    $calibrationBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationBlocker.project_id) -Expected "project-finance" `
        -Message "Rollup should preserve calibration blocker project id."
    Assert-Equal -Actual ([string]$calibrationBlocker.template_name) -Expected "invoice-template" `
        -Message "Rollup should preserve calibration blocker template name."
    Assert-Equal -Actual ([string]$calibrationBlocker.candidate_type) -Expected "rename" `
        -Message "Rollup should preserve calibration blocker candidate type."
    Assert-ContainsText -Text ([string]$calibrationBlocker.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Rollup should preserve calibration blocker raw source JSON."
    Assert-ContainsText -Text ([string]$calibrationBlocker.origin_source_report_display) -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Rollup should preserve calibration blocker origin source report display."
    $calibrationAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "resolve_pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationAction.project_id) -Expected "project-finance" `
        -Message "Rollup should preserve calibration action project id."
    Assert-Equal -Actual ([string]$calibrationAction.template_name) -Expected "invoice-template" `
        -Message "Rollup should preserve calibration action template name."
    Assert-Equal -Actual ([string]$calibrationAction.candidate_type) -Expected "rename" `
        -Message "Rollup should preserve calibration action candidate type."
    Assert-ContainsText -Text ([string]$calibrationAction.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Rollup should preserve calibration action raw source JSON."
    Assert-ContainsText -Text ([string]$calibrationAction.origin_source_report_display) -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Rollup should preserve calibration action origin source report display."
    $calibrationWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.unscored_candidates" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationWarning.project_id) -Expected "project-finance" `
        -Message "Rollup should preserve calibration warning project id."
    Assert-Equal -Actual ([string]$calibrationWarning.template_name) -Expected "invoice-template" `
        -Message "Rollup should preserve calibration warning template name."
    Assert-Equal -Actual ([string]$calibrationWarning.candidate_type) -Expected "rename" `
        -Message "Rollup should preserve calibration warning candidate type."
    Assert-ContainsText -Text ([string]$calibrationWarning.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Rollup should preserve calibration warning raw source JSON."
    Assert-ContainsText -Text ([string]$calibrationWarning.origin_source_report_display) -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Rollup should preserve calibration warning origin source report display."
    $pdfPreflightWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "pdf_controlled_visual_smoke.unavailable_or_failed" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$pdfPreflightWarning.action) -Expected "rerun_pdf_controlled_visual_smoke_check" `
        -Message "Rollup should preserve PDF preflight warning action."
    Assert-Equal -Actual ([string]$pdfPreflightWarning.source_schema) -Expected "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
        -Message "Rollup should preserve PDF preflight warning source schema."
    Assert-ContainsText -Text ([string]$pdfPreflightWarning.source_json_display) -ExpectedText "controlled-visual-smoke-failed.json" `
        -Message "Rollup should preserve PDF preflight warning source JSON display."
    Assert-ContainsText -Text ([string]$pdfPreflightWarning.message) -ExpectedText "Controlled PDF visual smoke evidence was provided but is not passing." `
        -Message "Rollup should preserve PDF preflight warning message."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blocker Rollup Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke.schema_approval" `
        -Message "Markdown should include release candidate blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include source JSON display details."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report:" `
        -Message "Markdown should include raw source report paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json:" `
        -Message "Markdown should include raw source JSON paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "origin_source_report_display:" `
        -Message "Markdown should include origin source report display paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_action: ``review_schema_update_candidate``" `
        -Message "Markdown should include project-template reviewer action summaries."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_actions: ``review_schema_update_candidate``" `
        -Message "Markdown should include project-template reviewer action lists."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metrics" `
        -Message "Markdown should include governance metrics."
    Assert-ContainsText -Text $markdown -ExpectedText "Source Report Contracts" `
        -Message "Markdown should include source report contracts."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Markdown should include project-template delivery readiness contract schema."
    Assert-ContainsText -Text $markdown -ExpectedText "latest_schema_approval_gate_status" `
        -Message "Markdown should include project-template gate status field."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_approval_status_summary" `
        -Message "Markdown should include project-template schema approval status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "full_visual_gate_status" `
        -Message "Markdown should include PDF full visual gate status from source report contracts."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "reviewer_action: ``review_schema_update_candidate``" -ExpectedFragments @(
        'reviewer_action:',
        'reviewer_action_reason:',
        'reviewer_actions:'
    ) -Message "Markdown should keep reviewer action fields together for project-template readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "not_run_by_preflight_governance" `
        -Message "Markdown should make clear that PDF preflight did not run the full visual gate."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Markdown should include PDF preflight warning ids."
    Assert-ContainsText -Text $markdown -ExpectedText "controlled-visual-smoke-failed.json" `
        -Message "Markdown should include PDF preflight warning source JSON display paths."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_verdict" `
        -Message "Markdown should include PDF visual gate verdict evidence from release candidate summaries."
    Assert-ContainsText -Text $markdown -ExpectedText "full_visual_gate_status: ``pass``" `
        -Message "Markdown should expose the full PDF visual gate conclusion from release candidate summaries."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_verdict: ``pass``" `
        -Message "Markdown should make the PDF visual gate pass verdict explicit."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_summary_json_display" `
        -Message "Markdown should include PDF visual gate summary display evidence."
    Assert-ContainsText -Text $markdown -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Markdown should include PDF visual gate aggregate contact-sheet evidence."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_cjk_copy_search_count: ``43``" `
        -Message "Markdown should include PDF visual gate CJK copy/search count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_visual_baseline_count: ``44``" `
        -Message "Markdown should include PDF visual gate visual baseline count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_summary_count: ``7``" `
        -Message "Markdown should include PDF bounded CTest summary count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_skipped_test_count: ``0``" `
        -Message "Markdown should include PDF bounded CTest skipped test count."
    Assert-ContainsText -Text $markdown -ExpectedText "regression-business-samples" `
        -Message "Markdown should include PDF bounded CTest subset names."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_status: ``pass``" `
        -Message "Markdown should include PDF full CTest readiness status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``" `
        -Message "Markdown should include PDF full CTest readiness verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_full_ctest_status: ``timeout``" `
        -Message "Markdown should include PDF full CTest observed status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_completed_test_count: ``133``" `
        -Message "Markdown should include PDF full CTest completed count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``" `
        -Message "Markdown should include PDF full CTest zero-failure observation."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_status: ``partial``" `
        -Message "Markdown should include PDF visual gate attempt status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_verdict: ``not_complete``" `
        -Message "Markdown should include PDF visual gate attempt verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``" `
        -Message "Markdown should include PDF visual gate attempt outer guard status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``" `
        -Message "Markdown should include PDF visual gate attempt outer guard timeout flag."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``" `
        -Message "Markdown should include PDF visual gate attempt outer guard timeout seconds."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``" `
        -Message "Markdown should include PDF visual gate attempt skipped count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``" `
        -Message "Markdown should include PDF visual gate attempt render status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_status: ``pass``" `
        -Message "Markdown should include segmented PDF visual gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``" `
        -Message "Markdown should include segmented PDF visual gate full-status boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Markdown should include segmented PDF visual gate boundary."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "featherdoc.release_candidate_summary" -ExpectedFragments @(
        "pdf_visual_gate_status: ``loaded``",
        "full_visual_gate_status: ``pass``",
        "pdf_visual_gate_verdict: ``pass``",
        "pdf_visual_gate_finalizable: ``True``",
        "pdf_visual_gate_summary_json_display:",
        "summary.json",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count: ``43``",
        "pdf_visual_gate_cjk_copy_search_count: ``43``",
        "pdf_visual_gate_cjk_missing_text_count: ``0``",
        "pdf_visual_gate_visual_baseline_manifest_count: ``42``",
        "pdf_visual_gate_visual_baseline_count: ``44``",
        "pdf_bounded_ctest_summary_count: ``7``",
        "pdf_bounded_ctest_pass_count: ``7``",
        "pdf_bounded_ctest_skipped_test_count: ``0``",
        "pdf_bounded_ctest_selected_test_count: ``70``",
        "pdf_bounded_ctest_subsets:",
        "regression-table-layout",
        "pdf_bounded_ctest_summary_json_display:",
        "pdf-ctest-bounded-regression-business-samples-current\summary.json",
        "pdf_full_ctest_readiness_status: ``pass``",
        "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``",
        "pdf_full_ctest_readiness_release_ready: ``True``",
        "pdf_full_ctest_readiness_summary_json_display:",
        "pdf-release-readiness-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_summary_json_display:",
        "pdf-ctest-full-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_status: ``timeout``",
        "pdf_full_ctest_readiness_full_ctest_verdict: ``not_complete``",
        "pdf_full_ctest_readiness_selected_test_count: ``139``",
        "pdf_full_ctest_readiness_completed_test_count: ``133``",
        "pdf_full_ctest_readiness_failed_test_count: ``0``",
        "pdf_full_ctest_readiness_not_run_test_count: ``6``",
        "pdf_full_ctest_readiness_completion_percent: ``95.7``",
        "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``",
        "pdf_full_ctest_readiness_marker: ``pdf_full_ctest_readiness_trace``",
        "pdf_visual_gate_attempt_status: ``partial``",
        "pdf_visual_gate_attempt_verdict: ``not_complete``",
        "pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``",
        "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``",
        "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``",
        "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``",
        "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``",
        "pdf_visual_gate_attempt_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_status: ``pass``",
        "pdf_visual_segmented_gate_verdict: ``pass``",
        "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``",
        "pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``",
        "pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``",
        "pdf_visual_segmented_gate_summary_json_display:",
        "segmented-summary.json",
        "pdf_visual_segmented_gate_slice_summary_count: ``4``",
        "pdf_visual_segmented_gate_slice_pass_count: ``4``",
        "pdf_visual_segmented_gate_covered_baseline_count: ``44``",
        "pdf_visual_segmented_gate_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``",
        "manifest_signoff_entrypoints_status: ``declared``",
        "manifest_signoff_entrypoints_release_assets_manifest_display:",
        "release_assets_manifest.json",
        "manifest_signoff_entrypoints_required_entrypoint_count: ``3``",
        "manifest_signoff_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "manifest_signoff_entrypoints_required_contracts:",
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract",
        "manifest_signoff_entrypoints_required_fields:",
        "status",
        "release_ready",
        "release_blocker_count",
        "warning_count",
        "schema_approval_status_summary",
        "onboarding_governance_next_action",
        "onboarding_governance_next_action_summary",
        "onboarding_governance_next_action_group_count",
        "next_action",
        "next_action_summary",
        "next_action_group_count",
        "source_report_display",
        "source_json_display",
        "manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``",
        "project_template_readiness_checklist_entrypoints_status: ``declared``",
        "project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``",
        "project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids:",
        "project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``",
        "``reviewer_checklist``: required=``True``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``",
        "assert_release_material_safety.ps1",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``",
        "word_visual_standard_review_metadata_count: ``4``",
        "word_visual_standard_review_task_keys: ``smoke, fixed_grid, section_page_setup, page_number_fields``",
        "word_visual_standard_review_status_summary: ``reviewed=4``",
        "word_visual_standard_review_verdict_summary: ``pass=4``",
        "word_visual_standard_review_metadata:",
        "``smoke``: review_task_key=``document`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "``fixed_grid``: review_task_key=``fixed_grid`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "``section_page_setup``: review_task_key=``section_page_setup`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "``page_number_fields``: review_task_key=``page_number_fields`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "review_result_path:",
        "word-visual-release-gate\review-tasks\document\report\review_result.json",
        "final_review_path:",
        "word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
    ) -Message "Markdown should keep release-candidate PDF visual source-report evidence in one Source Report Contracts block."
    Assert-DoesNotContainText -Text $markdown -UnexpectedText "review_note" `
        -Message "Markdown should not expose private Word visual review notes."
    Assert-ContainsText -Text $markdown -ExpectedText "controlled_visual_smoke_status" `
        -Message "Markdown should include controlled PDF visual smoke status."
    Assert-ContainsText -Text $markdown -ExpectedText "controlled_visual_smoke_json_display" `
        -Message "Markdown should include controlled PDF visual smoke JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "controlled-visual-smoke-check-latest.json" `
        -Message "Markdown should point reviewers at the controlled visual smoke JSON."
    Assert-ContainsText -Text $markdown -ExpectedText "readiness_action_evidence" `
        -Message "Markdown should include PDF preflight readiness action evidence."
    Assert-ContainsText -Text $markdown -ExpectedText "provide_pdf_dependency_input" `
        -Message "Markdown should include dependency input readiness action."
    Assert-ContainsText -Text $markdown -ExpectedText "enable_pdf_build_option" `
        -Message "Markdown should include PDF build option readiness action."
    Assert-ContainsText -Text $markdown -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT" `
        -Message "Markdown should include the disabled PDF build option name."
    Assert-ContainsText -Text $markdown -ExpectedText "Blocker source schemas:" `
        -Message "Markdown should include blocker source-schema rollup for reviewer filtering."
    Assert-ContainsText -Text $markdown -ExpectedText "Action item source schemas:" `
        -Message "Markdown should include action source-schema rollup for reviewer filtering."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1=1" `
        -Message "Markdown should include calibration action/warning source-schema counts."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1=1" `
        -Message "Markdown should include project-template action source-schema counts."
    Assert-ContainsText -Text $markdown -ExpectedText "Warning source schemas:" `
        -Message "Markdown should include warning source-schema rollup for reviewer filtering."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1=1" `
        -Message "Markdown should include PDF preflight warning source-schema counts."
    Assert-ContainsText -Text $markdown -ExpectedText "Informational action item source schemas: ``(none)``" `
        -Message "Markdown should mark empty informational action source-schema rollup explicitly."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metric Review Focus" `
        -Message "Markdown should include governance metric review focus."
    Assert-ContainsText -Text $markdown -ExpectedText "Numbering real-corpus confidence" `
        -Message "Markdown should highlight numbering real-corpus confidence for reviewers."
    Assert-ContainsText -Text $markdown -ExpectedText "Table/layout delivery quality" `
        -Message "Markdown should highlight table layout delivery quality for reviewers."
    Assert-ContainsText -Text $markdown -ExpectedText "real_corpus_confidence" `
        -Message "Markdown should include numbering confidence metric."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_governance.real_corpus_confidence" `
        -Message "Markdown should include full numbering confidence metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Markdown should include numbering confidence source schema."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering-governance\summary.json" `
        -Message "Markdown should include numbering confidence source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report: ``$([string]$numberingMetric.source_report)``" `
        -Message "Markdown should include raw metric source report paths."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json: ``$([string]$numberingMetric.source_json)``" `
        -Message "Markdown should include raw metric source JSON paths."
    Assert-ContainsText -Text $markdown -ExpectedText "delivery_quality" `
        -Message "Markdown should include table delivery quality metric."
    Assert-ContainsText -Text $markdown -ExpectedText "table_layout_delivery_governance.delivery_quality" `
        -Message "Markdown should include full table delivery quality metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "table-layout\summary.json" `
        -Message "Markdown should include table delivery source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "catalog_coverage_percent=100" `
        -Message "Markdown should include numbering confidence detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "matched_document_count=2" `
        -Message "Markdown should include numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "catalog_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering catalog document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "baseline_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering baseline document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "matched_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering matched document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "style_numbering_issues(count=3, penalty=15)" `
        -Message "Markdown should include numbering confidence penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "unresolved_item_count=10" `
        -Message "Markdown should include table layout delivery detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "table_position_automatic_count=2" `
        -Message "Markdown should include automatic floating table detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "table_position_review_count=1" `
        -Message "Markdown should include review floating table detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_supported_geometry_percent=44" `
        -Message "Markdown should include PDF floating table supported geometry percentage."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_tracked_geometry_count=9" `
        -Message "Markdown should include PDF floating table tracked geometry count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage: ``4/9 supported (44 percent); metadata_only=5``" `
        -Message "Markdown should include PDF floating table support coverage summary."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage=4/9 supported (44 percent); metadata_only=5" `
        -Message "Markdown should include PDF floating table metadata-only count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus: ``review metadata-only tblpPr fields before approving PDF-layout-sensitive release.``" `
        -Message "Markdown should include PDF floating table reviewer focus marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "Markdown should include PDF floating table reviewer focus guidance."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_metadata_only_fields: ``leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap``" `
        -Message "Markdown should include concrete PDF floating table metadata-only fields."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_review_required_fields: ``full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution``" `
        -Message "Markdown should include concrete PDF floating table reviewer-required fields."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap" `
        -Message "Markdown details should include generic PDF floating table metadata-only fields."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution" `
        -Message "Markdown details should include generic PDF floating table reviewer-required fields."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata_only_fields: ``leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap``" `
        -Message "Markdown should include generic PDF floating table metadata-only field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields: ``full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution``" `
        -Message "Markdown should include generic PDF floating table review-required field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "floating_table_plans_pending(count=3, penalty=14)" `
        -Message "Markdown should include table layout delivery penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "review_source_table_position_plan" `
        -Message "Markdown should include table layout blocker repair strategy."
    Assert-ContainsText -Text $markdown -ExpectedText "apply-table-position-plan <table-position.plan.json> --dry-run --json" `
        -Message "Markdown should include table layout command template."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy" `
        -Message "Markdown should include repair strategy details."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
        -Message "Markdown should include command template details."
    Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-finance` template=`invoice-template` candidate=`rename`' `
        -Message "Markdown should include calibration project/template/candidate routing fields."
}
