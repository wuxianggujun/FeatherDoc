param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = "",
    [string]$ReleaseVersion = ""
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
    if ($null -eq $property) {
        return ""
    }

    if ($null -eq $property.Value) {
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

. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")
. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

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

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_handoff.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$summaryReleaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$projectVersion = if (-not [string]::IsNullOrWhiteSpace($ReleaseVersion)) {
    $ReleaseVersion
} elseif (-not [string]::IsNullOrWhiteSpace($summaryReleaseVersion)) {
    $summaryReleaseVersion
} else {
    Get-ProjectVersion -RepoRoot $repoRoot
}
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseBodyPath = Get-OptionalPropertyValue -Object $summary -Name "release_body_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseBodyPath)) {
    $releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
}
$releaseSummaryPath = Get-OptionalPropertyValue -Object $summary -Name "release_summary_zh_cn"
if ([string]::IsNullOrWhiteSpace($releaseSummaryPath)) {
    $releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
}
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}
$taskOutputRoot = Get-OptionalPropertyValue -Object $summary -Name "task_output_root"
$templateSchemaManifestSummary = Get-OptionalPropertyObject -Object $summary -Name "template_schema_manifest"
$templateSchemaManifestStep = Get-OptionalPropertyObject -Object $summary.steps -Name "template_schema_manifest"
$templateSchemaManifestStatus = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "status"
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
$templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestSummaryPath)) {
    $templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "summary_json"
}
$templateSchemaManifestOutputDir = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "output_dir"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestOutputDir)) {
    $templateSchemaManifestOutputDir = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "output_dir"
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
$projectTemplateSmokeDirtySchemaBaselineCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "dirty_schema_baseline_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeDirtySchemaBaselineCount)) {
    $projectTemplateSmokeDirtySchemaBaselineCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "dirty_schema_baseline_count"
}
$projectTemplateSmokeSchemaBaselineDriftCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "schema_baseline_drift_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSchemaBaselineDriftCount)) {
    $projectTemplateSmokeSchemaBaselineDriftCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "schema_baseline_drift_count"
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
$projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryPath)) {
    $projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "summary_json"
}
$projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "output_dir"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeOutputDir)) {
    $projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "output_dir"
}
$projectTemplateSmokeCandidateCoverage = Get-OptionalPropertyObject -Object $projectTemplateSmokeStep -Name "candidate_coverage"
if ($null -eq $projectTemplateSmokeCandidateCoverage) {
    $projectTemplateSmokeCandidateCoverage = Get-OptionalPropertyObject -Object $projectTemplateSmokeSummary -Name "candidate_coverage"
}
$projectTemplateSmokeRequireFullCoverage = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "require_full_coverage"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeRequireFullCoverage)) {
    $projectTemplateSmokeRequireFullCoverage = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "require_full_coverage"
}
$projectTemplateSmokeCandidateDiscoveryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "candidate_discovery_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeCandidateDiscoveryJson)) {
    $projectTemplateSmokeCandidateDiscoveryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "candidate_discovery_json"
}
$projectTemplateSmokeCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeCandidateCount)) {
    $projectTemplateSmokeCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "candidate_count"
}
$projectTemplateSmokeRegisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "registered_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeRegisteredCandidateCount)) {
    $projectTemplateSmokeRegisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "registered_candidate_count"
}
$projectTemplateSmokeUnregisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "unregistered_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeUnregisteredCandidateCount)) {
    $projectTemplateSmokeUnregisteredCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "unregistered_candidate_count"
}
$projectTemplateSmokeExcludedCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeCandidateCoverage -Name "excluded_candidate_count"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeExcludedCandidateCount)) {
    $projectTemplateSmokeExcludedCandidateCount = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "excluded_candidate_count"
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
$documentTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "document"
$fixedGridTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeReviewNote = Get-VisualTaskReviewNote -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewNote = Get-VisualTaskReviewNote -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewNote = Get-VisualTaskReviewNote -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewNote = Get-VisualTaskReviewNote -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeReviewedAt = Get-VisualTaskReviewedAt -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewedAt = Get-VisualTaskReviewedAt -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewedAt = Get-VisualTaskReviewedAt -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewedAt = Get-VisualTaskReviewedAt -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeReviewMethod = Get-VisualTaskReviewMethod -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewMethod = Get-VisualTaskReviewMethod -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewMethod = Get-VisualTaskReviewMethod -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewMethod = Get-VisualTaskReviewMethod -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)
$visualReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine -VisualGateSummary $visualGateStep -GateSummary $gateSummary
$supersededReviewTasksReportPath = Get-SupersededReviewTasksReportPath -Summary $summary -VisualGateSummary $visualGateStep
if ([string]::IsNullOrWhiteSpace($taskOutputRoot) -and -not [string]::IsNullOrWhiteSpace($supersededReviewTasksReportPath)) {
    $taskOutputRoot = Split-Path -Parent $supersededReviewTasksReportPath
}
$supersededReviewTasksCount = Get-SupersededReviewTaskCount -ReportPath $supersededReviewTasksReportPath

