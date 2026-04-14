<#
.SYNOPSIS
Synchronizes screenshot-backed review verdicts into gate and release summaries.

.DESCRIPTION
Reads the review_result.json files referenced by a Word visual gate summary,
computes the consolidated visual verdict, updates gate_summary.json and
gate_final_review.md, and can optionally update a release-candidate summary
plus refresh the release note bundle.

.PARAMETER GateSummaryJson
Path to report/gate_summary.json produced by run_word_visual_release_gate.ps1.

.PARAMETER ReleaseCandidateSummaryJson
Optional path to report/summary.json produced by
run_release_candidate_checks.ps1. When provided, the script also updates the
release-candidate summary/final review and can refresh the release bundle.

.PARAMETER RefreshReleaseBundle
When used with -ReleaseCandidateSummaryJson, reruns
write_release_note_bundle.ps1 after the summary sync completes.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson output\word-visual-release-gate\report\gate_summary.json `
    -ReleaseCandidateSummaryJson output\release-candidate-checks\report\summary.json `
    -RefreshReleaseBundle
#>
param(
    [string]$GateSummaryJson = "output/word-visual-release-gate/report/gate_summary.json",
    [string]$ReleaseCandidateSummaryJson = "",
    [switch]$RefreshReleaseBundle
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[sync-visual-review-verdict] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
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
        [AllowNull()]$Object,
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

function Set-PropertyValue {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        $Value
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -NotePropertyName $Name -NotePropertyValue $Value
    } else {
        $property.Value = $Value
    }
}

function Normalize-Verdict {
    param([string]$Value)

    switch ($Value.ToLowerInvariant()) {
        "pass" { return "pass" }
        "fail" { return "fail" }
        "undetermined" { return "undetermined" }
        "undecided" { return "undecided" }
        "pending_manual_review" { return "pending_manual_review" }
        default { return "undecided" }
    }
}

function Resolve-TaskReviewPaths {
    param($TaskInfo)

    $reviewResultPath = Get-OptionalPropertyValue -Object $TaskInfo -Name "review_result_path"
    $finalReviewPath = Get-OptionalPropertyValue -Object $TaskInfo -Name "final_review_path"
    $taskDir = Get-OptionalPropertyValue -Object $TaskInfo -Name "task_dir"

    if ([string]::IsNullOrWhiteSpace($reviewResultPath) -and -not [string]::IsNullOrWhiteSpace($taskDir)) {
        $reviewResultPath = Join-Path $taskDir "report\review_result.json"
    }
    if ([string]::IsNullOrWhiteSpace($finalReviewPath) -and -not [string]::IsNullOrWhiteSpace($taskDir)) {
        $finalReviewPath = Join-Path $taskDir "report\final_review.md"
    }

    return [ordered]@{
        review_result_path = $reviewResultPath
        final_review_path = $finalReviewPath
    }
}

function Read-TaskReview {
    param(
        [string]$Label,
        $TaskInfo
    )

    $paths = Resolve-TaskReviewPaths -TaskInfo $TaskInfo
    $result = [ordered]@{
        label = $Label
        task_id = Get-OptionalPropertyValue -Object $TaskInfo -Name "task_id"
        task_dir = Get-OptionalPropertyValue -Object $TaskInfo -Name "task_dir"
        review_result_path = $paths.review_result_path
        final_review_path = $paths.final_review_path
        status = "missing"
        verdict = "undecided"
        findings_count = 0
    }

    if ([string]::IsNullOrWhiteSpace($result.review_result_path) -or
        -not (Test-Path -LiteralPath $result.review_result_path)) {
        return [pscustomobject]$result
    }

    $review = Get-Content -Raw -LiteralPath $result.review_result_path | ConvertFrom-Json
    $result.status = Get-OptionalPropertyValue -Object $review -Name "status"
    $result.verdict = Normalize-Verdict -Value (Get-OptionalPropertyValue -Object $review -Name "verdict")

    $findings = Get-OptionalPropertyObject -Object $review -Name "findings"
    if ($null -ne $findings) {
        $result.findings_count = @($findings).Count
    }

    if ([string]::IsNullOrWhiteSpace($result.status)) {
        $result.status = "pending_review"
    }

    return [pscustomobject]$result
}

