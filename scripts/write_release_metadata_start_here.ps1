param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = "",
    [switch]$ArtifactRootLayout
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

. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")

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
    if ($null -eq $taskInfo) {
        return ""
    }

    $nestedTask = Get-OptionalPropertyObject -Object $taskInfo -Name "task"
    if ($null -ne $nestedTask) {
        $taskInfo = $nestedTask
    }

    return Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
}

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}
$summaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedSummaryPath

$reportDir = Split-Path -Parent $resolvedSummaryPath
$summaryRootDir = Split-Path -Parent $reportDir
$summaryRootLeaf = Split-Path -Leaf $summaryRootDir

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path $summaryRootDir "START_HERE.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$releaseVersion = Get-OptionalPropertyValue -Object $summary -Name "release_version"
$installDir = Get-OptionalPropertyValue -Object $summary -Name "install_dir"
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
$projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "output_dir"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeOutputDir)) {
    $projectTemplateSmokeOutputDir = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "output_dir"
}
$projectTemplateSmokeSummaryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryJson)) {
    $projectTemplateSmokeSummaryJson = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "summary_json"
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
$gateSummaryPath = Get-OptionalPropertyValue -Object $visualGateStep -Name "summary_json"
$gateSummary = $null
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath) -and (Test-Path -LiteralPath $gateSummaryPath)) {
    $gateSummary = Get-Content -Raw $gateSummaryPath | ConvertFrom-Json
}
$visualVerdict = Get-OptionalPropertyValue -Object $visualGateStep -Name "visual_verdict"
if ([string]::IsNullOrWhiteSpace($visualVerdict)) {
    $visualVerdict = Get-OptionalPropertyValue -Object $summary -Name "visual_verdict"
}
if ([string]::IsNullOrWhiteSpace($visualVerdict)) {
    $visualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
}
$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$sectionPageSetupTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)
$installDirLeaf = if ([string]::IsNullOrWhiteSpace($installDir)) {
    "build-msvc-install"
} else {
    Split-Path -Leaf $installDir
}

$guideLocalPath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistLocalPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$releaseSummaryLocalPath = Join-Path $reportDir "release_summary.zh-CN.md"
$releaseBodyLocalPath = Join-Path $reportDir "release_body.zh-CN.md"
$quickstartLocalPath = Join-Path $installDir "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$templateLocalPath = Join-Path $installDir "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"

$startHereArtifactPath = Join-Path ("output\{0}" -f $summaryRootLeaf) "START_HERE.md"
$guideArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "ARTIFACT_GUIDE.md"
$reviewerChecklistArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "REVIEWER_CHECKLIST.md"
$releaseSummaryArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "release_summary.zh-CN.md"
$releaseBodyArtifactPath = Join-Path ("output\{0}\report" -f $summaryRootLeaf) "release_body.zh-CN.md"
$quickstartArtifactPath = Join-Path $installDirLeaf "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
$templateArtifactPath = Join-Path $installDirLeaf "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"

$syncLatestCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1"
$syncProjectTemplateSmokeCommand = ""
if (-not [string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryJson)) {
    $syncProjectTemplateSmokeCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_smoke_visual_verdict.ps1 -SummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        (Get-RepoRelativePath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryJson),
        $summaryCommandPath
}
$packageAssetsCommand = if ($releaseVersion) {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -ReleaseVersion "{1}"' -f `
        $summaryCommandPath, $releaseVersion
} else {
    'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
        $summaryCommandPath
}
$githubRefreshCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\publish_github_release.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
if ($releaseVersion) {
    $githubRefreshCommand += (' -ReleaseTag "v{0}"' -f $releaseVersion)
}
$githubPublishCommand = $githubRefreshCommand + " -Publish"
$refreshWorkflowName = "Release Refresh"
$refreshWorkflowFile = ".github/workflows/release-refresh.yml"
$publishWorkflowName = "Release Publish"
$publishWorkflowFile = ".github/workflows/release-publish.yml"

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# Release Metadata Start Here")
[void]$lines.Add("")
if ($ArtifactRootLayout) {
    [void]$lines.Add("If you just downloaded the release metadata artifact, use this file as the shortest jump into the generated report bundle.")
} else {
    [void]$lines.Add('Use this file as the shortest jump into the generated release-metadata bundle after a local `run_release_candidate_checks.ps1` run.')
}
[void]$lines.Add("")
[void]$lines.Add("## Open These First")
[void]$lines.Add("")

