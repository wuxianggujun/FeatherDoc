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

function Convert-ReviewTimestamp {
    param([AllowNull()]$Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Get-DisplayValue {
    param([AllowNull()]$Value)

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return "(not available)"
    }

    return [string]$Value
}

function Set-GateFlowReviewMetadata {
    param(
        [AllowNull()]$FlowInfo,
        [AllowNull()]$Review
    )

    if ($null -eq $FlowInfo -or $null -eq $Review) {
        return
    }

    Set-PropertyValue -Object $FlowInfo -Name "review_status" -Value $Review.status
    Set-PropertyValue -Object $FlowInfo -Name "review_verdict" -Value $Review.verdict
    Set-PropertyValue -Object $FlowInfo -Name "reviewed_at" -Value $Review.reviewed_at
    Set-PropertyValue -Object $FlowInfo -Name "review_method" -Value $Review.review_method
    Set-PropertyValue -Object $FlowInfo -Name "review_note" -Value $Review.review_note
}

function Set-ReleaseVisualReviewMetadata {
    param(
        [Parameter(Mandatory = $true)]$VisualGateStep,
        [string]$Prefix,
        [AllowNull()]$Review
    )

    if ($null -eq $Review) {
        return
    }

    Set-PropertyValue -Object $VisualGateStep -Name ("{0}_review_status" -f $Prefix) -Value $Review.status
    Set-PropertyValue -Object $VisualGateStep -Name ("{0}_verdict" -f $Prefix) -Value $Review.verdict
    Set-PropertyValue -Object $VisualGateStep -Name ("{0}_reviewed_at" -f $Prefix) -Value $Review.reviewed_at
    Set-PropertyValue -Object $VisualGateStep -Name ("{0}_review_method" -f $Prefix) -Value $Review.review_method
    Set-PropertyValue -Object $VisualGateStep -Name ("{0}_review_note" -f $Prefix) -Value $Review.review_note
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
        [string]$DisplayLabel = "",
        [string]$Id = "",
        $TaskInfo
    )

    $paths = Resolve-TaskReviewPaths -TaskInfo $TaskInfo
    $result = [ordered]@{
        label = $Label
        display_label = if ([string]::IsNullOrWhiteSpace($DisplayLabel)) { $Label } else { $DisplayLabel }
        id = $Id
        task_id = Get-OptionalPropertyValue -Object $TaskInfo -Name "task_id"
        task_dir = Get-OptionalPropertyValue -Object $TaskInfo -Name "task_dir"
        review_result_path = $paths.review_result_path
        final_review_path = $paths.final_review_path
        status = "missing"
        verdict = "undecided"
        reviewed_at = ""
        review_method = ""
        review_note = ""
        findings_count = 0
    }

    if ([string]::IsNullOrWhiteSpace($result.review_result_path) -or
        -not (Test-Path -LiteralPath $result.review_result_path)) {
        return [pscustomobject]$result
    }

    $review = Get-Content -Raw -LiteralPath $result.review_result_path | ConvertFrom-Json
    $result.status = Get-OptionalPropertyValue -Object $review -Name "status"
    $result.verdict = Normalize-Verdict -Value (Get-OptionalPropertyValue -Object $review -Name "verdict")
    $result.reviewed_at = Convert-ReviewTimestamp -Value (Get-OptionalPropertyObject -Object $review -Name "reviewed_at")
    $result.review_method = Get-OptionalPropertyValue -Object $review -Name "review_method"
    $result.review_note = Get-OptionalPropertyValue -Object $review -Name "review_note"

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


function New-GateReleaseSummaryDiscoveryMarkdown {
    param(
        [string]$RepoRoot,
        [AllowNull()]$GateSummary
    )

    $discovery = Get-OptionalPropertyObject -Object $GateSummary -Name "release_summary_discovery"
    if ($null -eq $discovery) {
        return ""
    }

    $selectedSummaryPath = Get-OptionalPropertyValue -Object $discovery -Name "selected_summary_path"
    if ([string]::IsNullOrWhiteSpace($selectedSummaryPath)) {
        $selectedSummaryPath = Get-OptionalPropertyValue -Object $GateSummary -Name "selected_release_summary_path"
    }

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("## Release summary discovery")
    [void]$lines.Add("")
    [void]$lines.Add("- Mode: $(Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $discovery -Name 'mode'))")
    [void]$lines.Add("- Reason: $(Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $discovery -Name 'reason'))")
    [void]$lines.Add("- Selected release summary: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $selectedSummaryPath)")
    [void]$lines.Add("- Output search root: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $discovery -Name 'output_search_root'))")
    [void]$lines.Add("- Release bundle refresh requested: $(Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $discovery -Name 'release_bundle_refresh_requested'))")
    [void]$lines.Add("")

    return ($lines -join [Environment]::NewLine)
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
    $curatedFlows = @()
    $curatedFlowsProperty = Get-OptionalPropertyObject -Object $GateSummary -Name "curated_visual_regressions"
    if ($null -ne $curatedFlowsProperty) {
        $curatedFlows = @($curatedFlowsProperty)
    }

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
    $curatedVisualStatusLines = if ($curatedFlows.Count -gt 0) {
        ($curatedFlows | ForEach-Object {
                if ((Get-OptionalPropertyValue -Object $_ -Name "status") -eq "completed") {
                    "- $($_.label) flow: completed ($(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $_ -Name 'summary_json')))"
                } else {
                    "- $($_.label) flow: $(Get-OptionalPropertyValue -Object $_ -Name 'status')"
                }
            }) -join [Environment]::NewLine
    } else {
        "- Curated visual regression bundles: not requested"
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

    $reviewTasks = Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks"
    $documentTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "document"
    $fixedGridTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "fixed_grid"
    $sectionPageSetupTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "section_page_setup"
    $pageNumberFieldsTask = Get-OptionalPropertyObject -Object $reviewTasks -Name "page_number_fields"
    $curatedReviewTasks = @()
    $curatedReviewTasksProperty = Get-OptionalPropertyObject -Object $reviewTasks -Name "curated_visual_regressions"
    if ($null -ne $curatedReviewTasksProperty) {
        $curatedReviewTasks = @($curatedReviewTasksProperty)
    }
    $documentReview = $Reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
    $fixedGridReview = $Reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
    $sectionPageSetupReview = $Reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
    $pageNumberFieldsReview = $Reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1

    $documentTaskSummary = if ($null -ne $documentTask) {
        @(
            "- Document task id: $(Get-OptionalPropertyValue -Object $documentTask -Name 'task_id')"
            "- Document task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $documentTask -Name 'task_dir'))"
            "- Document verdict: $($documentReview.verdict)"
            "- Document review status: $(Get-DisplayValue -Value $documentReview.status)"
            "- Document reviewed at: $(Get-DisplayValue -Value $documentReview.reviewed_at)"
            "- Document review method: $(Get-DisplayValue -Value $documentReview.review_method)"
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
            "- Fixed-grid review status: $(Get-DisplayValue -Value $fixedGridReview.status)"
            "- Fixed-grid reviewed at: $(Get-DisplayValue -Value $fixedGridReview.reviewed_at)"
            "- Fixed-grid review method: $(Get-DisplayValue -Value $fixedGridReview.review_method)"
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
            "- Section page setup review status: $(Get-DisplayValue -Value $sectionPageSetupReview.status)"
            "- Section page setup reviewed at: $(Get-DisplayValue -Value $sectionPageSetupReview.reviewed_at)"
            "- Section page setup review method: $(Get-DisplayValue -Value $sectionPageSetupReview.review_method)"
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
            "- Page number fields review status: $(Get-DisplayValue -Value $pageNumberFieldsReview.status)"
            "- Page number fields reviewed at: $(Get-DisplayValue -Value $pageNumberFieldsReview.reviewed_at)"
            "- Page number fields review method: $(Get-DisplayValue -Value $pageNumberFieldsReview.review_method)"
            "- Page number fields review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $pageNumberFieldsReview.review_result_path)"
            "- Page number fields final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $pageNumberFieldsReview.final_review_path)"
        ) -join [Environment]::NewLine
    } else {
        "- Page number fields review task: not available"
    }
    $curatedVisualTaskSummary = if ($curatedReviewTasks.Count -gt 0) {
        ($curatedReviewTasks | ForEach-Object {
                $bundleId = Get-OptionalPropertyValue -Object $_ -Name "id"
                $bundleLabel = Get-OptionalPropertyValue -Object $_ -Name "label"
                $bundleTask = Get-OptionalPropertyObject -Object $_ -Name "task"
                $bundleReview = $Reviews | Where-Object { $_.label -eq "curated:$bundleId" } | Select-Object -First 1
                $bundleVerdict = if ($null -ne $bundleReview) { $bundleReview.verdict } else { "undecided" }
                $bundleReviewResultPath = if ($null -ne $bundleReview) {
                    $bundleReview.review_result_path
                } else {
                    Get-OptionalPropertyValue -Object $bundleTask -Name "review_result_path"
                }
                $bundleFinalReviewPath = if ($null -ne $bundleReview) {
                    $bundleReview.final_review_path
                } else {
                    Get-OptionalPropertyValue -Object $bundleTask -Name "final_review_path"
                }

                @(
                    "- $bundleLabel task id: $(Get-OptionalPropertyValue -Object $bundleTask -Name 'task_id')"
                    "- $bundleLabel task dir: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path (Get-OptionalPropertyValue -Object $bundleTask -Name 'task_dir'))"
                    "- $bundleLabel verdict: $bundleVerdict"
                    "- $bundleLabel review status: $(Get-DisplayValue -Value $bundleReview.status)"
                    "- $bundleLabel reviewed at: $(Get-DisplayValue -Value $bundleReview.reviewed_at)"
                    "- $bundleLabel review method: $(Get-DisplayValue -Value $bundleReview.review_method)"
                    "- $bundleLabel review result: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $bundleReviewResultPath)"
                    "- $bundleLabel final review: $(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $bundleFinalReviewPath)"
                ) -join [Environment]::NewLine
            }) -join [Environment]::NewLine
    } else {
        "- Curated visual regression review tasks: not available"
    }

    $releaseSummaryDiscoverySection = New-GateReleaseSummaryDiscoveryMarkdown -RepoRoot $RepoRoot -GateSummary $GateSummary

    $nextSteps = switch (Get-OptionalPropertyValue -Object $GateSummary -Name "visual_verdict") {
        "pass" {
            @(
                "1. All screenshot-backed visual reviews, including curated regression bundles, are signed off as pass."
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
$curatedVisualStatusLines
$readmeGalleryStatusLine

## Review tasks

$documentTaskSummary
$fixedGridTaskSummary
$sectionPageSetupTaskSummary
$pageNumberFieldsTaskSummary
$curatedVisualTaskSummary

$releaseSummaryDiscoverySection## Next steps

$nextSteps
"@
}

function Get-ReleaseVisualReviewProvenanceMarkdown {
    param(
        [string]$RepoRoot,
        [AllowNull()]$VisualGateStep
    )

    if ($null -eq $VisualGateStep) {
        return ""
    }

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $standardFlows = @(
        [pscustomobject]@{ Label = "Smoke"; Prefix = "smoke"; TaskProperty = "document_task_dir" },
        [pscustomobject]@{ Label = "Fixed-grid"; Prefix = "fixed_grid"; TaskProperty = "fixed_grid_task_dir" },
        [pscustomobject]@{ Label = "Section page setup"; Prefix = "section_page_setup"; TaskProperty = "section_page_setup_task_dir" },
        [pscustomobject]@{ Label = "Page number fields"; Prefix = "page_number_fields"; TaskProperty = "page_number_fields_task_dir" }
    )

    foreach ($flow in $standardFlows) {
        $verdict = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_verdict" -f $flow.Prefix)
        $reviewStatus = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_status" -f $flow.Prefix)
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyObject -Object $VisualGateStep -Name ("{0}_reviewed_at" -f $flow.Prefix))
        $reviewMethod = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_method" -f $flow.Prefix)
        if ([string]::IsNullOrWhiteSpace($verdict) -and
            [string]::IsNullOrWhiteSpace($reviewStatus) -and
            [string]::IsNullOrWhiteSpace($reviewedAt) -and
            [string]::IsNullOrWhiteSpace($reviewMethod)) {
            continue
        }

        $line = "- {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $flow.Label,
            (Get-DisplayValue -Value $verdict),
            (Get-DisplayValue -Value $reviewStatus),
            (Get-DisplayValue -Value $reviewedAt),
            (Get-DisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $VisualGateStep -Name $flow.TaskProperty
        if (-not [string]::IsNullOrWhiteSpace($taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }
        [void]$lines.Add($line)
    }

    $curatedEntries = @(Get-OptionalPropertyObject -Object $VisualGateStep -Name "curated_visual_regressions")
    foreach ($entry in $curatedEntries) {
        $verdict = Get-OptionalPropertyValue -Object $entry -Name "verdict"
        $reviewStatus = Get-OptionalPropertyValue -Object $entry -Name "review_status"
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyObject -Object $entry -Name "reviewed_at")
        $reviewMethod = Get-OptionalPropertyValue -Object $entry -Name "review_method"
        if ([string]::IsNullOrWhiteSpace($verdict) -and
            [string]::IsNullOrWhiteSpace($reviewStatus) -and
            [string]::IsNullOrWhiteSpace($reviewedAt) -and
            [string]::IsNullOrWhiteSpace($reviewMethod)) {
            continue
        }

        $label = Get-OptionalPropertyValue -Object $entry -Name "label"
        if ([string]::IsNullOrWhiteSpace($label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "id"
        }
        if ([string]::IsNullOrWhiteSpace($label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "task_id"
        }
        if ([string]::IsNullOrWhiteSpace($label)) {
            $label = "Curated visual regression"
        }

        $line = "- Curated - {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $label,
            (Get-DisplayValue -Value $verdict),
            (Get-DisplayValue -Value $reviewStatus),
            (Get-DisplayValue -Value $reviewedAt),
            (Get-DisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $entry -Name "task_dir"
        if (-not [string]::IsNullOrWhiteSpace($taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }
        [void]$lines.Add($line)
    }

    if ($lines.Count -eq 0) {
        return ""
    }

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    [void]$sectionLines.Add("## Visual review provenance")
    [void]$sectionLines.Add("")
    foreach ($line in $lines) {
        [void]$sectionLines.Add($line)
    }
    [void]$sectionLines.Add("")
    return ($sectionLines -join [Environment]::NewLine)
}

function New-ReleaseCandidateFinalReviewContent {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $readmeGallery = Get-OptionalPropertyObject -Object $Summary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $reviewTaskSummary = Get-OptionalPropertyObject -Object $Summary.steps.visual_gate -Name "review_task_summary"
    $reviewTaskSummaryLine = ""
    if ($null -ne $reviewTaskSummary) {
        $reviewTaskSummaryLine = "- Review task count: {0} total ({1} standard, {2} curated)" -f `
            (Get-OptionalPropertyValue -Object $reviewTaskSummary -Name "total_count"),
            (Get-OptionalPropertyValue -Object $reviewTaskSummary -Name "standard_count"),
            (Get-OptionalPropertyValue -Object $reviewTaskSummary -Name "curated_count")
    }
    $visualReviewProvenance = Get-ReleaseVisualReviewProvenanceMarkdown -RepoRoot $RepoRoot -VisualGateStep $Summary.steps.visual_gate
    $releaseSummaryDiscoverySection = New-GateReleaseSummaryDiscoveryMarkdown -RepoRoot $RepoRoot -GateSummary $Summary
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
$reviewTaskSummaryLine
$readmeGalleryStatusLine

$visualReviewProvenance$releaseSummaryDiscoverySection## Key outputs

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
$curatedVisualReviewTasks = @()
$curatedVisualReviewTasksProperty = Get-OptionalPropertyObject -Object $reviewTasks -Name "curated_visual_regressions"
if ($null -ne $curatedVisualReviewTasksProperty) {
    $curatedVisualReviewTasks = @($curatedVisualReviewTasksProperty)
}
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
foreach ($curatedBundleTask in $curatedVisualReviewTasks) {
    $bundleId = Get-OptionalPropertyValue -Object $curatedBundleTask -Name "id"
    $bundleLabel = Get-OptionalPropertyValue -Object $curatedBundleTask -Name "label"
    $taskInfo = Get-OptionalPropertyObject -Object $curatedBundleTask -Name "task"
    if ($null -ne $taskInfo) {
        $reviews += Read-TaskReview -Label "curated:$bundleId" -DisplayLabel $bundleLabel -Id $bundleId -TaskInfo $taskInfo
    }
}
if ($reviews.Count -eq 0) {
    throw "Gate summary does not contain review-task metadata. Re-run the gate without -SkipReviewTasks."
}

$overallVerdict = Get-OverallVisualVerdict -Reviews $reviews
Write-Step "Computed visual verdict: $overallVerdict"

$documentReview = $reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
$fixedGridReview = $reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
$sectionPageSetupReview = $reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
$pageNumberFieldsReview = $reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1
$curatedVisualReviews = @($reviews | Where-Object { $_.label -like "curated:*" })

Set-GateFlowReviewMetadata -Flow (Get-OptionalPropertyObject -Object $gateSummary -Name "smoke") -Review $documentReview
Set-GateFlowReviewMetadata -Flow (Get-OptionalPropertyObject -Object $gateSummary -Name "fixed_grid") -Review $fixedGridReview
Set-GateFlowReviewMetadata -Flow (Get-OptionalPropertyObject -Object $gateSummary -Name "section_page_setup") -Review $sectionPageSetupReview
Set-GateFlowReviewMetadata -Flow (Get-OptionalPropertyObject -Object $gateSummary -Name "page_number_fields") -Review $pageNumberFieldsReview
$curatedGateFlows = @(Get-OptionalPropertyObject -Object $gateSummary -Name "curated_visual_regressions")
foreach ($curatedGateFlow in $curatedGateFlows) {
    $bundleId = Get-OptionalPropertyValue -Object $curatedGateFlow -Name "id"
    $bundleReview = $curatedVisualReviews | Where-Object { $_.id -eq $bundleId -or $_.label -eq "curated:$bundleId" } | Select-Object -First 1
    Set-GateFlowReviewMetadata -Flow $curatedGateFlow -Review $bundleReview
}

$manualReviewSummary = [pscustomobject]@{
    updated_at = (Get-Date).ToString("s")
    consolidated_verdict = $overallVerdict
    tasks = [pscustomobject]@{
        document = $reviews | Where-Object { $_.label -eq "document" } | Select-Object -First 1
        fixed_grid = $reviews | Where-Object { $_.label -eq "fixed_grid" } | Select-Object -First 1
        section_page_setup = $reviews | Where-Object { $_.label -eq "section_page_setup" } | Select-Object -First 1
        page_number_fields = $reviews | Where-Object { $_.label -eq "page_number_fields" } | Select-Object -First 1
        curated_visual_regressions = @($reviews | Where-Object { $_.label -like "curated:*" })
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
    $reviewTaskSummary = Get-OptionalPropertyObject -Object $gateSummary -Name "review_task_summary"

    Set-PropertyValue -Object $summary -Name "visual_verdict" -Value $overallVerdict
    Set-PropertyValue -Object $summary -Name "readme_gallery" -Value $readmeGallery
    Set-PropertyValue -Object $summary.steps.visual_gate -Name "visual_verdict" -Value $overallVerdict
    if ($null -ne $reviewTaskSummary) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "review_task_summary" -Value $reviewTaskSummary
    }

    if ($null -ne $documentReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "document_verdict" -Value $documentReview.verdict
        Set-ReleaseVisualReviewMetadata -VisualGateStep $summary.steps.visual_gate -Prefix "smoke" -Review $documentReview
    }
    if ($null -ne $fixedGridReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "fixed_grid_verdict" -Value $fixedGridReview.verdict
        Set-ReleaseVisualReviewMetadata -VisualGateStep $summary.steps.visual_gate -Prefix "fixed_grid" -Review $fixedGridReview
    }
    if ($null -ne $sectionPageSetupReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "section_page_setup_verdict" -Value $sectionPageSetupReview.verdict
        Set-ReleaseVisualReviewMetadata -VisualGateStep $summary.steps.visual_gate -Prefix "section_page_setup" -Review $sectionPageSetupReview
    }
    if ($null -ne $pageNumberFieldsReview) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "page_number_fields_verdict" -Value $pageNumberFieldsReview.verdict
        Set-ReleaseVisualReviewMetadata -VisualGateStep $summary.steps.visual_gate -Prefix "page_number_fields" -Review $pageNumberFieldsReview
    }
    if ($curatedVisualReviews.Count -gt 0) {
        Set-PropertyValue -Object $summary.steps.visual_gate -Name "curated_visual_regressions" -Value @(
            $curatedVisualReviews | ForEach-Object {
                [pscustomobject]@{
                    id = $_.id
                    label = $_.display_label
                    verdict = $_.verdict
                    review_status = $_.status
                    reviewed_at = $_.reviewed_at
                    review_method = $_.review_method
                    review_note = $_.review_note
                    task_id = $_.task_id
                    task_dir = $_.task_dir
                    review_result_path = $_.review_result_path
                    final_review_path = $_.final_review_path
                }
            }
        )
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
