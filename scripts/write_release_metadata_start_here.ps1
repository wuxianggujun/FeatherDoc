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

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [AllowEmptyString()][string]$Text
    )

    $content = if ($null -eq $Text) { "" } else { $Text }
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $content, $encoding)
}

. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")
. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")

function Get-DisplayValue {
    param($Value)

    if ($null -eq $Value) {
        return "(not available)"
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    $text = [string]$Value
    if ([string]::IsNullOrWhiteSpace($text)) {
        return "(not available)"
    }

    return $text
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

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($candidate)
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
$requiresProjectTemplateGovernanceSignoff = Test-ReleaseManifestSignoffRequiresProjectTemplateGovernance -Summary $summary
$projectTemplateChecklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $summary
$projectTemplateChecklistMaterialSafetyAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $summary
$wordVisualStandardReviewMetadataEvidenceLine = Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine -Summary $summary
$hasProjectTemplateReleaseEntryEvidence = (
    -not [string]::IsNullOrWhiteSpace($projectTemplateChecklistHandoffEvidenceLine) -or
    -not [string]::IsNullOrWhiteSpace($projectTemplateChecklistMaterialSafetyAuditEvidenceLine)
)
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
$smokeReviewResultPath = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewResultPath = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewResultPath = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewResultPath = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeFinalReviewPath = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridFinalReviewPath = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupFinalReviewPath = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsFinalReviewPath = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$sectionPageSetupTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsTaskDir = Get-VisualTaskDir -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)
$visualReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine -VisualGateSummary $visualGateStep -GateSummary $gateSummary
$pdfVisualGateSummaryPath = Get-PdfVisualGateSummaryPath -Summary $summary
$pdfVisualGateEvidence = Get-PdfVisualGateEvidence -SummaryPath $pdfVisualGateSummaryPath -RepoRoot $repoRoot
$pdfBoundedCtestEvidence = Get-PdfBoundedCtestEvidence -Summary $summary
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
$releaseAssetsManifestPath = if ($releaseVersion) {
    Join-Path (Join-Path "output\release-assets" ("v{0}" -f $releaseVersion)) "release_assets_manifest.json"
} else {
    "output\release-assets\v<version>\release_assets_manifest.json"
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
[void]$lines.Add("- Release blockers: $(Get-ReleaseBlockerCount -Summary $summary)")
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
if ($requiresProjectTemplateGovernanceSignoff -or $hasProjectTemplateReleaseEntryEvidence) {
    [void]$lines.Add('- Project template release readiness checklist: `docs/project_template_release_readiness_checklist_zh.rst`')
}
if (-not [string]::IsNullOrWhiteSpace($projectTemplateChecklistHandoffEvidenceLine)) {
    [void]$lines.Add("- $projectTemplateChecklistHandoffEvidenceLine")
}
if (-not [string]::IsNullOrWhiteSpace($projectTemplateChecklistMaterialSafetyAuditEvidenceLine)) {
    [void]$lines.Add("- $projectTemplateChecklistMaterialSafetyAuditEvidenceLine")
}
if (-not [string]::IsNullOrWhiteSpace($wordVisualStandardReviewMetadataEvidenceLine)) {
    [void]$lines.Add("- $wordVisualStandardReviewMetadataEvidenceLine")
}
[void]$lines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
if (-not [string]::IsNullOrWhiteSpace($visualReviewTaskSummaryLine)) {
    [void]$lines.Add("- $visualReviewTaskSummaryLine")
}
if (-not [string]::IsNullOrWhiteSpace($pdfVisualGateEvidence.summary_json)) {
    [void]$lines.Add("- PDF visual gate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pdfVisualGateEvidence.summary_json)")
    [void]$lines.Add("- PDF visual gate evidence status: $(Get-DisplayValue -Value $pdfVisualGateEvidence.status)")
    if ($pdfVisualGateEvidence.status -eq "loaded") {
        [void]$lines.Add("- PDF visual gate verdict: $(Get-DisplayValue -Value $pdfVisualGateEvidence.verdict)")
        [void]$lines.Add("- PDF visual gate aggregate contact sheet: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pdfVisualGateEvidence.aggregate_contact_sheet)")
        [void]$lines.Add("- PDF CJK manifest samples: $(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_manifest_count)")
        [void]$lines.Add("- PDF CJK copy/search samples: $(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_copy_search_count)")
        [void]$lines.Add("- PDF CJK missing text count: $(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_missing_text_count)")
        [void]$lines.Add("- PDF visual baseline manifest samples: $(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_manifest_count)")
        [void]$lines.Add("- PDF visual baselines: $(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_count)")
    } elseif (-not [string]::IsNullOrWhiteSpace($pdfVisualGateEvidence.error)) {
        [void]$lines.Add("- PDF visual gate evidence error: $($pdfVisualGateEvidence.error)")
    }
}
if ($pdfBoundedCtestEvidence.status -ne "not_available") {
    [void]$lines.Add("- PDF bounded CTest auxiliary evidence: status=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.status), summaries=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.summary_count), pass=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.pass_count), selected_tests=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.selected_test_count), skipped_tests=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.skipped_test_count)")
    [void]$lines.Add("- PDF bounded CTest auxiliary subsets: $(Get-DisplayValue -Value (@($pdfBoundedCtestEvidence.subsets) -join ', '))")
    [void]$lines.Add("- PDF bounded CTest auxiliary summaries: $(Get-DisplayValue -Value (@($pdfBoundedCtestEvidence.summary_json_display) -join ', '))")
}
[void]$lines.Add('- PDF release readiness checklist: `docs/pdf_release_readiness_checklist_zh.rst`')
[void]$lines.Add("- Smoke verdict: $(Get-DisplayValue -Value $smokeVerdict)")
[void]$lines.Add("- Smoke review status: $(Get-DisplayValue -Value $smokeReviewStatus)")
[void]$lines.Add("- Smoke reviewed at: $(Get-DisplayValue -Value $smokeReviewedAt)")
[void]$lines.Add("- Smoke review method: $(Get-DisplayValue -Value $smokeReviewMethod)")
[void]$lines.Add("- Smoke review note: $(Get-DisplayValue -Value $smokeReviewNote)")
[void]$lines.Add("- Smoke review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $smokeReviewResultPath)")
[void]$lines.Add("- Smoke final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $smokeFinalReviewPath)")
[void]$lines.Add("- Fixed-grid verdict: $(Get-DisplayValue -Value $fixedGridVerdict)")
[void]$lines.Add("- Fixed-grid review status: $(Get-DisplayValue -Value $fixedGridReviewStatus)")
[void]$lines.Add("- Fixed-grid reviewed at: $(Get-DisplayValue -Value $fixedGridReviewedAt)")
[void]$lines.Add("- Fixed-grid review method: $(Get-DisplayValue -Value $fixedGridReviewMethod)")
[void]$lines.Add("- Fixed-grid review note: $(Get-DisplayValue -Value $fixedGridReviewNote)")
[void]$lines.Add("- Fixed-grid review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $fixedGridReviewResultPath)")
[void]$lines.Add("- Fixed-grid final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $fixedGridFinalReviewPath)")
[void]$lines.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$lines.Add("- Section page setup review status: $(Get-DisplayValue -Value $sectionPageSetupReviewStatus)")
[void]$lines.Add("- Section page setup reviewed at: $(Get-DisplayValue -Value $sectionPageSetupReviewedAt)")
[void]$lines.Add("- Section page setup review method: $(Get-DisplayValue -Value $sectionPageSetupReviewMethod)")
[void]$lines.Add("- Section page setup review note: $(Get-DisplayValue -Value $sectionPageSetupReviewNote)")
[void]$lines.Add("- Section page setup review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupReviewResultPath)")
[void]$lines.Add("- Section page setup final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupFinalReviewPath)")
[void]$lines.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$lines.Add("- Page number fields review status: $(Get-DisplayValue -Value $pageNumberFieldsReviewStatus)")
[void]$lines.Add("- Page number fields reviewed at: $(Get-DisplayValue -Value $pageNumberFieldsReviewedAt)")
[void]$lines.Add("- Page number fields review method: $(Get-DisplayValue -Value $pageNumberFieldsReviewMethod)")
[void]$lines.Add("- Page number fields review note: $(Get-DisplayValue -Value $pageNumberFieldsReviewNote)")
[void]$lines.Add("- Page number fields review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsReviewResultPath)")
[void]$lines.Add("- Page number fields final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsFinalReviewPath)")
[void]$lines.Add("- Curated visual regression bundles: $($curatedVisualReviewEntries.Count)")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")
    [void]$lines.Add("- $($curatedVisualReview.label) review status: $(Get-DisplayValue -Value $curatedVisualReview.review_status)")
    [void]$lines.Add("- $($curatedVisualReview.label) reviewed at: $(Get-DisplayValue -Value $curatedVisualReview.reviewed_at)")
    [void]$lines.Add("- $($curatedVisualReview.label) review method: $(Get-DisplayValue -Value $curatedVisualReview.review_method)")
    [void]$lines.Add("- $($curatedVisualReview.label) review note: $(Get-DisplayValue -Value $curatedVisualReview.review_note)")
    [void]$lines.Add("- $($curatedVisualReview.label) review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.review_result_path)")
    [void]$lines.Add("- $($curatedVisualReview.label) final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.final_review_path)")
}
Add-ReleaseBlockerMarkdownSection -Lines $lines -Summary $summary -RepoRoot $repoRoot
Add-ReleaseGovernanceRollupMarkdownSection -Lines $lines -Summary $summary -RepoRoot $repoRoot
Add-ReleaseGovernanceHandoffMarkdownSection -Lines $lines -Summary $summary -RepoRoot $repoRoot

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
if ($requiresProjectTemplateGovernanceSignoff -or $hasProjectTemplateReleaseEntryEvidence) {
    [void]$lines.Add('- Project template release readiness checklist: `docs/project_template_release_readiness_checklist_zh.rst`')
}

[void]$lines.Add("")
[void]$lines.Add("## Visual Task Shortcuts")
[void]$lines.Add("")
[void]$lines.Add("- Section page setup review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$lines.Add("- Page number fields review task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
if (-not [string]::IsNullOrWhiteSpace($pdfVisualGateEvidence.summary_json)) {
    [void]$lines.Add("- PDF visual gate summary: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pdfVisualGateEvidence.summary_json)")
    [void]$lines.Add("- PDF visual gate verdict: $(Get-DisplayValue -Value $pdfVisualGateEvidence.verdict)")
    [void]$lines.Add("- PDF visual gate aggregate contact sheet: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pdfVisualGateEvidence.aggregate_contact_sheet)")
} else {
    [void]$lines.Add("- PDF visual gate evidence: (not available)")
}
[void]$lines.Add("- PDF bounded CTest auxiliary evidence: $(Get-DisplayValue -Value $pdfBoundedCtestEvidence.status)")
[void]$lines.Add("- PDF bounded CTest summaries / pass: $(Get-DisplayValue -Value ('{0}/{1}' -f $pdfBoundedCtestEvidence.summary_count, $pdfBoundedCtestEvidence.pass_count))")
[void]$lines.Add("- PDF bounded CTest selected / skipped tests: $(Get-DisplayValue -Value ('{0}/{1}' -f $pdfBoundedCtestEvidence.selected_test_count, $pdfBoundedCtestEvidence.skipped_test_count))")
[void]$lines.Add("- PDF bounded CTest auxiliary summaries: $(Get-DisplayValue -Value (@($pdfBoundedCtestEvidence.summary_json_display) -join ', '))")
[void]$lines.Add('- PDF release readiness checklist: `docs/pdf_release_readiness_checklist_zh.rst`')
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
[void]$lines.Add('- Confirm the fixed PDF release readiness checklist has been reviewed: `docs/pdf_release_readiness_checklist_zh.rst`.')
if ($requiresProjectTemplateGovernanceSignoff -or $hasProjectTemplateReleaseEntryEvidence) {
    [void]$lines.Add('- Confirm the fixed Project template release readiness checklist has been reviewed: `docs/project_template_release_readiness_checklist_zh.rst`.')
}
if (-not [string]::IsNullOrWhiteSpace($projectTemplateChecklistHandoffEvidenceLine)) {
    [void]$lines.Add("- Confirm release governance handoff carries project-template readiness checklist entrypoint evidence: $projectTemplateChecklistHandoffEvidenceLine.")
}
if (-not [string]::IsNullOrWhiteSpace($projectTemplateChecklistMaterialSafetyAuditEvidenceLine)) {
    [void]$lines.Add("- Confirm release governance handoff carries packaged project-template readiness checklist material-safety audit evidence: $projectTemplateChecklistMaterialSafetyAuditEvidenceLine.")
}
if (-not [string]::IsNullOrWhiteSpace($wordVisualStandardReviewMetadataEvidenceLine)) {
    [void]$lines.Add("- Confirm release governance handoff carries Word visual standard review metadata evidence: $wordVisualStandardReviewMetadataEvidenceLine.")
}
[void]$lines.Add("- Run the local packaging command first; it regenerates ZIP files and does not contact GitHub.")
if ($requiresProjectTemplateGovernanceSignoff -or $hasProjectTemplateReleaseEntryEvidence) {
    [void]$lines.Add(('- Open `{0}` after packaging and confirm `project_template_delivery_readiness_contract` plus `project_template_onboarding_governance_contract` both preserve `status`, `release_ready`, `release_blocker_count`, `warning_count`, `schema_approval_status_summary`, `source_report_display`, and `source_json_display` before refreshing or publishing GitHub Release assets.' -f $releaseAssetsManifestPath))
}
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
Write-Utf8NoBomFile -Path $resolvedOutputPath -Text ($lines -join [Environment]::NewLine)

Write-Host "Start here: $resolvedOutputPath"