function Get-OverallVisualVerdict {
    param([object[]]$Reviews)

    if ($Reviews.Count -eq 0) {
        return "pending_manual_review"
    }

    $failedReviews = @($Reviews | Where-Object { $_.verdict -eq "fail" })
    if ($failedReviews.Count -gt 0) {
        return "fail"
    }

    $pendingReviews = @($Reviews | Where-Object {
            $_.verdict -eq "undecided" -or
            $_.verdict -eq "pending_manual_review" -or
            $_.status -eq "pending_review" -or
            $_.status -eq "missing"
        })
    if ($pendingReviews.Count -gt 0) {
        return "pending_manual_review"
    }

    $passedReviews = @($Reviews | Where-Object { $_.verdict -eq "pass" })
    if ($passedReviews.Count -eq $Reviews.Count) {
        return "pass"
    }

    $undeterminedReviews = @($Reviews | Where-Object { $_.verdict -eq "undetermined" })
    if ($undeterminedReviews.Count -gt 0) {
        return "undetermined"
    }

    return "pending_manual_review"
}

function New-GateFinalReviewContent {
    param(
        [string]$RepoRoot,
        $GateSummary,
        [object[]]$Reviews
    )

    $smokeFlow = Get-OptionalPropertyObject -Object $GateSummary -Name "smoke"
    $fixedGridFlow = Get-OptionalPropertyObject -Object $GateSummary -Name "fixed_grid"
    $sectionPageSetupFlow = Get-OptionalPropertyObject -Object $GateSummary -Name "section_page_setup"
    $pageNumberFieldsFlow = Get-OptionalPropertyObject -Object $GateSummary -Name "page_number_fields"

    $smokeStatusLine = if ((Get-OptionalPropertyValue -Object $smokeFlow -Name "status") -eq "completed") {
        "- Smoke flow: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $smokeFlow -Name 'docx_path')))"
    } else {
        "- Smoke flow: skipped"
    }

    $fixedGridStatusLine = if ((Get-OptionalPropertyValue -Object $fixedGridFlow -Name "status") -eq "completed") {
        "- Fixed-grid flow: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $fixedGridFlow -Name 'summary_json')))"
    } else {
        "- Fixed-grid flow: skipped"
    }
    $sectionPageSetupStatusLine = if ((Get-OptionalPropertyValue -Object $sectionPageSetupFlow -Name "status") -eq "completed") {
        "- Section page setup flow: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $sectionPageSetupFlow -Name 'summary_json')))"
    } else {
        "- Section page setup flow: skipped"
    }
    $pageNumberFieldsStatusLine = if ((Get-OptionalPropertyValue -Object $pageNumberFieldsFlow -Name "status") -eq "completed") {
        "- Page number fields flow: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $pageNumberFieldsFlow -Name 'summary_json')))"
    } else {
        "- Page number fields flow: skipped"
    }

    $readmeGallery = Get-OptionalPropertyObject -Object $GateSummary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryAssetsDir = Get-OptionalPropertyValue -Object $readmeGallery -Name "assets_dir"
    $readmeGalleryStatusLine = if ($readmeGalleryStatus -eq "completed") {
        "- README gallery refresh: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $readmeGalleryAssetsDir))"
    } elseif ([string]::IsNullOrWhiteSpace($readmeGalleryStatus)) {
        "- README gallery refresh: unknown"
    } else {
        "- README gallery refresh: $readmeGalleryStatus"
    }

    $documentTask = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks") -Name "document"
    $fixedGridTask = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks") -Name "fixed_grid"
    $sectionPageSetupTask = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks") -Name "section_page_setup"
    $pageNumberFieldsTask = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks") -Name "page_number_fields"
    $documentReview = $Reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
    $fixedGridReview = $Reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
    $sectionPageSetupReview = $Reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
    $pageNumberFieldsReview = $Reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1

    $documentTaskSummary = if ($null -ne $documentTask) {
        @(
            "- Document task id: $(Get-OptionalPropertyValue -Object $documentTask -Name 'task_id')"
            "- Document task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $documentTask -Name 'task_dir'))"
            "- Document verdict: $($documentReview.verdict)"
            "- Document review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $documentReview.review_result_path)"
            "- Document final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $documentReview.final_review_path)"
        ) -join [Environment]::NewLine
    } else {
        "- Document review task: not available"
    }

    $fixedGridTaskSummary = if ($null -ne $fixedGridTask) {
        @(
            "- Fixed-grid task id: $(Get-OptionalPropertyValue -Object $fixedGridTask -Name 'task_id')"
            "- Fixed-grid task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $fixedGridTask -Name 'task_dir'))"
            "- Fixed-grid verdict: $($fixedGridReview.verdict)"
            "- Fixed-grid review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $fixedGridReview.review_result_path)"
            "- Fixed-grid final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $fixedGridReview.final_review_path)"
        ) -join [Environment]::NewLine
    } else {
        "- Fixed-grid review task: not available"
    }
    $sectionPageSetupTaskSummary = if ($null -ne $sectionPageSetupTask) {
        @(
            "- Section page setup task id: $(Get-OptionalPropertyValue -Object $sectionPageSetupTask -Name 'task_id')"
            "- Section page setup task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $sectionPageSetupTask -Name 'task_dir'))"
            "- Section page setup verdict: $($sectionPageSetupReview.verdict)"
            "- Section page setup review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $sectionPageSetupReview.review_result_path)"
            "- Section page setup final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $sectionPageSetupReview.final_review_path)"
        ) -join [Environment]::NewLine
    } else {
        "- Section page setup review task: not available"
    }
    $pageNumberFieldsTaskSummary = if ($null -ne $pageNumberFieldsTask) {
        @(
            "- Page number fields task id: $(Get-OptionalPropertyValue -Object $pageNumberFieldsTask -Name 'task_id')"
            "- Page number fields task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $pageNumberFieldsTask -Name 'task_dir'))"
            "- Page number fields verdict: $($pageNumberFieldsReview.verdict)"
            "- Page number fields review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $pageNumberFieldsReview.review_result_path)"
            "- Page number fields final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $pageNumberFieldsReview.final_review_path)"
        ) -join [Environment]::NewLine
    } else {
        "- Page number fields review task: not available"
    }

    $nextSteps = switch (Get-OptionalPropertyValue -Object $GateSummary -Name "visual_verdict") {
        "pass" {
            @(
                "1. Screenshot-backed document, fixed-grid, section page setup, and page number fields reviews are signed off as pass."
                "2. If any evidence is regenerated later, rerun this sync script before shipping."
            ) -join [Environment]::NewLine
        }
        "fail" {
            @(
                "1. Do not ship until the failing screenshot-backed visual regressions are repaired."
                "2. Re-render evidence, update task review_result.json files, then rerun this sync script."
            ) -join [Environment]::NewLine
        }
        "undetermined" {
            @(
                "1. At least one review ended as undetermined, so release signoff is still blocked."
                "2. Regenerate missing evidence or rerun the affected task review before shipping."
            ) -join [Environment]::NewLine
        }
        default {
            @(
                "1. Finish all screenshot-backed task reviews."
                "2. Rerun sync_visual_review_verdict.ps1 to promote the final verdict."
            ) -join [Environment]::NewLine
        }
    }

    return @"
# Word visual release gate

- Generated at: $(Get-Date -Format s)
- Workspace: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $RepoRoot)
- Gate output directory: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $GateSummary -Name 'gate_output_dir'))
- Gate summary JSON: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $resolvedGateSummaryPath)
- Execution status: $(Get-OptionalPropertyValue -Object $GateSummary -Name 'execution_status')
- Visual review status: $(Get-OptionalPropertyValue -Object $GateSummary -Name 'visual_verdict')