if ($ArtifactRootLayout) {
    [void]$lines.Add(('- `START_HERE.md`: `{0}`' -f $startHereArtifactPath))
    [void]$lines.Add(('- `ARTIFACT_GUIDE.md`: `{0}`' -f $guideArtifactPath))
    [void]$lines.Add(('- `REVIEWER_CHECKLIST.md`: `{0}`' -f $reviewerChecklistArtifactPath))
    [void]$lines.Add(('- `release_summary.zh-CN.md`: `{0}`' -f $releaseSummaryArtifactPath))
    [void]$lines.Add(('- `release_body.zh-CN.md`: `{0}`' -f $releaseBodyArtifactPath))
    [void]$lines.Add(('- Installed quickstart: `{0}`' -f $quickstartArtifactPath))
    [void]$lines.Add(('- Installed release template: `{0}`' -f $templateArtifactPath))
} else {
    [void]$lines.Add('- `ARTIFACT_GUIDE.md`: `report/ARTIFACT_GUIDE.md`')
    [void]$lines.Add('- `REVIEWER_CHECKLIST.md`: `report/REVIEWER_CHECKLIST.md`')
    [void]$lines.Add('- `release_summary.zh-CN.md`: `report/release_summary.zh-CN.md`')
    [void]$lines.Add('- `release_body.zh-CN.md`: `report/release_body.zh-CN.md`')
    [void]$lines.Add(('- Installed quickstart: `{0}`' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $quickstartLocalPath)))
    [void]$lines.Add(('- Installed release template: `{0}`' -f (Get-DisplayPath -RepoRoot $repoRoot -Path $templateLocalPath)))
}

[void]$lines.Add("")
[void]$lines.Add("## Verification Snapshot")
[void]$lines.Add("")
[void]$lines.Add("- Execution status: $($summary.execution_status)")
[void]$lines.Add("- Template schema gate status: $(Get-DisplayValue -Value $templateSchemaStatus)")
[void]$lines.Add("- Template schema matches baseline: $(Get-DisplayValue -Value $templateSchemaMatches)")
[void]$lines.Add("- Template schema drift counts (added/removed/changed): $(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $templateSchemaAddedTargetCount, $templateSchemaRemovedTargetCount, $templateSchemaChangedTargetCount))")
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
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")
}

[void]$lines.Add("")
[void]$lines.Add("## Template Schema Evidence")
[void]$lines.Add("")
[void]$lines.Add("- Template schema input DOCX: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaInputDocx)")
[void]$lines.Add("- Template schema baseline: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaBaseline)")
[void]$lines.Add("- Template schema generated output: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaGeneratedOutput)")

[void]$lines.Add("")
[void]$lines.Add("## Template Schema Manifest Evidence")
[void]$lines.Add("")
[void]$lines.Add("- Template schema manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestPath)")
[void]$lines.Add("- Template schema manifest summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestSummaryJson)")
[void]$lines.Add("- Template schema manifest output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $templateSchemaManifestOutputDir)")
[void]$lines.Add("- Project template smoke manifest: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeManifestPath)")
[void]$lines.Add("- Project template smoke summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeSummaryJson)")
[void]$lines.Add("- Project template smoke output dir: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeOutputDir)")
[void]$lines.Add("- Project template smoke candidate discovery: $(Get-DisplayPath -RepoRoot $repoRoot -Path $projectTemplateSmokeCandidateDiscoveryJson)")

[void]$lines.Add("")
[void]$lines.Add("## Visual Task Shortcuts")
[void]$lines.Add("")
[void]$lines.Add("- Section page setup review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$lines.Add("- Page number fields review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.task_dir)")
    if (-not [string]::IsNullOrWhiteSpace($curatedVisualReview.id)) {
        [void]$lines.Add("- $($curatedVisualReview.label) open-latest command: pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind $($curatedVisualReview.id)-visual-regression-bundle -PrintPrompt")
    }
}

