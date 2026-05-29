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

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
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

function Write-TestJson {
    param(
        [string]$Path,
        [object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
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
        "ReleaseEvidenceScope"
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
Assert-Equal -Actual ([string]$sanitizedObject.path) -Expected (Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfSummaryPath) `
    -Message "Release material object sanitizer should rewrite repo-root paths recursively."
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

$candidateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-checks"
$candidateBuildDir = Join-Path $resolvedWorkingDir "build"
$candidateInstallDir = Join-Path $resolvedWorkingDir "install"
$candidateConsumerBuildDir = Join-Path $resolvedWorkingDir "consumer-build"
$candidateGateOutputDir = Join-Path $resolvedWorkingDir "word-visual-gate"
$candidateTaskOutputRoot = Join-Path $resolvedWorkingDir "visual-tasks"
$releaseGovernanceHandoffInputRoot = Join-Path $resolvedWorkingDir "release-governance-handoff-input"
$releaseGovernanceHandoffOutputDir = Join-Path $resolvedWorkingDir "release-governance-handoff"
$releaseGovernanceSourceDir = Join-Path $resolvedWorkingDir "release-candidate-checks-source"
$releaseGovernanceSourcePath = Join-Path $releaseGovernanceSourceDir "summary.json"
New-Item -ItemType Directory -Path $releaseGovernanceHandoffInputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $releaseGovernanceSourceDir -Force | Out-Null
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "numbering-catalog-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
        real_corpus_confidence_score = 100
        real_corpus_confidence_level = "high"
        real_corpus_confidence = [ordered]@{
            document_count = 2
            catalog_exemplar_count = 2
            baseline_entry_count = 2
            matched_document_count = 2
            unmatched_catalog_document_count = 0
            unmatched_baseline_document_count = 0
            alignment_gap_count = 0
            catalog_coverage_percent = 100
            baseline_coverage_percent = 100
            coverage_score = 100
            catalog_document_keys = @("catalog.invoice", "catalog.statement")
            baseline_document_keys = @("catalog.invoice", "catalog.statement")
            matched_document_keys = @("catalog.invoice", "catalog.statement")
            penalty_summary = @(
                [ordered]@{
                    factor = "style_numbering_issues"
                    count = 0
                    penalty = 0
                }
            )
            style_numbering_issues = @()
        }
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
        delivery_quality_score = 100
        delivery_quality_level = "high"
        delivery_quality = [ordered]@{
            document_count = 3
            ready_document_count = 3
            ready_document_percent = 100
            needs_review_document_count = 0
            failed_document_count = 0
            table_style_issue_count = 0
            automatic_tblLook_fix_count = 0
            manual_table_style_fix_count = 0
            table_position_automatic_count = 0
            table_position_review_count = 0
            command_failure_count = 0
            unresolved_item_count = 0
            penalty_summary = @(
                [ordered]@{
                    factor = "floating_table_plans_pending"
                    count = 0
                    penalty = 0
                }
            )
            floating_table_plans_pending = 0
        }
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.content_control_data_binding_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "ready"
        release_ready = $true
        latest_schema_approval_gate_status = "not_required"
        schema_approval_status_summary = @(
            [ordered]@{
                status = "not_required"
                count = 3
            }
        )
        template_count = 3
        ready_template_count = 3
        blocked_template_count = 0
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "project-template-onboarding-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "ready"
        release_ready = $true
        schema_approval_status_summary = @(
            [ordered]@{
                status = "not_required"
                count = 3
            }
        )
        entry_count = 3
        blocked_entry_count = 0
        pending_review_entry_count = 0
        not_evaluated_entry_count = 0
        approved_entry_count = 0
        not_required_entry_count = 3
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "schema-patch-confidence-calibration\summary.json") -Value ([ordered]@{
        schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "docx-functional-smoke-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.docx_functional_smoke_readiness.v1"
        status = "pass"
        verdict = "pass"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
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
        word_visual_standard_review_metadata_count = 4
        word_visual_standard_review_task_keys = @(
            "smoke",
            "fixed_grid",
            "section_page_setup",
            "page_number_fields"
        )
        word_visual_standard_review_status_summary = "reviewed=4"
        word_visual_standard_review_verdict_summary = "pass=4"
        word_visual_standard_review_metadata = @(
            [ordered]@{
                task_key = "smoke"
                review_task_key = "document"
                label = "Word visual smoke"
                verdict = "pass"
                review_status = "reviewed"
                reviewed_at = "2026-04-12T12:10:00"
                review_method = "operator_supplied"
                review_result_path = ".\output\word-visual-release-gate\review-tasks\document\report\review_result.json"
                final_review_path = ".\output\word-visual-release-gate\review-tasks\document\report\final_review.md"
                review_note = "Private operator note"
            },
            [ordered]@{
                task_key = "fixed_grid"
                review_task_key = "fixed_grid"
                label = "Fixed-grid merge/unmerge"
                verdict = "pass"
                review_status = "reviewed"
                reviewed_at = "2026-04-12T12:20:00"
                review_method = "operator_supplied"
                review_result_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json"
                final_review_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md"
            },
            [ordered]@{
                task_key = "section_page_setup"
                review_task_key = "section_page_setup"
                label = "Section page setup"
                verdict = "pass"
                review_status = "reviewed"
                reviewed_at = "2026-04-12T12:30:00"
                review_method = "operator_supplied"
                review_result_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json"
                final_review_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md"
            },
            [ordered]@{
                task_key = "page_number_fields"
                review_task_key = "page_number_fields"
                label = "Page number fields"
                verdict = "pass"
                review_status = "reviewed"
                reviewed_at = "2026-04-12T12:40:00"
                review_method = "operator_supplied"
                review_result_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json"
                final_review_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
            }
        )
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
    -PdfVisualGateAttemptSummaryJson $pdfAttemptSummaryPath `
    -PdfVisualSegmentedGateSummaryJson $pdfSegmentedSummaryPath `
    -PdfBoundedCtestSummaryJson @($boundedSmokePath, $boundedBusinessPath) `
    -PdfReleaseReadinessSummaryJson $pdfReadinessSummaryPath
if ($LASTEXITCODE -ne 0) {
    throw "Release candidate dry run with PDF visual gate summary failed with exit code $LASTEXITCODE."
}

$candidateSummaryPath = Join-Path $candidateOutputDir "report\summary.json"
if (-not (Test-Path -LiteralPath $candidateSummaryPath)) {
    throw "Release candidate dry run did not write summary.json."
}

$candidateSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateSummaryPath | ConvertFrom-Json
$expectedPdfSummaryPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfSummaryPath
$expectedPdfContactSheetPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfContactSheetPath
$expectedPdfAttemptSummaryPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfAttemptSummaryPath
$expectedPdfSegmentedSummaryPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfSegmentedSummaryPath
$expectedPdfReadinessSummaryPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $pdfReadinessSummaryPath
$expectedFullPdfCtestSummaryPath = Get-RepoRelativePath -RepoRoot $resolvedRepoRoot -Path $fullPdfCtestSummaryPath
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
    [int]$candidateSummary.steps.pdf_visual_gate.cjk_copy_search_missing_text_count -ne 0 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_manifest_count -ne 42 -or
    [int]$candidateSummary.steps.pdf_visual_gate.visual_baseline_count -ne 44) {
    throw "Release candidate summary did not preserve PDF visual gate sample counts."
}
if ([string]$candidateSummary.pdf_visual_gate_summary_json -ne $expectedPdfSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_gate.summary_json -ne $expectedPdfSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_gate.aggregate_contact_sheet -ne $expectedPdfContactSheetPath) {
    throw "Release candidate summary did not preserve PDF visual gate evidence paths."
}
if ($candidateSummary.steps.pdf_visual_segmented_gate.status -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.verdict -ne "pass" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.full_visual_gate_status -ne "not_complete" -or
    $candidateSummary.steps.pdf_visual_segmented_gate.evidence_scope -ne "segmented_visual_gate_auxiliary_only") {
    throw "Release candidate summary did not preserve segmented PDF visual gate auxiliary status."
}
if ([string]$candidateSummary.pdf_visual_segmented_gate_summary_json -ne $expectedPdfSegmentedSummaryPath -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.slice_pass_count -ne 4 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.covered_baseline_count -ne 44 -or
    [int]$candidateSummary.steps.pdf_visual_segmented_gate.expected_visual_render_count -ne 44 -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.summary_json -ne $expectedPdfSegmentedSummaryPath -or
    [string]$candidateSummary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet -ne $expectedPdfContactSheetPath) {
    throw "Release candidate summary did not preserve segmented PDF visual gate counts or paths."
}
if ([int]$candidateSummary.steps.pdf_bounded_ctest.summary_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.pass_count -ne 2 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.selected_test_count -ne 20 -or
    [int]$candidateSummary.steps.pdf_bounded_ctest.skipped_test_count -ne 0 -or
    @($candidateSummary.steps.pdf_bounded_ctest.subsets) -notcontains "regression-business-samples") {
    throw "Release candidate summary did not preserve PDF bounded CTest auxiliary evidence."
}
if ([string]$candidateSummary.pdf_release_readiness_summary_json -ne $expectedPdfReadinessSummaryPath -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.summary_json -ne $expectedPdfReadinessSummaryPath -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.status -ne "pass" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.verdict -ne "pass_with_warnings" -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.release_ready -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_release_evidence_accepted -or
    [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_fresh_full_guarded_evidence -or
    [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_pass_summary_before_outer_timeout -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.visual_gate_segmented_full_coverage_evidence -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.visual_full_gate_status -ne "timeout" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_status -ne "timeout" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_verdict -ne "not_complete" -or
    [string]$candidateSummary.steps.pdf_full_ctest_readiness.full_ctest_summary_json -ne $expectedFullPdfCtestSummaryPath -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.selected_test_count -ne 139 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.completed_test_count -ne 133 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.failed_test_count -ne 0 -or
    [int]$candidateSummary.steps.pdf_full_ctest_readiness.not_run_test_count -ne 6 -or
    [double]$candidateSummary.steps.pdf_full_ctest_readiness.completion_percent -ne 95.7 -or
    -not [bool]$candidateSummary.steps.pdf_full_ctest_readiness.zero_failed_tests_observed) {
    throw "Release candidate summary did not preserve PDF full CTest readiness evidence."
}
if ($candidateSummary.steps.pdf_visual_gate_attempt.status -ne "partial" -or
    $candidateSummary.steps.pdf_visual_gate_attempt.verdict -ne "not_complete" -or
    $candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_status -ne "timed_out" -or
    -not [bool]$candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_timed_out -or
    [int]$candidateSummary.steps.pdf_visual_gate_attempt.outer_guard_timeout_seconds -ne 60) {
    throw "Release candidate summary did not preserve PDF visual gate attempt outer-guard evidence."
}
if ([int]$candidateSummary.warning_count -ne 1 -or @($candidateSummary.warnings).Count -ne 1) {
    throw "Release candidate summary should expose one machine-readable PDF fresh-attempt warning."
}
$pdfAttemptWarning = @($candidateSummary.warnings) |
    Where-Object { [string]$_.id -eq "pdf_visual_gate_attempt.incomplete_fresh_render" } |
    Select-Object -First 1
if ($null -eq $pdfAttemptWarning -or
    [string]$pdfAttemptWarning.action -ne "review_pdf_visual_gate_attempt_and_finalize_evidence" -or
    [string]$pdfAttemptWarning.source_schema -ne "featherdoc.release_candidate_summary" -or
    [string]$candidateSummary.pdf_visual_gate_attempt_summary_json -ne $expectedPdfAttemptSummaryPath -or
    [string]$pdfAttemptWarning.source_json -ne $expectedPdfAttemptSummaryPath -or
    [string]$pdfAttemptWarning.source_json_display -notmatch [regex]::Escape("attempt-summary.json") -or
    [string]$pdfAttemptWarning.outer_guard_status -ne "timed_out" -or
    -not [bool]$pdfAttemptWarning.outer_guard_timed_out -or
    [int]$pdfAttemptWarning.outer_guard_timeout_seconds -ne 60 -or
    -not [bool]$pdfAttemptWarning.visual_gate_release_evidence_accepted -or
    [bool]$pdfAttemptWarning.visual_gate_fresh_full_guarded_evidence -or
    [bool]$pdfAttemptWarning.visual_gate_pass_summary_before_outer_timeout -or
    -not [bool]$pdfAttemptWarning.visual_gate_segmented_full_coverage_evidence -or
    [bool]$pdfAttemptWarning.visual_gate_finalize_only -or
    -not [bool]$pdfAttemptWarning.release_owner_acceptance_required -or
    [string]$pdfAttemptWarning.release_owner_acceptance_boundary -ne "acceptance_does_not_replace_fresh_single_run_full_visual_gate" -or
    [string]$pdfAttemptWarning.visual_baseline_render_status -ne "partial" -or
    [int]$pdfAttemptWarning.visual_baseline_fresh_missing_sample_count -ne 7 -or
    -not [bool]$pdfAttemptWarning.visual_baseline_resume_needed -or
    [int]$pdfAttemptWarning.visual_baseline_resume_slice_offset -ne 37 -or
    [int]$pdfAttemptWarning.visual_baseline_resume_slice_limit -ne 7 -or
    [string]$pdfAttemptWarning.visual_baseline_resume_slice_command_template -notmatch "VisualBaselineSliceOnly" -or
    [string]$pdfAttemptWarning.aggregate_contact_sheet_status -ne "stale") {
    throw "Release candidate summary did not materialize the PDF fresh-attempt warning with reviewer-facing evidence."
}
Assert-ContainsText -Text ([string]$pdfAttemptWarning.message) -ExpectedText "segmented full-coverage visual evidence" `
    -Message "PDF fresh-attempt warning should explain that release evidence is accepted through segmented coverage."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.release_owner_acceptance_policy) -ExpectedText "segmented_full_coverage" `
    -Message "PDF fresh-attempt warning should expose the release-owner acceptance policy."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.release_owner_acceptance_command_template) -ExpectedText "run_release_candidate_checks.ps1" `
    -Message "PDF fresh-attempt warning should expose the release-owner acceptance command template."
Assert-ContainsText -Text ([string]$pdfAttemptWarning.visual_baseline_resume_slice_command_template) -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "PDF fresh-attempt warning should expose the visual baseline resume command template."
if ([string]$pdfAttemptWarning.message -match "relies on FinalizeOnly") {
    throw "PDF fresh-attempt warning should not claim FinalizeOnly evidence when segmented full coverage is accepted."
}

$passAttemptWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary ([ordered]@{
            pdf_visual_gate_attempt = [ordered]@{
                status = "pass"
                verdict = "pass"
                full_visual_gate_status = "pass"
                outer_guard_status = "timed_out_after_pass_summary"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
                visual_baseline_render_status = "pass"
                aggregate_contact_sheet_status = "pass"
            }
            pdf_full_ctest_readiness = [ordered]@{
                verdict = "pass"
                visual_gate_release_evidence_accepted = $true
                visual_gate_fresh_full_guarded_evidence = $true
                visual_gate_pass_summary_before_outer_timeout = $true
                visual_gate_segmented_full_coverage_evidence = $false
            }
            pdf_visual_gate = [ordered]@{
                finalize_only = $false
                skip_preflight = $true
            }
        }) -RepoRoot $resolvedRepoRoot)
if ($passAttemptWarnings.Count -ne 0) {
    throw "Passing PDF visual attempt with pass-summary-before-timeout evidence should not emit a fresh-attempt warning."
}

$acceptedSegmentedAttemptWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary ([ordered]@{
            pdf_visual_gate_attempt = [ordered]@{
                status = "partial"
                verdict = "not_complete"
                full_visual_gate_status = "not_complete"
                evidence_scope = "bounded_attempt_auxiliary_only"
                stage_count = 6
                passed_stage_count = 6
                failed_stage_count = 0
                incomplete_stage_count = 0
                pdf_regression_status = "pass"
                pdf_regression_failed_test_count = 0
                cjk_copy_search_status = "pass"
                cjk_copy_search_missing_text_count = 0
                visual_baseline_render_status = "pass"
                visual_baseline_fresh_rendered_count = 44
                expected_visual_render_count = 44
                aggregate_contact_sheet_status = "pass"
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
            }
            pdf_full_ctest_readiness = [ordered]@{
                release_ready = $true
                verdict = "pass_with_warnings"
                visual_gate_release_evidence_accepted = $true
                visual_gate_fresh_full_guarded_evidence = $false
                visual_gate_pass_summary_before_outer_timeout = $false
                visual_gate_segmented_full_coverage_evidence = $true
            }
            pdf_visual_gate = [ordered]@{
                finalize_only = $false
                skip_preflight = $false
            }
        }) -RepoRoot $resolvedRepoRoot)
if ($acceptedSegmentedAttemptWarnings.Count -ne 0) {
    throw "Completed auxiliary PDF visual attempt with accepted segmented full coverage should not occupy release warning_count."
}

if ([int]$candidateSummary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count -ne 2 -or
    [int]$candidateSummary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count -ne 2) {
    throw "Release candidate summary did not consume project-template release entry evidence from release governance handoff."
}
if ([int]$candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_report_count -ne 1 -or
    @($candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_reports).Count -ne 1 -or
    [int]$candidateSummary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_report_count -ne 1 -or
    @($candidateSummary.steps.release_governance_handoff.word_visual_standard_review_metadata_source_reports).Count -ne 1) {
    throw "Release candidate summary did not consume Word visual standard review metadata evidence from release governance handoff."
}
$wordVisualMetadataReport = @($candidateSummary.release_governance_handoff.word_visual_standard_review_metadata_source_reports) |
    Where-Object { [string]$_.schema -eq "featherdoc.release_candidate_summary" } |
    Select-Object -First 1
if ($null -eq $wordVisualMetadataReport -or
    [int]$wordVisualMetadataReport.word_visual_standard_review_metadata_count -ne 4 -or
    [string]$wordVisualMetadataReport.word_visual_standard_review_status_summary -ne "reviewed=4" -or
    [string]$wordVisualMetadataReport.word_visual_standard_review_verdict_summary -ne "pass=4" -or
    @($wordVisualMetadataReport.word_visual_standard_review_task_keys).Count -ne 4 -or
    @($wordVisualMetadataReport.word_visual_standard_review_metadata).Count -ne 4) {
    throw "Release candidate summary lost Word visual standard review metadata details."
}
foreach ($entry in @($wordVisualMetadataReport.word_visual_standard_review_metadata)) {
    if ($entry.PSObject.Properties.Name -contains "review_note") {
        throw "Release candidate summary exposed Word visual standard review notes in handoff metadata."
    }
}
if ([int]$candidateSummary.governance_metric_count -ne 2 -or @($candidateSummary.governance_metrics).Count -ne 2) {
    throw "Release candidate summary did not expose both release governance metrics as top-level material safety evidence."
}
$numberingMetric = @($candidateSummary.governance_metrics) |
    Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
    Select-Object -First 1
if ($null -eq $numberingMetric -or [string]::IsNullOrWhiteSpace([string]$numberingMetric.level) -or $null -eq $numberingMetric.score -or
    [int]$numberingMetric.details.catalog_coverage_percent -ne 100 -or
    @($numberingMetric.details.catalog_document_keys).Count -ne 2 -or
    @($numberingMetric.details.style_numbering_issues).Count -ne 0) {
    throw "Release candidate summary lost the numbering governance metric details."
}
$tableMetric = @($candidateSummary.governance_metrics) |
    Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
    Select-Object -First 1
if ($null -eq $tableMetric -or [string]::IsNullOrWhiteSpace([string]$tableMetric.level) -or $null -eq $tableMetric.score -or
    [int]$tableMetric.details.ready_document_percent -ne 100 -or
    [int]$tableMetric.details.unresolved_item_count -ne 0 -or
    [int]$tableMetric.details.floating_table_plans_pending -ne 0) {
    throw "Release candidate summary lost the table-layout delivery governance metric details."
}
if ([int]$candidateSummary.release_governance_handoff.report_count -ne 6 -or
    @($candidateSummary.release_governance_handoff.reports).Count -ne 6) {
    throw "Release candidate summary did not preserve release governance source reports for material rendering."
}
if ($null -eq $candidateSummary.release_governance_handoff.project_template_delivery_readiness_contract -or
    [string]$candidateSummary.release_governance_handoff.project_template_delivery_readiness_contract.status -ne "ready" -or
    $null -eq $candidateSummary.release_governance_handoff.project_template_onboarding_governance_contract -or
    [string]$candidateSummary.release_governance_handoff.project_template_onboarding_governance_contract.status -ne "ready") {
    throw "Release candidate summary did not preserve project-template governance contracts from handoff."
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
Assert-ContainsText -Text $finalReviewContent -ExpectedText "- Warnings: 1" `
    -Message "final_review.md should expose the top-level warning count."
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

$candidateReleaseHandoff = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseHandoffPath
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "Warnings: 1" `
    -Message "release_handoff.md should expose the propagated warning count."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "pdf_visual_gate_attempt.incomplete_fresh_render" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning id."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "review_pdf_visual_gate_attempt_and_finalize_evidence" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning action."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "attempt-summary.json" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning evidence path."

$candidateFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateFinalReviewPath
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Key outputs" -Fragments @(
    "PDF visual gate summary:",
    "summary.json",
    "PDF visual gate contact sheet:",
    "aggregate-contact-sheet.png",
    "PDF bounded CTest summaries:",
    "smoke-summary.json",
    "business-summary.json",
    "PDF release readiness summary:",
    "release-readiness-summary.json",
    "PDF full CTest summary:",
    "pdf-ctest-full-current\summary.json"
) -Message "final_review.md should keep PDF visual and full CTest readiness outputs inside the Key outputs section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Step status" -Fragments @(
    "PDF bounded CTest summaries: 2 summaries, 2 pass",
    "PDF bounded CTest subsets: smoke-import, regression-business-samples",
    "PDF bounded CTest selected tests: 20",
    "PDF bounded CTest skipped tests: 0",
    "PDF visual release evidence accepted: True (fresh full guarded False, pass summary before outer timeout False, segmented full coverage True)",
    "PDF full CTest readiness: timeout (95.7% complete)",
    "PDF full CTest progress: 133/139 completed, 6 not run",
    "PDF full CTest observed failures: 0, zero failed observed True"
) -Message "final_review.md should expose bounded and full PDF CTest auxiliary evidence in the Step status section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Project-template release entry evidence" -Fragments @(
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=2",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=",
    "report\summary.json",
    "Project-template readiness checklist packaged audit evidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=2",
    "assert_release_material_safety.ps1",
    "audited_entrypoints=start_here, artifact_guide, reviewer_checklist",
    "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
) -Message "final_review.md should expose project-template checklist handoff and packaged audit evidence consumed from release governance handoff."
Assert-MarkdownListRunContainsAll -Text $candidateFinalReview -Anchor "project_template_delivery_readiness / project_template_onboarding.schema_approval" -Fragments @(
    "project_template_delivery_readiness / project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "schema_approval_status_summary=not_required=3",
    "readiness_status: ready",
    "readiness_release_ready: True",
    "source_report_display:",
    "project-template-onboarding-governance",
    "source_json_display:",
    "project-template-onboarding-governance"
) -Message "final_review.md should keep project-template readiness and onboarding governance contracts in one material-safety block."

foreach ($assertion in @(
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "project_template_delivery_readiness:" -Fragments @(
        "project_template_delivery_readiness",
        "project_template_delivery_readiness_contract",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "status: ready",
        "release_ready: True",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary=not_required=3",
        "source_report_display:",
        "project-template-delivery-readiness",
        "source_json_display:",
        "project-template-delivery-readiness"
    ) -Message ("{0} should keep project-template delivery readiness contract evidence in one list block." -f $assertion.Label)
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "project_template_onboarding.schema_approval" -Fragments @(
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "status: ready",
        "release_ready: True",
        "schema_approval_status_summary=not_required=3",
        "source_report_display:",
        "project-template-onboarding-governance",
        "source_json_display:",
        "project-template-onboarding-governance"
    ) -Message ("{0} should keep project-template onboarding governance contract evidence in one list block." -f $assertion.Label)
}

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

$localAbsolutePathPattern = '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+'
foreach ($releaseMaterialPath in @(
        $candidateSummaryPath,
        $candidateFinalReviewPath,
        $candidateReleaseBodyPath,
        $candidateReleaseSummaryPath,
        $candidateReleaseHandoffPath,
        $candidateArtifactGuidePath,
        $candidateReviewerChecklistPath,
        $candidateStartHerePath
    )) {
    $pathLeak = Select-String -LiteralPath $releaseMaterialPath -Pattern $localAbsolutePathPattern -AllMatches | Select-Object -First 1
    if ($null -ne $pathLeak) {
        throw "Release material contains a local absolute path leak: $releaseMaterialPath"
    }
}

Write-Host "Release candidate visual verdict passthrough regression passed."
