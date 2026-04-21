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
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $visualGateStep -GateSummary $gateSummary)
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
$syncExplicitCommand = ""
if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
    $gateSummaryCommandPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $gateSummaryPath
    $syncExplicitCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 -GateSummaryJson "{0}" -ReleaseCandidateSummaryJson "{1}" -RefreshReleaseBundle' -f `
        $gateSummaryCommandPath, $summaryCommandPath
}
$releaseGateCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1"
$releaseChecksCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1"
$packageAssetsCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}"' -f `
    $summaryCommandPath
$packageAndUploadCommand = ""
if ($projectVersion) {
    $packageAndUploadCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 -SummaryJson "{0}" -UploadReleaseTag "v{1}"' -f `
        $summaryCommandPath, $projectVersion
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
[void]$handoffLines.Add("- Visual verdict: $(Get-DisplayValue -Value $visualVerdict)")
[void]$handoffLines.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$handoffLines.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$handoffLines.Add("- Curated visual regression bundles: $($curatedVisualReviewEntries.Count)")
[void]$handoffLines.Add("- Superseded review tasks: $(Get-DisplayValue -Value $supersededReviewTasksCount)")
[void]$handoffLines.Add("- Superseded task audit: $(Get-DisplayPath -RepoRoot $repoRoot -Path $supersededReviewTasksReportPath)")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("## Verification Snapshot")
[void]$handoffLines.Add("")
[void]$handoffLines.Add("- Configure: $($summary.steps.configure.status)")
[void]$handoffLines.Add("- Build: $($summary.steps.build.status)")
[void]$handoffLines.Add("- Tests: $($summary.steps.tests.status)")
[void]$handoffLines.Add("- Install smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Visual gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- README gallery refresh: $(Get-DisplayValue -Value $readmeGalleryStatus)")
[void]$handoffLines.Add("- Superseded review tasks: $(Get-DisplayValue -Value $supersededReviewTasksCount)")
[void]$handoffLines.Add("- Section page setup task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupTaskDir)")
[void]$handoffLines.Add("- Page number fields task: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsTaskDir)")
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
[void]$handoffLines.Add('```')
[void]$handoffLines.Add("")
[void]$handoffLines.Add("That command auto-detects the latest review task, syncs the final verdict back into the gate summary, and refreshes the detected release bundle.")
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
if (-not [string]::IsNullOrWhiteSpace($packageAndUploadCommand)) {
    [void]$handoffLines.Add("Upload or refresh the GitHub Release attachments with:")
    [void]$handoffLines.Add("")
    [void]$handoffLines.Add('```powershell')
    [void]$handoffLines.Add($packageAndUploadCommand)
    [void]$handoffLines.Add('```')
    [void]$handoffLines.Add("")
}
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
[void]$handoffLines.Add("When that same one-shot flow should also publish the GitHub Release:")
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
[void]$handoffLines.Add("- install + find_package smoke: $($summary.steps.install_smoke.status)")
[void]$handoffLines.Add("- Word visual release gate: $($summary.steps.visual_gate.status)")
[void]$handoffLines.Add("- Visual verdict: $(if ($visualVerdict) { $visualVerdict } else { '<pass|fail|pending_manual_review>' })")
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
[void]$handoffLines.Add("- $(if ($gateSummaryPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $gateSummaryPath } else { 'output/word-visual-release-gate/report/gate_summary.json' })")
[void]$handoffLines.Add("- $(if ($gateFinalReviewPath) { Get-DisplayPath -RepoRoot $repoRoot -Path $gateFinalReviewPath } else { 'output/word-visual-release-gate/report/gate_final_review.md' })")
[void]$handoffLines.Add('```')

$handoff = $handoffLines -join [Environment]::NewLine

$handoff | Set-Content -Path $resolvedOutputPath -Encoding UTF8

Write-Host "Release handoff: $resolvedOutputPath"
