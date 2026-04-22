param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Get-DisplayValue {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "(not available)"
    }

    return $Value
}

function Get-DisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    return Get-RepoRelativePath -RepoRoot $RepoRoot -Path $Path
}

function Get-SupersededReviewTasksReportPath {
    param(
        $Summary,
        $VisualGateSummary
    )

    $reportPath = Get-OptionalPropertyValue -Object $VisualGateSummary -Name "superseded_review_tasks_report"
    if (-not [string]::IsNullOrWhiteSpace($reportPath)) {
        return $reportPath
    }

    return Get-OptionalPropertyValue -Object $Summary -Name "superseded_review_tasks_report"
}

function Get-SupersededReviewTaskCount {
    param([string]$ReportPath)

    if ([string]::IsNullOrWhiteSpace($ReportPath) -or -not (Test-Path -LiteralPath $ReportPath)) {
        return ""
    }

    try {
        $report = Get-Content -Raw -LiteralPath $ReportPath | ConvertFrom-Json
    } catch {
        return ""
    }

    return Get-OptionalPropertyValue -Object $report -Name "superseded_task_count"
}

function Get-VisualTaskDir {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryTaskDir = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_task_dir" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryTaskDir)) {
        return $summaryTaskDir
    }

    $reviewTasks = Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks"
    $taskInfo = Get-OptionalPropertyObject -Object $reviewTasks -Name $TaskKey
    return Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
}

function Get-VisualTaskVerdict {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_verdict" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $manualReview = Get-OptionalPropertyObject -Object $GateSummary -Name "manual_review"
    $tasks = Get-OptionalPropertyObject -Object $manualReview -Name "tasks"
    $taskReview = Get-OptionalPropertyObject -Object $tasks -Name $TaskKey
    return Get-OptionalPropertyValue -Object $taskReview -Name "verdict"
}

function Get-OptionalPropertyArray {
    param(
        $Object,
        [string]$Name
    )

    $propertyValue = Get-OptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return @()
    }

    return @($propertyValue)
}

function Get-OrCreateCuratedVisualReviewEntry {
    param(
        [hashtable]$EntryMap,
        [System.Collections.Generic.List[string]]$EntryOrder,
        $Source,
        [int]$FallbackIndex
    )

    $id = Get-OptionalPropertyValue -Object $Source -Name "id"
    $displayLabel = Get-OptionalPropertyValue -Object $Source -Name "display_label"
    $label = if (-not [string]::IsNullOrWhiteSpace($displayLabel)) {
        $displayLabel
    } else {
        Get-OptionalPropertyValue -Object $Source -Name "label"
    }

    $key = if (-not [string]::IsNullOrWhiteSpace($id)) {
        $id
    } elseif (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
        $label
    } else {
        "__curated_{0}" -f $FallbackIndex
    }

    if (-not $EntryMap.ContainsKey($key)) {
        $EntryMap[$key] = [ordered]@{
            id = ""
            label = ""
            verdict = ""
            task_dir = ""
            review_result_path = ""
            final_review_path = ""
        }
        [void]$EntryOrder.Add($key)
    }

    return $EntryMap[$key]
}

function Merge-CuratedVisualReviewEntry {
    param(
        $Entry,
        $Source
    )

    if ($null -eq $Source) {
        return
    }

    $id = Get-OptionalPropertyValue -Object $Source -Name "id"
    if (-not [string]::IsNullOrWhiteSpace($id)) {
        $Entry.id = $id
    }

    $displayLabel = Get-OptionalPropertyValue -Object $Source -Name "display_label"
    $label = if (-not [string]::IsNullOrWhiteSpace($displayLabel)) {
        $displayLabel
    } else {
        Get-OptionalPropertyValue -Object $Source -Name "label"
    }
    if (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
        $Entry.label = $label
    }

    $verdict = Get-OptionalPropertyValue -Object $Source -Name "verdict"
    if (-not [string]::IsNullOrWhiteSpace($verdict)) {
        $Entry.verdict = $verdict
    }

    $taskInfo = Get-OptionalPropertyObject -Object $Source -Name "task"
    if ($null -eq $taskInfo) {
        $taskInfo = $Source
    }

    $taskDir = Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
    if (-not [string]::IsNullOrWhiteSpace($taskDir)) {
        $Entry.task_dir = $taskDir
    }

    $reviewResultPath = Get-OptionalPropertyValue -Object $taskInfo -Name "review_result_path"
    if (-not [string]::IsNullOrWhiteSpace($reviewResultPath)) {
        $Entry.review_result_path = $reviewResultPath
    }

    $finalReviewPath = Get-OptionalPropertyValue -Object $taskInfo -Name "final_review_path"
    if (-not [string]::IsNullOrWhiteSpace($finalReviewPath)) {
        $Entry.final_review_path = $finalReviewPath
    }
}