[void]$lines.Add("")
[void]$lines.Add("## Reviewer Flow")
[void]$lines.Add("")
if ($ArtifactRootLayout) {
    [void]$lines.Add('1. Open `START_HERE.md` next for the summary-root view of the generated bundle.')
    [void]$lines.Add('2. Open `ARTIFACT_GUIDE.md` for the artifact file map, then follow `REVIEWER_CHECKLIST.md` line by line.')
    [void]$lines.Add('3. Use `release_summary.zh-CN.md` and `release_body.zh-CN.md` as the final release notes.')
} else {
    [void]$lines.Add('1. Open `ARTIFACT_GUIDE.md` for the artifact file map.')
    [void]$lines.Add('2. Follow `REVIEWER_CHECKLIST.md` line by line.')
    [void]$lines.Add('3. Use `release_summary.zh-CN.md` and `release_body.zh-CN.md` as the final release notes.')
}
[void]$lines.Add("")
[void]$lines.Add("## Refresh After A Later Visual Verdict Update")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($syncLatestCommand)
[void]$lines.Add($(if (-not [string]::IsNullOrWhiteSpace($syncProjectTemplateSmokeCommand)) { $syncProjectTemplateSmokeCommand } else { "" }))
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("Use that command locally after the screenshot-backed manual review changes the final visual verdict. It will sync the detected latest task verdict back into the gate summary and refresh the release bundle.")
[void]$lines.Add("If project template smoke also carries manual visual review, use the second command to sync its refreshed verdict back into the smoke summary and the same release bundle.")
[void]$lines.Add("")
[void]$lines.Add("## Publish Checklist")
[void]$lines.Add("")
[void]$lines.Add('- Confirm `Execution status`, `Visual verdict`, and project-template visual verdict are all `pass`.')
[void]$lines.Add("- Run the local packaging command first; it regenerates ZIP files and does not contact GitHub.")
[void]$lines.Add("- Use GitHub refresh to upload ZIP assets and sync audited release notes without flipping a draft release public.")
[void]$lines.Add("- Use GitHub publish only after final signoff, because it can make the target release public.")
[void]$lines.Add("")
[void]$lines.Add("## Package Public Release Assets (Local Only)")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($packageAssetsCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add('Run this after the release notes are finalized. It creates the install ZIP, visual gallery ZIP, release-evidence ZIP, and manifest under `output/release-assets/v<version>`.')
[void]$lines.Add("")
[void]$lines.Add("## GitHub Release Refresh (Upload Assets + Sync Notes)")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($githubRefreshCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add('Use this when the target GitHub Release should receive refreshed ZIP assets plus the audited `release_body.zh-CN.md` notes. If the target release is a draft, it remains a draft; if it is already public, this updates public assets and notes.')
[void]$lines.Add("")
[void]$lines.Add("## GitHub Release Publish (Final Public Step)")
[void]$lines.Add("")
[void]$lines.Add("Run this only when the release is ready to go live and final local signoff is complete:")
[void]$lines.Add("")
[void]$lines.Add('```powershell')
[void]$lines.Add($githubPublishCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add('This command uploads the ZIPs, syncs audited notes, and passes `-Publish` through so the target GitHub Release can become public.')
[void]$lines.Add("")
[void]$lines.Add(('If the validated bundle already exists in a self-hosted Windows runner workspace, you may trigger GitHub Actions `{0}` (`{1}`) for a safe refresh, or `{2}` (`{3}`) for the final public release, instead of running the wrapper locally.' -f $refreshWorkflowName, $refreshWorkflowFile, $publishWorkflowName, $publishWorkflowFile))
[void]$lines.Add("")
[void]$lines.Add("### GitHub Web UI: 4-Step Runbook")
[void]$lines.Add("")
[void]$lines.Add(('1. Open the repository `Actions` tab and choose `{0}` for a safe refresh, or `{1}` for the final public release.' -f $refreshWorkflowName, $publishWorkflowName))
[void]$lines.Add('2. Pick the branch that already contains this validated release bundle in the self-hosted runner workspace and click `Run workflow`.')
[void]$lines.Add('3. No additional form input is required; wait for the job to finish, then inspect the uploaded artifact (`release-refresh-output` or `release-publish-output`) if you need the packaged ZIPs.')
[void]$lines.Add("4. Check the GitHub Release page. Use the refresh workflow for note/asset updates that should stay private, and the publish workflow only after final local Word signoff is complete.")

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
($lines -join [Environment]::NewLine) | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Start here: $resolvedOutputPath"
