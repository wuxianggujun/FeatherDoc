if (Test-Scenario -Name "aggregate") {
    $inputRoot = Join-Path $resolvedWorkingDir "aggregate-input"
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Aggregate handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_governance_handoff.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Aggregate handoff should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Aggregate handoff should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
        -Message "Summary should expose release governance handoff schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Aggregate handoff should be blocked when source blockers exist."
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 6 `
        -Message "Aggregate handoff should load all six default reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Aggregate handoff should not mark default reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Aggregate handoff should normalize release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 5 `
        -Message "Aggregate handoff should normalize action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Aggregate handoff should preserve warning counts."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Aggregate handoff should preserve governance metric count."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "handoff release blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "handoff warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "handoff action items" `
        -ExpectOpenCommand $true
    Assert-GovernanceTraceMetadata -Items @($summary.informational_action_items) -CollectionName "handoff informational action items" `
        -ExpectOpenCommand $true
    $projectTemplateReport = ($summary.reports |
        Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_report_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_report_display."
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_json_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_json_display."
    $docxReadinessReport = ($summary.reports |
        Where-Object { [string]$_.id -eq "docx_functional_smoke_readiness" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$docxReadinessReport.evidence_scope) -Expected "persisted_docx_functional_smoke_evidence_only" `
        -Message "Aggregate handoff should expose DOCX readiness evidence scope."
    Assert-Equal -Actual ([string]$docxReadinessReport.marker) -Expected "docx_functional_smoke_readiness_trace" `
        -Message "Aggregate handoff should expose DOCX readiness trace marker."
    Assert-ContainsText -Text ([string]$docxReadinessReport.summary_json_display) -ExpectedText "docx-functional-smoke-readiness\summary.json" `
        -Message "Aggregate handoff should expose DOCX readiness summary display path."
    Assert-ContainsText -Text ([string]$docxReadinessReport.report_markdown_display) -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Aggregate handoff should expose DOCX readiness report markdown display path."
    Assert-ContainsText -Text ([string]$docxReadinessReport.boundary) -ExpectedText "does not claim a fresh Word COM render" `
        -Message "Aggregate handoff should expose DOCX readiness boundary."
    $projectTemplateBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.report_id -eq "project_template_delivery_readiness" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateBlocker.readiness_status) -Expected "blocked" `
        -Message "Aggregate handoff should propagate project-template readiness status to blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.readiness_release_ready) -Expected "False" `
        -Message "Aggregate handoff should propagate project-template release_ready to blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.source_schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Aggregate handoff should preserve project-template onboarding source schema on blockers."
    Assert-ContainsText -Text ([string]$projectTemplateBlocker.source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should preserve project-template onboarding source_json_display on blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_status) -Expected "pending_review" `
        -Message "Aggregate handoff should carry onboarding governance report status separately from delivery readiness."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_release_ready) -Expected "False" `
        -Message "Aggregate handoff should carry onboarding governance report release_ready separately from delivery readiness."
    Assert-ContainsText -Text ([string]$projectTemplateBlocker.onboarding_governance_source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should point onboarding governance contract at onboarding summary JSON."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_next_action.action) -Expected "approve_project_template_schema" `
        -Message "Aggregate handoff should carry onboarding governance next action on blockers."
    Assert-Equal -Actual ([int]$projectTemplateBlocker.onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Aggregate handoff should carry onboarding governance next-action group count on blockers."
    Assert-Equal -Actual ([bool]$projectTemplateBlocker.requires_reviewer_action) -Expected $true `
        -Message "Aggregate handoff should carry reviewer-action requirement on project-template blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.reviewer_action_summary) -Expected "review_schema_update_candidate" `
        -Message "Aggregate handoff should carry reviewer action summary on project-template blockers."
    Assert-ContainsText -Text ([string]$projectTemplateBlocker.reviewer_action_reason) -ExpectedText "latest_review_state=pending" `
        -Message "Aggregate handoff should carry reviewer action reason on project-template blockers."
    Assert-True -Condition (@($projectTemplateBlocker.reviewer_actions) -contains "review_schema_update_candidate") `
        -Message "Aggregate handoff should carry reviewer actions on project-template blockers."
    $projectTemplateAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "approve_project_template_schema" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateAction.readiness_status) -Expected "blocked" `
        -Message "Aggregate handoff should propagate project-template readiness status to action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.readiness_release_ready) -Expected "False" `
        -Message "Aggregate handoff should propagate project-template release_ready to action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.source_schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Aggregate handoff should preserve project-template onboarding source schema on action items."
    Assert-ContainsText -Text ([string]$projectTemplateAction.source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should preserve project-template onboarding source_json_display on action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_status) -Expected "pending_review" `
        -Message "Aggregate handoff should carry onboarding governance report status separately from action readiness."
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_release_ready) -Expected "False" `
        -Message "Aggregate handoff should carry onboarding governance report release_ready separately from action readiness."
    Assert-ContainsText -Text ([string]$projectTemplateAction.onboarding_governance_source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should point action onboarding governance contract at onboarding summary JSON."
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_next_action.action) -Expected "approve_project_template_schema" `
        -Message "Aggregate handoff should carry onboarding governance next action on action items."
    Assert-Equal -Actual ([int]$projectTemplateAction.onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Aggregate handoff should carry onboarding governance next-action group count on action items."
    Assert-Equal -Actual ([bool]$projectTemplateAction.requires_reviewer_action) -Expected $true `
        -Message "Aggregate handoff should carry reviewer-action requirement on project-template action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.reviewer_action_summary) -Expected "review_schema_update_candidate" `
        -Message "Aggregate handoff should carry reviewer action summary on project-template action items."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.report_id):$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    Assert-ContainsText -Text $metricText -ExpectedText "numbering_catalog_governance:real_corpus_confidence:low:56" `
        -Message "Aggregate handoff should preserve numbering real-corpus confidence metric."
    Assert-ContainsText -Text $metricText -ExpectedText "table_layout_delivery_governance:delivery_quality:release_ready:100" `
        -Message "Aggregate handoff should preserve table layout delivery quality metric."
    $numberingMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingMetric.source_json) -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Aggregate handoff should preserve numbering confidence raw source JSON."
    Assert-ContainsText -Text ([string]$numberingMetric.source_json_display) -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Aggregate handoff should preserve numbering confidence source JSON display."
    Assert-Equal -Actual ([int]$numberingMetric.details.catalog_coverage_percent) -Expected 100 `
        -Message "Aggregate handoff should preserve numbering confidence detail fields."
    Assert-Equal -Actual ([int]$numberingMetric.details.matched_document_count) -Expected 2 `
        -Message "Aggregate handoff should preserve numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text (($numberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Aggregate handoff should preserve numbering catalog document keys."
    Assert-ContainsText -Text (($numberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Aggregate handoff should preserve numbering baseline document keys."
    Assert-ContainsText -Text (($numberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Aggregate handoff should preserve numbering matched document keys."
    Assert-ContainsText -Text (($numberingMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "style_numbering_issues" `
        -Message "Aggregate handoff should preserve numbering confidence penalty summary."
    $tableMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableMetric.source_json_display) -ExpectedText "table-layout-delivery-governance\summary.json" `
        -Message "Aggregate handoff should preserve table delivery source JSON display."
    Assert-Equal -Actual ([int]$tableMetric.details.unresolved_item_count) -Expected 0 `
        -Message "Aggregate handoff should preserve table layout delivery quality detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_automatic_count) -Expected 0 `
        -Message "Aggregate handoff should preserve automatic floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_review_count) -Expected 0 `
        -Message "Aggregate handoff should preserve review floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_tracked_geometry_count) -Expected 9 `
        -Message "Aggregate handoff should preserve tracked PDF floating table geometry detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Aggregate handoff should preserve supported PDF floating table geometry percentage detail fields."
    Assert-ContainsText -Text (($tableMetric.details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "Aggregate handoff should preserve generic PDF floating table metadata-only fields."
    Assert-ContainsText -Text (($tableMetric.details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "table overlap avoidance and collision resolution" `
        -Message "Aggregate handoff should preserve generic PDF floating table review-required fields."
    Assert-ContainsText -Text (($tableMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "safe_tblLook_fixes_pending" `
        -Message "Aggregate handoff should preserve table layout delivery penalty summary."
    Assert-Equal -Actual (@($summary.warnings).Count) -Expected 2 `
        -Message "Aggregate handoff should normalize warning details."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Aggregate handoff should preserve numbering warning id."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Aggregate handoff should preserve warning source schema."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Aggregate handoff should preserve warning source JSON display."
    $numberingWarning = @($summary.warnings |
        Where-Object { [string]$_.id -eq "numbering_catalog_manifest_summary_missing" })[0]
    Assert-Equal -Actual ([string]$numberingWarning.repair_strategy) -Expected "rebuild_numbering_catalog_manifest_summary" `
        -Message "Aggregate handoff should preserve warning repair strategy."
    Assert-ContainsText -Text ([string]$numberingWarning.repair_hint) -ExpectedText "do not synthesize" `
        -Message "Aggregate handoff should preserve warning repair hints."
    Assert-ContainsText -Text ([string]$numberingWarning.command_template) -ExpectedText "check_numbering_catalog_manifest.ps1" `
        -Message "Aggregate handoff should preserve warning command templates."
    $calibrationBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationBlocker.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration blocker project id."
    Assert-Equal -Actual ([string]$calibrationBlocker.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration blocker template name."
    Assert-Equal -Actual ([string]$calibrationBlocker.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration blocker candidate type."
    Assert-ContainsText -Text ([string]$calibrationBlocker.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration blocker raw source report."
    Assert-ContainsText -Text ([string]$calibrationBlocker.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration blocker raw source JSON."
    $calibrationAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "resolve_pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationAction.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration action project id."
    Assert-Equal -Actual ([string]$calibrationAction.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration action template name."
    Assert-Equal -Actual ([string]$calibrationAction.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration action candidate type."
    Assert-ContainsText -Text ([string]$calibrationAction.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration action raw source report."
    Assert-ContainsText -Text ([string]$calibrationAction.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration action raw source JSON."
    $calibrationWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.unscored_candidates" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationWarning.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration warning project id."
    Assert-Equal -Actual ([string]$calibrationWarning.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration warning template name."
    Assert-Equal -Actual ([string]$calibrationWarning.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration warning candidate type."
    Assert-ContainsText -Text ([string]$calibrationWarning.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration warning raw source report."
    Assert-ContainsText -Text ([string]$calibrationWarning.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration warning raw source JSON."
    Assert-ContainsText -Text (($summary.next_commands | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "ReleaseBlockerRollupAutoDiscover" `
        -Message "Aggregate handoff should hand off to release candidate auto-discovery."
    $numberingAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "preview_style_numbering_repair" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingAction.open_command) -ExpectedText "build_numbering_catalog_governance_report.ps1" `
        -Message "Aggregate handoff should provide a reviewer open command when numbering action input omits one."
    Assert-Equal -Actual ([string]$numberingAction.action) -Expected "preview_style_numbering_repair" `
        -Message "Aggregate handoff should fall back to action item id when action is absent."
    $tableVisualAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "run_table_style_quality_visual_regression" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableVisualAction.open_command) -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
        -Message "Aggregate handoff should provide a reviewer open command when table action input omits one."
    $contentControlBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlBlocker.repair_strategy) -Expected "sync_bound_content_control" `
        -Message "Aggregate handoff should preserve content-control blocker repair strategy."
    Assert-ContainsText -Text ([string]$contentControlBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
        -Message "Aggregate handoff should preserve content-control blocker command template."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_report) -ExpectedText "content-control-data-binding-governance/summary.json" `
        -Message "Aggregate handoff should preserve content-control blocker raw source report."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_report_display) -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Aggregate handoff should preserve content-control blocker source report display."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_json) -ExpectedText "content-control-data-binding/inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control blocker raw source JSON."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_json_display) -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control blocker source JSON display."
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx) -Expected "samples/invoice.docx" `
        -Message "Aggregate handoff should preserve content-control blocker input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Aggregate handoff should preserve content-control blocker input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve content-control blocker template_name provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.schema_target) -Expected "invoice" `
        -Message "Aggregate handoff should preserve content-control blocker schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.target_mode) -Expected "resolved-section-targets" `
        -Message "Aggregate handoff should preserve content-control blocker target_mode provenance."
    $contentControlAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_duplicate_content_control_binding" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlAction.repair_strategy) -Expected "deduplicate_or_confirm_shared_binding" `
        -Message "Aggregate handoff should preserve content-control action repair strategy."
    Assert-ContainsText -Text ([string]$contentControlAction.command_template) -ExpectedText "inspect-content-controls" `
        -Message "Aggregate handoff should preserve content-control action command template."
    Assert-ContainsText -Text ([string]$contentControlAction.source_report) -ExpectedText "content-control-data-binding-governance/summary.json" `
        -Message "Aggregate handoff should preserve content-control action raw source report."
    Assert-ContainsText -Text ([string]$contentControlAction.source_report_display) -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Aggregate handoff should preserve content-control action source report display."
    Assert-ContainsText -Text ([string]$contentControlAction.source_json) -ExpectedText "content-control-data-binding/inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control action raw source JSON."
    Assert-ContainsText -Text ([string]$contentControlAction.source_json_display) -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control action source JSON display."
    Assert-Equal -Actual ([string]$contentControlAction.input_docx) -Expected "samples/invoice.docx" `
        -Message "Aggregate handoff should preserve content-control action input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlAction.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Aggregate handoff should preserve content-control action input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlAction.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve content-control action template_name provenance."
    Assert-Equal -Actual ([string]$contentControlAction.schema_target) -Expected "invoice" `
        -Message "Aggregate handoff should preserve content-control action schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlAction.target_mode) -Expected "resolved-section-targets" `
        -Message "Aggregate handoff should preserve content-control action target_mode provenance."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Governance Handoff" `
        -Message "Markdown should include handoff title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Markdown should include project-template delivery readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Markdown should include project-template delivery readiness schema."
    Assert-ContainsText -Text $markdown -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Markdown should include project-template source display path."
    Assert-ContainsText -Text $markdown -ExpectedText "latest_schema_approval_gate_status" `
        -Message "Markdown should include project-template schema approval gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_approval_status_summary" `
        -Message "Markdown should include project-template schema approval status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_action: ``review_schema_update_candidate``" `
        -Message "Markdown should include project-template reviewer action summaries."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness`' -ExpectedFragments @(
        'status=',
        'ready=',
        'source_report_display:',
        'source_json_display:',
        'latest_schema_approval_gate_status:',
        'schema_approval_status_summary:'
    ) -Message "Markdown should keep project-template readiness status, ready flag, schema approval summary, and source displays in one report-status block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`docx_functional_smoke_readiness`' -ExpectedFragments @(
        'docx_functional_smoke_ready:',
        'evidence_scope: `persisted_docx_functional_smoke_evidence_only`',
        'evidence_scope_note:',
        'does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering',
        'boundary:',
        'does not claim a fresh Word COM render',
        'marker: `docx_functional_smoke_readiness_trace`',
        'summary_json_display:',
        'docx-functional-smoke-readiness\summary.json',
        'report_markdown_display:',
        'docx_functional_smoke_readiness.md'
    ) -Message "Markdown should keep DOCX readiness evidence boundary in one report-status block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `project_template_delivery.pending_schema_approval`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`',
        'next_action:',
        'next_action_summary:',
        'next_action_group_count: `1`'
    ) -Message "Markdown should keep project-template readiness status with the handoff blocker evidence block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `approve_project_template_schema`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`',
        'next_action:',
        'next_action_summary:',
        'next_action_group_count: `1`'
    ) -Message "Markdown should keep project-template onboarding contract status with the handoff action item evidence block."
    Assert-ContainsText -Text $markdown -ExpectedText "content_control_data_binding_governance" `
        -Message "Markdown should include content-control data-binding governance."
    Assert-ContainsText -Text $markdown -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Markdown should include content-control source report display."
    Assert-ContainsText -Text $markdown -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Markdown should include content-control source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_patch_confidence_calibration" `
        -Message "Markdown should include schema patch confidence calibration."
    Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-finance` template=`invoice-template` candidate=`rename`' `
        -Message "Markdown should include calibration project/template/candidate routing fields."
    Assert-ContainsText -Text $markdown -ExpectedText "## Warnings" `
        -Message "Markdown should include handoff warnings."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include warning source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_hint:" `
        -Message "Markdown should include warning repair hints."
    Assert-ContainsText -Text $markdown -ExpectedText "check_numbering_catalog_manifest.ps1" `
        -Message "Markdown should include warning command templates."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Markdown should include per-report source failure counts."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report:" `
        -Message "Markdown should include raw source report paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json:" `
        -Message "Markdown should include raw source JSON paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metrics" `
        -Message "Markdown should include governance metrics."
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
    Assert-ContainsText -Text $markdown -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Markdown should include numbering confidence source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report: ``$([string]$numberingMetric.source_report)``" `
        -Message "Markdown should include raw metric source report paths."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json: ``$([string]$numberingMetric.source_json)``" `
        -Message "Markdown should include raw metric source JSON paths."
    Assert-ContainsText -Text $markdown -ExpectedText "delivery_quality" `
        -Message "Markdown should include table delivery quality metric."
    Assert-ContainsText -Text $markdown -ExpectedText "table_layout_delivery_governance.delivery_quality" `
        -Message "Markdown should include full table delivery quality metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "table-layout-delivery-governance\summary.json" `
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
    Assert-ContainsText -Text $markdown -ExpectedText "unresolved_item_count=0" `
        -Message "Markdown should include table layout delivery detail fields."
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
    Assert-ContainsText -Text $markdown -ExpectedText "safe_tblLook_fixes_pending(count=0, penalty=0)" `
        -Message "Markdown should include table layout delivery penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy" `
        -Message "Markdown should include repair strategy details."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
        -Message "Markdown should include command template details."
}