function Get-CuratedVisualReviewEntries {
    param(
        $VisualGateSummary,
        $GateSummary
    )

    $entryMap = @{}
    $entryOrder = New-Object 'System.Collections.Generic.List[string]'
    $fallbackIndex = 0

    $reviewTasks = Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks"
    $manualReview = Get-OptionalPropertyObject -Object $GateSummary -Name "manual_review"
    $manualTasks = Get-OptionalPropertyObject -Object $manualReview -Name "tasks"

    $sources = @(
        (Get-OptionalPropertyArray -Object $VisualGateSummary -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $reviewTasks -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $manualTasks -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $GateSummary -Name "curated_visual_regressions")
    )

    foreach ($sourceGroup in $sources) {
        foreach ($source in $sourceGroup) {
            $fallbackIndex += 1
            $entry = Get-OrCreateCuratedVisualReviewEntry `
                -EntryMap $entryMap `
                -EntryOrder $entryOrder `
                -Source $source `
                -FallbackIndex $fallbackIndex
            Merge-CuratedVisualReviewEntry -Entry $entry -Source $source
        }
    }

    $entries = @()
    foreach ($key in $entryOrder) {
        $entry = $entryMap[$key]
        if ([string]::IsNullOrWhiteSpace($entry.label)) {
            if (-not [string]::IsNullOrWhiteSpace($entry.id)) {
                $entry.label = $entry.id
            } else {
                $entry.label = "Curated visual regression bundle"
            }
        }

        $entries += [pscustomobject]$entry
    }

    return $entries
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
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

function Add-CheckboxLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Text
    )

    [void]$Lines.Add("- [ ] $Text")
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "REVIEWER_CHECKLIST.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$releaseHandoffPath = Get-OptionalPropertyValue -Object $summary -Name "release_handoff"
if ([string]::IsNullOrWhiteSpace($releaseHandoffPath)) {
    $releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
}
$releaseBodyPath = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseBodyPath)) {
    $releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
}
$releaseSummaryPath = Get-OptionalPropertyValue -Object $summary -Name "release_summary_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseSummaryPath)) {
    $releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
}
$finalReviewPath = Join-Path $reportDir "final_review.md"
$taskOutputRoot = Get-OptionalPropertyValue -Object $summary -Name "task_output_root"

