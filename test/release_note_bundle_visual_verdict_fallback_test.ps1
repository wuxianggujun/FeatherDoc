param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function Assert-LineContainsAll {
    param(
        [string]$Path,
        [string[]]$Fragments,
        [string]$Label
    )

    $lines = Get-Content -LiteralPath $Path
    foreach ($line in $lines) {
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

    throw "$Label does not contain a line with all expected fragments: $($Fragments -join ', ')"
}

function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
. (Join-Path $resolvedRepoRoot "scripts\release_visual_metadata_helpers.ps1")

$missingReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine `
    -VisualGateSummary ([pscustomobject]@{}) `
    -GateSummary ([pscustomobject]@{})
if ($missingReviewTaskSummaryLine -ne "") {
    throw "Missing review_task_summary unexpectedly rendered '$missingReviewTaskSummaryLine'."
}

$emptyReleaseReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine `
    -VisualGateSummary ([pscustomobject]@{ review_task_summary = [pscustomobject]@{ total_count = "" } }) `
    -GateSummary ([pscustomobject]@{})
if ($emptyReleaseReviewTaskSummaryLine -ne "") {
    throw "Incomplete release review_task_summary unexpectedly rendered '$emptyReleaseReviewTaskSummaryLine'."
}

$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"
$gateReportDir = Join-Path $resolvedWorkingDir "word-visual-release-gate\report"
$taskOutputRoot = Join-Path $resolvedWorkingDir "tasks"
$smokeTaskDir = Join-Path $taskOutputRoot "smoke"
$fixedGridTaskDir = Join-Path $taskOutputRoot "fixed-grid"
$sectionPageSetupTaskDir = Join-Path $taskOutputRoot "section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $taskOutputRoot "page-number-fields"
$curatedBundleId = "curated-review-verdict-fallback"
$curatedBundleLabel = "Curated review verdict fallback"
$curatedBundleTaskDir = Join-Path $taskOutputRoot $curatedBundleId
$supersededReviewTasksReportPath = Join-Path $taskOutputRoot "superseded_review_tasks.json"

foreach ($path in @(
        $reportDir,
        $installDir,
        $gateReportDir,
        $taskOutputRoot,
        $smokeTaskDir,
        $fixedGridTaskDir,
        $sectionPageSetupTaskDir,
        $pageNumberFieldsTaskDir,
        $curatedBundleTaskDir
    )) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"

$gateSummary = [ordered]@{
    generated_at = "2026-04-28T12:00:00"
    workspace = $resolvedRepoRoot
    report_dir = $gateReportDir
    visual_verdict = "pending_manual_review"
    smoke = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $smokeTaskDir }
        review_status = "reviewed"
        review_verdict = "pass"
        reviewed_at = "2026-04-28T12:30:00"
        review_method = "operator_supplied"
        review_note = "smoke contact sheet reviewed"
    }
    fixed_grid = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $fixedGridTaskDir }
        review_status = "reviewed"
        review_verdict = "fail"
        reviewed_at = "2026-04-28T12:32:00"
        review_method = "operator_supplied"
        review_note = "fixed-grid mismatch documented"
    }
    section_page_setup = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $sectionPageSetupTaskDir }
        review_status = "reviewed"
        review_verdict = "undetermined"
        reviewed_at = "2026-04-28T12:33:00"
        review_method = "operator_supplied"
        review_note = "section setup needs reviewer decision"
    }
    page_number_fields = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $pageNumberFieldsTaskDir }
        review_status = "reviewed"
        review_verdict = "pending_manual_review"
        reviewed_at = "2026-04-28T12:34:00"
        review_method = "operator_supplied"
        review_note = "page number fields awaiting reviewer"
    }
    review_task_summary = [ordered]@{
        total_count = 5
        standard_count = 4
        curated_count = 1
    }
    review_tasks = [ordered]@{
        curated_visual_regressions = @(
            [ordered]@{
                id = $curatedBundleId
                label = $curatedBundleLabel
                task = [ordered]@{ task_dir = $curatedBundleTaskDir }
                review_status = "reviewed"
                review_verdict = "pass"
                reviewed_at = "2026-04-28T12:35:00"
                review_method = "operator_supplied"
                review_note = "curated visual evidence checked"
            }
        )
    }
    curated_visual_regressions = @(
        [ordered]@{
            id = $curatedBundleId
            label = $curatedBundleLabel
            status = "completed"
            task = [ordered]@{ task_dir = $curatedBundleTaskDir }
            review_status = "reviewed"
            review_verdict = "pass"
            reviewed_at = "2026-04-28T12:35:00"
            review_method = "operator_supplied"
            review_note = "curated visual evidence checked"
        }
    )
}
($gateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value "# Gate Final Review"
(@{
    generated_at = "2026-04-28T12:00:00"
    task_output_root = $taskOutputRoot
    report_path = $supersededReviewTasksReportPath
    group_count = 0
    superseded_task_count = 0
    groups = @()
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $supersededReviewTasksReportPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-28T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    task_output_root = $taskOutputRoot
    superseded_review_tasks_report = $supersededReviewTasksReportPath
    install_dir = $installDir
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installDir
            consumer_document = (Join-Path $resolvedWorkingDir "consumer\install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            superseded_review_tasks_report = $supersededReviewTasksReportPath
            review_task_summary = [ordered]@{
                total_count = ""
            }
            smoke_reviewed_at = "2026-04-28T12:31:00"
            smoke_review_method = "release_summary_override"
        }
    }
}
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath

$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"
$guidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Review task count: 5 total (4 standard, 1 curated)" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke reviewed at: 2026-04-28T12:31:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review method: release_summary_override" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review note: smoke contact sheet reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid verdict: fail" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid reviewed at: 2026-04-28T12:32:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review note: fixed-grid mismatch documented" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup verdict: undetermined" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup reviewed at: 2026-04-28T12:33:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review note: section setup needs reviewer decision" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields verdict: pending_manual_review" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields reviewed at: 2026-04-28T12:34:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review note: page number fields awaiting reviewer" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict fallback verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict fallback review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict fallback reviewed at: 2026-04-28T12:35:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict fallback review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict fallback review note: curated visual evidence checked" -Label $assertion.Label
}

