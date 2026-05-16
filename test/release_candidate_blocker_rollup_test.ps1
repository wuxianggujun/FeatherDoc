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
$numberingSummaryPath = Join-Path $inputRoot "numbering-catalog-governance\summary.json"
$tableSummaryPath = Join-Path $inputRoot "table-layout-governance\summary.json"
$contentControlSummaryPath = Join-Path $inputRoot "content-control-data-binding-governance\summary.json"
$summaryOutputDir = Join-Path $resolvedWorkingDir "release-candidate"
$autoDiscoverOutputRoot = Join-Path $resolvedWorkingDir "auto-discover-output"
$autoDiscoverNumberingSummaryPath = Join-Path $autoDiscoverOutputRoot "numbering-catalog-governance\summary.json"
$autoDiscoverTableSummaryPath = Join-Path $autoDiscoverOutputRoot "table-layout-delivery-governance\summary.json"
$autoDiscoverContentControlSummaryPath = Join-Path $autoDiscoverOutputRoot "content-control-data-binding-governance\summary.json"
$autoDiscoverProjectSummaryPath = Join-Path $autoDiscoverOutputRoot "project-template-delivery-readiness\summary.json"
$autoDiscoverRestoreAuditSummaryPath = Join-Path $autoDiscoverOutputRoot "document-skeleton-governance\style-merge.restore-audit.summary.json"

Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    release_blocker_count = 1
    warning_count = 1
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
    warnings = @(
        [ordered]@{
            id = "document_skeleton.style_merge_suggestions_pending"
            source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
            action = "review_style_merge_suggestions"
            style_merge_suggestion_count = 2
            message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
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
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
        }
    )
})

Write-JsonFile -Path $autoDiscoverNumberingSummaryPath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    release_blocker_count = 1
    warning_count = 1
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
    warnings = @(
        [ordered]@{
            id = "document_skeleton.style_merge_suggestions_pending"
            source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
            action = "review_style_merge_suggestions"
            style_merge_suggestion_count = 2
            message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
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
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_duplicate_content_control_binding"
            action = "review_duplicate_content_control_binding"
            title = "Review repeated content controls that share one Custom XML binding"
        }
    )
})

Write-JsonFile -Path $autoDiscoverProjectSummaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_delivery.pending_schema_approval"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered project template schema approval is pending."
            action = "approve_project_template_schema"
        }
    )
    action_items = @(
        [ordered]@{
            id = "approve_project_template_schema"
            action = "approve_project_template_schema"
            title = "Approve project template schema before release"
        }
    )
})

