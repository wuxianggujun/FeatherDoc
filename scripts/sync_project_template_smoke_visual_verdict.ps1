<#
.SYNOPSIS
Synchronizes project template smoke summaries with the latest visual review verdicts.

.DESCRIPTION
Reads a `summary.json` produced by `run_project_template_smoke.ps1`, reloads the
`review_result.json` files referenced by each visual-smoke entry, refreshes the
entry-level review metadata, recomputes aggregate summary status, and rewrites
both `summary.json` and `summary.md`.
#>
param(
    [string]$SummaryJson = "output/project-template-smoke/summary.json",
    [string]$SummaryMarkdown = "",
    [string]$ReleaseCandidateSummaryJson = "",
    [switch]$RefreshReleaseBundle
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke-sync] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    return Resolve-ProjectTemplateSmokePath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
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

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "$Label was not found: $Path"
    }
}

function Get-RepoRelativeDisplayPath {
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

function Set-ProjectTemplateSmokePropertyValue {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -NotePropertyName $Name -NotePropertyValue $Value
    } else {
        $property.Value = $Value
    }
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

function Normalize-ProjectTemplateSmokeVisualVerdict {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "undecided"
    }

    switch ($Value.ToLowerInvariant()) {
        "pass" { return "pass" }
        "fail" { return "fail" }
        "undetermined" { return "undetermined" }
        "undecided" { return "undecided" }
        "pending_manual_review" { return "pending_manual_review" }
        "skipped" { return "skipped" }
        default { return "undecided" }
    }
}

function Get-ProjectTemplateSmokeVisualReviewState {
    param(
        [string]$ReviewStatus,
        [string]$ReviewVerdict
    )

    $normalizedStatus = if ([string]::IsNullOrWhiteSpace($ReviewStatus)) {
        "missing"
    } else {
        $ReviewStatus
    }
    $normalizedVerdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value $ReviewVerdict

    $failed = ($normalizedStatus -eq "failed" -or $normalizedVerdict -eq "fail")
    $pending = (-not $failed) -and (
        $normalizedStatus -eq "missing" -or
        $normalizedStatus -eq "pending_review" -or
        $normalizedVerdict -eq "undecided" -or
        $normalizedVerdict -eq "pending_manual_review"
    )
    $undetermined = (-not $failed) -and (-not $pending) -and $normalizedVerdict -eq "undetermined"
    $passed = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and $normalizedVerdict -eq "pass"
    $skipped = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and (-not $passed) -and (
        $normalizedStatus -eq "skipped" -or
        $normalizedVerdict -eq "skipped"
    )

    return [pscustomobject]@{
        review_status = $normalizedStatus
        review_verdict = $normalizedVerdict
        failed = $failed
        manual_review_pending = $pending
        review_undetermined = $undetermined
        review_passed = $passed
        skipped = $skipped
    }
}

function Get-ProjectTemplateSmokeEntryStatus {
    param(
        [bool]$EntryPassed,
        [bool]$ManualReviewPending,
        [bool]$VisualReviewUndetermined
    )

    if (-not $EntryPassed) {
        return "failed"
    }
    if ($ManualReviewPending) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndetermined) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeOverallStatus {
    param(
        [bool]$SummaryPassed,
        [int]$ManualReviewPendingCount,
        [int]$VisualReviewUndeterminedCount
    )

    if (-not $SummaryPassed) {
        return "failed"
    }
    if ($ManualReviewPendingCount -gt 0) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndeterminedCount -gt 0) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeVisualVerdict {
    param([object[]]$VisualSmokeResults)

    if ($VisualSmokeResults.Count -eq 0) {
        return "not_applicable"
    }

    $states = @(
        foreach ($visualSmokeResult in $VisualSmokeResults) {
            Get-ProjectTemplateSmokeVisualReviewState `
                -ReviewStatus (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmokeResult -Name "review_status") `
                -ReviewVerdict (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmokeResult -Name "review_verdict")
        }
    )

    if (@($states | Where-Object { $_.failed }).Count -gt 0) {
        return "fail"
    }
    if (@($states | Where-Object { $_.manual_review_pending }).Count -gt 0) {
        return "pending_manual_review"
    }
    if (@($states | Where-Object { $_.review_undetermined }).Count -gt 0) {
        return "undetermined"
    }
    if (@($states | Where-Object { $_.review_passed }).Count -gt 0) {
        return "pass"
    }
    if (@($states | Where-Object { $_.skipped }).Count -eq $states.Count) {
        return "skipped"
    }

    return "pending_manual_review"
}