$templateSchemaSummary = Get-OptionalPropertyObject -Object $summary -Name "template_schema"
$templateSchemaStep = Get-OptionalPropertyObject -Object $summary.steps -Name "template_schema"
$templateSchemaRequested = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "requested"
$templateSchemaStatus = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "status"
if ([string]::IsNullOrWhiteSpace($templateSchemaStatus)) {
    $templateSchemaStatus = if ($templateSchemaRequested -eq "True") { "requested" } else { "not_requested" }
}
$templateSchemaMatches = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "matches"
if ([string]::IsNullOrWhiteSpace($templateSchemaMatches)) {
    $templateSchemaMatches = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "matches"
}
$templateSchemaBaseline = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "baseline"
if ([string]::IsNullOrWhiteSpace($templateSchemaBaseline)) {
    $templateSchemaBaseline = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "schema_file"
}
$templateSchemaInputDocx = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "input_docx"
$templateSchemaGeneratedOutput = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "generated_output"
if ([string]::IsNullOrWhiteSpace($templateSchemaGeneratedOutput)) {
    $templateSchemaGeneratedOutput = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "generated_output_path"
}
$templateSchemaAddedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "added_target_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaAddedTargetCount)) {
    $templateSchemaAddedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "added_target_count"
}
$templateSchemaRemovedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "removed_target_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaRemovedTargetCount)) {
    $templateSchemaRemovedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "removed_target_count"
}
$templateSchemaChangedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaStep -Name "changed_target_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaChangedTargetCount)) {
    $templateSchemaChangedTargetCount = Get-OptionalPropertyValue -Object $templateSchemaSummary -Name "changed_target_count"
}
$templateSchemaManifestSummary = Get-OptionalPropertyObject -Object $summary -Name "template_schema_manifest"
$templateSchemaManifestStep = Get-OptionalPropertyObject -Object $summary.steps -Name "template_schema_manifest"
$templateSchemaManifestRequested = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "requested"
$templateSchemaManifestStatus = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "status"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestStatus)) {
    $templateSchemaManifestStatus = if ($templateSchemaManifestRequested -eq "True") { "requested" } else { "not_requested" }
}
$templateSchemaManifestPassed = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "passed"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestPassed)) {
    $templateSchemaManifestPassed = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "passed"
}
$templateSchemaManifestEntryCount = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "entry_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestEntryCount)) {
    $templateSchemaManifestEntryCount = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "entry_count"
}
$templateSchemaManifestDriftCount = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "drift_count"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestDriftCount)) {
    $templateSchemaManifestDriftCount = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "drift_count"
}
$templateSchemaManifestPath = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "manifest_path"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestPath)) {
    $templateSchemaManifestPath = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "manifest_path"
}
$templateSchemaManifestOutputDir = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "output_dir"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestOutputDir)) {
    $templateSchemaManifestOutputDir = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "output_dir"
}
$templateSchemaManifestSummaryJson = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestSummaryJson)) {
    $templateSchemaManifestSummaryJson = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "summary_json"
}
$projectTemplateSmokeSummary = Get-OptionalPropertyObject -Object $summary -Name "project_template_smoke"
$projectTemplateSmokeStep = Get-OptionalPropertyObject -Object $summary.steps -Name "project_template_smoke"
$projectTemplateSmokeRequested = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "requested"
$projectTemplateSmokeStatus = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "status"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeStatus)) {
    $projectTemplateSmokeStatus = if ($projectTemplateSmokeRequested -eq "True") { "requested" } else { "not_requested" }
}
$projectTemplateSmokePassed = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "passed"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokePassed)) {
    $projectTemplateSmokePassed = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "passed"
}
$projectTemplateSmokeEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "entry_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeEntryCount)) {
    $projectTemplateSmokeEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "entry_count"
}
$projectTemplateSmokeFailedEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "failed_entry_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeFailedEntryCount)) {
    $projectTemplateSmokeFailedEntryCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "failed_entry_count"
}
$projectTemplateSmokeVisualVerdict = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "visual_verdict"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeVisualVerdict)) {
    $projectTemplateSmokeVisualVerdict = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "visual_verdict"
}
$projectTemplateSmokePendingReviewCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "manual_review_pending_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokePendingReviewCount)) {
    $projectTemplateSmokePendingReviewCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "manual_review_pending_count"
}
$projectTemplateSmokeManifestPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "manifest_path"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeManifestPath)) {
    $projectTemplateSmokeManifestPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "manifest_path"
}
$projectTemplateSmokeSummaryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryJson)) {
    $projectTemplateSmokeSummaryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "summary_json"
}
$projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "output_dir"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeOutputDir)) {
    $projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "output_dir"
}

$visualGateStep = Get-OptionalPropertyObject -Object $summary.steps -Name "visual_gate"
$installPrefix = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "install_prefix"
$consumerDocument = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "consumer_document"
$gateSummaryPath = Get-OptionalPropertyValue -Object $visualGateStep -Name "summary_json"
$gateFinalReviewPath = Get-OptionalPropertyValue -Object $visualGateStep -Name "final_review"

