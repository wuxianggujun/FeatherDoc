param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("aggregate", "fail_on_blocker", "fail_on_warning", "markdown_counts")]
    [string]$Scenario = "aggregate"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_release_governance_pipeline_report_test"
}

. (Join-Path $PSScriptRoot "build_release_governance_pipeline_report_test_helpers.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputRoot = Join-Path $resolvedWorkingDir "input"
$outputRoot = Join-Path $resolvedWorkingDir "pipeline"
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_pipeline_report.ps1"

if ($Scenario -eq "markdown_counts") {
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-DocxVisualSmokeFixture -Root (Join-Path $inputRoot "docx-functional-smoke-visual\passing-smoke")
    $result = Invoke-Pipeline -Arguments @(
        "-InputRoot"
        $inputRoot
        "-OutputRoot"
        $outputRoot
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Pipeline markdown-counts run should finish without fail switches. Output: $($result.Text)"

    $markdownPath = Join-Path $outputRoot "release_governance_pipeline.md"
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Pipeline markdown-counts run should write Markdown."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Failed stages: ``0``" `
        -Message "Pipeline Markdown should keep missing default inputs as warnings, not failed stages."
    Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``0``" `
        -Message "Pipeline Markdown should show that default stage summaries were still written."
    Assert-ContainsText -Text $markdown -ExpectedText "Status: ``needs_review``" `
        -Message "Pipeline Markdown should expose needs-review status for empty input roots."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``needs_review``" `
        -Message "Pipeline Markdown should expose needs-review stage status."
    Assert-MatchesText -Text $markdown -Pattern "missing_reports=``\d+``" `
        -Message "Pipeline Markdown should include stage missing report counts."
    Assert-MatchesText -Text $markdown -Pattern "failed_reports=``\d+``" `
        -Message "Pipeline Markdown should include stage failed report counts."
    Assert-MatchesText -Text $markdown -Pattern "source_failures=``\d+``" `
        -Message "Pipeline Markdown should include stage source failure counts."
    Assert-MatchesText -Text $markdown -Pattern "source_failure_count=``\d+``" `
        -Message "Pipeline Markdown should expose machine-readable stage source failure counts."
    Assert-ContainsText -Text $markdown -ExpectedText "Warnings:" `
        -Message "Pipeline Markdown should preserve warning counts in empty-input runs."
    Assert-ContainsText -Text $markdown -ExpectedText "docx_functional_smoke_readiness" `
        -Message "Pipeline Markdown should preserve the DOCX readiness stage in empty-input runs."
    Write-Host "Release governance pipeline markdown count regression passed."
    return
}

New-InputFixture -Root $inputRoot

$arguments = @(
    "-InputRoot"
    $inputRoot
    "-OutputRoot"
    $outputRoot
)
if ($Scenario -eq "aggregate") {
    $arguments += "-IncludeHandoffRollup"
}
if ($Scenario -eq "fail_on_blocker") {
    $arguments += "-FailOnBlocker"
}
if ($Scenario -eq "fail_on_warning") {
    $arguments += "-FailOnWarning"
}

$result = Invoke-Pipeline -Arguments $arguments
if ($Scenario -eq "fail_on_blocker") {
    if ($result.ExitCode -eq 0) {
        throw "Pipeline fail-on-blocker run should fail. Output: $($result.Text)"
    }
    $summaryPath = Join-Path $outputRoot "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Pipeline fail-on-blocker run should still write a summary."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-True -Condition ([int]$summary.release_blocker_count -gt 0) `
        -Message "Pipeline fail-on-blocker run should report release blockers."
    Write-Host "Release governance pipeline fail-on-blocker regression passed."
    return
} elseif ($Scenario -eq "fail_on_warning") {
    if ($result.ExitCode -eq 0) {
        throw "Pipeline fail-on-warning run should fail. Output: $($result.Text)"
    }
    $summaryPath = Join-Path $outputRoot "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Pipeline fail-on-warning run should still write a summary."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-True -Condition ([int]$summary.warning_count -gt 0) `
        -Message "Pipeline fail-on-warning run should report warnings."
    Write-Host "Release governance pipeline fail-on-warning regression passed."
    return
} else {
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Pipeline aggregate run should pass. Output: $($result.Text)"
}

$summaryPath = Join-Path $outputRoot "summary.json"
$markdownPath = Join-Path $outputRoot "release_governance_pipeline.md"
$handoffSummaryPath = Join-Path $outputRoot "release-governance-handoff\summary.json"
$nestedHandoffRollupPath = Join-Path $outputRoot "release-governance-handoff\release-blocker-rollup\summary.json"
$rollupSummaryPath = Join-Path $outputRoot "release-blocker-rollup\summary.json"
foreach ($path in @($summaryPath, $markdownPath, $handoffSummaryPath, $nestedHandoffRollupPath, $rollupSummaryPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected pipeline artifact to exist: $path"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_pipeline_report.v1" `
    -Message "Pipeline summary should expose schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
    -Message "Pipeline should be blocked by fixture governance reports."
Assert-Equal -Actual ([string]$summary.governance_detail_source) -Expected "release_blocker_rollup" `
    -Message "Pipeline should expose final rollup as the top-level governance detail source."
Assert-Equal -Actual ([string]$summary.local_governance_closure.schema) -Expected "featherdoc.release_governance_local_closure.v1" `
    -Message "Pipeline should expose the local governance closure schema."
Assert-Equal -Actual ([string]$summary.local_governance_closure.status) -Expected "closed" `
    -Message "Pipeline should mark the local governance closure closed when DOCX readiness, handoff, and rollup all complete."
Assert-Equal -Actual ([bool]$summary.local_governance_closure.closed) -Expected $true `
    -Message "Pipeline should expose a machine-readable local governance closure flag."
Assert-Equal -Actual ([int]$summary.local_governance_closure.required_stage_count) -Expected 3 `
    -Message "Pipeline should track the three required local closure stages."
Assert-Equal -Actual ([int]$summary.local_governance_closure.completed_required_stage_count) -Expected 3 `
    -Message "Pipeline should count all completed local closure stages."
Assert-Equal -Actual ([string]$summary.local_governance_closure.governance_detail_source) -Expected "release_blocker_rollup" `
    -Message "Local governance closure should mirror the final governance detail source."
Assert-ContainsText -Text ([string]$summary.local_governance_closure.pipeline_summary_json_display) `
    -ExpectedText "pipeline\summary.json" `
    -Message "Local governance closure should point reviewers at the pipeline summary."
Assert-ContainsText -Text ([string]$summary.local_governance_closure.pipeline_report_markdown_display) `
    -ExpectedText "release_governance_pipeline.md" `
    -Message "Local governance closure should point reviewers at the pipeline Markdown."
Assert-ContainsText -Text (($summary.local_governance_closure.final_governance_reports | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "docx-functional-smoke-readiness\summary.json" `
    -Message "Local governance closure should include DOCX readiness as final governance evidence."
Assert-ContainsText -Text (($summary.local_governance_closure.final_governance_reports | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf-visual-release-gate-preflight-governance\summary.json" `
    -Message "Local governance closure should preserve optional PDF preflight evidence when present."
$closureStageIds = @($summary.local_governance_closure.required_stages | ForEach-Object { [string]$_.id })
foreach ($expectedClosureStage in @(
        "docx_functional_smoke_readiness",
        "release_governance_handoff",
        "release_blocker_rollup"
    )) {
    Assert-ContainsText -Text ($closureStageIds -join "`n") -ExpectedText $expectedClosureStage `
        -Message "Local governance closure should include required stage $expectedClosureStage."
}
foreach ($closureStage in @($summary.local_governance_closure.required_stages)) {
    Assert-Equal -Actual ([bool]$closureStage.closed) -Expected $true `
        -Message "Local governance closure stage $($closureStage.id) should be marked closed."
    Assert-ContainsText -Text ([string]$closureStage.summary_json_display) `
        -ExpectedText "summary.json" `
        -Message "Local governance closure stage $($closureStage.id) should keep a summary display path."
}
Assert-Equal -Actual ([int]$summary.stage_count) -Expected 8 `
    -Message "Pipeline should run eight read-only stages."
Assert-Equal -Actual ([int]$summary.completed_stage_count) -Expected 8 `
    -Message "Pipeline should complete every stage."
Assert-Equal -Actual ([int]$summary.failed_stage_count) -Expected 0 `
    -Message "Pipeline should not record stage failures."
Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 12 `
    -Message "Pipeline should mirror final rollup blocker count."
Assert-True -Condition ([int]$summary.action_item_count -ge 4) `
    -Message "Pipeline should mirror final rollup action count."
Assert-ContainsText -Text (($summary.final_governance_reports | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf-visual-release-gate-preflight-governance\summary.json" `
    -Message "Pipeline should include an existing PDF preflight governance summary in final governance inputs."
Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline top-level blocker details should mirror final rollup blockers."
Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline top-level action details should mirror final rollup actions."
Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
    -Message "Pipeline top-level warning details should mirror final rollup warnings."
Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Pipeline top-level warnings should preserve PDF controlled smoke source JSON display."

$stageIds = @($summary.stages | ForEach-Object { [string]$_.id })
foreach ($expectedStage in @(
        "numbering_catalog_governance",
        "table_layout_delivery_governance",
        "content_control_data_binding_governance",
        "project_template_delivery_readiness",
        "schema_patch_confidence_calibration",
        "docx_functional_smoke_readiness",
        "release_governance_handoff",
        "release_blocker_rollup"
    )) {
    Assert-ContainsText -Text ($stageIds -join "`n") -ExpectedText $expectedStage `
        -Message "Pipeline should include stage $expectedStage."
}

foreach ($stage in @($summary.stages)) {
    Assert-StageGovernanceItemSourceTrace -Stage $stage -PropertyName "release_blockers"
    Assert-StageGovernanceItemSourceTrace -Stage $stage -PropertyName "warnings"
    Assert-StageGovernanceItemSourceTrace -Stage $stage -PropertyName "action_items" `
        -ExpectOpenCommandProperty $true
    Assert-StageGovernanceItemSourceTrace -Stage $stage -PropertyName "informational_action_items" `
        -ExpectOpenCommandProperty $true
}

$numberingStage = Get-StageById -Summary $summary -Id "numbering_catalog_governance"
Assert-ContainsText -Text (($numberingStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Pipeline numbering stage should expose document skeleton blocker source schema."
Assert-ContainsText -Text (($numberingStage.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "document-skeleton-governance-rollup\summary.json" `
    -Message "Pipeline numbering stage should expose document skeleton source report display."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "document-skeleton-governance-rollup\summary.json" `
    -Message "Pipeline numbering stage should expose document skeleton action source JSON display."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.audit_command }) -join "`n") `
    -ExpectedText "audit-style-numbering" `
    -Message "Pipeline numbering stage should preserve action audit commands."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.review_command }) -join "`n") `
    -ExpectedText "build_document_skeleton_governance_rollup_report.ps1" `
    -Message "Pipeline numbering stage should preserve action review commands."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_numbering_catalog_governance_report.ps1" `
    -Message "Pipeline numbering stage should provide stage rerun command when source actions omit open commands."
$numberingInformationalActions = @($numberingStage.informational_action_items | Where-Object {
        [string]$_.id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")
    })
Assert-Equal -Actual $numberingInformationalActions.Count -Expected 2 `
    -Message "Pipeline numbering stage should preserve release checklist actions as informational evidence."
Assert-Equal -Actual (@($numberingStage.action_items | Where-Object {
            [string]$_.id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")
        }).Count) -Expected 0 `
    -Message "Pipeline numbering stage should not expose release checklist actions as actionable items."
foreach ($item in $numberingInformationalActions) {
    Assert-Equal -Actual ([string]$item.category) -Expected "release_checklist" `
        -Message "Pipeline numbering informational action should preserve release-checklist category."
    Assert-Equal -Actual ([string]$item.severity) -Expected "info" `
        -Message "Pipeline numbering informational action should preserve info severity."
    Assert-Equal -Actual ($item.release_blocking -is [bool]) -Expected $true `
        -Message "Pipeline numbering informational action should keep release_blocking as JSON bool."
    Assert-Equal -Actual ([bool]$item.release_blocking) -Expected $false `
        -Message "Pipeline numbering informational action should be non-blocking."
    Assert-Equal -Actual ($item.optional -is [bool]) -Expected $true `
        -Message "Pipeline numbering informational action should keep optional as JSON bool."
    Assert-Equal -Actual ([bool]$item.optional) -Expected $true `
        -Message "Pipeline numbering informational action should remain optional."
}

$tableStage = Get-StageById -Summary $summary -Id "table_layout_delivery_governance"
Assert-ContainsText -Text (($tableStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
    -Message "Pipeline table-layout stage should provide stage rerun command when source actions omit open commands."
$tableStageSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath ([string]$tableStage.summary_json) | ConvertFrom-Json
Assert-Equal -Actual ([int]$tableStageSummary.pdf_floating_table_tracked_geometry_count) -Expected 9 `
    -Message "Pipeline table-layout stage should preserve PDF floating table tracked geometry count."
Assert-Equal -Actual ([int]$tableStageSummary.pdf_floating_table_supported_geometry_percent) -Expected 44 `
    -Message "Pipeline table-layout stage should preserve PDF floating table supported geometry percent."
Assert-ContainsText -Text (($tableStageSummary.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "tblOverlap" `
    -Message "Pipeline table-layout stage should preserve metadata-only PDF floating table fields."
Assert-ContainsText -Text (($tableStageSummary.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "table overlap avoidance and collision resolution" `
    -Message "Pipeline table-layout stage should preserve review-required PDF floating table fields."
Assert-ContainsText -Text (($tableStageSummary.delivery_quality.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "topFromText outside paragraph anchoring" `
    -Message "Pipeline table-layout delivery_quality should preserve metadata-only PDF floating table fields."
Assert-ContainsText -Text (($tableStageSummary.delivery_quality.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "full Word-compatible floating table text wrapping" `
    -Message "Pipeline table-layout delivery_quality should preserve review-required PDF floating table fields."

$contentControlStage = Get-StageById -Summary $summary -Id "content_control_data_binding_governance"
Assert-ContainsText -Text (($contentControlStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Pipeline content-control stage should expose governance blocker source schema."
Assert-ContainsText -Text (($contentControlStage.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "sync-content-controls-from-custom-xml.json" `
    -Message "Pipeline content-control stage should expose sync evidence JSON display."
Assert-ContainsText -Text (($contentControlStage.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "content-control-data-binding-governance\summary.json" `
    -Message "Pipeline content-control stage should expose governance blocker source report display."
$placeholderBlockers = @($contentControlStage.release_blockers | Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" })
Assert-Equal -Actual $placeholderBlockers.Count -Expected 1 `
    -Message "Pipeline content-control stage should include one bound-placeholder blocker."
$placeholderBlocker = $placeholderBlockers[0]
Assert-Equal -Actual ([string]$placeholderBlocker.repair_strategy) -Expected "sync_bound_content_control" `
    -Message "Pipeline content-control stage should preserve blocker repair strategy."
Assert-ContainsText -Text ([string]$placeholderBlocker.repair_hint) `
    -ExpectedText "Rerun Custom XML sync" `
    -Message "Pipeline content-control stage should preserve blocker repair hint."
Assert-ContainsText -Text ([string]$placeholderBlocker.command_template) `
    -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Pipeline content-control stage should preserve blocker command template."
Assert-ContainsText -Text (($contentControlStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Pipeline content-control stage should expose reviewer open command."
Assert-ContainsText -Text (($contentControlStage.action_items | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "content-control-data-binding-governance\summary.json" `
    -Message "Pipeline content-control stage should expose action source report display."
$lockActions = @($contentControlStage.action_items | Where-Object { [string]$_.id -eq "review_content_control_lock_strategy" })
Assert-Equal -Actual $lockActions.Count -Expected 1 `
    -Message "Pipeline content-control stage should include one lock review action."
$lockAction = $lockActions[0]
Assert-Equal -Actual ([string]$lockAction.repair_strategy) -Expected "review_lock_state" `
    -Message "Pipeline content-control stage should preserve action repair strategy."
Assert-ContainsText -Text ([string]$lockAction.command_template) `
    -ExpectedText "--clear-lock" `
    -Message "Pipeline content-control stage should preserve action command template."

$projectStage = Get-StageById -Summary $summary -Id "project_template_delivery_readiness"
Assert-ContainsText -Text (($projectStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
    -Message "Pipeline project stage should expose onboarding blocker source schema."
Assert-ContainsText -Text (($projectStage.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "project-template-onboarding-governance\summary.json" `
    -Message "Pipeline project stage should expose onboarding blocker source report display."
Assert-ContainsText -Text (($projectStage.action_items | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "project-template-onboarding-governance\summary.json" `
    -Message "Pipeline project stage should expose onboarding action source JSON display."
Assert-ContainsText -Text (($projectStage.action_items | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "project-template-onboarding-governance\summary.json" `
    -Message "Pipeline project stage should expose onboarding action source report display."
Assert-ContainsText -Text (($projectStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
    -Message "Pipeline project stage should expose reviewer open command."
Assert-ContainsText -Text (($projectStage.input_json | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "project-template-smoke\summary.json" `
    -Message "Pipeline project stage should pass real smoke summary evidence into delivery readiness."

$calibrationStage = Get-StageById -Summary $summary -Id "schema_patch_confidence_calibration"
Assert-ContainsText -Text (($calibrationStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
    -Message "Pipeline calibration stage should expose blocker source schema."
Assert-ContainsText -Text (($calibrationStage.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Pipeline calibration stage should expose blocker source report display."
Assert-ContainsText -Text (($calibrationStage.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Pipeline calibration stage should expose warning source JSON display."
Assert-ContainsText -Text (($calibrationStage.warnings | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Pipeline calibration stage should expose warning source report display."
Assert-ContainsText -Text (($calibrationStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Pipeline calibration stage should expose reviewer open command."
Assert-ContainsText -Text (($calibrationStage.action_items | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Pipeline calibration stage should expose action source report display."
Assert-ContainsText -Text (($calibrationStage.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
    -ExpectedText "project-finance" `
    -Message "Pipeline calibration stage should preserve blocker project id."
Assert-ContainsText -Text (($calibrationStage.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
    -ExpectedText "invoice-template" `
    -Message "Pipeline calibration stage should preserve action template name."
Assert-ContainsText -Text (($calibrationStage.warnings | ForEach-Object { [string]$_.candidate_type }) -join "`n") `
    -ExpectedText "rename" `
    -Message "Pipeline calibration stage should preserve warning candidate type."

$docxStage = Get-StageById -Summary $summary -Id "docx_functional_smoke_readiness"
Assert-ContainsText -Text ([string]$docxStage.summary_json_display) `
    -ExpectedText "docx-functional-smoke-readiness\summary.json" `
    -Message "Pipeline DOCX stage should write a governance-consumable summary."
if ([int]$docxStage.warning_count -gt 0) {
    Assert-ContainsText -Text (($docxStage.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "word_visual_smoke.pending_manual_review" `
        -Message "Pipeline DOCX stage should preserve pending visual review warnings when review is not closed."
    Assert-ContainsText -Text (($docxStage.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.docx_functional_smoke_readiness.v1" `
        -Message "Pipeline DOCX stage should expose DOCX readiness warning source schema."
} else {
    Assert-Equal -Actual ([string]$docxStage.status) -Expected "pass" `
        -Message "Pipeline DOCX stage without warnings should represent reviewed pass evidence."
    Assert-Equal -Actual ([int]$docxStage.release_blocker_count) -Expected 0 `
        -Message "Pipeline DOCX reviewed pass evidence should not add release blockers."
}

$handoffSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$handoffSummary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
    -Message "Pipeline handoff should expose schema."
Assert-Equal -Actual ([bool]$handoffSummary.release_blocker_rollup.included) -Expected $true `
    -Message "Pipeline handoff should include nested release blocker rollup."
$handoffTableMetric = @($handoffSummary.governance_metrics |
    Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" })
Assert-Equal -Actual $handoffTableMetric.Count -Expected 1 `
    -Message "Pipeline handoff should expose one table-layout delivery quality governance metric."
Assert-ContainsText -Text (($handoffTableMetric[0].details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "tblOverlap" `
    -Message "Pipeline handoff should preserve table-layout metadata-only PDF alias fields."
Assert-ContainsText -Text (($handoffTableMetric[0].details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "table overlap avoidance and collision resolution" `
    -Message "Pipeline handoff should preserve table-layout review-required PDF alias fields."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
$rollupTableMetric = @($rollupSummary.governance_metrics |
    Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" })
Assert-Equal -Actual $rollupTableMetric.Count -Expected 1 `
    -Message "Pipeline final rollup should expose one table-layout delivery quality governance metric."
Assert-ContainsText -Text (($rollupTableMetric[0].details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "topFromText outside paragraph anchoring" `
    -Message "Pipeline final rollup should preserve table-layout metadata-only PDF alias fields."
Assert-ContainsText -Text (($rollupTableMetric[0].details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "full Word-compatible floating table text wrapping" `
    -Message "Pipeline final rollup should preserve table-layout review-required PDF alias fields."
Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline final rollup should include PDF preflight blockers."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline final rollup should include PDF preflight actions."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "Pipeline final rollup should preserve the real PDF preflight governance writer command."
Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Pipeline final rollup should preserve the PDF preflight source schema."
Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
    -Message "Pipeline final rollup should include PDF preflight warnings."
Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Pipeline final rollup should preserve PDF preflight warning source JSON display."
$rollupInformationalActions = @($rollupSummary.informational_action_items | Where-Object {
        [string]$_.id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")
    })
Assert-Equal -Actual $rollupInformationalActions.Count -Expected 2 `
    -Message "Pipeline final rollup should preserve release checklist actions as informational evidence."
Assert-Equal -Actual (@($summary.action_items | Where-Object {
            [string]$_.id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")
        }).Count) -Expected 0 `
    -Message "Pipeline top-level summary should not expose release checklist actions as actionable items."
foreach ($item in @($summary.informational_action_items | Where-Object {
            [string]$_.id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")
        })) {
    Assert-Equal -Actual ($item.release_blocking -is [bool]) -Expected $true `
        -Message "Pipeline top-level informational action should keep release_blocking as JSON bool."
    Assert-Equal -Actual ([bool]$item.release_blocking) -Expected $false `
        -Message "Pipeline top-level informational action should be non-blocking."
    Assert-Equal -Actual ($item.optional -is [bool]) -Expected $true `
        -Message "Pipeline top-level informational action should keep optional as JSON bool."
    Assert-Equal -Actual ([bool]$item.optional) -Expected $true `
        -Message "Pipeline top-level informational action should remain optional."
}

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
Assert-ContainsText -Text $markdown -ExpectedText "# Release Governance Pipeline" `
    -Message "Pipeline Markdown should include title."
Assert-ContainsText -Text $markdown -ExpectedText 'Governance detail source: `release_blocker_rollup`' `
    -Message "Pipeline Markdown should include the governance detail source."
Assert-ContainsText -Text $markdown -ExpectedText "Local governance closure: ``closed`` (3/3 required stages)" `
    -Message "Pipeline Markdown should expose the local closure state."
Assert-ContainsText -Text $markdown -ExpectedText "## Local Governance Closure" `
    -Message "Pipeline Markdown should include a local closure section."
Assert-ContainsText -Text $markdown -ExpectedText '`docx_functional_smoke_readiness`: status=`pass` closed=`True`' `
    -Message "Pipeline Markdown should expose the DOCX readiness closure stage."
Assert-ContainsText -Text $markdown -ExpectedText '`release_governance_handoff`: status=`blocked` closed=`True`' `
    -Message "Pipeline Markdown should expose the handoff closure stage."
Assert-ContainsText -Text $markdown -ExpectedText '`release_blocker_rollup`: status=`blocked` closed=`True`' `
    -Message "Pipeline Markdown should expose the final rollup closure stage."
Assert-ContainsText -Text $markdown -ExpectedText "release_blocker_rollup" `
    -Message "Pipeline Markdown should include final rollup stage."
Assert-ContainsText -Text $markdown -ExpectedText "Failed stages: ``0``" `
    -Message "Pipeline Markdown should include failed stage count."
Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``0``" `
    -Message "Pipeline Markdown should include missing report count."
Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
    -Message "Pipeline Markdown should include stage source report displays."
Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
    -Message "Pipeline Markdown should include stage source JSON displays."
Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
    -Message "Pipeline Markdown should include stage reviewer open commands."
Assert-ContainsText -Text $markdown -ExpectedText "audit_command:" `
    -Message "Pipeline Markdown should include stage audit commands."
Assert-ContainsText -Text $markdown -ExpectedText "review_command:" `
    -Message "Pipeline Markdown should include stage review commands."
Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy:" `
    -Message "Pipeline Markdown should include stage repair strategies."
Assert-ContainsText -Text $markdown -ExpectedText "command_template:" `
    -Message "Pipeline Markdown should include stage repair command templates."
Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-finance` template=`invoice-template` candidate=`rename`' `
    -Message "Pipeline Markdown should include calibration project/template/candidate routing fields."
Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline Markdown should include PDF preflight blocker ids."
Assert-ContainsText -Text $markdown -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline Markdown should include PDF preflight action ids."

Write-Host "Release governance pipeline report regression passed."
