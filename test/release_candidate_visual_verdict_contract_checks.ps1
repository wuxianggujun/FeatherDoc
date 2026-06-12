if ($Scenario -ne "candidate") {
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
        "ReleaseBlockerRollupAutoDiscover",
        "ReleaseBlockerRollupAutoDiscoverRoot",
        "ReleaseBlockerRollupOutputDir",
        "ReleaseBlockerRollupFailOnBlocker",
        "ReleaseBlockerRollupFailOnWarning",
        "ReleaseGovernanceHandoff",
        "ReleaseGovernanceHandoffInputRoot",
        "ReleaseGovernanceHandoffInputJson",
        "ReleaseGovernanceHandoffOutputDir",
        "ReleaseGovernanceHandoffIncludeRollup",
        "ReleaseGovernanceHandoffFailOnMissing",
        "ReleaseGovernanceHandoffFailOnBlocker",
        "ReleaseGovernanceHandoffFailOnWarning",
        "ReleaseGovernanceHandoffExpectedReportProfile",
        "ReleaseEvidenceScope",
        "SkipMaterialSafetyAudit"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText $name `
        -Message ("Release preflight should expose {0}." -f $name)
}

foreach ($marker in @(
    '[ValidateSet("full", "explicit-only")]',
    '[ValidateSet("full", "pdf-only")]',
    '$releaseManifestSignoffEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only")',
    'release_evidence_scope = $ReleaseEvidenceScope',
    'manifest_signoff_entrypoints = $releaseManifestSignoffEntrypoints',
    '-IncludeRollup ([bool]$ReleaseGovernanceHandoffIncludeRollup)',
    '-FailOnWarning ([bool]$ReleaseGovernanceHandoffFailOnWarning)',
    '-ExpectedReportProfile $ReleaseGovernanceHandoffExpectedReportProfile'
)) {
    Assert-ContainsText -Text $scriptText -ExpectedText $marker `
        -Message "Release preflight should keep PDF-only release governance scope marker '$marker'."
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

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-PdfVisualGateAttemptReleaseWarnings' `
    -Message "Release preflight should derive machine-readable warnings from incomplete fresh PDF visual gate attempts."

Assert-ContainsText -Text $scriptText -ExpectedText 'id = "pdf_visual_gate_attempt.incomplete_fresh_render"' `
    -Message "Release warnings should include a stable id for incomplete fresh PDF visual gate attempts."

Assert-ContainsText -Text $scriptText -ExpectedText 'release_owner_acceptance_boundary' `
    -Message "Release warnings should keep the release-owner acceptance boundary explicit."

Assert-ContainsText -Text $scriptText -ExpectedText 'visual_baseline_resume_slice_command_template' `
    -Message "Release warnings should carry the PDF visual baseline resume command template."

Assert-ContainsText -Text $scriptText -ExpectedText 'warning_count = 0' `
    -Message "Release summary should expose a top-level warning count."

Assert-ContainsText -Text $scriptText -ExpectedText 'warnings = @()' `
    -Message "Release summary should expose top-level machine-readable warnings."

Assert-ContainsText -Text $scriptText -ExpectedText 'Set-ReleaseSummaryWarnings -ReleaseSummary $summary -Warnings $releaseWarnings' `
    -Message "Release summary should materialize PDF attempt warnings before governance rollups consume the summary."

Assert-ContainsText -Text $scriptText -ExpectedText '- Warnings: $($summary.warning_count)' `
    -Message "Release final review should render the top-level warning count."

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

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual release evidence accepted: $($summary.steps.pdf_full_ctest_readiness.visual_gate_release_evidence_accepted)' `
    -Message "Release final review should expose PDF visual release-evidence acceptance from readiness."

Assert-ContainsText -Text $scriptText -ExpectedText '- PDF visual segmented gate summary: $pdfVisualSegmentedGateSummaryDisplayPath' `
    -Message "Release final review should link segmented PDF visual gate evidence."

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-PdfVisualSegmentedGateSummaryInfo -SummaryJson $resolvedPdfVisualSegmentedGateSummaryJson' `
    -Message "Release preflight should load machine-readable segmented PDF visual gate metadata."

Assert-ContainsText -Text $scriptText -ExpectedText 'outer_guard_status = ""' `
    -Message "PDF visual gate attempt summary metadata should expose outer_guard_status."

Assert-ContainsText -Text $scriptText -ExpectedText 'outer_guard_timed_out = $false' `
    -Message "PDF visual gate attempt summary metadata should expose outer_guard_timed_out."

Assert-ContainsText -Text $scriptText -ExpectedText 'outer_guard_timeout_seconds = 0' `
    -Message "PDF visual gate attempt summary metadata should expose outer_guard_timeout_seconds."

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

Assert-ContainsText -Text $scriptText -ExpectedText 'Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine -Summary $summary' `
    -Message "Release preflight final_review.md should derive compact Word visual metadata evidence from release governance handoff."

Assert-ContainsText -Text $scriptText -ExpectedText 'if (-not $SkipMaterialSafetyAudit.IsPresent)' `
    -Message "Release preflight should keep final material safety audit enabled by default."

Assert-ContainsText -Text $scriptText -ExpectedText 'Skipping final release material safety audit' `
    -Message "Release preflight should make partial-fixture material safety audit skips explicit."

Assert-ContainsText -Text $scriptText -ExpectedText '## Project-template release entry evidence' `
    -Message "Release preflight final_review.md should expose project-template release entry evidence as a reviewer-facing section."

Assert-ContainsText -Text $scriptText -ExpectedText '## Word visual standard review metadata evidence' `
    -Message "Release preflight final_review.md should expose compact Word visual metadata evidence as a reviewer-facing section."

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
}