$visualVerdict = ""
$readmeGalleryStatus = ""
$readmeGalleryAssetsDir = ""
$gateSummary = $null
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath) -and (Test-Path -LiteralPath $gateSummaryPath)) {
    $gateSummary = Get-Content -Raw $gateSummaryPath | ConvertFrom-Json
    $visualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
    $readmeGallery = Get-OptionalPropertyObject -Object $gateSummary -Name "readme_gallery"
    $readmeGalleryStatus = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    $readmeGalleryAssetsDir = Get-OptionalPropertyValue -Object $readmeGallery -Name "assets_dir"
}
$sectionPageSetupTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)
$supersededReviewTasksReportPath = Get-SupersededReviewTasksReportPath -Summary $summary -VisualGateSummary $visualGateStep
if ([string]::IsNullOrWhiteSpace($taskOutputRoot) -and -not [string]::IsNullOrWhiteSpace($supersededReviewTasksReportPath)) {
    $taskOutputRoot = Split-Path -Parent $supersededReviewTasksReportPath
}
$supersededReviewTasksCount = Get-SupersededReviewTaskCount -ReportPath $supersededReviewTasksReportPath
if ([string]::IsNullOrWhiteSpace($visualVerdict)) {
    $visualVerdict = "pending_manual_review"
}

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$syncProjectTemplateSmokeCommand = ""
if (-not [string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryJson)) {
    $syncProjectTemplateSmokeCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_smoke_visual_verdict.ps1 -SummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryJson),
        $summaryCommandPath
}
$findSupersededTasksCommand = ""
if (-not [string]::IsNullOrWhiteSpace($taskOutputRoot)) {
    $findSupersededTasksCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\find_superseded_review_tasks.ps1 -TaskOutputRoot "{0}"' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $taskOutputRoot)
}
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$packageAssetsCommand = if ($releaseVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -UploadReleaseTag "v{1}"' -f `
        $summaryCommandPath, $releaseVersion
} else {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
        $summaryCommandPath
}
$syncReleaseNotesCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_github_release_notes.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($releaseVersion) {
    $syncReleaseNotesCommand += (' -ReleaseTag "v{0}"' -f $releaseVersion)
}
$publishReleaseCommand = $syncReleaseNotesCommand + " -Publish"
$publishWorkflowCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($releaseVersion) {
    $publishWorkflowCommand += (' -ReleaseTag "v{0}"' -f $releaseVersion)
}
$publishWorkflowFinalCommand = $publishWorkflowCommand + " -Publish"
$refreshWorkflowName = "Release Refresh"
$refreshWorkflowFile = ".github/workflows/release-refresh.yml"
$publishWorkflowName = "Release Publish"
$publishWorkflowFile = ".github/workflows/release-publish.yml"

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Release Reviewer Checklist")
[void]$lines.Add("")
[void]$lines.Add("- Execution status: $($summary.execution_status)")
[void]$lines.Add("- Template schema gate status: $(Get-DisplayValue -Value $templateSchemaStatus)")
[void]$lines.Add("- Template schema matches baseline: $(Get-DisplayValue -Value $templateSchemaMatches)")
[void]$lines.Add("- Template schema drift counts (added/removed/changed): $(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $templateSchemaAddedTargetCount, $templateSchemaRemovedTargetCount, $templateSchemaChangedTargetCount))")
[void]$lines.Add("- Template schema baseline: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaBaseline)")
[void]$lines.Add("- Template schema manifest status: $(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$lines.Add("- Template schema manifest passed: $(Get-DisplayValue -Value $templateSchemaManifestPassed)")
[void]$lines.Add("- Template schema manifest entries / drifts: $(Get-DisplayValue -Value ('{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount))")
[void]$lines.Add("- Template schema manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestPath)")
[void]$lines.Add("- Project template smoke status: $(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$lines.Add("- Project template smoke passed: $(Get-DisplayValue -Value $projectTemplateSmokePassed)")
[void]$lines.Add("- Project template smoke entries / failed: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount))")
[void]$lines.Add("- Project template smoke visual verdict: $(Get-DisplayValue -Value $projectTemplateSmokeVisualVerdict)")
[void]$lines.Add("- Project template smoke pending visual reviews: $(Get-DisplayValue -Value $projectTemplateSmokePendingReviewCount)")
[void]$lines.Add("- Project template smoke manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeManifestPath)")
[void]$lines.Add("- Visual gate status: $($summary.steps.visual_gate.status)")
[void]$lines.Add("- Visual verdict: $visualVerdict")
[void]$lines.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$lines.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$lines.Add("- Curated visual regression bundles: $($curatedVisualReviewEntries.Count)")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")
}
[void]$lines.Add("- Superseded review tasks: $(Get-DisplayValue -Value $supersededReviewTasksCount)")
[void]$lines.Add("- Superseded task audit: $(Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath)")
[void]$lines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$lines.Add("- Artifact guide: $(Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath)")
[void]$lines.Add("")
[void]$lines.Add("## Step 1: Read The Release Notes")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text ('Open `ARTIFACT_GUIDE.md` and confirm the file map still matches this artifact: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_summary.zh-CN.md` and confirm the front-page bullets are still accurate: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_body.zh-CN.md` and confirm the longer release body does not contradict the short summary: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath))
Add-CheckboxLine -Lines $lines -Text ('Open `release_handoff.md` and confirm the evidence links and repro commands are still valid: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $releaseHandoffPath))
[void]$lines.Add("")
[void]$lines.Add("## Step 2: Check The Verification State")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text ('Confirm `summary.json` reports the expected step statuses: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath))
Add-CheckboxLine -Lines $lines -Text ('Confirm `final_review.md` still matches the current release status: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath))
if (-not [string]::IsNullOrWhiteSpace($supersededReviewTasksCount)) {
    Add-CheckboxLine -Lines $lines -Text ('Confirm `superseded_review_tasks.json` reports zero stale task directories or intentionally explains any preserved older tasks (current count: {0}): {1}' -f `
            $supersededReviewTasksCount, (Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath))
} else {
    Add-CheckboxLine -Lines $lines -Text ('Confirm `superseded_review_tasks.json` reports zero stale task directories or intentionally explains any preserved older tasks: {0}' -f `
            (Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath))
}
if ($templateSchemaRequested -eq "True" -or $templateSchemaStatus -ne "not_requested") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the template schema input DOCX is the intended release candidate document: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaInputDocx))
    Add-CheckboxLine -Lines $lines -Text ('Confirm the committed template schema baseline is the expected one for this release: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaBaseline))
    if (-not [string]::IsNullOrWhiteSpace($templateSchemaGeneratedOutput)) {
        Add-CheckboxLine -Lines $lines -Text ('Open the generated normalized template schema output when you need to inspect the compared result: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaGeneratedOutput))
    }

    if ($templateSchemaMatches -eq "True") {
        Add-CheckboxLine -Lines $lines -Text ('Confirm the template schema gate is green and drift counts stay at `0/0/0` for added/removed/changed targets.')
    } elseif ($templateSchemaMatches -eq "False") {
        Add-CheckboxLine -Lines $lines -Text ('Stop here until the template schema drift is either intentionally re-baselined or the document/code regression is fixed.')
    }
}
if ($templateSchemaManifestRequested -eq "True" -or $templateSchemaManifestStatus -ne "not_requested") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the template schema manifest matches the intended repository baseline set: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestPath))
    Add-CheckboxLine -Lines $lines -Text ('Open the template schema manifest summary when you need per-entry status and drift counts: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestSummaryJson))
    Add-CheckboxLine -Lines $lines -Text ('Inspect the generated manifest output directory if any repository baseline needs a generated schema artifact: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestOutputDir))

    if ($templateSchemaManifestPassed -eq "True") {
        Add-CheckboxLine -Lines $lines -Text ('Confirm the template schema manifest gate stays green and the drift count remains `0` across all registered baselines.')
    } elseif ($templateSchemaManifestPassed -eq "False") {
        Add-CheckboxLine -Lines $lines -Text ('Stop here until every registered template schema baseline either matches again or is intentionally re-baselined.')
    }
}
if ($projectTemplateSmokeRequested -eq "True" -or $projectTemplateSmokeStatus -ne "not_requested") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the project template smoke manifest matches the intended real-template regression set: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeManifestPath))
    Add-CheckboxLine -Lines $lines -Text ('Open the project template smoke summary when you need per-entry smoke status, visual verdicts, and artifact links: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryJson))
    Add-CheckboxLine -Lines $lines -Text ('Inspect the project template smoke output directory when a real-template artifact needs deeper review: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeOutputDir))

    if ($projectTemplateSmokePassed -eq "True") {
        Add-CheckboxLine -Lines $lines -Text ('Confirm the project template smoke gate stays green and the failed entry count remains `0` across the registered real-template set.')
    } elseif ($projectTemplateSmokePassed -eq "False") {
        Add-CheckboxLine -Lines $lines -Text ('Stop here until every failing project template smoke entry is fixed or intentionally removed from the registered release-candidate set.')
    }

    if ($projectTemplateSmokeVisualVerdict -in @("pass", "not_applicable")) {
        Add-CheckboxLine -Lines $lines -Text ('Confirm the project template smoke visual verdict is already signed off as `{0}`.' -f $projectTemplateSmokeVisualVerdict)
    } elseif (-not [string]::IsNullOrWhiteSpace($projectTemplateSmokeVisualVerdict)) {
        Add-CheckboxLine -Lines $lines -Text ('Stop here until the project template smoke visual verdict changes from `{0}`.' -f $projectTemplateSmokeVisualVerdict)
    }
}

if ($summary.steps.visual_gate.status -eq "skipped") {
    Add-CheckboxLine -Lines $lines -Text 'Treat this as a CI metadata artifact only; do not treat the visual verdict as the final Word screenshot-backed release signoff.'
} elseif ($visualVerdict -eq "pass") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the local Word visual evidence is already signed off as `pass`: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath))
} else {
    Add-CheckboxLine -Lines $lines -Text ('Stop here until the local Word visual review is completed and the visual verdict changes from `{0}`.' -f $visualVerdict)
}

