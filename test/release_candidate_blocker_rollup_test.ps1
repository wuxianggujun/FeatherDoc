param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("rollup", "handoff")]
    [string]$Scenario = "rollup"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputRoot = Join-Path $resolvedWorkingDir "governance-input"
$documentSkeletonRollupPath = Join-Path $inputRoot "document-skeleton-governance-rollup\summary.json"
$numberingSummaryPath = Join-Path $inputRoot "numbering-catalog-governance\summary.json"
$tableSummaryPath = Join-Path $inputRoot "table-layout-governance\summary.json"
$contentControlSummaryPath = Join-Path $inputRoot "content-control-data-binding-governance\summary.json"
$summaryOutputDir = Join-Path $resolvedWorkingDir "release-candidate"
$autoDiscoverOutputRoot = Join-Path $resolvedWorkingDir "auto-discover-output"
$autoDiscoverDocumentSkeletonRollupPath = Join-Path $autoDiscoverOutputRoot "document-skeleton-governance-rollup\summary.json"
$autoDiscoverNumberingSummaryPath = Join-Path $autoDiscoverOutputRoot "numbering-catalog-governance\summary.json"
$autoDiscoverTableSummaryPath = Join-Path $autoDiscoverOutputRoot "table-layout-delivery-governance\summary.json"
$autoDiscoverContentControlSummaryPath = Join-Path $autoDiscoverOutputRoot "content-control-data-binding-governance\summary.json"
$autoDiscoverProjectSummaryPath = Join-Path $autoDiscoverOutputRoot "project-template-delivery-readiness\summary.json"
$autoDiscoverCalibrationSummaryPath = Join-Path $autoDiscoverOutputRoot "schema-patch-confidence-calibration\summary.json"

Write-JsonFile -Path $documentSkeletonRollupPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    status = "needs_review"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            status = "needs_review"
            message = "Document skeleton rollup found style numbering issues."
            action = "review_style_numbering_audit"
            source_json = "output/document-skeleton-governance/contract/style-numbering-audit.json"
            source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "open_document_skeleton_rollup"
            action = "open_document_skeleton_rollup"
            title = "Open document skeleton rollup"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "document_skeleton.exemplar_catalog_missing"
            action = "open_document_skeleton_rollup"
            message = "One exemplar catalog path is missing."
            source_json = "output/document-skeleton-governance-rollup/summary.json"
            source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
        }
    )
})

Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "numbering_catalog_governance.style_numbering_issues"
            severity = "error"
            status = "blocked"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
        }
    )
})

Write-JsonFile -Path $tableSummaryPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "table_layout_delivery.floating_table_review_pending"
            severity = "warning"
            status = "needs_review"
            message = "Floating table plans still need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_table_style_quality_visual_regression"
            action = "run_table_style_quality_visual_regression"
            title = "Generate Word-rendered table layout evidence"
        }
    )
})

Write-JsonFile -Path $contentControlSummaryPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "Bound content control still shows placeholder text."
            action = "sync_or_fill_bound_content_control"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
})