## Included flows

$smokeStatusLine
$fixedGridStatusLine
$sectionPageSetupStatusLine
$pageNumberFieldsStatusLine
$readmeGalleryStatusLine

## Review tasks

$documentTaskSummary
$fixedGridTaskSummary
$sectionPageSetupTaskSummary
$pageNumberFieldsTaskSummary

## Next steps

$nextSteps
"@
}

function New-ReleaseCandidateFinalReviewContent {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $readmeGallery = Get-OptionalPropertyObject -Object $Summary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryStatusLine = switch ($readmeGalleryStatus) {
        "completed" { "- README gallery refresh: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $readmeGallery -Name 'assets_dir')))" }
        "visual_gate_skipped" { "- README gallery refresh: unavailable (visual gate skipped)" }
        "pending" { "- README gallery refresh: pending" }
        "not_requested" { "- README gallery refresh: not requested" }
        default {
            if ([string]::IsNullOrWhiteSpace($readmeGalleryStatus)) {
                "- README gallery refresh: unknown"
            } else {
                "- README gallery refresh: $readmeGalleryStatus"
            }
        }
    }

    return @"
# Release Candidate Checks

- Generated at: $(Get-Date -Format s)
- Workspace: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $RepoRoot)
- Summary JSON: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $resolvedReleaseSummaryPath)
- Execution status: $(Get-OptionalPropertyValue -Object $Summary -Name 'execution_status')
- Visual verdict: $(Get-OptionalPropertyValue -Object $Summary -Name 'visual_verdict')
- Failed step: $(Get-OptionalPropertyValue -Object $Summary -Name 'failed_step')
- Error: $(Get-OptionalPropertyValue -Object $Summary -Name 'error')
- MSVC bootstrap mode: $(Get-OptionalPropertyValue -Object $Summary -Name 'msvc_bootstrap_mode')

