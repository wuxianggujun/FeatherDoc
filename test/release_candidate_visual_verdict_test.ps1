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

function Assert-LineContainsAll {
    param(
        [string]$Text,
        [string[]]$Fragments,
        [string]$Message
    )

    foreach ($line in ($Text -split "\r?\n")) {
        $lineMatches = $true
        foreach ($fragment in $Fragments) {
            if ($line -notmatch [regex]::Escape($fragment)) {
                $lineMatches = $false
                break
            }
        }

        if ($lineMatches) {
            return
        }
    }

    throw $Message
}

function Assert-MarkdownListRunContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Fragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $runStart = $lineIndex
        while ($runStart -gt 0 -and $lines[$runStart - 1] -match '^\s*-\s*') {
            $runStart--
        }

        $runEnd = $lineIndex
        while ($runEnd + 1 -lt $lines.Count -and $lines[$runEnd + 1] -match '^\s*-\s*') {
            $runEnd++
        }

        $run = ($lines[$runStart..$runEnd]) -join "`n"
        $runMatches = $true
        foreach ($fragment in $Fragments) {
            if ($run -notmatch [regex]::Escape($fragment)) {
                $runMatches = $false
                break
            }
        }

        if ($runMatches) {
            return
        }
    }

    throw $Message
}

function Assert-MarkdownSectionContainsAll {
    param(
        [string]$Text,
        [string]$Heading,
        [string[]]$Fragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = $lines[$lineIndex].Trim()
        if ($line -ne $Heading -or $line -notmatch '^(#+)\s+') {
            continue
        }

        $headingLevel = $Matches[1].Length
        $sectionEnd = $lines.Count - 1
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            if ($lines[$nextLineIndex] -match '^(#+)\s+' -and $Matches[1].Length -le $headingLevel) {
                $sectionEnd = $nextLineIndex - 1
                break
            }
        }

        $section = ($lines[$lineIndex..$sectionEnd]) -join "`n"
        $sectionMatches = $true
        foreach ($fragment in $Fragments) {
            if ($section -notmatch [regex]::Escape($fragment)) {
                $sectionMatches = $false
                break
            }
        }

        if ($sectionMatches) {
            return
        }
    }

    throw $Message
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath
$metadataHelpersPath = Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1"
$metadataHelpersText = Get-Content -Raw -LiteralPath $metadataHelpersPath
. $metadataHelpersPath
$templateSchemaCommonPath = Join-Path $resolvedRepoRoot "scripts\template_schema_cli_common.ps1"
. $templateSchemaCommonPath

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

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$PdfVisualGateSummaryJson = ""' `
    -Message "Release preflight should expose an optional PDF visual gate summary path."

Assert-ContainsText -Text $scriptText -ExpectedText 'pdf_visual_gate_summary_json = $resolvedPdfVisualGateSummaryJson' `
    -Message "Release summary should preserve the PDF visual gate summary path."

Assert-ContainsText -Text $scriptText -ExpectedText 'pdf_visual_gate = $pdfVisualGateSummaryInfo' `
    -Message "Release summary should expose PDF visual gate metadata."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate: $($summary.steps.pdf_visual_gate.status)' `
    -Message "Release final review should include PDF visual gate status."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate verdict: $($summary.steps.pdf_visual_gate.verdict)' `
    -Message "Release final review should include PDF visual gate verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate counts: $($summary.steps.pdf_visual_gate.visual_baseline_count) visual baselines, $($summary.steps.pdf_visual_gate.cjk_copy_search_count) CJK copy/search' `
    -Message "Release final review should include PDF visual gate evidence counts."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate manifest counts: $($summary.steps.pdf_visual_gate.visual_baseline_manifest_count) visual baseline manifest samples, $($summary.steps.pdf_visual_gate.cjk_manifest_count) CJK manifest samples' `
    -Message "Release final review should include PDF visual gate manifest counts."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate finalizable: $($summary.steps.pdf_visual_gate.finalizable)' `
    -Message "Release final review should include PDF visual gate finalizable status."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate summary: $pdfVisualGateSummaryDisplayPath' `
    -Message "Release final review should link PDF visual gate evidence."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual gate contact sheet: $pdfVisualGateContactSheetDisplayPath' `
    -Message "Release final review should link PDF visual gate contact sheet evidence."

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-PdfVisualGateSummaryInfo -SummaryJson $resolvedPdfVisualGateSummaryJson' `
    -Message "Release preflight should load machine-readable PDF visual gate verdict metadata."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$PdfVisualSegmentedGateSummaryJson = ""' `
    -Message "Release preflight should expose an optional segmented PDF visual gate summary path."