$scriptAsts = foreach ($releaseCandidateScriptPath in $releaseCandidateScriptPaths) {
    $parseTokens = $null
    $parseErrors = $null
    $scriptAst = [System.Management.Automation.Language.Parser]::ParseFile($releaseCandidateScriptPath, [ref]$parseTokens, [ref]$parseErrors)
    if ($parseErrors.Count -gt 0) {
        throw "Release preflight script has parse errors: $releaseCandidateScriptPath"
    }

    $scriptAst
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
    "Get-PdfVisualGateAttemptSummaryInfo",
    "Get-PdfVisualGateAttemptReleaseWarnings",
    "Get-PdfVisualSegmentedGateSummaryInfo",
    "Get-PdfBoundedCtestSummaryInfo",
    "Get-PdfFullCtestReadinessSummaryInfo",
    "Convert-ReviewTimestamp",
    "Get-ReleaseCandidateDisplayValue",
    "Read-ReleaseBlockerRollupSummary",
    "Get-CompleteVisualGateReviewTaskSummary",
    "Get-VisualGateReviewTaskSummaryLine",
    "Get-VisualGateReviewSummaryMarkdown",
    "Convert-ReleaseMaterialString",
    "Convert-ReleaseMaterialObject"
)
$functionAsts = foreach ($scriptAst in $scriptAsts) {
    $scriptAst.FindAll({
            param($Node)
            $Node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
                $functionNames -contains $Node.Name
        }, $true)
}
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
$pdfComputedSummaryPath = Join-Path $pdfSummaryDir "computed-summary.json"
([ordered]@{
        verdict = "pass"
        aggregate_contact_sheet = $pdfContactSheetPath
        cjk_manifest_count = 43
        cjk_copy_search_count = 43
        cjk_copy_search = @(
            [ordered]@{ missing_text = @() },
            [ordered]@{ missing_text = @("missing sample text") }
        )
        visual_baseline_manifest_count = 42
        baselines_count = 44
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $pdfComputedSummaryPath -Encoding UTF8

$repoRootCommandText = 'pwsh -ExecutionPolicy Bypass -File "{0}" -SummaryOutputDir "{1}"' -f `
    (Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"),
    (Join-Path $resolvedRepoRoot "output\release-candidate-checks")
$sanitizedCommandText = Convert-ReleaseMaterialString -RepoRoot $resolvedRepoRoot -Text $repoRootCommandText
Assert-DoesNotContainText -Text $sanitizedCommandText -UnexpectedText $resolvedRepoRoot `
    -Message "Release material string sanitizer should remove repo-root absolute paths from embedded commands."
Assert-ContainsText -Text $sanitizedCommandText -ExpectedText ".\scripts\run_release_candidate_checks.ps1" `
    -Message "Release material string sanitizer should keep embedded script paths repo-relative."
Assert-ContainsText -Text $sanitizedCommandText -ExpectedText ".\output\release-candidate-checks" `
    -Message "Release material string sanitizer should keep embedded output paths repo-relative."

$sanitizedObject = Convert-ReleaseMaterialObject -RepoRoot $resolvedRepoRoot -Value ([ordered]@{
        path = $pdfSummaryPath
        nested = @(
            [pscustomobject]@{
                command = $repoRootCommandText
            }
        )
    })
Assert-Equal -Actual ([string]$sanitizedObject.path) -Expected ".\pdf-summary\summary.json" `
    -Message "Release material object sanitizer should rewrite external absolute paths recursively."
Assert-DoesNotContainText -Text ([string]$sanitizedObject.nested[0].command) -UnexpectedText $resolvedRepoRoot `
    -Message "Release material object sanitizer should rewrite nested command strings."

$pdfVisualGateInfo = Get-PdfVisualGateSummaryInfo -SummaryJson $pdfSummaryPath
if ($pdfVisualGateInfo.status -ne "loaded" -or
    $pdfVisualGateInfo.verdict -ne "pass" -or
    [int]$pdfVisualGateInfo.cjk_manifest_count -ne 43 -or
    [int]$pdfVisualGateInfo.cjk_copy_search_count -ne 43 -or
    [int]$pdfVisualGateInfo.cjk_copy_search_missing_text_count -ne 0 -or
    [int]$pdfVisualGateInfo.visual_baseline_manifest_count -ne 42 -or
    [int]$pdfVisualGateInfo.visual_baseline_count -ne 44 -or
    -not [bool]$pdfVisualGateInfo.finalizable) {
    throw "PDF visual gate summary metadata was not loaded into a finalizable release summary object."
}

$computedPdfVisualGateInfo = Get-PdfVisualGateSummaryInfo -SummaryJson $pdfComputedSummaryPath
if ([int]$computedPdfVisualGateInfo.cjk_copy_search_missing_text_count -ne 1) {
    throw "PDF visual gate summary metadata should derive missing CJK text count from cjk_copy_search entries."
}

$missingPdfVisualGateInfo = Get-PdfVisualGateSummaryInfo -SummaryJson (Join-Path $resolvedWorkingDir "missing-summary.json")
if ($missingPdfVisualGateInfo.status -ne "missing" -or [bool]$missingPdfVisualGateInfo.finalizable) {
    throw "Missing PDF visual gate summary should not be marked finalizable."
}

$pdfAttemptSummaryPath = Join-Path $pdfSummaryDir "attempt-summary.json"
([ordered]@{
        schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
        status = "partial"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "bounded_attempt_auxiliary_only"
        stage_count = 6
        passed_stage_count = 4
        failed_stage_count = 0
        incomplete_stage_count = 2
        pdf_cli_export_status = "pass"
        pdf_regression_status = "pass"
        pdf_regression_selected_test_count = 91
        pdf_regression_failed_test_count = 0
        pdf_regression_skipped_test_count = 7
        unicode_font_status = "pass"
        cjk_copy_search_status = "pass"
        cjk_copy_search_count = 43
        cjk_copy_search_missing_text_count = 0
        visual_baseline_render_status = "partial"
        visual_baseline_fresh_rendered_count = 37
        expected_visual_render_count = 44
        visual_baseline_fresh_missing_sample_count = 7
        visual_baseline_resume_needed = $true
        visual_baseline_resume_slice_offset = 37
        visual_baseline_resume_slice_limit = 7
        visual_baseline_resume_slice_command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir <build-dir> -OutputDir .\output\pdf-visual-release-gate-current -VisualBaselineSliceOnly -VisualBaselineOffset 37 -VisualBaselineLimit 7 -SkipPreflight"
        outer_guard_status = "timed_out"
        outer_guard_timed_out = $true
        outer_guard_timeout_seconds = 60
        aggregate_contact_sheet_status = "stale"
        aggregate_contact_sheet = $pdfContactSheetPath
        aggregate_contact_sheet_bytes = 1822428
    } | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $pdfAttemptSummaryPath -Encoding UTF8

$pdfAttemptInfo = Get-PdfVisualGateAttemptSummaryInfo -SummaryJson $pdfAttemptSummaryPath
if ($pdfAttemptInfo.status -ne "partial" -or
    $pdfAttemptInfo.verdict -ne "not_complete" -or
    $pdfAttemptInfo.full_visual_gate_status -ne "not_complete" -or
    $pdfAttemptInfo.outer_guard_status -ne "timed_out" -or
    -not [bool]$pdfAttemptInfo.outer_guard_timed_out -or
    [int]$pdfAttemptInfo.outer_guard_timeout_seconds -ne 60 -or
    [int]$pdfAttemptInfo.visual_baseline_fresh_rendered_count -ne 37 -or
    [int]$pdfAttemptInfo.expected_visual_render_count -ne 44 -or
    [int]$pdfAttemptInfo.visual_baseline_fresh_missing_sample_count -ne 7 -or
    -not [bool]$pdfAttemptInfo.visual_baseline_resume_needed -or
    [int]$pdfAttemptInfo.visual_baseline_resume_slice_offset -ne 37 -or
    [int]$pdfAttemptInfo.visual_baseline_resume_slice_limit -ne 7 -or
    [string]$pdfAttemptInfo.visual_baseline_resume_slice_command_template -notmatch "VisualBaselineSliceOnly") {
    throw "PDF visual gate attempt summary metadata was not loaded with outer-guard evidence."
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

$fullPdfCtestSummaryPath = Join-Path $boundedCtestDir "full-pdf-ctest-summary.json"
Set-Content -LiteralPath $fullPdfCtestSummaryPath -Encoding UTF8 -Value "{}"
$pdfReadinessSummaryPath = Join-Path $pdfSummaryDir "release-readiness-summary.json"
([ordered]@{
        status = "pass"
        verdict = "pass_with_warnings"
        release_ready = $true
        warning_count = 1
        visual_gate_release_evidence_accepted = $true
        visual_gate_fresh_full_guarded_evidence = $false
        visual_gate_pass_summary_before_outer_timeout = $false
        visual_gate_segmented_full_coverage_evidence = $true
        visual_full_gate_status = "timeout"
        visual_full_gate_verdict = "not_complete"
        full_ctest_status = "timeout"
        full_ctest_verdict = "not_complete"
        full_ctest_summary_json = $fullPdfCtestSummaryPath
        full_ctest_summary_json_display = ".\build\pdf-ctest-full-current\summary.json"
        full_ctest_outer_guard_status = "timed_out"
        full_ctest_outer_guard_timed_out = $true
        full_ctest_selected_test_count = 139
        full_ctest_completed_test_count = 133
        full_ctest_passed_test_count = 126
        full_ctest_failed_test_count = 0
        full_ctest_skipped_test_count = 7
        full_ctest_not_run_test_count = 6
        full_ctest_completion_percent = 95.7
        full_ctest_remaining_test_count = 6
        full_ctest_zero_failed_tests_observed = $true
        boundary = "full_ctest_timeout_is_not_release_blocking_when_zero_failures_observed"
        marker = "pdf_full_ctest_readiness_trace"
    } | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath $pdfReadinessSummaryPath -Encoding UTF8
$pdfFullCtestReadinessInfo = Get-PdfFullCtestReadinessSummaryInfo `
    -SummaryJson $pdfReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ($pdfFullCtestReadinessInfo.status -ne "pass" -or
    $pdfFullCtestReadinessInfo.verdict -ne "pass_with_warnings" -or
    -not [bool]$pdfFullCtestReadinessInfo.release_ready -or
    -not [bool]$pdfFullCtestReadinessInfo.visual_gate_release_evidence_accepted -or
    [bool]$pdfFullCtestReadinessInfo.visual_gate_fresh_full_guarded_evidence -or
    [bool]$pdfFullCtestReadinessInfo.visual_gate_pass_summary_before_outer_timeout -or
    -not [bool]$pdfFullCtestReadinessInfo.visual_gate_segmented_full_coverage_evidence -or
    $pdfFullCtestReadinessInfo.visual_full_gate_status -ne "timeout" -or
    $pdfFullCtestReadinessInfo.full_ctest_status -ne "timeout" -or
    $pdfFullCtestReadinessInfo.full_ctest_verdict -ne "not_complete" -or
    [int]$pdfFullCtestReadinessInfo.selected_test_count -ne 139 -or
    [int]$pdfFullCtestReadinessInfo.completed_test_count -ne 133 -or
    [int]$pdfFullCtestReadinessInfo.failed_test_count -ne 0 -or
    [int]$pdfFullCtestReadinessInfo.not_run_test_count -ne 6 -or
    [double]$pdfFullCtestReadinessInfo.completion_percent -ne 95.7 -or
    -not [bool]$pdfFullCtestReadinessInfo.zero_failed_tests_observed) {
    throw "PDF full CTest readiness summary was not loaded as release governance evidence."
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

if ($Scenario -eq "contract") {
    Write-Host "Release candidate visual verdict contract regression passed."
    return
}