## Step status

- Configure: $(Get-OptionalPropertyValue -Object $Summary.steps.configure -Name 'status')
- Build: $(Get-OptionalPropertyValue -Object $Summary.steps.build -Name 'status')
- Tests: $(Get-OptionalPropertyValue -Object $Summary.steps.tests -Name 'status')
- Install smoke: $(Get-OptionalPropertyValue -Object $Summary.steps.install_smoke -Name 'status')
- Visual gate: $(Get-OptionalPropertyValue -Object $Summary.steps.visual_gate -Name 'status')
$readmeGalleryStatusLine

## Key outputs

- Build directory: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'build_dir'))
- Install directory: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'install_dir'))
- Consumer build directory: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'consumer_build_dir'))
- Visual gate output: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'gate_output_dir'))
- Review task root: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'task_output_root'))
- Release handoff: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'release_handoff'))
- Release body: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'release_body_zh_cn'))
- Release summary: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'release_summary_zh_cn'))
- Artifact guide: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'artifact_guide'))
- Reviewer checklist: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'reviewer_checklist'))
- Start here: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $Summary -Name 'start_here'))
"@
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $GateSummaryJson
Assert-PathExists -Path $resolvedGateSummaryPath -Label "gate summary JSON"
$resolvedReleaseSummaryPath = if ([string]::IsNullOrWhiteSpace($ReleaseCandidateSummaryJson)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseCandidateSummaryJson
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Assert-PathExists -Path $resolvedReleaseSummaryPath -Label "release-candidate summary JSON"
}

$gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
$reviewTasks = Get-OptionalPropertyObject -Object $gateSummary -Name "review_tasks"
$documentTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "document"
$fixedGridTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "fixed_grid"
$sectionPageSetupTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "section_page_setup"
$pageNumberFieldsTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "page_number_fields"
$reviews = @()
if ($null -ne $documentTask) {
    $reviews += Read-TaskReview -Label "document" -TaskInfo $documentTask
}
if ($null -ne $fixedGridTask) {
    $reviews += Read-TaskReview -Label "fixed_grid" -TaskInfo $fixedGridTask
}
if ($null -ne $sectionPageSetupTask) {
    $reviews += Read-TaskReview -Label "section_page_setup" -TaskInfo $sectionPageSetupTask
}
if ($null -ne $pageNumberFieldsTask) {
    $reviews += Read-TaskReview -Label "page_number_fields" -TaskInfo $pageNumberFieldsTask
}
if ($reviews.Count -eq 0) {
    throw "Gate summary does not contain review-task metadata. Re-run the gate without -SkipReviewTasks."
}