function Get-ProjectTemplateSmokeEntryDerivedState {
    param($Entry)

    $entryFailed = @(
        Get-ProjectTemplateSmokeArrayProperty -Object $Entry -Name "issues"
    ).Count -gt 0
    $manualReviewPending = $false
    $visualReviewUndetermined = $false

    $checks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Entry -Name "checks"
    foreach ($validation in @(Get-ProjectTemplateSmokeArrayProperty -Object $checks -Name "template_validations")) {
        if (-not [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $validation -Name "passed")) {
            $entryFailed = $true
            break
        }
    }

    $schemaValidation = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_validation"
    if ($null -ne $schemaValidation -and
        [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaValidation -Name "enabled") -and
        -not [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaValidation -Name "passed")) {
        $entryFailed = $true
    }

    $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_baseline"
    if ($null -ne $schemaBaseline -and
        [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "enabled") -and
        -not [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "matches")) {
        $entryFailed = $true
    }

    $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "visual_smoke"
    if ($null -ne $visualSmoke -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $visualSmoke -Name "enabled")) {
        $visualReviewState = Get-ProjectTemplateSmokeVisualReviewState `
            -ReviewStatus (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "review_status") `
            -ReviewVerdict (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "review_verdict")
        if ($visualReviewState.failed) {
            $entryFailed = $true
        }
        $manualReviewPending = $visualReviewState.manual_review_pending
        $visualReviewUndetermined = $visualReviewState.review_undetermined
    }

    $status = Get-ProjectTemplateSmokeEntryStatus `
        -EntryPassed (-not $entryFailed) `
        -ManualReviewPending $manualReviewPending `
        -VisualReviewUndetermined $visualReviewUndetermined

    return [pscustomobject]@{
        failed = $entryFailed
        passed = (-not $entryFailed)
        manual_review_pending = $manualReviewPending
        visual_review_undetermined = $visualReviewUndetermined
        status = $status
    }
}

function Write-SummaryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $Summary
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("# Project Template Smoke")
    [void]$lines.Add("")
    [void]$lines.Add("- Generated at: $($Summary.generated_at)")
    $visualReviewSyncedAt = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "visual_review_synced_at"
    if (-not [string]::IsNullOrWhiteSpace($visualReviewSyncedAt)) {
        [void]$lines.Add("- Visual review synced at: $visualReviewSyncedAt")
    }
    [void]$lines.Add("- Manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "manifest_path"))")
    [void]$lines.Add("- Output directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name "output_dir"))")
    [void]$lines.Add("- Overall status: $($Summary.overall_status)")
    [void]$lines.Add("- Passed flag: $($Summary.passed)")
    [void]$lines.Add("- Entry count: $($Summary.entry_count)")
    [void]$lines.Add("- Failed entries: $($Summary.failed_entry_count)")
    [void]$lines.Add("- Visual smoke entries: $($Summary.visual_entry_count)")
    [void]$lines.Add("- Visual verdict: $($Summary.visual_verdict)")
    [void]$lines.Add("- Entries with pending visual review: $($Summary.manual_review_pending_count)")
    [void]$lines.Add("- Entries with undetermined visual review: $($Summary.visual_review_undetermined_count)")
    [void]$lines.Add("")

    foreach ($entry in @(Get-ProjectTemplateSmokeArrayProperty -Object $Summary -Name "entries")) {
        [void]$lines.Add("## $($entry.name)")
        [void]$lines.Add("")
        [void]$lines.Add("- Status: $($entry.status)")
        [void]$lines.Add("- Input DOCX: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx"))")
        [void]$lines.Add("- Entry artifact directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "artifact_dir"))")

        $checks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "checks"
        foreach ($validation in @(Get-ProjectTemplateSmokeArrayProperty -Object $checks -Name "template_validations")) {
            [void]$lines.Add("- Template validation '$($validation.name)': passed=$($validation.passed) output=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $validation -Name "output_json"))")
        }

        $schemaValidation = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_validation"
        if ($null -ne $schemaValidation -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaValidation -Name "enabled")) {
            [void]$lines.Add("- Schema validation: passed=$($schemaValidation.passed) output=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaValidation -Name "output_json"))")
        }

        $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "schema_baseline"
        if ($null -ne $schemaBaseline -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $schemaBaseline -Name "enabled")) {
            [void]$lines.Add("- Schema baseline: matches=$($schemaBaseline.matches) log=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "log_path"))")
        }

        $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "visual_smoke"
        if ($null -ne $visualSmoke -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $visualSmoke -Name "enabled")) {
            $findingsCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "findings_count"
            if ([string]::IsNullOrWhiteSpace($findingsCount)) {
                $findingsCount = "0"
            }
            [void]$lines.Add("- Visual smoke: review_status=$($visualSmoke.review_status) review_verdict=$($visualSmoke.review_verdict) findings=$findingsCount contact_sheet=$(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "contact_sheet"))")
        }

        foreach ($issue in @(Get-ProjectTemplateSmokeArrayProperty -Object $entry -Name "issues")) {
            [void]$lines.Add("- Issue: $issue")
        }

        [void]$lines.Add("")
    }

    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-ReleaseCandidateFinalReviewContent {
    param(
        [string]$RepoRoot,
        $Summary,
        [string]$ReleaseSummaryPath
    )

    $readmeGallery = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "readme_gallery"
    $readmeGalleryStatus = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryStatusLine = switch ($readmeGalleryStatus) {
        "completed" { "- README gallery refresh: completed ($(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $readmeGallery -Name 'assets_dir')))" }
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

    $projectTemplateSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Summary -Name "project_template_smoke"
    $projectTemplateSmokeSummaryPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "summary_json"
    $projectTemplateSmokeManifestPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "manifest_path"
    $projectTemplateSmokeOutputDir = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmoke -Name "output_dir"

    return @"
# Release Candidate Checks

- Generated at: $(Get-Date -Format s)
- Workspace: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $RepoRoot)
- Summary JSON: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryPath)
- Execution status: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'execution_status')
- Visual verdict: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'visual_verdict')
- Failed step: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'failed_step')
- Error: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'error')
- MSVC bootstrap mode: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'msvc_bootstrap_mode')

## Step status

- Configure: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.configure -Name 'status')
- Build: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.build -Name 'status')
- Tests: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.tests -Name 'status')
- Template schema: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.template_schema -Name 'status')
- Template schema manifest: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.template_schema_manifest -Name 'status')
- Project template smoke: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.project_template_smoke -Name 'status')
- Install smoke: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.install_smoke -Name 'status')
- Visual gate: $(Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary.steps.visual_gate -Name 'status')
$readmeGalleryStatusLine

## Key outputs

- Build directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'build_dir'))
- Install directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'install_dir'))
- Consumer build directory: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'consumer_build_dir'))
- Visual gate output: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'gate_output_dir'))
- Review task root: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'task_output_root'))
- Project template smoke manifest: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $projectTemplateSmokeManifestPath)
- Project template smoke summary: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $projectTemplateSmokeSummaryPath)
- Project template smoke output dir: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path $projectTemplateSmokeOutputDir)
- Release handoff: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'release_handoff'))
- Release body: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'release_body_zh_cn'))
- Release summary: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'release_summary_zh_cn'))
- Artifact guide: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'artifact_guide'))
- Reviewer checklist: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'reviewer_checklist'))
- Start here: $(Get-RepoRelativeDisplayPath -RepoRoot $RepoRoot -Path (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Summary -Name 'start_here'))
"@
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedSummaryMarkdown = if ([string]::IsNullOrWhiteSpace($SummaryMarkdown)) {
    Join-Path (Split-Path -Parent $resolvedSummaryJson) "summary.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryMarkdown -AllowMissing
}
$resolvedReleaseSummaryPath = if ([string]::IsNullOrWhiteSpace($ReleaseCandidateSummaryJson)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseCandidateSummaryJson
}

if (-not (Test-Path -LiteralPath $resolvedSummaryJson)) {
    throw "Project template smoke summary was not found: $resolvedSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Assert-PathExists -Path $resolvedReleaseSummaryPath -Label "release-candidate summary JSON"
}

$summary = Get-Content -Raw -LiteralPath $resolvedSummaryJson | ConvertFrom-Json
$entries = @(Get-ProjectTemplateSmokeArrayProperty -Object $summary -Name "entries")
$visualSmokeResults = New-Object 'System.Collections.Generic.List[object]'
$failedEntryCount = 0
$manualReviewPendingCount = 0
$visualReviewUndeterminedCount = 0

foreach ($entry in $entries) {
    $checks = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "checks"
    $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $checks -Name "visual_smoke"
    if ($null -ne $visualSmoke -and [bool](Get-ProjectTemplateSmokeOptionalPropertyObject -Object $visualSmoke -Name "enabled")) {
        $reviewResultPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "review_result_json"
        $reviewStatus = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "review_status"
        $reviewVerdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "review_verdict")
        $findingsCount = 0

        if (-not [string]::IsNullOrWhiteSpace($reviewResultPath) -and (Test-Path -LiteralPath $reviewResultPath)) {
            $review = Get-Content -Raw -LiteralPath $reviewResultPath | ConvertFrom-Json
            $reviewStatus = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $review -Name "status"
            $reviewVerdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $review -Name "verdict")
            $findingsCount = @(Get-ProjectTemplateSmokeArrayProperty -Object $review -Name "findings").Count
        } else {
            $existingFindingsCount = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "findings_count"
            if (-not [string]::IsNullOrWhiteSpace($existingFindingsCount)) {
                $findingsCount = [int]$existingFindingsCount
            }
            if ([string]::IsNullOrWhiteSpace($reviewStatus)) {
                $reviewStatus = "missing"
            }
        }

        Set-ProjectTemplateSmokePropertyValue -Object $visualSmoke -Name "review_status" -Value $reviewStatus
        Set-ProjectTemplateSmokePropertyValue -Object $visualSmoke -Name "review_verdict" -Value $reviewVerdict
        Set-ProjectTemplateSmokePropertyValue -Object $visualSmoke -Name "findings_count" -Value $findingsCount
        [void]$visualSmokeResults.Add($visualSmoke)
    }

    $entryState = Get-ProjectTemplateSmokeEntryDerivedState -Entry $entry
    Set-ProjectTemplateSmokePropertyValue -Object $entry -Name "status" -Value $entryState.status
    Set-ProjectTemplateSmokePropertyValue -Object $entry -Name "passed" -Value $entryState.passed
    Set-ProjectTemplateSmokePropertyValue -Object $entry -Name "manual_review_pending" -Value $entryState.manual_review_pending

    if ($entryState.failed) {
        $failedEntryCount += 1
    }
    if ($entryState.manual_review_pending) {
        $manualReviewPendingCount += 1
    }
    if ($entryState.visual_review_undetermined) {
        $visualReviewUndeterminedCount += 1
    }
}

$visualEntryCount = $visualSmokeResults.Count
$summaryPassed = $failedEntryCount -eq 0
$overallStatus = Get-ProjectTemplateSmokeOverallStatus `
    -SummaryPassed $summaryPassed `
    -ManualReviewPendingCount $manualReviewPendingCount `
    -VisualReviewUndeterminedCount $visualReviewUndeterminedCount