Assert-ContainsText -Text $scriptText -ExpectedText 'pdf_visual_segmented_gate_summary_json = $resolvedPdfVisualSegmentedGateSummaryJson' `
    -Message "Release summary should preserve the segmented PDF visual gate summary path."

Assert-ContainsText -Text $scriptText -ExpectedText 'pdf_visual_segmented_gate = $pdfVisualSegmentedGateSummaryInfo' `
    -Message "Release summary should expose segmented PDF visual gate metadata."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual segmented gate: $($summary.steps.pdf_visual_segmented_gate.status)' `
    -Message "Release final review should include segmented PDF visual gate status."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual segmented gate verdict: $($summary.steps.pdf_visual_segmented_gate.verdict)' `
    -Message "Release final review should include segmented PDF visual gate verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual segmented gate full status: $($summary.steps.pdf_visual_segmented_gate.full_visual_gate_status)' `
    -Message "Release final review should keep segmented evidence separate from full visual gate status."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual segmented gate summary: $pdfVisualSegmentedGateSummaryDisplayPath' `
    -Message "Release final review should link segmented PDF visual gate evidence."

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-PdfVisualSegmentedGateSummaryInfo -SummaryJson $resolvedPdfVisualSegmentedGateSummaryJson' `
    -Message "Release preflight should load machine-readable segmented PDF visual gate metadata."

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

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $summary' `
    -Message "Release preflight final_review.md should derive project-template checklist handoff evidence from release governance handoff."

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $summary' `
    -Message "Release preflight final_review.md should derive packaged project-template checklist audit evidence from release governance handoff."

Assert-ContainsText -Text $scriptText -ExpectedText '## Project-template release entry evidence' `
    -Message "Release preflight final_review.md should expose project-template release entry evidence as a reviewer-facing section."

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

Assert-ContainsText -Text $scriptText -ExpectedText 'release_blocker_metadata_helpers.ps1' `
    -Message "Release preflight should load shared release governance metadata helpers."

Assert-ContainsText -Text $metadataHelpersText -ExpectedText 'function Get-NormalizedReleaseGovernanceWarnings' `
    -Message "Shared metadata helpers should normalize governance warning details before copying them into summary.json."

Assert-ContainsText -Text $metadataHelpersText -ExpectedText 'function Add-ReleaseGovernanceWarningsMarkdownSection' `
    -Message "Shared metadata helpers should render governance warning details."

Assert-ContainsText -Text $metadataHelpersText -ExpectedText '## Release Governance Warnings' `
    -Message "Shared metadata helpers should add a governance warning details section when warnings exist."

Assert-ContainsText -Text $metadataHelpersText -ExpectedText '$($sectionInfo.Context) warnings' `
    -Message "Shared metadata helpers should label rollup, handoff, and nested warning details."

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
    "Get-RepoRelativePath",
    "Get-OptionalPropertyValue",
    "Get-PdfVisualGateSummaryInfo",
    "Get-PdfVisualSegmentedGateSummaryInfo",
    "Get-PdfBoundedCtestSummaryInfo",
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