$overallVerdict = Get-OverallVisualVerdict -Reviews $reviews
Write-Step "Computed visual verdict: $overallVerdict"

$manualReviewSummary = [pscustomobject]@{
    updated_at = (Get-Date).ToString("s")
    consolidated_verdict = $overallVerdict
    tasks = [pscustomobject]@{
        document = $reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
        fixed_grid = $reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
        section_page_setup = $reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
        page_number_fields = $reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1
    }
}

Set-PropertyValue -Object $gateSummary -Name "visual_verdict" -Value $overallVerdict
Set-PropertyValue -Object $gateSummary -Name "manual_review" -Value $manualReviewSummary

$gateFinalReviewPath = Join-Path (Get-OptionalPropertyValue -Object $gateSummary -Name "report_dir") "gate_final_review.md"
$gateFinalReviewContent = New-GateFinalReviewContent -RepoRoot $repoRoot -GateSummary $gateSummary -Reviews $reviews
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $resolvedGateSummaryPath -Encoding UTF8
$gateFinalReviewContent | Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8
Write-Step "Updated gate summary and final review"

if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    $summary = Get-Content -Raw -LiteralPath $resolvedReleaseSummaryPath | ConvertFrom-Json
    $readmeGallery = Get-OptionalPropertyObject -Object $gateSummary -Name "readme_gallery"

    Set-PropertyValue -Object $summary -Name "visual_verdict" -Value $overallVerdict
    Set-PropertyValue -Object $summary -Name "readme_gallery" -Value $readmeGallery
    Set-PropertyValue -Object $summary.steps.visual_gate -Name "visual_verdict" -Value $overallVerdict

    $documentReview = $reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
    $fixedGridReview = $reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
    $sectionPageSetupReview = $reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
    $pageNumberFieldsReview = $reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1
    if ($null -ne $documentReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "document_verdict" -Value $documentReview.verdict
    }
    if ($null -ne $fixedGridReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "fixed_grid_verdict" -Value $fixedGridReview.verdict
    }
    if ($null -ne $sectionPageSetupReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "section_page_setup_verdict" -Value $sectionPageSetupReview.verdict
    }
    if ($null -ne $pageNumberFieldsReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "page_number_fields_verdict" -Value $pageNumberFieldsReview.verdict
    }

    ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $resolvedReleaseSummaryPath -Encoding UTF8

    $releaseFinalReviewPath = Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) "final_review.md"
    $releaseFinalReviewContent = New-ReleaseCandidateFinalReviewContent -RepoRoot $repoRoot -Summary $summary
    $releaseFinalReviewContent | Set-Content -LiteralPath $releaseFinalReviewPath -Encoding UTF8
    Write-Step "Updated release-candidate summary and final review"

    if ($RefreshReleaseBundle) {
        $releaseNoteBundleScript = Join-Path $repoRoot "scripts\write_release_note_bundle.ps1"
        Invoke-ChildPowerShell -ScriptPath $releaseNoteBundleScript `
            -Arguments @(
                "-SummaryJson"
                $resolvedReleaseSummaryPath
            ) `
            -FailureMessage "Failed to refresh the release note bundle."
        Write-Step "Refreshed release note bundle"
    }
}

Write-Host "Gate summary: $resolvedGateSummaryPath"
Write-Host "Gate final review: $gateFinalReviewPath"
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Write-Host "Release summary: $resolvedReleaseSummaryPath"
    Write-Host "Release final review: $(Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) 'final_review.md')"
}