if (-not [string]::IsNullOrWhiteSpace($consumerDocument)) {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the install smoke consumer document exists for spot checks: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $consumerDocument))
}

if (-not [string]::IsNullOrWhiteSpace($gateFinalReviewPath)) {
    Add-CheckboxLine -Lines $lines -Text ('Spot-check the visual gate final review notes if anything in the release notes feels risky: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath))
}
if (-not [string]::IsNullOrWhiteSpace($sectionPageSetupTaskDir)) {
    Add-CheckboxLine -Lines $lines -Text ('Open the section page setup review task if the release touches layout or orientation behavior: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir))
}
if (-not [string]::IsNullOrWhiteSpace($pageNumberFieldsTaskDir)) {
    Add-CheckboxLine -Lines $lines -Text ('Open the page number fields review task if the release touches page numbers, PAGE/NUMPAGES fields, or field refresh behavior: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir))
}
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    if (-not [string]::IsNullOrWhiteSpace($curatedVisualReview.task_dir)) {
        Add-CheckboxLine -Lines $lines -Text ('Open the {0} review task if the release touches this curated visual bundle: {1}' -f `
                $curatedVisualReview.label, (Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.task_dir))
    }
}
if ($readmeGalleryStatus -eq "completed") {
    Add-CheckboxLine -Lines $lines -Text ('Confirm the repository README gallery PNGs were refreshed from the latest Word evidence: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $readmeGalleryAssetsDir))
}

[void]$lines.Add("")
[void]$lines.Add("## Step 3: Publish Or Refresh")
[void]$lines.Add("")
Add-CheckboxLine -Lines $lines -Text 'Use `release_summary.zh-CN.md` for the GitHub Release first-screen bullets.'
Add-CheckboxLine -Lines $lines -Text 'Use `release_body.zh-CN.md` for the full release notes or translated handoff notes.'
Add-CheckboxLine -Lines $lines -Text ('Generate or refresh the public release ZIP files before publishing: `{0}`' -f $packageAssetsCommand)
Add-CheckboxLine -Lines $lines -Text ('Sync the audited full release body into the GitHub Release notes: `{0}`' -f $syncReleaseNotesCommand)
Add-CheckboxLine -Lines $lines -Text ('When all gates pass and the GitHub Release is ready to go live, publish it with: `{0}`' -f $publishReleaseCommand)
Add-CheckboxLine -Lines $lines -Text ('Use the one-shot wrapper when you want ZIP upload plus note sync together: `{0}`' -f $publishWorkflowCommand)
Add-CheckboxLine -Lines $lines -Text ('Use the same wrapper with final publish enabled when the release is ready to go live: `{0}`' -f $publishWorkflowFinalCommand)
Add-CheckboxLine -Lines $lines -Text ('If a self-hosted Windows runner already carries the validated bundle, use GitHub Actions `{0}` (`{1}`) for ZIP upload plus note sync without publishing.' -f $refreshWorkflowName, $refreshWorkflowFile)
Add-CheckboxLine -Lines $lines -Text ('Use GitHub Actions `{0}` (`{1}`) only after final local Word signoff when the GitHub Release should go live publicly.' -f $publishWorkflowName, $publishWorkflowFile)
Add-CheckboxLine -Lines $lines -Text ('For the GitHub web flow, go to `Actions`, choose `{0}` or `{1}`, click `Run workflow`, then inspect `release-refresh-output` / `release-publish-output` and the target GitHub Release page.' -f $refreshWorkflowName, $publishWorkflowName)
Add-CheckboxLine -Lines $lines -Text ('If the visual verdict changes later, rerun the verdict sync command so the gate summary and release notes stay in sync: `{0}`' -f $syncLatestCommand)
if (-not [string]::IsNullOrWhiteSpace($syncProjectTemplateSmokeCommand)) {
    Add-CheckboxLine -Lines $lines -Text ('If the project template smoke visual verdict changes later, rerun its sync command so the smoke summary and release notes stay in sync: `{0}`' -f $syncProjectTemplateSmokeCommand)
}
if (-not [string]::IsNullOrWhiteSpace($findSupersededTasksCommand)) {
    Add-CheckboxLine -Lines $lines -Text ('Rerun the superseded review-task audit when you need to recheck older preserved task directories: `{0}`' -f $findSupersededTasksCommand)
}

if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedQuickstartZh = Join-Path $installPrefix "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateZh = Join-Path $installPrefix "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
    Add-CheckboxLine -Lines $lines -Text ('Use the installed quickstart for the shortest preview -> command -> review-task path: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $installedQuickstartZh))
    Add-CheckboxLine -Lines $lines -Text ('Use the installed release template when the reviewer needs a copy-paste release-note skeleton: {0}' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $installedTemplateZh))
}

[void]$lines.Add("")
[void]$lines.Add("## Stop Conditions")
[void]$lines.Add("")
[void]$lines.Add('- Do not approve for public release when `execution_status` is not `pass`.')
[void]$lines.Add('- Do not approve for public release when a requested template schema gate does not report `matches = true`.')
[void]$lines.Add('- Do not approve for public release when a requested template schema manifest gate does not report `passed = true`.')
[void]$lines.Add('- Do not approve for public release when a requested project template smoke gate does not report `passed = true`.')
[void]$lines.Add('- Do not approve for public release when a requested project template smoke visual verdict is neither `pass` nor `not_applicable`.')
[void]$lines.Add('- Do not approve for public release when the final local visual verdict is not `pass`.')
[void]$lines.Add("- Do not treat a CI-only artifact with visual gate = skipped as the final screenshot-backed release signoff.")

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Reviewer checklist: $resolvedOutputPath"