foreach ($fragments in @(
        @("Smoke verdict", "pass"),
        @("Smoke review status", "reviewed"),
        @("Fixed-grid verdict", "fail"),
        @("Fixed-grid review status", "reviewed"),
        @("Section page setup verdict", "undetermined"),
        @("Section page setup review status", "reviewed"),
        @("Page number fields verdict", "pending_manual_review"),
        @("Page number fields review status", "reviewed"),
        @("Curated review verdict fallback verdict", "pass"),
        @("Curated review verdict fallback review status", "reviewed")
    )) {
    Assert-LineContainsAll -Path $bodyPath -Fragments $fragments -Label "release_body.zh-CN.md"
}

foreach ($unexpectedNote in @(
        "smoke contact sheet reviewed",
        "fixed-grid mismatch documented",
        "section setup needs reviewer decision",
        "page number fields awaiting reviewer",
        "curated visual evidence checked"
    )) {
    $bodyContent = Get-Content -Raw -LiteralPath $bodyPath
    if ($bodyContent -match [regex]::Escape($unexpectedNote)) {
        throw "release_body.zh-CN.md unexpectedly rendered review note '$unexpectedNote'."
    }
}

foreach ($unexpectedProvenance in @(
        "2026-04-28T12:31:00",
        "2026-04-28T12:32:00",
        "2026-04-28T12:33:00",
        "2026-04-28T12:34:00",
        "2026-04-28T12:35:00",
        "release_summary_override",
        "operator_supplied"
    )) {
    $bodyContent = Get-Content -Raw -LiteralPath $bodyPath
    if ($bodyContent -match [regex]::Escape($unexpectedProvenance)) {
        throw "release_body.zh-CN.md unexpectedly rendered review provenance '$unexpectedProvenance'."
    }
}

foreach ($fragment in @(
        'smoke=`pass`',
        'fixed-grid=`fail`',
        'section page setup=`undetermined`',
        'page number fields=`pending_manual_review`',
        'Curated review verdict fallback=`pass`'
    )) {
    Assert-Contains -Path $shortPath -ExpectedText $fragment -Label "release_summary.zh-CN.md"
}

Write-Host "Release note bundle visual verdict fallback regression passed."
