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

Assert-ContainsText -Text $scriptText -ExpectedText 'if ($Object -is [System.Collections.IDictionary] -and $Object.Contains($Name))' `
    -Message "Optional property lookup should support the release summary dictionary used while rendering final_review.md."

Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-VisualGateReviewTaskSummaryLine' `
    -Message "Release preflight final_review.md should render visual review task counts."

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

$parseTokens = $null
$parseErrors = $null
$scriptAst = [System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) {
    throw "Release preflight script has parse errors."
}

$functionNames = @(
    "Get-RepoRelativePath",
    "Get-OptionalPropertyValue",
    "Convert-ReviewTimestamp",
    "Get-ReleaseCandidateDisplayValue",
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

Write-Host "Release candidate visual verdict passthrough regression passed."