Write-JsonFile -Path $autoDiscoverDocumentSkeletonRollupPath -Value `
    (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonRollupPath | ConvertFrom-Json)

Write-JsonFile -Path $autoDiscoverNumberingSummaryPath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "numbering_catalog_governance.style_numbering_issues"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered numbering governance issue."
            action = "review_style_numbering_audit"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
        }
    )
})

Write-JsonFile -Path $autoDiscoverTableSummaryPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "table_layout_delivery.floating_table_review_pending"
            severity = "warning"
            status = "needs_review"
            message = "Autodiscovered floating table plan still needs review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_table_style_quality_visual_regression"
            action = "run_table_style_quality_visual_regression"
            title = "Generate Word-rendered table layout evidence"
        }
    )
})

Write-JsonFile -Path $autoDiscoverContentControlSummaryPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "Autodiscovered bound content control still shows placeholder text."
            action = "sync_or_fill_bound_content_control"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "custom_xml_sync_evidence_missing"
            action = "run_content_control_custom_xml_sync"
            message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
            source_json = "output/content-control-data-binding-governance/summary.json"
            source_json_display = ".\output\content-control-data-binding-governance\summary.json"
        }
    )
})

Write-JsonFile -Path $autoDiscoverProjectSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval"
            project_id = "project-alpha"
            template_name = "invoice-template"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered project template schema approval is pending."
            action = "review_schema_update_candidate"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json = "output/project-template-onboarding-governance/summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_invoice_schema"
            project_id = "project-alpha"
            template_name = "invoice-template"
            action = "review_schema_update_candidate"
            title = "Review invoice schema before release"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json = "output/project-template-onboarding-governance/summary.json"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
})

Write-JsonFile -Path $autoDiscoverCalibrationSummaryPath -Value ([ordered]@{
    schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    status = "pending_review"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.pending_schema_approvals"
            severity = "error"
            status = "pending_review"
            message = "Autodiscovered schema patch confidence calibration has pending approvals."
            action = "resolve_pending_schema_approvals"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    action_item_count = 1
    action_items = @(
        [ordered]@{
            id = "resolve_pending_schema_approvals"
            action = "resolve_pending_schema_approvals"
            title = "Resolve pending schema approvals before tightening confidence thresholds"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "schema_patch_confidence_calibration.unscored_candidates"
            action = "add_explicit_confidence_metadata"
            message = "Some schema patch candidates do not carry explicit confidence metadata."
            source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            source_json = "output/schema-patch-confidence-calibration/summary.json"
            source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
        }
    )
})

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"

if ($Scenario -eq "handoff") {
    $handoffOutputDir = Join-Path $resolvedWorkingDir "release-candidate-governance-handoff"
    $handoffArguments = @(
        "-NoProfile",
        "-NonInteractive",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-SkipConfigure",
        "-SkipBuild",
        "-SkipTests",
        "-SkipInstallSmoke",
        "-SkipVisualGate",
        "-SummaryOutputDir",
        $handoffOutputDir,
        "-ReleaseGovernanceHandoff",
        "-ReleaseGovernanceHandoffInputRoot",
        $autoDiscoverOutputRoot,
        "-ReleaseGovernanceHandoffIncludeRollup"
    )
    $handoffResult = @(& (Get-Process -Id $PID).Path @handoffArguments 2>&1)
    $handoffExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $handoffText = (@($handoffResult | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    Assert-Equal -Actual $handoffExitCode -Expected 0 `
        -Message "Release candidate governance handoff run should pass. Output: $handoffText"

    $handoffReleaseSummaryPath = Join-Path $handoffOutputDir "report\summary.json"
    $handoffFinalReviewPath = Join-Path $handoffOutputDir "report\final_review.md"
    $handoffReleaseHandoffPath = Join-Path $handoffOutputDir "report\release_handoff.md"
    $handoffArtifactGuidePath = Join-Path $handoffOutputDir "report\ARTIFACT_GUIDE.md"
    $handoffChecklistPath = Join-Path $handoffOutputDir "report\REVIEWER_CHECKLIST.md"
    $handoffSummaryPath = Join-Path $handoffOutputDir "report\release-governance-handoff\summary.json"
    $handoffMarkdownPath = Join-Path $handoffOutputDir "report\release-governance-handoff\release_governance_handoff.md"
    $handoffNestedRollupSummaryPath = Join-Path $handoffOutputDir "report\release-governance-handoff\release-blocker-rollup\summary.json"
    foreach ($path in @($handoffReleaseSummaryPath, $handoffFinalReviewPath, $handoffReleaseHandoffPath, $handoffArtifactGuidePath, $handoffChecklistPath, $handoffSummaryPath, $handoffMarkdownPath, $handoffNestedRollupSummaryPath)) {
        Assert-True -Condition (Test-Path -LiteralPath $path) `
            -Message "Expected release governance handoff artifact to exist: $path"
    }

    $handoffReleaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffReleaseSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$handoffReleaseSummary.msvc_bootstrap_mode) -Expected "not_required" `
        -Message "Handoff-only release candidate run should not require MSVC discovery."
    Assert-Equal -Actual ([string]$handoffReleaseSummary.release_governance_handoff.status) -Expected "blocked" `
        -Message "Release candidate summary should surface governance handoff status."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.expected_report_count) -Expected 5 `
        -Message "Release candidate summary should surface handoff expected report count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.loaded_report_count) -Expected 5 `
        -Message "Release candidate summary should surface handoff loaded report count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.release_blocker_count) -Expected 5 `
        -Message "Release candidate summary should surface handoff blocker count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.action_item_count) -Expected 5 `
        -Message "Release candidate summary should surface handoff action count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.warning_count) -Expected 2 `
        -Message "Release candidate summary should surface handoff warning count."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Release candidate summary should carry handoff blocker source schema."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
        -ExpectedText "project-alpha" `
        -Message "Release candidate summary should carry handoff blocker project id."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
        -ExpectedText "invoice-template" `
        -Message "Release candidate summary should carry handoff action template name."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "schema_patch_confidence_calibration.unscored_candidates" `
        -Message "Release candidate summary should carry handoff warning ids."
    Assert-ContainsText -Text (($handoffReleaseSummary.steps.release_governance_handoff.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Release candidate step summary should carry handoff action open command."
    Assert-ContainsText -Text (($handoffReleaseSummary.steps.release_governance_handoff.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "build_numbering_catalog_governance_report.ps1" `
        -Message "Release candidate handoff should default missing numbering action open commands."
    Assert-ContainsText -Text (($handoffReleaseSummary.steps.release_governance_handoff.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
        -Message "Release candidate handoff should default missing table action open commands."
    Assert-Equal -Actual ([string]$handoffReleaseSummary.steps.release_governance_handoff.status) -Expected "blocked" `
        -Message "Release candidate step status should mirror governance handoff status."

    $handoffSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$handoffSummary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
        -Message "Nested release governance handoff should write its schema."
    Assert-Equal -Actual ([bool]$handoffSummary.release_blocker_rollup.included) -Expected $true `
        -Message "Governance handoff should record the nested release blocker rollup."

    $handoffFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffFinalReviewPath
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "- Release governance handoff: blocked" `
        -Message "Final review should include release governance handoff step status."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "Release governance handoff counts: 5/5 reports, 0 missing, 5 blockers, 5 actions" `
        -Message "Final review should include release governance handoff counts."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "Release governance handoff details" `
        -Message "Final review should include release governance handoff details."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "project_template_onboarding.schema_approval" `
        -Message "Final review should include handoff blocker id."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "schema_patch_confidence_calibration.unscored_candidates" `
        -Message "Final review should include handoff warning id."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "source_schema=featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Final review should include handoff source schema."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "project_template: project_id=project-alpha template_name=invoice-template" `
        -Message "Final review should include project-template handoff identity."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Final review should include handoff action item open command."

    $handoffReleaseHandoff = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffReleaseHandoffPath
    $handoffArtifactGuide = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffArtifactGuidePath
    $handoffChecklist = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffChecklistPath
    $handoffMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffMarkdownPath
    Assert-ContainsText -Text $handoffReleaseHandoff -ExpectedText "Release Governance Handoff Details" `
        -Message "Release handoff bundle should include governance handoff details."
    Assert-ContainsText -Text $handoffReleaseHandoff -ExpectedText "source_json_display: .\output\schema-patch-confidence-calibration\summary.json" `
        -Message "Release handoff bundle should include handoff source JSON display."
    Assert-ContainsText -Text $handoffReleaseHandoff -ExpectedText "project_template: project_id=project-alpha template_name=invoice-template" `
        -Message "Release handoff bundle should include project-template identity."
    Assert-ContainsText -Text $handoffArtifactGuide -ExpectedText "Release Governance Handoff Details" `
        -Message "Artifact guide should include governance handoff details."
    Assert-ContainsText -Text $handoffArtifactGuide -ExpectedText "source_report_display:" `
        -Message "Artifact guide should include handoff source report display."
    Assert-ContainsText -Text $handoffArtifactGuide -ExpectedText "auto-discover-output\schema-patch-confidence-calibration\summary.json" `
        -Message "Artifact guide should point handoff source report display at the discovered source report."
    Assert-ContainsText -Text $handoffArtifactGuide -ExpectedText "source_json_display: .\output\schema-patch-confidence-calibration\summary.json" `
        -Message "Artifact guide should include handoff source JSON display."
    Assert-ContainsText -Text $handoffChecklist -ExpectedText "Handoff Action Items" `
        -Message "Reviewer checklist should include handoff action items."
    Assert-ContainsText -Text $handoffChecklist -ExpectedText "open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1" `
        -Message "Reviewer checklist should include handoff open command."
    Assert-ContainsText -Text $handoffChecklist -ExpectedText "project_template: project_id=project-alpha template_name=invoice-template" `
        -Message "Reviewer checklist should include project-template identity."
    Assert-ContainsText -Text $handoffMarkdown -ExpectedText 'project=`project-alpha` template=`invoice-template`' `
        -Message "Nested handoff Markdown should include project-template identity."
    Assert-ContainsText -Text $handoffMarkdown -ExpectedText 'schema=`featherdoc.schema_patch_confidence_calibration_report.v1`' `
        -Message "Nested handoff Markdown should include source schema."
    Assert-ContainsText -Text $handoffMarkdown -ExpectedText 'source_json_display: `.\output\schema-patch-confidence-calibration\summary.json`' `
        -Message "Nested handoff Markdown should include source JSON display."
    Assert-ContainsText -Text $handoffMarkdown -ExpectedText 'open_command: `pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1`' `
        -Message "Nested handoff Markdown should include handoff open command."

    Write-Host "Release candidate governance handoff regression passed."
    exit 0
}

$scriptArguments = @(
    "-NoProfile",
    "-NonInteractive",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $scriptPath,
    "-SkipConfigure",
    "-SkipBuild",
    "-SkipTests",
    "-SkipInstallSmoke",
    "-SkipVisualGate",
    "-SummaryOutputDir",
    $summaryOutputDir,
    "-ReleaseBlockerRollupInputJson",
    "$documentSkeletonRollupPath,$numberingSummaryPath,$tableSummaryPath"
)
$result = @(& (Get-Process -Id $PID).Path @scriptArguments 2>&1)
$exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
$text = (@($result | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
Assert-Equal -Actual $exitCode -Expected 0 `
    -Message "Release candidate rollup-only run should pass. Output: $text"

$summaryPath = Join-Path $summaryOutputDir "report\summary.json"
$finalReviewPath = Join-Path $summaryOutputDir "report\final_review.md"
$rollupSummaryPath = Join-Path $summaryOutputDir "report\release-blocker-rollup\summary.json"
$rollupMarkdownPath = Join-Path $summaryOutputDir "report\release-blocker-rollup\release_blocker_rollup.md"

foreach ($path in @($summaryPath, $finalReviewPath, $rollupSummaryPath, $rollupMarkdownPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected release candidate rollup artifact to exist: $path"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.msvc_bootstrap_mode) -Expected "not_required" `
    -Message "Rollup-only release candidate run should not require MSVC discovery."
Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Release candidate summary should surface the rollup status."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 3 `
    -Message "Release candidate summary should surface source report count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 3 `
    -Message "Release candidate summary should surface blocker count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 3 `
    -Message "Release candidate summary should surface action item count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 1 `
    -Message "Release candidate summary should surface warning count."
Assert-Equal -Actual ([string]$summary.steps.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Release candidate step status should mirror rollup status."
Assert-ContainsText -Text (($summary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Release candidate summary should carry rollup blocker source schema."
Assert-ContainsText -Text (($summary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "style-numbering-audit.json" `
    -Message "Release candidate summary should carry rollup blocker source JSON display."
Assert-ContainsText -Text (($summary.steps.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_document_skeleton_governance_rollup_report.ps1" `
    -Message "Release candidate step summary should carry action item open command."
Assert-ContainsText -Text (($summary.steps.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_numbering_catalog_governance_report.ps1" `
    -Message "Release candidate step summary should default missing numbering action open commands."
Assert-ContainsText -Text (($summary.steps.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
    -Message "Release candidate step summary should default missing table action open commands."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
    -Message "Nested release blocker rollup should write its schema."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "run_table_style_quality_visual_regression" `
    -Message "Nested release blocker rollup should retain table layout action items."

$finalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $finalReviewPath
Assert-ContainsText -Text $finalReview -ExpectedText "- Release blocker rollup: blocked" `
    -Message "Final review should include release blocker rollup step status."
Assert-ContainsText -Text $finalReview -ExpectedText "Release blocker rollup counts: 3 blockers, 3 actions, 1 warnings" `
    -Message "Final review should include release blocker rollup counts."
Assert-ContainsText -Text $finalReview -ExpectedText "document_skeleton.style_numbering_issues" `
    -Message "Final review should include rollup blocker id."
Assert-ContainsText -Text $finalReview -ExpectedText "source_schema=featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Final review should include rollup blocker source schema."
Assert-ContainsText -Text $finalReview -ExpectedText "open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1" `
    -Message "Final review should include rollup action item open command."

$gateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-fail-on-blocker"
$gateArguments = @($scriptArguments)
$summaryOutputIndex = [Array]::IndexOf($gateArguments, "-SummaryOutputDir")
$gateArguments[$summaryOutputIndex + 1] = $gateOutputDir
$gateArguments += "-ReleaseBlockerRollupFailOnBlocker"
${previousErrorActionPreference} = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    $gateResult = @(& (Get-Process -Id $PID).Path @gateArguments 2>&1)
} finally {
    $ErrorActionPreference = ${previousErrorActionPreference}
}
$gateExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
if ($gateExitCode -eq 0) {
    $gateText = (@($gateResult | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    throw "Release candidate rollup fail-on-blocker run should fail. Output: $gateText"
}
$gateSummaryPath = Join-Path $gateOutputDir "report\summary.json"
Assert-True -Condition (Test-Path -LiteralPath $gateSummaryPath) `
    -Message "Fail-on-blocker run should still write release candidate summary."
$gateSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $gateSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$gateSummary.release_blocker_rollup.status) -Expected "failed" `
    -Message "Fail-on-blocker run should mark rollup as failed in release summary."

$autoDiscoverOutputDir = Join-Path $resolvedWorkingDir "release-candidate-auto-discover"
$autoDiscoverArguments = @(
    "-NoProfile",
    "-NonInteractive",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    $scriptPath,
    "-SkipConfigure",
    "-SkipBuild",
    "-SkipTests",
    "-SkipInstallSmoke",
    "-SkipVisualGate",
    "-SummaryOutputDir",
    $autoDiscoverOutputDir,
    "-ReleaseBlockerRollupAutoDiscoverRoot",
    $autoDiscoverOutputRoot,
    "-ReleaseBlockerRollupAutoDiscover"
)
$autoDiscoverResult = @(& (Get-Process -Id $PID).Path @autoDiscoverArguments 2>&1)
$autoDiscoverExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
$autoDiscoverText = (@($autoDiscoverResult | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
Assert-Equal -Actual $autoDiscoverExitCode -Expected 0 `
    -Message "Release candidate auto-discovered rollup run should pass. Output: $autoDiscoverText"

$autoDiscoverSummaryPath = Join-Path $autoDiscoverOutputDir "report\summary.json"
$autoDiscoverRollupSummaryPath = Join-Path $autoDiscoverOutputDir "report\release-blocker-rollup\summary.json"
foreach ($path in @($autoDiscoverSummaryPath, $autoDiscoverRollupSummaryPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected auto-discovered release candidate artifact to exist: $path"
}

$autoDiscoverSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $autoDiscoverSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$autoDiscoverSummary.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Auto-discovered rollup should surface the blocker status."
Assert-Equal -Actual ([bool]$autoDiscoverSummary.release_blocker_rollup.auto_discover) -Expected $true `
    -Message "Release summary should record that auto-discovery was enabled."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.auto_discovered_input_json.Count) -Expected 6 `
    -Message "Release summary should record all six auto-discovered governance reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.source_report_count) -Expected 6 `
    -Message "Auto-discovered rollup should aggregate the six default governance reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.release_blocker_count) -Expected 6 `
    -Message "Auto-discovered rollup should surface blocker count from default governance reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.action_item_count) -Expected 6 `
    -Message "Auto-discovered rollup should surface action count from default governance reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.warning_count) -Expected 3 `
    -Message "Auto-discovered rollup should surface warning count from default governance reports."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "document_skeleton.exemplar_catalog_missing" `
    -Message "Auto-discovered rollup should surface document skeleton warnings."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "custom_xml_sync_evidence_missing" `
    -Message "Auto-discovered rollup should surface content-control warnings."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "schema_patch_confidence_calibration.unscored_candidates" `
    -Message "Auto-discovered rollup should surface calibration warnings."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Auto-discovered rollup should carry content-control source schema."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
    -Message "Auto-discovered rollup should carry onboarding governance source schema through delivery readiness."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
    -Message "Auto-discovered rollup should carry calibration source schema."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
    -Message "Auto-discovered rollup should carry content-control source JSON display."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "project-template-onboarding-governance\summary.json" `
    -Message "Auto-discovered rollup should carry onboarding governance source JSON display."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
    -ExpectedText "project-alpha" `
    -Message "Auto-discovered rollup should carry project-template blocker project id."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
    -ExpectedText "invoice-template" `
    -Message "Auto-discovered rollup should carry project-template action template name."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Auto-discovered rollup should carry calibration source JSON display."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Auto-discovered rollup should carry content-control action open command."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "sync_project_template_schema_approval.ps1" `
    -Message "Auto-discovered rollup should carry onboarding governance action open command."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Auto-discovered rollup should carry calibration action open command."

$autoDiscoverRollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $autoDiscoverRollupSummaryPath | ConvertFrom-Json
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "document-skeleton-governance-rollup" `
    -Message "Auto-discovered rollup should include document skeleton governance rollup."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "project-template-delivery-readiness" `
    -Message "Auto-discovered rollup should include project-template delivery readiness governance."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "content-control-data-binding-governance" `
    -Message "Auto-discovered rollup should include content-control data-binding governance."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration" `
    -Message "Auto-discovered rollup should include schema patch confidence calibration."

Write-Host "Release candidate blocker rollup regression passed."
