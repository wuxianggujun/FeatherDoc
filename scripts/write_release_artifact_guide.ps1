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

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    $gateFlowVerdict = Get-OptionalPropertyValue -Object $gateFlow -Name "review_verdict"
    if (-not [string]::IsNullOrWhiteSpace($gateFlowVerdict)) {
        return $gateFlowVerdict
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

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "ARTIFACT_GUIDE.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
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
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}
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
$sectionPageSetupTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)

$installLeaf = ""
if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installLeaf = Split-Path -Leaf $installPrefix
}
if ([string]::IsNullOrWhiteSpace($installLeaf)) {
    $installLeaf = "build-msvc-install"
}

$artifactQuickstartPath = Join-Path $installLeaf "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$artifactTemplatePath = Join-Path $installLeaf "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
$artifactPreviewDir = Join-Path $installLeaf "share\FeatherDoc\visual-validation"

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$syncProjectTemplateSmokeCommand = ""
if (-not [string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryPath)) {
    $syncProjectTemplateSmokeCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_smoke_visual_verdict.ps1 -SummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryPath),
        $summaryCommandPath
}
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$assetOutputDir = if ($releaseVersion) {
    Join-Path ("output\release-assets") ("v{0}" -f $releaseVersion)
} else {
    "output\release-assets\v<version>"
}
$packageAssetsCommand = if ($releaseVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -ReleaseVersion "{1}"' -f `
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
[void]$lines.Add("# Release Metadata Artifact Guide")
[void]$lines.Add("")
[void]$lines.Add("- Generated at: $(Get-Date -Format s)")
[void]$lines.Add("- Execution status: $($summary.execution_status)")
[void]$lines.Add("- Template schema manifest status: $(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$lines.Add("- Template schema manifest passed: $(Get-DisplayValue -Value $templateSchemaManifestPassed)")
[void]$lines.Add("- Template schema manifest entries / drifts: $(Get-DisplayValue -Value ('{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount))")
[void]$lines.Add("- Project template smoke status: $(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$lines.Add("- Project template smoke passed: $(Get-DisplayValue -Value $projectTemplateSmokePassed)")
[void]$lines.Add("- Project template smoke entries / failed: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount))")
[void]$lines.Add("- Project template smoke schema baseline dirty / drift: $(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeDirtySchemaBaselineCount, $projectTemplateSmokeSchemaBaselineDriftCount))")
[void]$lines.Add("- Project template smoke visual verdict: $(Get-DisplayValue -Value $projectTemplateSmokeVisualVerdict)")
[void]$lines.Add("- Project template smoke pending visual reviews: $(Get-DisplayValue -Value $projectTemplateSmokePendingReviewCount)")
[void]$lines.Add("- Project template smoke full coverage required: $(Get-DisplayValue -Value $projectTemplateSmokeRequireFullCoverage)")
[void]$lines.Add("- Project template smoke candidates registered / unregistered / excluded: $(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount))")
[void]$lines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
[void]$lines.Add("- Smoke verdict: $(Get-DisplayValue -Value $smokeVerdict)")
[void]$lines.Add("- Fixed-grid verdict: $(Get-DisplayValue -Value $fixedGridVerdict)")
[void]$lines.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$lines.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$lines.Add("- Curated visual regression bundles: $($curatedVisualReviewEntries.Count)")
[void]$lines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$lines.Add("- Summary JSON: $(Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedSummaryPath)")
[void]$lines.Add("")
[void]$lines.Add("## Start Here")
[void]$lines.Add("")
[void]$lines.Add('1. Open `REVIEWER_CHECKLIST.md` first for the reviewer flow.')
[void]$lines.Add('2. Open `release_summary.zh-CN.md` next for the GitHub Release front-page bullets.')
[void]$lines.Add('3. Open `release_body.zh-CN.md` for the full Chinese release body, then `release_handoff.md` for evidence and repro links.')
[void]$lines.Add("")
[void]$lines.Add("## Files In This Report Directory")
[void]$lines.Add("")
[void]$lines.Add("- Reviewer checklist: $(Get-DisplayPath -RepoRoot $repoRoot -Path $reviewerChecklistPath)")
[void]$lines.Add("- Short summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseSummaryPath)")
[void]$lines.Add("- Full release body: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBodyPath)")
[void]$lines.Add("- Release handoff: $(Get-DisplayPath -RepoRoot $repoRoot -Path $releaseHandoffPath)")
[void]$lines.Add("- Final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $finalReviewPath)")
[void]$lines.Add("- Consumer smoke document: $(Get-DisplayPath -RepoRoot $repoRoot -Path $consumerDocument)")
[void]$lines.Add("- Template schema manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestPath)")
[void]$lines.Add("- Template schema manifest summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestSummaryPath)")
[void]$lines.Add("- Template schema manifest output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestOutputDir)")
[void]$lines.Add("- Project template smoke manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeManifestPath)")
[void]$lines.Add("- Project template smoke summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryPath)")
[void]$lines.Add("- Project template smoke output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeOutputDir)")
[void]$lines.Add("- Project template smoke candidate discovery: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeCandidateDiscoveryJson)")
[void]$lines.Add("- Visual gate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath)")
[void]$lines.Add("- Visual gate final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath)")
[void]$lines.Add("- README gallery assets: $(Get-DisplayPath -RepoRoot $repoRoot -Path $readmeGalleryAssetsDir)")
[void]$lines.Add("- Section page setup review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$lines.Add("- Page number fields review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
[void]$lines.Add("")
[void]$lines.Add("## Curated Visual Regression Bundles")
[void]$lines.Add("")
if ($curatedVisualReviewEntries.Count -gt 0) {
    foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
        [void]$lines.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")
        [void]$lines.Add("- $($curatedVisualReview.label) review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.task_dir)")
        [void]$lines.Add("")
    }
} else {
    [void]$lines.Add("- Curated visual regression bundles: (not available)")
    [void]$lines.Add("")
}
[void]$lines.Add("## Installed Package Entry Points")
[void]$lines.Add("")
[void]$lines.Add("If you are browsing the uploaded release-metadata artifact, start from these artifact-local paths:")
[void]$lines.Add("")
[void]$lines.Add(('- `{0}`' -f $artifactQuickstartPath))
[void]$lines.Add(('- `{0}`' -f $artifactTemplatePath))
[void]$lines.Add(('- `{0}`' -f $artifactPreviewDir))
[void]$lines.Add("")
[void]$lines.Add("Those files point from preview PNGs to the local repro scripts and review-task wrappers.")
[void]$lines.Add("")
[void]$lines.Add("## Package Release Assets")
[void]$lines.Add("")
[void]$lines.Add("- Release assets output dir: $assetOutputDir")
[void]$lines.Add('```powershell')
[void]$lines.Add($packageAssetsCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("To refresh GitHub Release ZIP assets and audited notes without changing draft/public state:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($publishWorkflowCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("To sync the audited `release_body.zh-CN.md` into the matching GitHub Release notes:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($syncReleaseNotesCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("When the final local Windows + Word signoff is complete and the GitHub Release is ready to go live:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($publishReleaseCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("If you want a single command that uploads the ZIPs and syncs the audited notes together:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($publishWorkflowCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("When the GitHub publish flow should also make the Release public:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($publishWorkflowFinalCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add(("If the validated bundle already exists in a self-hosted Windows runner workspace, you can use GitHub Actions `{0}` (`{1}`) for a one-click asset/note refresh, or `{2}` (`{3}`) for the final public release." -f $refreshWorkflowName, $refreshWorkflowFile, $publishWorkflowName, $publishWorkflowFile))
[void]$lines.Add("")
[void]$lines.Add("### GitHub Web UI: 4-Step Runbook")
[void]$lines.Add("")
[void]$lines.Add(("1. Open the repository `Actions` tab and choose `{0}` for a safe refresh, or `{1}` for the final public release." -f $refreshWorkflowName, $publishWorkflowName))
[void]$lines.Add("2. Pick the branch that already contains this validated release bundle in the self-hosted runner workspace and click `Run workflow`.")
[void]$lines.Add("3. No additional form input is required; wait for the job to finish, then inspect the uploaded artifact (`release-refresh-output` or `release-publish-output`) if you need the packaged ZIPs.")
[void]$lines.Add("4. Check the GitHub Release page. Use the refresh workflow for note/asset updates that should stay private, and the publish workflow only after final local Word signoff is complete.")
[void]$lines.Add("")
[void]$lines.Add("## Refresh After A Later Visual Verdict Update")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($syncLatestCommand)
[void]$lines.Add($(if (-not [string]::IsNullOrWhiteSpace($syncProjectTemplateSmokeCommand)) { $syncProjectTemplateSmokeCommand } else { "" }))
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("Use that command when the screenshot-backed manual review changes the visual verdict and you want to sync the latest task verdict back into the gate summary while refreshing the generated release-facing materials without rerunning the full preflight.")
[void]$lines.Add("If project template smoke also has a later manual visual verdict update, use the second command to sync its smoke summary and refresh the same release-facing materials.")
[void]$lines.Add("")
[void]$lines.Add("## Notes")
[void]$lines.Add("")
[void]$lines.Add("- CI artifacts intentionally keep the Word visual gate skipped; the final screenshot-backed verdict still belongs to a local Windows + Microsoft Word run.")
[void]$lines.Add('- The installed `share/FeatherDoc` tree carries the preview PNGs, quickstarts, and release templates; the `report` directory carries the generated release notes and evidence indexes.')

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Artifact guide: $resolvedOutputPath"