$visualVerdict = Get-ProjectTemplateSmokeVisualVerdict -VisualSmokeResults $visualSmokeResults.ToArray()

Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "visual_review_synced_at" -Value (Get-Date).ToString("s")
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "entry_count" -Value $entries.Count
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "failed_entry_count" -Value $failedEntryCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "visual_entry_count" -Value $visualEntryCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "visual_verdict" -Value $visualVerdict
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "manual_review_pending_count" -Value $manualReviewPendingCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "visual_review_undetermined_count" -Value $visualReviewUndeterminedCount
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "passed" -Value $summaryPassed
Set-ProjectTemplateSmokePropertyValue -Object $summary -Name "overall_status" -Value $overallStatus

($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
Write-SummaryMarkdown -Path $resolvedSummaryMarkdown -RepoRoot $repoRoot -Summary $summary

if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    $releaseSummary = Get-Content -Raw -LiteralPath $resolvedReleaseSummaryPath | ConvertFrom-Json
    $projectTemplateSmokeStep = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $releaseSummary.steps -Name "project_template_smoke"
    if ($null -eq $projectTemplateSmokeStep) {
        $projectTemplateSmokeStep = [ordered]@{}
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary.steps -Name "project_template_smoke" -Value $projectTemplateSmokeStep
    }
    $projectTemplateSmokeSummary = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $releaseSummary -Name "project_template_smoke"
    if ($null -eq $projectTemplateSmokeSummary) {
        $projectTemplateSmokeSummary = [ordered]@{}
        Set-ProjectTemplateSmokePropertyValue -Object $releaseSummary -Name "project_template_smoke" -Value $projectTemplateSmokeSummary
    }

    $statusValue = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $projectTemplateSmokeStep -Name "status"
    if ([string]::IsNullOrWhiteSpace($statusValue)) {
        $statusValue = "completed"
    }
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "status" -Value $statusValue
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "summary_json" -Value $resolvedSummaryJson
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "manifest_path" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "manifest_path")
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "output_dir" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "output_dir")
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "passed" -Value $summaryPassed
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "entry_count" -Value $entries.Count
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "failed_entry_count" -Value $failedEntryCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "visual_entry_count" -Value $visualEntryCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "visual_verdict" -Value $visualVerdict
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "manual_review_pending_count" -Value $manualReviewPendingCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "visual_review_undetermined_count" -Value $visualReviewUndeterminedCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeStep -Name "overall_status" -Value $overallStatus

    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json" -Value $resolvedSummaryJson
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "manifest_path" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "manifest_path")
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "output_dir" -Value (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $summary -Name "output_dir")
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "passed" -Value $summaryPassed
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "entry_count" -Value $entries.Count
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "failed_entry_count" -Value $failedEntryCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "visual_entry_count" -Value $visualEntryCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "visual_verdict" -Value $visualVerdict
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "manual_review_pending_count" -Value $manualReviewPendingCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "visual_review_undetermined_count" -Value $visualReviewUndeterminedCount
    Set-ProjectTemplateSmokePropertyValue -Object $projectTemplateSmokeSummary -Name "overall_status" -Value $overallStatus

    ($releaseSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedReleaseSummaryPath -Encoding UTF8

    $releaseFinalReviewPath = Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) "final_review.md"
    $releaseFinalReviewContent = New-ReleaseCandidateFinalReviewContent `
        -RepoRoot $repoRoot `
        -Summary $releaseSummary `
        -ReleaseSummaryPath $resolvedReleaseSummaryPath
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

Write-Step "Summary JSON: $resolvedSummaryJson"
Write-Step "Summary Markdown: $resolvedSummaryMarkdown"
Write-Step "Overall status: $overallStatus"
Write-Step "Visual verdict: $visualVerdict"
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Write-Host "Release summary: $resolvedReleaseSummaryPath"
    Write-Host "Release final review: $(Join-Path (Split-Path -Parent $resolvedReleaseSummaryPath) 'final_review.md')"
}

if (-not $summaryPassed) {
    exit 1
}

exit 0