$installedDataDir = ""
$installedQuickstartEn = ""
$installedQuickstartZh = ""
$installedTemplateEn = ""
$installedTemplateZh = ""

if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedDataDir = Join-Path $installPrefix "share\FeatherDoc"
    $installedQuickstartEn = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.md"
    $installedQuickstartZh = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateEn = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.md"
    $installedTemplateZh = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
}

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$syncProjectTemplateSmokeCommand = ""
if (-not [string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryPath)) {
    $syncProjectTemplateSmokeCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_smoke_visual_verdict.ps1 -SummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryPath),
        $summaryCommandPath
}
$syncExplicitCommand = ""
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
    $gateSummaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $gateSummaryPath
    $syncExplicitCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 -GateSummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        $gateSummaryCommandPath, $summaryCommandPath
}
$releaseGateCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1"
$releaseChecksCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1"
$packageAssetsCommand = if ($projectVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -ReleaseVersion "{1}"' -f `
        $summaryCommandPath, $projectVersion
} else {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
        $summaryCommandPath
}
$syncReleaseNotesCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_github_release_notes.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($projectVersion) {
    $syncReleaseNotesCommand += (' -ReleaseTag "v{0}"' -f $projectVersion)
}
$publishReleaseCommand = $syncReleaseNotesCommand + " -Publish"
$publishWorkflowCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($projectVersion) {
    $publishWorkflowCommand += (' -ReleaseTag "v{0}"' -f $projectVersion)
}
$publishWorkflowFinalCommand = $publishWorkflowCommand + " -Publish"
$refreshWorkflowName = "Release Refresh"
$refreshWorkflowFile = ".github/workflows/release-refresh.yml"
$publishWorkflowName = "Release Publish"
$publishWorkflowFile = ".github/workflows/release-publish.yml"
$openDocumentTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1"
$openFixedGridTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt"
$openSectionPageSetupTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_section_page_setup_review_task.ps1 -PrintPrompt"
$openPageNumberFieldsTaskCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_page_number_fields_review_task.ps1 -PrintPrompt"
$findSupersededTasksCommand = ""
if (-not [string]::IsNullOrWhiteSpace($taskOutputRoot)) {
    $findSupersededTasksCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\find_superseded_review_tasks.ps1 -TaskOutputRoot "{0}"' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $taskOutputRoot)
}
$openCuratedVisualTaskCommands = @(
    $curatedVisualReviewEntries |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_.id) } |
        ForEach-Object {
            'pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind {0}-visual-regression-bundle -PrintPrompt' -f `
                $_.id
        }
)

$handoffLines = New-Object 'System.Collections.Generic.List[string]'

[void]$handoffLines.Add("# Release Artifact Handoff")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Generated at: $(Get-Date -Format s)")
[void]$handoffLines.Add("- Project version: $(Get-DisplayValue -Value $projectVersion)")
[void]$handoffLines.Add("- Release candidate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
[void]$handoffLines.Add("- Release candidate final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath)")
[void]$handoffLines.Add("- Release handoff: $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputPath)")
[void]$handoffLines.Add("- Release body: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath)")
[void]$handoffLines.Add("- Release summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath)")
[void]$handoffLines.Add("- Artifact guide: $(Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath)")
[void]$handoffLines.Add("- Reviewer checklist: $(Get-DisplayPath -RepoRoot $repoRoot -Path $reviewerChecklistPath)")
[void]$handoffLines.Add("- Execution status: $($summary.execution_status)")
[void]$handoffLines.Add("- Release blockers: $(Get-ReleaseBlockerCount -Summary $summary)")
[void]$handoffLines.Add("- Template schema manifest status: $(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$handoffLines.Add("- Template schema manifest passed: $(Get-DisplayValue -Value $templateSchemaManifestPassed)")
[void]$handoffLines.Add("- Template schema manifest entries / drifts: $(Get-DisplayValue -Value ('{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount))")
[void]$handoffLines.Add("- Project template smoke status: $(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$handoffLines.Add("- Project template smoke passed: $(Get-DisplayValue -Value $projectTemplateSmokePassed)")
[void]$handoffLines.Add("- Project template smoke entries / failed: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount))")
[void]$handoffLines.Add("- Project template smoke schema baseline dirty / drift: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeDirtySchemaBaselineCount, $projectTemplateSmokeSchemaBaselineDriftCount))")
[void]$handoffLines.Add("- Project template smoke visual verdict: $(Get-DisplayValue -Value $projectTemplateSmokeVisualVerdict)")
[void]$handoffLines.Add("- Project template smoke pending visual reviews: $(Get-DisplayValue -Value $projectTemplateSmokePendingReviewCount)")
[void]$handoffLines.Add("- Project template smoke full coverage required: $(Get-DisplayValue -Value $projectTemplateSmokeRequireFullCoverage)")
[void]$handoffLines.Add("- Project template smoke candidates registered / unregistered / excluded: $(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount))")
[void]$handoffLines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
if (-not [string]::IsNullOrWhiteSpace($visualReviewTaskSummaryLine)) {
    [void]$handoffLines.Add("- $visualReviewTaskSummaryLine")
}
[void]$handoffLines.Add("- Smoke verdict: $(Get-DisplayValue -Value $smokeVerdict)")
[void]$handoffLines.Add("- Smoke review status: $(Get-DisplayValue -Value $smokeReviewStatus)")
[void]$handoffLines.Add("- Smoke reviewed at: $(Get-DisplayValue -Value $smokeReviewedAt)")
[void]$handoffLines.Add("- Smoke review method: $(Get-DisplayValue -Value $smokeReviewMethod)")
[void]$handoffLines.Add("- Smoke review note: $(Get-DisplayValue -Value $smokeReviewNote)")
[void]$handoffLines.Add("- Fixed-grid verdict: $(Get-DisplayValue -Value $fixedGridVerdict)")
[void]$handoffLines.Add("- Fixed-grid review status: $(Get-DisplayValue -Value $fixedGridReviewStatus)")
[void]$handoffLines.Add("- Fixed-grid reviewed at: $(Get-DisplayValue -Value $fixedGridReviewedAt)")
[void]$handoffLines.Add("- Fixed-grid review method: $(Get-DisplayValue -Value $fixedGridReviewMethod)")
[void]$handoffLines.Add("- Fixed-grid review note: $(Get-DisplayValue -Value $fixedGridReviewNote)")
[void]$handoffLines.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$handoffLines.Add("- Section page setup review status: $(Get-DisplayValue -Value $sectionPageSetupReviewStatus)")
[void]$handoffLines.Add("- Section page setup reviewed at: $(Get-DisplayValue -Value $sectionPageSetupReviewedAt)")
[void]$handoffLines.Add("- Section page setup review method: $(Get-DisplayValue -Value $sectionPageSetupReviewMethod)")
[void]$handoffLines.Add("- Section page setup review note: $(Get-DisplayValue -Value $sectionPageSetupReviewNote)")
[void]$handoffLines.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$handoffLines.Add("- Page number fields review status: $(Get-DisplayValue -Value $pageNumberFieldsReviewStatus)")
[void]$handoffLines.Add("- Page number fields reviewed at: $(Get-DisplayValue -Value $pageNumberFieldsReviewedAt)")
[void]$handoffLines.Add("- Page number fields review method: $(Get-DisplayValue -Value $pageNumberFieldsReviewMethod)")
[void]$handoffLines.Add("- Page number fields review note: $(Get-DisplayValue -Value $pageNumberFieldsReviewNote)")
[void]$handoffLines.Add("- Curated visual regression bundles: $($curatedVisualReviewEntries.Count)")
[void]$handoffLines.Add("- Superseded review tasks: $(Get-DisplayValue -Value $supersededReviewTasksCount)")
[void]$handoffLines.Add("- Superseded task audit: $(Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Verification Snapshot")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Configure: $($summary.steps.configure.status)")
[void]$handoffLines.Add("- Build: $($summary.steps.build.status)")
[void]$handoffLines.Add("- Tests: $($summary.steps.tests.status)")
[void]$handoffLines.Add("- Template schema manifest: $(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$handoffLines.Add("- Project template smoke: $(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$handoffLines.Add("- Project template smoke candidate coverage: $(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount)) registered/unregistered/excluded")
[void]$handoffLines.Add("- Project template smoke schema baseline dirty / drift: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeDirtySchemaBaselineCount, $projectTemplateSmokeSchemaBaselineDriftCount))")
[void]$handoffLines.Add("- Install smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Visual gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$handoffLines.Add("- Superseded review tasks: $(Get-DisplayValue -Value $supersededReviewTasksCount)")
[void]$handoffLines.Add("- Section page setup task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$handoffLines.Add("- Page number fields task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
Add-ReleaseBlockerMarkdownSection -Lines $handoffLines -Summary $summary -RepoRoot $repoRoot
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Installed Package Entry Points")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Installed data dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $installedDataDir)")
[void]$handoffLines.Add("- Quickstart (EN): $(Get-DisplayPath -RepoRoot $repoRoot -Path $installedQuickstartEn)")
[void]$handoffLines.Add("- Quickstart (ZH-CN): $(Get-DisplayPath -RepoRoot $repoRoot -Path $installedQuickstartZh)")
[void]$handoffLines.Add("- Release template (EN): $(Get-DisplayPath -RepoRoot $repoRoot -Path $installedTemplateEn)")
[void]$handoffLines.Add("- Release template (ZH-CN): $(Get-DisplayPath -RepoRoot $repoRoot -Path $installedTemplateZh)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Evidence Files")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Consumer smoke document: $(Get-DisplayPath -RepoRoot $repoRoot -Path $consumerDocument)")
[void]$handoffLines.Add("- Release body: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath)")
[void]$handoffLines.Add("- Release summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath)")
[void]$handoffLines.Add("- Artifact guide: $(Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath)")
[void]$handoffLines.Add("- Reviewer checklist: $(Get-DisplayPath -RepoRoot $repoRoot -Path $reviewerChecklistPath)")
[void]$handoffLines.Add("- Template schema manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestPath)")
[void]$handoffLines.Add("- Template schema manifest summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestSummaryPath)")
[void]$handoffLines.Add("- Template schema manifest output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestOutputDir)")
[void]$handoffLines.Add("- Project template smoke manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeManifestPath)")
[void]$handoffLines.Add("- Project template smoke summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryPath)")
[void]$handoffLines.Add("- Project template smoke output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeOutputDir)")
[void]$handoffLines.Add("- Project template smoke candidate discovery: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeCandidateDiscoveryJson)")
[void]$handoffLines.Add("- Visual gate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath)")
[void]$handoffLines.Add("- Visual gate final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath)")
[void]$handoffLines.Add("- Superseded task audit: $(Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath)")
[void]$handoffLines.Add("- README gallery assets: $(Get-DisplayPath -RepoRoot $repoRoot -Path $readmeGalleryAssetsDir)")
[void]$handoffLines.Add("- Document review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $documentTaskDir)")
[void]$handoffLines.Add("- Fixed-grid review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $fixedGridTaskDir)")
[void]$handoffLines.Add("- Section page setup review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$handoffLines.Add("- Page number fields review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Curated Visual Regression Bundles")
[void]$handoffLines.Add("")
if ($curatedVisualReviewEntries.Count -gt 0) {
    foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
        [void]$handoffLines.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")
        [void]$handoffLines.Add("- $($curatedVisualReview.label) review status: $(Get-DisplayValue -Value $curatedVisualReview.review_status)")
        [void]$handoffLines.Add("- $($curatedVisualReview.label) reviewed at: $(Get-DisplayValue -Value $curatedVisualReview.reviewed_at)")
        [void]$handoffLines.Add("- $($curatedVisualReview.label) review method: $(Get-DisplayValue -Value $curatedVisualReview.review_method)")
        [void]$handoffLines.Add("- $($curatedVisualReview.label) review note: $(Get-DisplayValue -Value $curatedVisualReview.review_note)")
        [void]$handoffLines.Add("- $($curatedVisualReview.label) review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.task_dir)")
        if (-not [string]::IsNullOrWhiteSpace($curatedVisualReview.id)) {
            [void]$handoffLines.Add("- $($curatedVisualReview.label) open-latest command: pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind $($curatedVisualReview.id)-visual-regression-bundle -PrintPrompt")
        }
        [void]$handoffLines.Add("")
    }
} else {
    [void]$handoffLines.Add("- Curated visual regression bundles: (not available)")
    [void]$handoffLines.Add("")
}
[void]$handoffLines.Add("## Reproduction Commands")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($releaseChecksCommand)
[void]$handoffLines.Add($releaseGateCommand)
[void]$handoffLines.Add($packageAssetsCommand)
[void]$handoffLines.Add($openDocumentTaskCommand)
[void]$handoffLines.Add($openFixedGridTaskCommand)
[void]$handoffLines.Add($openSectionPageSetupTaskCommand)
[void]$handoffLines.Add($openPageNumberFieldsTaskCommand)
foreach ($openCuratedVisualTaskCommand in $openCuratedVisualTaskCommands) {
    [void]$handoffLines.Add($openCuratedVisualTaskCommand)
}
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("If the visual verdict changes after screenshot-backed manual review, rerun the shortest verdict sync first:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($syncLatestCommand)
[void]$handoffLines.Add($(if (-not [string]::IsNullOrWhiteSpace($syncProjectTemplateSmokeCommand)) { $syncProjectTemplateSmokeCommand } else { "" }))
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("That command auto-detects the latest review task, syncs the final verdict back into the gate summary, and refreshes the detected release bundle.")
[void]$handoffLines.Add("If project template smoke also carries a later manual visual verdict, use the second command to sync its smoke summary and refresh the same release bundle.")
[void]$handoffLines.Add("")
if (-not [string]::IsNullOrWhiteSpace($findSupersededTasksCommand)) {
    [void]$handoffLines.Add("Rerun the stale-task audit directly when you need to inspect older preserved task directories:")
    [void]$handoffLines.Add("")
    [void]$handoffLines.Add('```powershell')
    [void]$handoffLines.Add($findSupersededTasksCommand)
    [void]$handoffLines.Add('```')
    [void]$handoffLines.Add("")
}
if (-not [string]::IsNullOrWhiteSpace($syncExplicitCommand)) {
    [void]$handoffLines.Add("If the inferred gate or release paths need to be overridden manually, use:")
    [void]$handoffLines.Add("")
    [void]$handoffLines.Add('```powershell')
    [void]$handoffLines.Add($syncExplicitCommand)
    [void]$handoffLines.Add('```')
    [void]$handoffLines.Add("")
}
[void]$handoffLines.Add("Package the release-facing ZIP files after the verification bundle is signed off:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($packageAssetsCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("Refresh GitHub Release ZIP assets and audited notes without changing draft/public state:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($publishWorkflowCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add('Sync the audited `release_body.zh-CN.md` into the GitHub Release notes with:')
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($syncReleaseNotesCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("When the final local signoff is complete and the GitHub Release is ready to go live:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($publishReleaseCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("If you want one command that uploads ZIPs and syncs the audited notes together:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($publishWorkflowCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("When the GitHub publish flow should also make the Release public:")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```powershell')
[void]$handoffLines.Add($publishWorkflowFinalCommand)
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add(('If the validated bundle already exists in a self-hosted Windows runner workspace, you can use GitHub Actions `{0}` (`{1}`) for a one-click asset/note refresh, or `{2}` (`{3}`) for the final public release.' -f $refreshWorkflowName, $refreshWorkflowFile, $publishWorkflowName, $publishWorkflowFile))
[void]$handoffLines.Add("")
[void]$handoffLines.Add("### GitHub Web UI: 4-Step Runbook")
[void]$handoffLines.Add("")
[void]$handoffLines.Add(('1. Open the repository `Actions` tab and choose `{0}` for a safe refresh, or `{1}` for the final public release.' -f $refreshWorkflowName, $publishWorkflowName))
[void]$handoffLines.Add('2. Pick the branch that already contains this validated release bundle in the self-hosted runner workspace and click `Run workflow`.')
[void]$handoffLines.Add('3. No additional form input is required; wait for the job to finish, then inspect the uploaded artifact (`release-refresh-output` or `release-publish-output`) if you need the packaged ZIPs.')
[void]$handoffLines.Add("4. Check the GitHub Release page. Use the refresh workflow for note/asset updates that should stay private, and the publish workflow only after final local Word signoff is complete.")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Release Body Seed")
[void]$handoffLines.Add("")
[void]$handoffLines.Add('```md')
[void]$handoffLines.Add("# FeatherDoc v$(if ($projectVersion) { $projectVersion } else { '<version>' })")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Highlights")
[void]$handoffLines.Add("- Fill from CHANGELOG.md.")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Validation")
[void]$handoffLines.Add("- MSVC configure/build: $($summary.steps.configure.status) / $($summary.steps.build.status)")
[void]$handoffLines.Add("- ctest: $($summary.steps.tests.status)")
[void]$handoffLines.Add("- Template schema manifest: $(if ($templateSchemaManifestStatus) { $templateSchemaManifestStatus } else { '<completed|failed|not_requested>' })")
[void]$handoffLines.Add("- Template schema manifest passed: $(if ($templateSchemaManifestPassed) { $templateSchemaManifestPassed } else { '<True|False>' })")
[void]$handoffLines.Add("- Template schema manifest entries/drifts: $(if ($templateSchemaManifestEntryCount -or $templateSchemaManifestDriftCount) { '{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount } else { '<entry_count>/<drift_count>' })")
[void]$handoffLines.Add("- Project template smoke: $(if ($projectTemplateSmokeStatus) { $projectTemplateSmokeStatus } else { '<completed|failed|not_requested>' })")
[void]$handoffLines.Add("- Project template smoke passed: $(if ($projectTemplateSmokePassed) { $projectTemplateSmokePassed } else { '<True|False>' })")
[void]$handoffLines.Add("- Project template smoke entries/failed: $(if ($projectTemplateSmokeEntryCount -or $projectTemplateSmokeFailedEntryCount) { '{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount } else { '<entry_count>/<failed_entry_count>' })")
[void]$handoffLines.Add("- Project template smoke schema baseline dirty/drift: $(if ($projectTemplateSmokeDirtySchemaBaselineCount -or $projectTemplateSmokeSchemaBaselineDriftCount) { '{0}/{1}' -f $projectTemplateSmokeDirtySchemaBaselineCount, $projectTemplateSmokeSchemaBaselineDriftCount } else { '<dirty_schema_baseline_count>/<schema_baseline_drift_count>' })")
[void]$handoffLines.Add("- Project template smoke candidates registered/unregistered/excluded: $(if ($projectTemplateSmokeRegisteredCandidateCount -or $projectTemplateSmokeUnregisteredCandidateCount -or $projectTemplateSmokeExcludedCandidateCount) { '{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount } else { '<registered>/<unregistered>/<excluded>' })")
[void]$handoffLines.Add("- Project template smoke visual verdict: $(if ($projectTemplateSmokeVisualVerdict) { $projectTemplateSmokeVisualVerdict } else { '<pass|fail|pending_manual_review|not_applicable>' })")
[void]$handoffLines.Add("- install + find_package smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Word visual release gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- Visual verdict: $(if ($visualVerdict) { $visualVerdict } else { '<pass|fail|pending_manual_review>' })")
[void]$handoffLines.Add("- Release blockers: $(Get-ReleaseBlockerCount -Summary $summary)")
[void]$handoffLines.Add("- Section page setup verdict: $(if ($sectionPageSetupVerdict) { $sectionPageSetupVerdict } else { '<pass|fail|pending_manual_review>' })")
[void]$handoffLines.Add("- Page number fields verdict: $(if ($pageNumberFieldsVerdict) { $pageNumberFieldsVerdict } else { '<pass|fail|pending_manual_review>' })")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$handoffLines.Add("- $($curatedVisualReview.label) verdict: $(if ($curatedVisualReview.verdict) { $curatedVisualReview.verdict } else { '<pass|fail|pending_manual_review>' })")
}
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Installed Package Entry Points")
[void]$handoffLines.Add("- $(if ($installedQuickstartZh) { Get-DisplayPath -RepoRoot $repoRoot -Path $installedQuickstartZh } else { 'share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($installedTemplateZh) { Get-DisplayPath -RepoRoot $repoRoot -Path $installedTemplateZh } else { 'share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($installedDataDir) { Get-DisplayPath -RepoRoot $repoRoot -Path (Join-Path $installedDataDir 'visual-validation') } else { 'share/FeatherDoc/visual-validation' })")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Evidence Files")
[void]$handoffLines.Add("- $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
[void]$handoffLines.Add("- $(if (Test-Path -LiteralPath $finalReviewPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath } else { 'output/release-candidate-checks/report/final_review.md' })")
[void]$handoffLines.Add("- $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputPath)")
[void]$handoffLines.Add("- $(if ($releaseBodyPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath } else { 'output/release-candidate-checks/report/release_body.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($releaseSummaryPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath } else { 'output/release-candidate-checks/report/release_summary.zh-CN.md' })")
[void]$handoffLines.Add("- $(if ($artifactGuidePath) { Get-DisplayPath -RepoRoot $repoRoot -Path $artifactGuidePath } else { 'output/release-candidate-checks/report/ARTIFACT_GUIDE.md' })")
[void]$handoffLines.Add("- $(if ($reviewerChecklistPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $reviewerChecklistPath } else { 'output/release-candidate-checks/report/REVIEWER_CHECKLIST.md' })")
[void]$handoffLines.Add("- $(if ($templateSchemaManifestSummaryPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestSummaryPath } else { 'output/release-candidate-checks/report/template-schema-manifest-checks/summary.json' })")
[void]$handoffLines.Add("- $(if ($projectTemplateSmokeSummaryPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryPath } else { 'output/release-candidate-checks/report/project-template-smoke/summary.json' })")
[void]$handoffLines.Add("- $(if ($projectTemplateSmokeCandidateDiscoveryJson) { Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeCandidateDiscoveryJson } else { 'output/release-candidate-checks/report/project-template-smoke/candidate_discovery.json' })")
[void]$handoffLines.Add("- $(if ($gateSummaryPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath } else { 'output/word-visual-release-gate/report/gate_summary.json' })")
[void]$handoffLines.Add("- $(if ($gateFinalReviewPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath } else { 'output/word-visual-release-gate/report/gate_final_review.md' })")
[void]$handoffLines.Add('```')

$handoff = $handoffLines -join [Environment]::NewLine

$handoff | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Release handoff: $resolvedOutputPath"
