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

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected '$Expected', got '$Actual'."
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_word_visual_release_gate.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SmokeReviewVerdict = "undecided"' `
    -Message "Release gate should expose SmokeReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SmokeReviewNote = ""' `
    -Message "Release gate should expose SmokeReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$smokeArgs += @("-ReviewVerdict", $SmokeReviewVerdict)' `
    -Message "Release gate should pass SmokeReviewVerdict into run_word_visual_smoke.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$smokeArgs += @("-ReviewNote", $SmokeReviewNote)' `
    -Message "Release gate should pass SmokeReviewNote into run_word_visual_smoke.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $smokeReviewResult -Name "verdict"' `
    -Message "Release gate summary should capture the smoke review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Smoke review verdict:' `
    -Message "Release gate final review should surface the smoke review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$FixedGridReviewVerdict = "undecided"' `
    -Message "Release gate should expose FixedGridReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$FixedGridReviewNote = ""' `
    -Message "Release gate should expose FixedGridReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $FixedGridReviewVerdict)' `
    -Message "Release gate should pass FixedGridReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $FixedGridReviewNote)' `
    -Message "Release gate should pass FixedGridReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $fixedGridTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the fixed-grid task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Fixed-grid review verdict:' `
    -Message "Release gate final review should surface the fixed-grid review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SectionPageSetupReviewVerdict = "undecided"' `
    -Message "Release gate should expose SectionPageSetupReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$SectionPageSetupReviewNote = ""' `
    -Message "Release gate should expose SectionPageSetupReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $SectionPageSetupReviewVerdict)' `
    -Message "Release gate should pass SectionPageSetupReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $SectionPageSetupReviewNote)' `
    -Message "Release gate should pass SectionPageSetupReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$sectionPageSetupTaskReview = Read-ReviewResult -ReviewResultPath $sectionPageSetupTaskInfo.review_result_path' `
    -Message "Release gate should read the section page setup task review result."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $sectionPageSetupTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the section page setup task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Section page setup review verdict:' `
    -Message "Release gate final review should surface the section page setup review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$PageNumberFieldsReviewVerdict = "undecided"' `
    -Message "Release gate should expose PageNumberFieldsReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$PageNumberFieldsReviewNote = ""' `
    -Message "Release gate should expose PageNumberFieldsReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $PageNumberFieldsReviewVerdict)' `
    -Message "Release gate should pass PageNumberFieldsReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $PageNumberFieldsReviewNote)' `
    -Message "Release gate should pass PageNumberFieldsReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$pageNumberFieldsTaskReview = Read-ReviewResult -ReviewResultPath $pageNumberFieldsTaskInfo.review_result_path' `
    -Message "Release gate should read the page number fields task review result."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $pageNumberFieldsTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the page number fields task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'Page number fields review verdict:' `
    -Message "Release gate final review should surface the page number fields review verdict."

Assert-ContainsText -Text $scriptText -ExpectedText '[string]$CuratedVisualReviewVerdict = "undecided"' `
    -Message "Release gate should expose CuratedVisualReviewVerdict."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$CuratedVisualReviewNote = ""' `
    -Message "Release gate should expose CuratedVisualReviewNote."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewVerdict", $CuratedVisualReviewVerdict)' `
    -Message "Release gate should pass CuratedVisualReviewVerdict into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$prepareTaskArgs += @("-ReviewNote", $CuratedVisualReviewNote)' `
    -Message "Release gate should pass CuratedVisualReviewNote into prepare_word_review_task.ps1."
Assert-ContainsText -Text $scriptText -ExpectedText '$bundleTaskReview = Read-ReviewResult -ReviewResultPath $bundleTaskInfo.review_result_path' `
    -Message "Release gate should read the curated task review result."
Assert-ContainsText -Text $scriptText -ExpectedText 'review_verdict = Get-OptionalPropertyValue -Object $bundleTaskReview -Name "verdict"' `
    -Message "Release gate summary should capture the curated task review verdict."
Assert-ContainsText -Text $scriptText -ExpectedText '$($_.label) review verdict:' `
    -Message "Release gate final review should surface curated task review verdicts."
Assert-ContainsText -Text $scriptText -ExpectedText 'function Get-ReviewTaskSummary' `
    -Message "Release gate should derive machine-readable review task summary counts."
Assert-ContainsText -Text $scriptText -ExpectedText 'function Test-ReviewTaskPresent' `
    -Message "Release gate should ignore empty review task placeholders while counting tasks."
Assert-ContainsText -Text $scriptText -ExpectedText '$gateSummary.review_task_summary = Get-ReviewTaskSummary -ReviewTasks $gateSummary.review_tasks' `
    -Message "Release gate should write review task summary metadata into gate_summary.json."
Assert-ContainsText -Text $scriptText -ExpectedText 'Review task count: $($gateSummary.review_task_summary.total_count) total' `
    -Message "Release gate final review should surface review task summary counts."

$parseTokens = $null
$parseErrors = $null
$scriptAst = [System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$parseTokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) {
    throw "Release gate script has parse errors."
}

$functionNames = @(
    "Get-OptionalPropertyValue",
    "Test-ReviewTaskPresent",
    "Get-ReviewTaskSummary"
)
$functionAsts = $scriptAst.FindAll({
        param($Node)
        $Node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
            $functionNames -contains $Node.Name
    }, $true)
foreach ($functionName in $functionNames) {
    $functionAst = $functionAsts | Where-Object { $_.Name -eq $functionName } | Select-Object -First 1
    if ($null -eq $functionAst) {
        throw "Missing function used by review task counting: $functionName"
    }

    Invoke-Expression $functionAst.Extent.Text
}

$dictionarySummary = Get-ReviewTaskSummary -ReviewTasks ([ordered]@{
        document = [ordered]@{ task_id = "document-task" }
        fixed_grid = ""
        section_page_setup = $null
        page_number_fields = [ordered]@{ task_id = "page-number-task" }
        curated_visual_regressions = @(
            [ordered]@{ id = "curated-task" },
            "",
            $null
        )
    })
Assert-Equal -Actual $dictionarySummary.standard_count -Expected 2 `
    -Message "Dictionary review task summary should ignore empty standard placeholders."
Assert-Equal -Actual $dictionarySummary.curated_count -Expected 1 `
    -Message "Dictionary review task summary should ignore empty curated placeholders."
Assert-Equal -Actual $dictionarySummary.total_count -Expected 3 `
    -Message "Dictionary review task summary total should match counted tasks."

$objectSummary = Get-ReviewTaskSummary -ReviewTasks ([pscustomobject]@{
        document = [pscustomobject]@{ task_id = "document-task" }
        fixed_grid = " "
        section_page_setup = [pscustomobject]@{ task_id = "section-task" }
        page_number_fields = $null
        curated_visual_regressions = @([pscustomobject]@{ id = "curated-task" })
    })
Assert-Equal -Actual $objectSummary.standard_count -Expected 2 `
    -Message "Object review task summary should ignore whitespace standard placeholders."
Assert-Equal -Actual $objectSummary.curated_count -Expected 1 `
    -Message "Object review task summary should count curated objects."
Assert-Equal -Actual $objectSummary.total_count -Expected 3 `
    -Message "Object review task summary total should match counted tasks."

Write-Host "Word visual release gate smoke verdict regression passed."
