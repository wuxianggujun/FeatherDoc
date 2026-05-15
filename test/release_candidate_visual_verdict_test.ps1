param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath

foreach ($name in @(
        "SmokeReviewVerdict",
        "FixedGridReviewVerdict",
        "SectionPageSetupReviewVerdict",
        "PageNumberFieldsReviewVerdict",
        "CuratedVisualReviewVerdict"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('[string]${0} = "undecided"' -f $name) `
        -Message ("Release preflight should expose {0}." -f $name)
}

foreach ($name in @(
        "SmokeReviewNote",
        "FixedGridReviewNote",
        "SectionPageSetupReviewNote",
        "PageNumberFieldsReviewNote",
        "CuratedVisualReviewNote"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('[string]${0} = ""' -f $name) `
        -Message ("Release preflight should expose {0}." -f $name)
}

foreach ($argument in @(
        "-SmokeReviewVerdict",
        "-FixedGridReviewVerdict",
        "-SectionPageSetupReviewVerdict",
        "-PageNumberFieldsReviewVerdict",
        "-CuratedVisualReviewVerdict"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('-VerdictArgument "{0}"' -f $argument) `
        -Message ("Release preflight should pass {0} through to the visual gate." -f $argument)
}

foreach ($argument in @(
        "-SmokeReviewNote",
        "-FixedGridReviewNote",
        "-SectionPageSetupReviewNote",
        "-PageNumberFieldsReviewNote",
        "-CuratedVisualReviewNote"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('-NoteArgument "{0}"' -f $argument) `
        -Message ("Release preflight should pass {0} through to the visual gate." -f $argument)
}

foreach ($prefix in @(
        "smoke",
        "fixed_grid",
        "section_page_setup",
        "page_number_fields"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText ('Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "{0}"' -f $prefix) `
        -Message ("Release summary should capture {0} review verdict metadata." -f $prefix)
}

Assert-ContainsText -Text $scriptText -ExpectedText '$summary.steps.visual_gate.curated_visual_regressions = $curatedVisualReviewEntries' `
    -Message "Release summary should capture curated visual review verdict metadata."

Assert-ContainsText -Text $scriptText -ExpectedText '$summary.steps.visual_gate.review_task_summary = $reviewTaskSummary' `
    -Message "Release summary should capture visual gate review task counts."

Assert-ContainsText -Text $scriptText -ExpectedText '[switch]$IncludeTableStyleQuality' `
    -Message "Release preflight should expose an opt-in table style quality visual gate switch."

Assert-ContainsText -Text $scriptText -ExpectedText '$visualGateArgs += "-IncludeTableStyleQuality"' `
    -Message "Release preflight should pass the table style quality visual gate opt-in to the visual gate."

foreach ($name in @(
        "ReleaseBlockerRollupInputJson",
        "ReleaseBlockerRollupInputRoot",
        "ReleaseBlockerRollupOutputDir",
        "ReleaseBlockerRollupFailOnBlocker",
        "ReleaseBlockerRollupFailOnWarning"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText $name `
        -Message ("Release preflight should expose {0}." -f $name)
}

Assert-ContainsText -Text $scriptText -ExpectedText 'build_release_blocker_rollup_report.ps1' `
    -Message "Release preflight should invoke the final release blocker rollup writer."

Assert-ContainsText -Text $scriptText -ExpectedText 'release_blocker_rollup = [ordered]@{' `
    -Message "Release summary should expose release blocker rollup metadata."

Assert-ContainsText -Text $scriptText -ExpectedText '- Release blocker rollup: $($summary.steps.release_blocker_rollup.status)' `
    -Message "Release final review should include release blocker rollup status."

Assert-ContainsText -Text $scriptText -ExpectedText '"-TableStyleQualityBuildDir"' `
    -Message "Release preflight should pass the shared build directory to table style quality visual gate."

Assert-ContainsText -Text $scriptText -ExpectedText 'if ($Object -is [System.Collections.IDictionary] -and $Object.Contains($Name))' `
    -Message "Optional property lookup should support the release summary dictionary used while rendering final_review.md."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-VisualGateReviewTaskSummaryLine' `
    -Message "Release preflight final_review.md should render visual review task counts."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-CompleteVisualGateReviewTaskSummary' `
    -Message "Release preflight should validate review task counts before copying or rendering them."

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-CompleteVisualGateReviewTaskSummary -Summary (Get-OptionalPropertyValue -Object $gateSummary -Name "review_task_summary")' `
    -Message "Release preflight summary should only copy complete visual review task counts."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-VisualGateReviewSummaryMarkdown' `
    -Message "Release preflight final_review.md should render visual review verdict metadata."

Assert-ContainsText -Text $scriptText -ExpectedText 'Review task count: {0} total ({1} standard, {2} curated)' `
    -Message "Release preflight final_review.md should format visual review task counts."

Assert-ContainsText -Text $scriptText -ExpectedText '## Visual review verdicts' `
    -Message "Release preflight final_review.md should include a visual review verdicts section when seeded metadata exists."

Assert-ContainsText -Text $scriptText -ExpectedText '- {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}' `
    -Message "Release preflight final_review.md should format standard flow review provenance."

Assert-ContainsText -Text $scriptText -ExpectedText '- Curated - {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}' `
    -Message "Release preflight final_review.md should format curated visual review provenance."

Assert-ContainsText -Text $scriptText -ExpectedText '$visualGateReviewTaskSummaryLine' `
    -Message "Release preflight final_review.md should embed the visual review task counts."

Assert-ContainsText -Text $scriptText -ExpectedText '$visualGateReviewSummary' `
    -Message "Release preflight final_review.md should embed the visual review verdict summary."

Assert-ContainsText -Text $scriptText -ExpectedText 'Project template smoke schema approval gate blocked.' `
    -Message "Release preflight should fail when schema approval gate is blocked."

Assert-ContainsText -Text $scriptText -ExpectedText 'schema_patch_approval_gate_status' `
    -Message "Release preflight should carry schema approval gate status into summaries."

Assert-ContainsText -Text $scriptText -ExpectedText 'schema_patch_approval_invalid_result_count' `
    -Message "Release preflight should carry invalid schema approval-result counts into summaries."

Assert-ContainsText -Text $scriptText -ExpectedText 'write_project_template_schema_approval_history.ps1' `
    -Message "Release preflight should invoke the schema approval history writer."

Assert-ContainsText -Text $scriptText -ExpectedText 'schema_patch_approval_history_json' `
    -Message "Release preflight should expose schema approval history JSON in summaries."

Assert-ContainsText -Text $scriptText -ExpectedText 'Project template smoke schema approval history:' `
    -Message "Release final review should link schema approval history artifacts."

Assert-ContainsText -Text $scriptText -ExpectedText 'release_blockers' `
    -Message "Release summary should expose machine-readable release blockers."

Assert-ContainsText -Text $scriptText -ExpectedText 'Release blockers:' `
    -Message "Release final review should render the release blocker count."

Assert-ContainsText -Text $scriptText -ExpectedText 'project_template_smoke.schema_approval' `
    -Message "Release blockers should include a stable schema approval blocker id."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-NormalizedReleaseGovernanceWarnings' `
    -Message "Release preflight should normalize governance warning details before copying them into summary.json."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-ReleaseGovernanceWarningSummaryMarkdown' `
    -Message "Release final review should render governance warning details."

Assert-ContainsText -Text $scriptText -ExpectedText 'action=$action' `
    -Message "Release final review should render governance warning actions."

Assert-ContainsText -Text $scriptText -ExpectedText 'source_schema=$sourceSchema' `
    -Message "Release final review should render governance warning source schema."

Assert-ContainsText -Text $scriptText -ExpectedText '## Release governance warnings' `
    -Message "Release final review should add a governance warning details section when warnings exist."

Assert-ContainsText -Text $scriptText -ExpectedText 'Release blocker rollup warnings' `
    -Message "Release final review should label rollup warning details."

Assert-ContainsText -Text $scriptText -ExpectedText 'Release governance handoff nested rollup warnings' `
    -Message "Release final review should label nested handoff rollup warning details."

Assert-ContainsText -Text $scriptText -ExpectedText '$releaseGovernanceWarningSummary' `
    -Message "Release final review should embed governance warning details."

Assert-ContainsText -Text $scriptText -ExpectedText 'Invoke-ProjectTemplateSchemaApprovalHistory' `
    -Message "Release preflight should isolate schema approval history generation."

$parseTokens = $null
$parseErrors = $null
$scriptAst = [System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) {
    throw "Release preflight script has parse errors."
}

$functionNames = @(
    "Get-OptionalIntegerProperty",
    "Get-OptionalObjectArrayProperty",
    "Get-SchemaPatchApprovalGateStatus",
    "Get-SchemaPatchApprovalItemCount",
    "Get-ProjectTemplateSmokeSchemaApprovalGateInfo",
    "New-ProjectTemplateSchemaApprovalReleaseBlockerItem",
    "Get-ProjectTemplateSchemaApprovalBlockedItems",
    "Get-ProjectTemplateSchemaApprovalIssueKeys",
    "New-ProjectTemplateSchemaApprovalReleaseBlocker",
    "Set-ProjectTemplateSchemaApprovalReleaseBlocker",
    "Get-ProjectTemplateSchemaApprovalHistoryInputList",
    "Get-ReleaseBlockerRollupInputList",
    "Expand-ReleaseBlockerRollupPathList",
    "Get-RepoRelativePath",
    "Get-OptionalPropertyValue",
    "Convert-ReviewTimestamp",
    "Get-ReleaseCandidateDisplayValue",
    "Read-ReleaseBlockerRollupSummary",
    "Get-CompleteVisualGateReviewTaskSummary",
    "Get-VisualGateReviewTaskSummaryLine",
    "Get-VisualGateReviewSummaryMarkdown"
)
$functionAsts = $scriptAst.FindAll({
        param($Node)
        $Node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
            $functionNames -contains $Node.Name
    }, $true)
foreach ($functionName in $functionNames) {
    $functionAst = $functionAsts | Where-Object { $_.Name -eq $functionName } | Select-Object -First 1
    if ($null -eq $functionAst) {
        throw "Missing function used by final_review.md visual verdict rendering: $functionName"
    }

    Invoke-Expression $functionAst.Extent.Text
}

$completeReviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary ([ordered]@{
        total_count = 3
        standard_count = 2
        curated_count = 1
    })
if ($null -eq $completeReviewTaskSummary -or $completeReviewTaskSummary.total_count -ne 3) {
    throw "Complete review task summary was not preserved."
}

$incompleteReviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary ([ordered]@{
        total_count = 3
    })
if ($null -ne $incompleteReviewTaskSummary) {
    throw "Incomplete review task summary was unexpectedly preserved."
}

$reviewTaskSummaryLine = Get-VisualGateReviewTaskSummaryLine -VisualGateStep ([ordered]@{
        review_task_summary = [ordered]@{
            total_count = 3
            standard_count = 2
            curated_count = 1
        }
    })
Assert-ContainsText -Text $reviewTaskSummaryLine -ExpectedText "- Review task count: 3 total (2 standard, 1 curated)" `
    -Message "Rendered review task summary line should include total, standard, and curated counts."

$incompleteReviewTaskSummaryLine = Get-VisualGateReviewTaskSummaryLine -VisualGateStep ([ordered]@{
        review_task_summary = [ordered]@{
            total_count = 3
        }
    })
if ($incompleteReviewTaskSummaryLine -ne "") {
    throw "Incomplete review task summary unexpectedly rendered '$incompleteReviewTaskSummaryLine'."
}

$missingReviewTaskSummaryLine = Get-VisualGateReviewTaskSummaryLine -VisualGateStep ([ordered]@{})
if ($missingReviewTaskSummaryLine -ne "") {
    throw "Missing review task summary unexpectedly rendered '$missingReviewTaskSummaryLine'."
}

$reviewMarkdown = Get-VisualGateReviewSummaryMarkdown -RepoRoot $resolvedRepoRoot -VisualGateStep ([ordered]@{
        section_page_setup_verdict = "pass"
        section_page_setup_review_status = "reviewed"
        section_page_setup_reviewed_at = "2026-04-28T13:00:00"
        section_page_setup_review_method = "operator_supplied"
        section_page_setup_task_dir = Join-Path $resolvedRepoRoot "output\section-task"
        curated_visual_regressions = @(
            [ordered]@{
                label = "Bookmark block visibility"
                verdict = "pass"
                review_status = "reviewed"
                reviewed_at = "2026-04-28T13:05:00"
                review_method = "operator_supplied"
                task_dir = Join-Path $resolvedRepoRoot "output\curated-task"
            }
        )
    })
Assert-ContainsText -Text $reviewMarkdown -ExpectedText "## Visual review verdicts" `
    -Message "Rendered review markdown should include the visual review verdicts heading."
Assert-ContainsText -Text $reviewMarkdown -ExpectedText "- Section page setup: verdict=pass, review_status=reviewed, reviewed_at=2026-04-28T13:00:00, review_method=operator_supplied" `
    -Message "Rendered review markdown should include seeded standard flow review provenance."
Assert-ContainsText -Text $reviewMarkdown -ExpectedText "task=.\output\section-task" `
    -Message "Rendered review markdown should include repo-relative standard flow task paths."
Assert-ContainsText -Text $reviewMarkdown -ExpectedText "- Curated - Bookmark block visibility: verdict=pass, review_status=reviewed, reviewed_at=2026-04-28T13:05:00, review_method=operator_supplied" `
    -Message "Rendered review markdown should include seeded curated review provenance."
Assert-ContainsText -Text $reviewMarkdown -ExpectedText "task=.\output\curated-task" `
    -Message "Rendered review markdown should include repo-relative curated task paths."


$blockedApprovalGate = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary ([ordered]@{
        schema_patch_approval_pending_count = 1
        schema_patch_approval_approved_count = 0
        schema_patch_approval_rejected_count = 1
        schema_patch_approval_compliance_issue_count = 2
        schema_patch_approval_items = @(
            [ordered]@{ status = "invalid_result" }
        )
    })
if (-not [bool]$blockedApprovalGate.gate_blocked -or $blockedApprovalGate.gate_status -ne "blocked") {
    throw "Schema approval gate should block compliance issues and invalid results."
}
if ([int]$blockedApprovalGate.invalid_result_count -ne 1) {
    throw "Schema approval gate should infer invalid_result item count."
}

$pendingApprovalGate = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary ([ordered]@{
        schema_patch_approval_pending_count = 1
        schema_patch_approval_items = @(
            [ordered]@{ status = "pending_review" }
        )
    })
if ([bool]$pendingApprovalGate.gate_blocked -or $pendingApprovalGate.gate_status -ne "pending") {
    throw "Pending schema approval gate should be pending but not compliance-blocked."
}

$passedApprovalGate = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary ([ordered]@{
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 1
        schema_patch_approval_rejected_count = 0
    })
if ([bool]$passedApprovalGate.gate_blocked -or $passedApprovalGate.gate_status -ne "passed" -or [int]$passedApprovalGate.item_count -ne 1) {
    throw "Passed schema approval gate should use rollup counts even when item arrays are absent."
}

$blockedApprovalSummary = [ordered]@{
    schema_patch_approval_pending_count = 1
    schema_patch_approval_approved_count = 1
    schema_patch_approval_rejected_count = 1
    schema_patch_approval_compliance_issue_count = 2
    schema_patch_approval_invalid_result_count = 1
    schema_patch_approval_items = @(
        [ordered]@{
            name = "schema-review-invalid"
            status = "invalid_result"
            decision = "approved"
            action = "fix_schema_patch_approval_result"
            compliance_issue_count = 2
            compliance_issues = @("missing_reviewer", "missing_reviewed_at")
            approval_result = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
            schema_update_candidate = Join-Path $resolvedWorkingDir "generated.template-schema.json"
            review_json = Join-Path $resolvedWorkingDir "schema_patch_review.json"
        }
    )
}
$schemaApprovalBlocker = New-ProjectTemplateSchemaApprovalReleaseBlocker `
    -ProjectTemplateSmokeSummary $blockedApprovalSummary `
    -HistorySummary $null `
    -SummaryJsonPath (Join-Path $resolvedWorkingDir "summary.json") `
    -HistoryJsonPath (Join-Path $resolvedWorkingDir "history.json") `
    -HistoryMarkdownPath (Join-Path $resolvedWorkingDir "history.md")
if ($null -eq $schemaApprovalBlocker -or [string]$schemaApprovalBlocker.id -ne "project_template_smoke.schema_approval") {
    throw "Schema approval release blocker should expose a stable id."
}
if ([int]$schemaApprovalBlocker.blocked_item_count -ne 1 -or [int]$schemaApprovalBlocker.compliance_issue_count -ne 2) {
    throw "Schema approval release blocker should carry blocked item and compliance counts."
}
if (@($schemaApprovalBlocker.issue_keys) -notcontains "missing_reviewer") {
    throw "Schema approval release blocker should aggregate compliance issue keys."
}

$releaseSummaryForBlockers = [ordered]@{
    release_blockers = @(
        [pscustomobject]@{
            id = "visual.manual_review"
            source = "visual_gate"
            severity = "warning"
            status = "blocked"
            action = "complete_visual_manual_review"
        }
    )
    release_blocker_count = 1
}
Set-ProjectTemplateSchemaApprovalReleaseBlocker `
    -ReleaseSummary $releaseSummaryForBlockers `
    -Blocker $schemaApprovalBlocker
if ([int]$releaseSummaryForBlockers.release_blocker_count -ne 2) {
    throw "Release blocker writer should preserve non-schema blockers while appending schema blockers."
}
Set-ProjectTemplateSchemaApprovalReleaseBlocker `
    -ReleaseSummary $releaseSummaryForBlockers `
    -Blocker $null
if ([int]$releaseSummaryForBlockers.release_blocker_count -ne 1 -or
    [string]$releaseSummaryForBlockers.release_blockers[0].id -ne "visual.manual_review") {
    throw "Release blocker writer should remove stale schema blockers and preserve unrelated blockers."
}

$historySmokeSummaryPath = Join-Path $resolvedWorkingDir "history-smoke-summary.json"
$historyReleaseSummaryPath = Join-Path $resolvedWorkingDir "history-release-summary.json"
Set-Content -LiteralPath $historySmokeSummaryPath -Value '{}' -Encoding UTF8
Set-Content -LiteralPath $historyReleaseSummaryPath -Value '{}' -Encoding UTF8
$historyInputList = @(Get-ProjectTemplateSchemaApprovalHistoryInputList `
        -ReleaseSummaryPath $historyReleaseSummaryPath `
        -ProjectTemplateSmokeSummaryPath $historySmokeSummaryPath)
if ($historyInputList.Count -ne 2 -or
    $historyInputList -notcontains $historySmokeSummaryPath -or
    $historyInputList -notcontains $historyReleaseSummaryPath) {
    throw "Schema approval history input list should include existing smoke and release summaries."
}
$historyMissingInputList = @(Get-ProjectTemplateSchemaApprovalHistoryInputList `
        -ReleaseSummaryPath $historyReleaseSummaryPath `
        -ProjectTemplateSmokeSummaryPath (Join-Path $resolvedWorkingDir "missing-summary.json"))
if ($historyMissingInputList.Count -ne 1 -or $historyMissingInputList[0] -ne $historyReleaseSummaryPath) {
    throw "Schema approval history input list should skip missing smoke summaries."
}

$expandedRollupPaths = @(Expand-ReleaseBlockerRollupPathList -Paths @("a.json,b.json", " c.json "))
if ($expandedRollupPaths.Count -ne 3 -or
    $expandedRollupPaths[0] -ne "a.json" -or
    $expandedRollupPaths[1] -ne "b.json" -or
    $expandedRollupPaths[2] -ne "c.json") {
    throw "Release blocker rollup path expansion should support comma-delimited and repeated arguments."
}

Write-Host "Release candidate visual verdict passthrough regression passed."