$pdfSummaryDir = Join-Path $resolvedWorkingDir "pdf-summary"
New-Item -ItemType Directory -Path $pdfSummaryDir -Force | Out-Null
$pdfContactSheetPath = Join-Path $pdfSummaryDir "aggregate-contact-sheet.png"
Set-Content -LiteralPath $pdfContactSheetPath -Encoding UTF8 -Value "not-empty"
$pdfSummaryPath = Join-Path $pdfSummaryDir "summary.json"
([ordered]@{
        verdict = "pass"
        aggregate_contact_sheet = $pdfContactSheetPath
        cjk_manifest_count = 43
        cjk_copy_search_count = 43
        visual_baseline_manifest_count = 42
        baselines_count = 44
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $pdfSummaryPath -Encoding UTF8

$pdfVisualGateInfo = Get-PdfVisualGateSummaryInfo -SummaryJson $pdfSummaryPath
if ($pdfVisualGateInfo.status -ne "loaded" -or
    $pdfVisualGateInfo.verdict -ne "pass" -or
    [int]$pdfVisualGateInfo.cjk_manifest_count -ne 43 -or
    [int]$pdfVisualGateInfo.cjk_copy_search_count -ne 43 -or
    [int]$pdfVisualGateInfo.visual_baseline_manifest_count -ne 42 -or
    [int]$pdfVisualGateInfo.visual_baseline_count -ne 44 -or
    -not [bool]$pdfVisualGateInfo.finalizable) {
    throw "PDF visual gate summary metadata was not loaded into a finalizable release summary object."
}

$missingPdfVisualGateInfo = Get-PdfVisualGateSummaryInfo -SummaryJson (Join-Path $resolvedWorkingDir "missing-summary.json")
if ($missingPdfVisualGateInfo.status -ne "missing" -or [bool]$missingPdfVisualGateInfo.finalizable) {
    throw "Missing PDF visual gate summary should not be marked finalizable."
}

$pdfSegmentedSummaryPath = Join-Path $pdfSummaryDir "segmented-summary.json"
([ordered]@{
        schema = "featherdoc.pdf_visual_segmented_gate_summary.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "segmented_visual_gate_auxiliary_only"
        boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
        slice_summary_count = 4
        slice_pass_count = 4
        slice_failed_count = 0
        covered_baseline_count = 44
        expected_visual_render_count = 44
        visual_baseline_manifest_count = 42
        attempt_status = "partial"
        attempt_verdict = "not_complete"
        attempt_full_visual_gate_status = "not_complete"
        attempt_stage_count = 6
        attempt_passed_stage_count = 6
        attempt_incomplete_stage_count = 0
        visual_baseline_render_status = "pass"
        visual_baseline_fresh_rendered_count = 44
        aggregate_contact_sheet_status = "pass"
        aggregate_contact_sheet = $pdfContactSheetPath
        aggregate_contact_sheet_bytes = 1822428
        aggregate_rebuild_status = "pass"
        aggregate_rebuild_verdict = "pass"
        aggregate_rebuild_selected_baseline_count = 44
        aggregate_rebuild_expected_visual_render_count = 44
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pdfSegmentedSummaryPath -Encoding UTF8

$pdfSegmentedGateInfo = Get-PdfVisualSegmentedGateSummaryInfo -SummaryJson $pdfSegmentedSummaryPath
if ($pdfSegmentedGateInfo.status -ne "pass" -or
    $pdfSegmentedGateInfo.verdict -ne "pass" -or
    $pdfSegmentedGateInfo.full_visual_gate_status -ne "not_complete" -or
    $pdfSegmentedGateInfo.evidence_scope -ne "segmented_visual_gate_auxiliary_only" -or
    [int]$pdfSegmentedGateInfo.slice_pass_count -ne 4 -or
    [int]$pdfSegmentedGateInfo.covered_baseline_count -ne 44 -or
    [int]$pdfSegmentedGateInfo.expected_visual_render_count -ne 44 -or
    [int]$pdfSegmentedGateInfo.attempt_passed_stage_count -ne 6 -or
    [int64]$pdfSegmentedGateInfo.aggregate_contact_sheet_bytes -ne 1822428) {
    throw "Segmented PDF visual gate summary metadata was not loaded as auxiliary release evidence."
}

$boundedCtestDir = Join-Path $resolvedWorkingDir "pdf-bounded-ctest"
New-Item -ItemType Directory -Path $boundedCtestDir -Force | Out-Null
$boundedSmokePath = Join-Path $boundedCtestDir "smoke-summary.json"
$boundedBusinessPath = Join-Path $boundedCtestDir "business-summary.json"
foreach ($bounded in @(
        [ordered]@{ path = $boundedSmokePath; subset = "smoke-import" },
        [ordered]@{ path = $boundedBusinessPath; subset = "regression-business-samples" }
    )) {
    ([ordered]@{
            status = "pass"
            verdict = "pass"
            subset = [string]$bounded.subset
            selected_test_count = 10
            skipped_test_count = 0
            ctest_timeout_seconds = 60
            exit_code = 0
        } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath ([string]$bounded.path) -Encoding UTF8
}
$pdfBoundedCtestInfo = Get-PdfBoundedCtestSummaryInfo `
    -SummaryJson @($boundedSmokePath, $boundedBusinessPath) `
    -RepoRoot $resolvedRepoRoot
if ($pdfBoundedCtestInfo.status -ne "pass" -or
    [int]$pdfBoundedCtestInfo.summary_count -ne 2 -or
    [int]$pdfBoundedCtestInfo.pass_count -ne 2 -or
    [int]$pdfBoundedCtestInfo.selected_test_count -ne 20 -or
    [int]$pdfBoundedCtestInfo.skipped_test_count -ne 0 -or
    @($pdfBoundedCtestInfo.subsets) -notcontains "regression-business-samples") {
    throw "PDF bounded CTest summaries were not aggregated as auxiliary release evidence."
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

$expandedRollupPaths = @(Expand-TemplateSchemaArgumentList -Values @("a.json,b.json", " c.json "))
if ($expandedRollupPaths.Count -ne 3 -or
    $expandedRollupPaths[0] -ne "a.json" -or
    $expandedRollupPaths[1] -ne "b.json" -or
    $expandedRollupPaths[2] -ne "c.json") {
    throw "Release blocker rollup path expansion should support comma-delimited and repeated arguments."
}

$candidateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-pdf-visual-gate-consumption"
$candidateBuildDir = Join-Path $resolvedWorkingDir "build"
$candidateInstallDir = Join-Path $resolvedWorkingDir "install"
$candidateConsumerBuildDir = Join-Path $resolvedWorkingDir "consumer-build"
$candidateGateOutputDir = Join-Path $resolvedWorkingDir "word-visual-gate"
$candidateTaskOutputRoot = Join-Path $resolvedWorkingDir "visual-tasks"
$releaseGovernanceHandoffInputRoot = Join-Path $resolvedWorkingDir "release-governance-handoff-input"
$releaseGovernanceHandoffOutputDir = Join-Path $resolvedWorkingDir "release-governance-handoff"
$releaseGovernanceSourceDir = Join-Path $resolvedWorkingDir "release-candidate-governance-source"
$releaseGovernanceSourcePath = Join-Path $releaseGovernanceSourceDir "summary.json"
New-Item -ItemType Directory -Path $releaseGovernanceHandoffInputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $releaseGovernanceSourceDir -Force | Out-Null
([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "ready"
        release_ready = $true
        project_template_smoke = [ordered]@{
            status = "ready"
        }
        project_template_readiness_checklist_entrypoints = [ordered]@{
            status = "declared"
            checklist_label = "Project template release readiness checklist"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            required_entrypoint_count = 3
            entrypoints = @(
                [ordered]@{
                    id = "start_here"
                    required = $true
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                },
                [ordered]@{
                    id = "artifact_guide"
                    required = $true
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    required = $true
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                }
            )
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        }
        release_entry_project_template_readiness_checklist_material_safety_audit = [ordered]@{
            status = "passed"
            audit_script = ".\scripts\assert_release_material_safety.ps1"
            audited_entrypoint_count = 3
            audited_entrypoints = @(
                "start_here",
                "artifact_guide",
                "reviewer_checklist"
            )
            compact_evidence_label = "Project-template readiness checklist handoff evidence"
            compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
            compact_evidence_source_schema = "featherdoc.release_candidate_summary"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
        }
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
    } | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $releaseGovernanceSourcePath -Encoding UTF8

& $scriptPath `
    -SkipConfigure `
    -SkipBuild `
    -SkipTests `
    -SkipInstallSmoke `
    -SkipVisualGate `
    -SkipReviewTasks `
    -BuildDir $candidateBuildDir `
    -InstallDir $candidateInstallDir `
    -ConsumerBuildDir $candidateConsumerBuildDir `
    -GateOutputDir $candidateGateOutputDir `
    -TaskOutputRoot $candidateTaskOutputRoot `
    -SummaryOutputDir $candidateOutputDir `
    -ReleaseGovernanceHandoff `
    -ReleaseGovernanceHandoffInputRoot $releaseGovernanceHandoffInputRoot `
    -ReleaseGovernanceHandoffInputJson $releaseGovernanceSourcePath `
    -ReleaseGovernanceHandoffOutputDir $releaseGovernanceHandoffOutputDir `
    -ReleaseGovernanceHandoffIncludeRollup `
    -PdfVisualGateSummaryJson $pdfSummaryPath `
    -PdfVisualSegmentedGateSummaryJson $pdfSegmentedSummaryPath `
    -PdfBoundedCtestSummaryJson @($boundedSmokePath, $boundedBusinessPath)
if ($LASTEXITCODE -ne 0) {
    throw "Release candidate dry run with PDF visual gate summary failed with exit code $LASTEXITCODE."
}

$candidateSummaryPath = Join-Path $candidateOutputDir "report\summary.json"
if (-not (Test-Path -LiteralPath $candidateSummaryPath)) {
    throw "Release candidate dry run did not write summary.json."
}

$candidateSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateSummaryPath | ConvertFrom-Json
if ($candidateSummary.execution_status -ne "pass") {
    throw "Release candidate dry run should pass when all heavy flows are skipped."
}
if ($candidateSummary.steps.pdf_visual_gate.status -ne "loaded" -or
    $candidateSummary.steps.pdf_visual_gate.verdict -ne "pass" -or
    -not [bool]$candidateSummary.steps.pdf_visual_gate.finalizable) {
    throw "Release candidate summary did not consume the PDF visual gate pass verdict as finalizable evidence."
}
if ([int]$candidateSummary.steps.pdf_visual_gate.cjk_manifest_count -ne 43 -or
    [int]$candidateSummary.steps.pdf_visual_gate.cjk_copy_search_count -ne 43 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_manifest_count -ne 42 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_count -ne 44) {
    throw "Release candidate summary did not preserve PDF visual gate sample counts."
}
if ([string]$candidateSummary.steps.pdf_visual_gate.summary_json -ne $pdfSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_gate.aggregate_contact_sheet -ne $pdfContactSheetPath) {
    throw "Release candidate summary did not preserve PDF visual gate evidence paths."
}
if ($candidateSummary.steps.pdf_visual_segmented_gate.status -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.verdict -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.full_visual_gate_status -ne "not_complete" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.evidence_scope -ne "segmented_visual_gate_auxiliary_only") {
    throw "Release candidate summary did not preserve segmented PDF visual gate auxiliary status."
}
if ([int]$candidateSummary.steps.pdf_visual_segmented_gate.slice_pass_count -ne 4 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.covered_baseline_count -ne 44 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.expected_visual_render_count -ne 44 -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.summary_json -ne $pdfSegmentedSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet -ne $pdfContactSheetPath) {
    throw "Release candidate summary did not preserve segmented PDF visual gate counts or paths."
}
if ([int]$candidateSummary.steps.pdf_bounded_ctest.summary_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.pass_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.selected_test_count -ne 20 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.skipped_test_count -ne 0 -or
    @($candidateSummary.steps.pdf_bounded_ctest.subsets) -notcontains "regression-business-samples") {
    throw "Release candidate summary did not preserve PDF bounded CTest auxiliary evidence."
}
if ([int]$candidateSummary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count -ne 1 -or
    [int]$candidateSummary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count -ne 1) {
    throw "Release candidate summary did not consume project-template release entry evidence from release governance handoff."
}
$manifestSignoff = $candidateSummary.manifest_signoff_entrypoints
$expectedReleaseAssetsManifest = "output\release-assets\v$($candidateSummary.release_version)\release_assets_manifest.json"
if ($manifestSignoff.status -ne "declared" -or
    [int]$manifestSignoff.required_entrypoint_count -ne 3 -or
    [string]$manifestSignoff.release_assets_manifest -notmatch [regex]::Escape($expectedReleaseAssetsManifest)) {
    throw "Release candidate summary did not declare the packaged manifest signoff entrypoints."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    $entrypoint = @($manifestSignoff.entrypoints) |
        Where-Object { [string]$_.id -eq $entrypointId } |
        Select-Object -First 1
    if ($null -eq $entrypoint -or -not [bool]$entrypoint.required -or [string]::IsNullOrWhiteSpace([string]$entrypoint.path_display)) {
        throw "Release candidate summary did not declare required manifest signoff entrypoint '$entrypointId'."
    }
}
foreach ($contractName in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
    if (@($manifestSignoff.required_contracts) -notcontains $contractName) {
        throw "Release candidate summary did not declare required manifest contract '$contractName'."
    }
}
foreach ($fieldName in @(
        "status",
        "release_ready",
        "schema_approval_status_summary",
        "source_report_display",
        "source_json_display"
    )) {
    if (@($manifestSignoff.required_fields) -notcontains $fieldName) {
        throw "Release candidate summary did not declare required manifest field '$fieldName'."
    }
}
if ([string]$manifestSignoff.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
    throw "Release candidate summary did not preserve the reviewer manifest checklist marker."
}

$projectTemplateChecklistEntrypoints = $candidateSummary.project_template_readiness_checklist_entrypoints
if ($projectTemplateChecklistEntrypoints.status -ne "declared" -or
    [int]$projectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3 -or
    [string]$projectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
    throw "Release candidate summary did not declare the project-template readiness checklist entrypoints."
}
if ([string]$projectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist" -or
    [string]$projectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
    throw "Release candidate summary did not preserve the project-template readiness checklist identity."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    $entrypoint = @($projectTemplateChecklistEntrypoints.entrypoints) |
        Where-Object { [string]$_.id -eq $entrypointId } |
        Select-Object -First 1
    if ($null -eq $entrypoint -or -not [bool]$entrypoint.required -or [string]::IsNullOrWhiteSpace([string]$entrypoint.path_display)) {
        throw "Release candidate summary did not declare required project-template checklist entrypoint '$entrypointId'."
    }
}

$candidateFinalReviewPath = Join-Path $candidateOutputDir "report\final_review.md"
$candidateReleaseBodyPath = Join-Path $candidateOutputDir "report\release_body.zh-CN.md"
$candidateReleaseSummaryPath = Join-Path $candidateOutputDir "report\release_summary.zh-CN.md"
$candidateReleaseHandoffPath = Join-Path $candidateOutputDir "report\release_handoff.md"
$candidateArtifactGuidePath = Join-Path $candidateOutputDir "report\ARTIFACT_GUIDE.md"
$candidateReviewerChecklistPath = Join-Path $candidateOutputDir "report\REVIEWER_CHECKLIST.md"
$candidateStartHerePath = Join-Path $candidateOutputDir "START_HERE.md"

foreach ($assertion in @(
        @{ Path = $candidateFinalReviewPath; Label = "final_review.md" },
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-ContainsText -Text $content -ExpectedText "PDF visual gate verdict: pass" `
        -Message ("{0} should expose the PDF visual gate verdict." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "44" `
        -Message ("{0} should expose the PDF visual baseline count." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "43" `
        -Message ("{0} should expose the PDF CJK count." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "aggregate-contact-sheet.png" `
        -Message ("{0} should expose the PDF visual contact sheet." -f $assertion.Label)
}

$finalReviewContent = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateFinalReviewPath
Assert-MarkdownListRunContainsAll -Text $finalReviewContent -Anchor "PDF visual segmented gate:" -Fragments @(
    "PDF visual segmented gate: pass",
    "PDF visual segmented gate verdict: pass",
    "PDF visual segmented gate full status: not_complete",
    "PDF visual segmented gate scope: segmented_visual_gate_auxiliary_only",
    "PDF visual segmented gate slices: 4/4 pass",
    "PDF visual segmented gate coverage: 44/44 baselines"
) -Message "final_review.md should keep segmented PDF visual gate auxiliary evidence in one step-status list run."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "PDF visual segmented gate summary:" `
    -Message "final_review.md should link segmented PDF visual gate summary evidence."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "segmented-summary.json" `
    -Message "final_review.md should expose the segmented PDF visual gate summary path."

foreach ($assertion in @(
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "PDF visual gate summary:" -Fragments @(
        "PDF visual gate summary:",
        "summary.json",
        "PDF visual gate evidence status: loaded",
        "PDF visual gate verdict: pass",
        "PDF visual gate aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples: 43",
        "PDF CJK copy/search samples: 43",
        "PDF CJK missing text count: 0",
        "PDF visual baseline manifest samples: 42",
        "PDF visual baselines: 44"
    ) -Message ("{0} should keep PDF visual status, verdict, paths, and counts in one Markdown list run." -f $assertion.Label)
}

$candidateFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateFinalReviewPath
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Key outputs" -Fragments @(
    "PDF visual gate summary:",
    "summary.json",
    "PDF visual gate contact sheet:",
    "aggregate-contact-sheet.png",
    "PDF bounded CTest summaries:",
    "smoke-summary.json",
    "business-summary.json"
) -Message "final_review.md should keep PDF visual summary and aggregate contact sheet inside the Key outputs section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Step status" -Fragments @(
    "PDF bounded CTest summaries: 2 summaries, 2 pass",
    "PDF bounded CTest subsets: smoke-import, regression-business-samples",
    "PDF bounded CTest selected tests: 20",
    "PDF bounded CTest skipped tests: 0"
) -Message "final_review.md should expose bounded PDF CTest auxiliary evidence in the Step status section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Project-template release entry evidence" -Fragments @(
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=1",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "Project-template readiness checklist packaged audit evidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1",
    "assert_release_material_safety.ps1",
    "audited_entrypoints=start_here, artifact_guide, reviewer_checklist",
    "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
) -Message "final_review.md should expose project-template checklist handoff and packaged audit evidence consumed from release governance handoff."

$candidateReleaseBody = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseBodyPath
foreach ($fragments in @(
        @("PDF visual gate verdict", "pass"),
        @("PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"),
        @("PDF CJK copy/search samples", "43"),
        @("PDF visual baselines", "44")
    )) {
    foreach ($fragment in $fragments) {
        Assert-ContainsText -Text $candidateReleaseBody -ExpectedText $fragment `
            -Message ("release_body.zh-CN.md should expose PDF visual gate fragment '{0}'." -f $fragment)
    }
}
Assert-MarkdownListRunContainsAll -Text $candidateReleaseBody -Anchor "PDF visual gate summary" -Fragments @(
    "PDF visual gate summary",
    "summary.json",
    "PDF visual gate evidence status",
    "loaded",
    "PDF visual gate verdict",
    "pass",
    "PDF visual gate aggregate contact sheet",
    "aggregate-contact-sheet.png",
    "PDF CJK manifest samples",
    "43",
    "PDF CJK copy/search samples",
    "43",
    "PDF CJK missing text count",
    "0",
    "PDF visual baseline manifest samples",
    "42",
    "PDF visual baselines",
    "44"
) -Message "release_body.zh-CN.md should keep PDF visual status, verdict, paths, and counts in one Markdown list run."

$candidateReleaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseSummaryPath
foreach ($fragment in @(
        "PDF visual gate",
        "verdict=pass",
        "aggregate-contact-sheet.png",
        "cjk_copy_search_count=43",
        "visual_baseline_count=44"
    )) {
    Assert-ContainsText -Text $candidateReleaseSummary -ExpectedText $fragment `
        -Message ("release_summary.zh-CN.md should expose PDF visual gate fragment '{0}'." -f $fragment)
}
Assert-LineContainsAll -Text $candidateReleaseSummary -Fragments @(
    "PDF visual gate",
    "verdict=pass",
    "summary=",
    "summary.json",
    "aggregate_contact_sheet=",
    "aggregate-contact-sheet.png",
    "cjk_manifest_count=43",
    "cjk_copy_search_count=43",
    "visual_baseline_manifest_count=42",
    "visual_baseline_count=44"
) -Message "release_summary.zh-CN.md should keep the PDF visual verdict, paths, and counts on one summary line."

$candidateReviewerChecklist = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReviewerChecklistPath
Assert-LineContainsAll -Text $candidateReviewerChecklist -Fragments @(
    'Confirm the PDF visual gate finalize evidence is signed off',
    'verdict `pass`',
    'summary',
    'summary.json',
    'aggregate contact sheet',
    'aggregate-contact-sheet.png',
    'CJK manifest samples `43`',
    'CJK copy/search samples `43`',
    'missing text `0`',
    'visual baseline manifest samples `42`',
    'visual baselines `44`'
) -Message "REVIEWER_CHECKLIST.md should keep PDF visual finalize verdict, paths, and counts on one reviewer signoff line."

Write-Host "Release candidate visual verdict passthrough regression passed."