Write-JsonFile -Path $autoDiscoverRestoreAuditSummaryPath -Value ([ordered]@{
    schema = "featherdoc.style_merge_restore_audit.v1"
    status = "blocked"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "style_merge.restore_audit_issues"
            severity = "error"
            status = "blocked"
            message = "Autodiscovered style merge restore audit found dry-run issues."
            action = "review_style_merge_restore_audit"
        }
    )
    action_item_count = 1
    action_items = @(
        [ordered]@{
            id = "review_style_merge_restore_audit"
            action = "review_style_merge_restore_audit"
            title = "Review style merge restore audit and Word render"
            command = "pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx -DocumentSourceKind style-merge-restore-audit -DocumentSourceLabel `"Style merge restore audit`" -Mode review-only"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit -PrintPrompt"
        }
    )
    warning_count = 0
    warnings = @()
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
    $handoffSummaryPath = Join-Path $handoffOutputDir "report\release-governance-handoff\summary.json"
    $handoffMarkdownPath = Join-Path $handoffOutputDir "report\release-governance-handoff\release_governance_handoff.md"
    $handoffNestedRollupSummaryPath = Join-Path $handoffOutputDir "report\release-governance-handoff\release-blocker-rollup\summary.json"
    foreach ($path in @($handoffReleaseSummaryPath, $handoffFinalReviewPath, $handoffSummaryPath, $handoffMarkdownPath, $handoffNestedRollupSummaryPath)) {
        Assert-True -Condition (Test-Path -LiteralPath $path) `
            -Message "Expected release governance handoff artifact to exist: $path"
    }

    $handoffReleaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffReleaseSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$handoffReleaseSummary.msvc_bootstrap_mode) -Expected "not_required" `
        -Message "Handoff-only release candidate run should not require MSVC discovery."
    Assert-Equal -Actual ([string]$handoffReleaseSummary.release_governance_handoff.status) -Expected "blocked" `
        -Message "Release candidate summary should surface governance handoff status."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.expected_report_count) -Expected 4 `
        -Message "Release candidate summary should surface handoff expected report count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.loaded_report_count) -Expected 4 `
        -Message "Release candidate summary should surface handoff loaded report count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.release_blocker_count) -Expected 4 `
        -Message "Release candidate summary should surface handoff blocker count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.action_item_count) -Expected 4 `
        -Message "Release candidate summary should surface handoff action count."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.warning_count) -Expected 1 `
        -Message "Release candidate summary should surface handoff warning count."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Release candidate summary should surface handoff warning details."
    $handoffWarning = @($handoffReleaseSummary.release_governance_handoff.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$handoffWarning[0].action) -Expected "review_style_merge_suggestions" `
        -Message "Release candidate summary should preserve handoff warning action."
    Assert-Equal -Actual ([string]$handoffWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Release candidate summary should preserve handoff warning source schema."
    Assert-Equal -Actual ([int]$handoffReleaseSummary.release_governance_handoff.release_blocker_rollup.warning_count) -Expected 1 `
        -Message "Release candidate summary should surface nested handoff rollup warning count."
    Assert-ContainsText -Text (($handoffReleaseSummary.release_governance_handoff.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Release candidate summary should surface nested handoff rollup warning details."
    $handoffNestedWarning = @($handoffReleaseSummary.release_governance_handoff.release_blocker_rollup.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$handoffNestedWarning[0].action) -Expected "review_style_merge_suggestions" `
        -Message "Release candidate summary should preserve nested handoff rollup warning action."
    Assert-Equal -Actual ([string]$handoffNestedWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Release candidate summary should preserve nested handoff rollup warning source schema."
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
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "Release governance handoff counts: 4/4 reports, 0 missing, 4 blockers, 4 actions, 1 warnings" `
        -Message "Final review should include release governance handoff counts."
    Assert-ContainsText -Text $handoffFinalReview -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Final review should surface governance handoff warning details."

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
    "$numberingSummaryPath,$tableSummaryPath"
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
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 2 `
    -Message "Release candidate summary should surface source report count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 2 `
    -Message "Release candidate summary should surface blocker count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 2 `
    -Message "Release candidate summary should surface action item count."
Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 1 `
    -Message "Release candidate summary should surface warning count."
Assert-ContainsText -Text (($summary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "numbering_catalog_governance.style_numbering_issues" `
    -Message "Release candidate summary should surface rollup blocker details."
Assert-ContainsText -Text (($summary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "review_style_numbering_audit" `
    -Message "Release candidate summary should preserve rollup blocker actions."
Assert-ContainsText -Text (($summary.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
    -Message "Release candidate summary should surface warning details."
$rollupWarning = @($summary.release_blocker_rollup.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
Assert-Equal -Actual ([string]$rollupWarning[0].action) -Expected "review_style_merge_suggestions" `
    -Message "Release candidate summary should preserve rollup warning action."
Assert-Equal -Actual ([string]$rollupWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Release candidate summary should preserve rollup warning source schema."
Assert-Equal -Actual ([string]$summary.steps.release_blocker_rollup.status) -Expected "blocked" `
    -Message "Release candidate step status should mirror rollup status."
Assert-ContainsText -Text (($summary.steps.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "numbering_catalog_governance.style_numbering_issues" `
    -Message "Release candidate step summary should mirror rollup blocker details."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
    -Message "Nested release blocker rollup should write its schema."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "run_table_style_quality_visual_regression" `
    -Message "Nested release blocker rollup should retain table layout action items."

$finalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $finalReviewPath
Assert-ContainsText -Text $finalReview -ExpectedText "- Release blocker rollup: blocked" `
    -Message "Final review should include release blocker rollup step status."
Assert-ContainsText -Text $finalReview -ExpectedText "Release blocker rollup counts: 2 blockers, 2 actions, 1 warnings" `
    -Message "Final review should include release blocker rollup counts."
Assert-ContainsText -Text $finalReview -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
    -Message "Final review should surface rollup warning details."
Assert-ContainsText -Text $finalReview -ExpectedText "action=review_style_merge_suggestions" `
    -Message "Final review should surface rollup warning actions."
Assert-ContainsText -Text $finalReview -ExpectedText "source_schema=featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Final review should surface rollup warning source schema."

$gateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-fail-on-blocker"
$gateArguments = @($scriptArguments)
$summaryOutputIndex = [Array]::IndexOf($gateArguments, "-SummaryOutputDir")
$gateArguments[$summaryOutputIndex + 1] = $gateOutputDir
$gateArguments += "-ReleaseBlockerRollupFailOnBlocker"
$gateResult = @(& (Get-Process -Id $PID).Path @gateArguments 2>&1)
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
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.auto_discovered_input_json.Count) -Expected 5 `
    -Message "Release summary should record default governance reports plus restore audit summaries."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.source_report_count) -Expected 5 `
    -Message "Auto-discovered rollup should aggregate default governance plus restore audit reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.release_blocker_count) -Expected 5 `
    -Message "Auto-discovered rollup should surface blocker count from default governance plus restore audit reports."
Assert-Equal -Actual ([int]$autoDiscoverSummary.release_blocker_rollup.action_item_count) -Expected 5 `
    -Message "Auto-discovered rollup should surface action count from default governance plus restore audit reports."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "style_merge.restore_audit_issues" `
    -Message "Release candidate summary should preserve auto-discovered restore audit blocker details."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.release_blockers | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "review_style_merge_restore_audit" `
    -Message "Release candidate summary should preserve auto-discovered restore audit blocker actions."
Assert-ContainsText -Text (($autoDiscoverSummary.release_blocker_rollup.action_items | ForEach-Object {
            if ($_.PSObject.Properties["open_command"]) { [string]$_.open_command }
        }) -join "`n") `
    -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Release candidate summary should preserve auto-discovered restore audit open-latest commands."

$autoDiscoverRollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $autoDiscoverRollupSummaryPath | ConvertFrom-Json
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "project-template-delivery-readiness" `
    -Message "Auto-discovered rollup should include project-template delivery readiness governance."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "content-control-data-binding-governance" `
    -Message "Auto-discovered rollup should include content-control data-binding governance."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "style-merge.restore-audit.summary.json" `
    -Message "Auto-discovered rollup should include style merge restore audit summaries."
Assert-ContainsText -Text (($autoDiscoverRollupSummary.action_items | ForEach-Object {
            if ($_.PSObject.Properties["open_command"]) { [string]$_.open_command }
        }) -join "`n") `
    -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Auto-discovered rollup should preserve restore audit open-latest commands."

$autoDiscoverFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $autoDiscoverOutputDir "report\final_review.md")
Assert-ContainsText -Text $autoDiscoverFinalReview -ExpectedText "## Release Governance Action Items" `
    -Message "Final review should include release governance action items."
Assert-ContainsText -Text $autoDiscoverFinalReview -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Final review should surface restore audit open-latest commands."
foreach ($bundlePath in @(
        (Join-Path $autoDiscoverOutputDir "START_HERE.md"),
        (Join-Path $autoDiscoverOutputDir "report\ARTIFACT_GUIDE.md"),
        (Join-Path $autoDiscoverOutputDir "report\REVIEWER_CHECKLIST.md"),
        (Join-Path $autoDiscoverOutputDir "report\release_handoff.md")
    )) {
    $bundleText = Get-Content -Raw -Encoding UTF8 -LiteralPath $bundlePath
    Assert-ContainsText -Text $bundleText -ExpectedText "## Release Governance Action Items" `
        -Message "Release note bundle file should include release governance action items: $bundlePath"
    Assert-ContainsText -Text $bundleText -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
        -Message "Release note bundle file should surface restore audit open-latest commands: $bundlePath"
}

Write-Host "Release candidate blocker rollup regression passed."
