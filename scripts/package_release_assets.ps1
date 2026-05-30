<#
.SYNOPSIS
Packages release-facing FeatherDoc artifacts from a release-candidate summary.

.DESCRIPTION
Builds the public release ZIP files for the installed MSVC package, the
visual-validation gallery, and the screenshot-backed release evidence bundle.
The script stages a normalized install tree so the public gallery always picks
up the latest repository README assets even when an existing install prefix was
created before the newest gallery files were added.

.PARAMETER SummaryJson
Path to the release-candidate summary JSON produced by
run_release_candidate_checks.ps1.

.PARAMETER OutputRoot
Root directory under which versioned release assets are written.

.PARAMETER ReleaseVersion
Optional explicit version override. Defaults to the version embedded in the
summary or CMakeLists.txt.

.PARAMETER InstallPrefix
Optional explicit install-prefix override. Defaults to
summary.steps.install_smoke.install_prefix.

.PARAMETER ReadmeAssetsDir
Optional explicit README gallery directory override. Defaults to
summary.readme_gallery.assets_dir or docs/assets/readme.

.PARAMETER UploadReleaseTag
Optional GitHub release tag. When set, existing same-name assets on that release
are deleted first and the generated ZIP files are uploaded.

.PARAMETER AllowIncomplete
Allows packaging even when execution_status / visual_verdict are not pass.

.PARAMETER KeepStaging
Keeps the staged packaging directory under output/release-assets for inspection.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json `
    -UploadReleaseTag v1.6.4
#>
param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputRoot = "output/release-assets",
    [string]$ReleaseVersion = "",
    [string]$InstallPrefix = "",
    [string]$ReadmeAssetsDir = "",
    [string]$UploadReleaseTag = "",
    [switch]$AllowIncomplete,
    [switch]$KeepStaging
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[package-release-assets] $Message"
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

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Missing ${Label}: $Path"
    }
}

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function New-CleanDirectory {
    param([string]$Path)

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }

    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Copy-PathTree {
    param(
        [string]$Source,
        [string]$Destination
    )

    Assert-PathExists -Path $Source -Label "copy source"

    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }

    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function Copy-FileToPath {
    param(
        [string]$Source,
        [string]$Destination
    )

    Assert-PathExists -Path $Source -Label "copy source file"
    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Test-TextLineContainsAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($line in ($Text -split "\r?\n")) {
        $containsAll = $true
        foreach ($needle in $Needles) {
            if ($line -notlike "*$needle*") {
                $containsAll = $false
                break
            }
        }

        if ($containsAll) {
            return $true
        }
    }

    return $false
}

function Assert-ProjectTemplateChecklistHandoffEvidenceLine {
    param(
        [string]$Path,
        [string]$Label
    )

    Assert-PathExists -Path $Path -Label $Label
    $content = Get-Content -Raw -LiteralPath $Path
    $requiredFragments = @(
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=",
        "status=",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "entrypoint_paths=",
        "START_HERE.md",
        "ARTIFACT_GUIDE.md",
        "REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report="
    )

    if (-not (Test-TextLineContainsAll -Text $content -Needles $requiredFragments)) {
        throw "$Label must keep project-template readiness checklist handoff evidence count, status, checklist path, required entrypoint count, entrypoint ids, entrypoint paths, marker, source schema, and source report on the same compact evidence line."
    }
}

function Assert-StagedProjectTemplateChecklistHandoffEvidence {
    param([string]$ReleaseCandidateRoot)

    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "START_HERE.md") `
        -Label "staged START_HERE.md project-template checklist handoff evidence"
    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "report\ARTIFACT_GUIDE.md") `
        -Label "staged ARTIFACT_GUIDE.md project-template checklist handoff evidence"
    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "report\REVIEWER_CHECKLIST.md") `
        -Label "staged REVIEWER_CHECKLIST.md project-template checklist handoff evidence"
}

function Assert-StagedWordVisualStandardReviewMetadataHandoffEvidence {
    param(
        [string]$ReleaseCandidateRoot,
        [int]$ExpectedMetadataCount
    )

    $path = Join-Path $ReleaseCandidateRoot "report\release_governance_handoff.md"
    $label = "staged release_governance_handoff.md Word visual standard review metadata evidence"
    Assert-PathExists -Path $path -Label $label

    $content = Get-Content -Raw -LiteralPath $path
    $requiredFragments = @(
        "Word visual standard review metadata source reports",
        "word_visual_standard_review_metadata_count: ``$ExpectedMetadataCount``",
        "word_visual_standard_review_task_keys",
        "word_visual_standard_review_status_summary",
        "word_visual_standard_review_verdict_summary",
        "review_result_path",
        "final_review_path"
    )

    foreach ($fragment in $requiredFragments) {
        if ($content.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
            throw "$label must keep detailed Word visual standard review metadata source reports, including '$fragment'."
        }
    }
}

function New-ZipArchive {
    param(
        [string[]]$SourcePaths,
        [string]$ZipPath
    )

    foreach ($source in $SourcePaths) {
        Assert-PathExists -Path $source -Label "zip source"
    }

    if (Test-Path -LiteralPath $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }

    Compress-Archive -LiteralPath $SourcePaths -DestinationPath $ZipPath -CompressionLevel Optimal
}

function Get-ResolvedReleaseVersion {
    param(
        [string]$ExplicitVersion,
        $Summary,
        [string]$RepoRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitVersion)) {
        return $ExplicitVersion
    }

    $summaryVersion = Get-OptionalPropertyValue -Object $Summary -Name "release_version"
    if (-not [string]::IsNullOrWhiteSpace($summaryVersion)) {
        return $summaryVersion
    }

    return Get-ProjectVersion -RepoRoot $RepoRoot
}

function Resolve-GateRoot {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $summaryJson = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        $resolvedSummaryJson = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $summaryJson
        if (Test-Path -LiteralPath $resolvedSummaryJson) {
            return Split-Path -Parent (Split-Path -Parent $resolvedSummaryJson)
        }
    }

    $gateOutputDir = Get-OptionalPropertyValue -Object $Summary -Name "gate_output_dir"
    if (-not [string]::IsNullOrWhiteSpace($gateOutputDir)) {
        return Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateOutputDir
    }

    return ""
}

function Resolve-PdfVisualGateSummaryJson {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryJson = Get-PdfVisualGateSummaryPath -Summary $Summary
    if ([string]::IsNullOrWhiteSpace($summaryJson)) {
        return ""
    }

    return Resolve-FullPath -RepoRoot $RepoRoot -InputPath $summaryJson
}

function Resolve-PdfVisualGateRoot {
    param(
        [string]$SummaryJson
    )

    if ([string]::IsNullOrWhiteSpace($SummaryJson) -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return ""
    }

    return Split-Path -Parent (Split-Path -Parent $SummaryJson)
}

function Get-GalleryFileNames {
    return @(
        "visual-smoke-contact-sheet.png",
        "visual-smoke-page-05.png",
        "visual-smoke-page-06.png",
        "reopened-fixed-layout-column-widths-page-01.png",
        "fixed-grid-merge-right-page-01.png",
        "fixed-grid-merge-down-page-01.png",
        "fixed-grid-aggregate-contact-sheet.png",
        "sample-chinese-template-page-01.png"
    )
}

function Resolve-GallerySourceFile {
    param(
        [string]$ReadmeAssetsDir,
        [string]$InstallPrefix,
        [string]$FileName
    )

    $readmeCandidate = Join-Path $ReadmeAssetsDir $FileName
    if (Test-Path -LiteralPath $readmeCandidate) {
        return $readmeCandidate
    }

    $installCandidate = Join-Path $InstallPrefix ("share\FeatherDoc\visual-validation\{0}" -f $FileName)
    if (Test-Path -LiteralPath $installCandidate) {
        return $installCandidate
    }

    throw "Could not resolve gallery asset '$FileName' from README assets or install prefix."
}

function Get-VisualVerdict {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $Summary -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $stepVerdict = Get-OptionalPropertyValue -Object $visualGate -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($stepVerdict)) {
        return $stepVerdict
    }

    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            return Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        }
    }

    return ""
}

function Get-VisualGateStatus {
    param($Summary)

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    return Get-OptionalPropertyValue -Object $visualGate -Name "status"
}

function Get-WordVisualStandardReviewMetadata {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    if ($null -eq $visualGate) {
        return @()
    }

    $visualGateStatus = Get-OptionalPropertyValue -Object $visualGate -Name "status"
    if ($visualGateStatus -eq "skipped") {
        return @()
    }

    $gateSummary = [pscustomobject]@{}
    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            try {
                $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            } catch {
                $gateSummary = [pscustomobject]@{}
            }
        }
    }

    $tasks = @(
        [ordered]@{ key = "smoke"; label = "Word visual smoke" },
        [ordered]@{ key = "fixed_grid"; label = "Fixed-grid merge/unmerge" },
        [ordered]@{ key = "section_page_setup"; label = "Section page setup" },
        [ordered]@{ key = "page_number_fields"; label = "Page number fields" }
    )

    $metadata = New-Object 'System.Collections.Generic.List[object]'
    foreach ($task in $tasks) {
        $taskKey = [string]$task.key
        [void]$metadata.Add([ordered]@{
            task_key = $taskKey
            review_task_key = Get-VisualTaskReviewTaskKey -TaskKey $taskKey
            label = [string]$task.label
            verdict = Get-VisualTaskVerdict -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_status = Get-VisualTaskReviewStatus -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            reviewed_at = Get-VisualTaskReviewedAt -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_method = Get-VisualTaskReviewMethod -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            review_result_path = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
            final_review_path = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGate -GateSummary $gateSummary -TaskKey $taskKey
        })
    }

    return @($metadata.ToArray())
}

function Get-GovernanceMetrics {
    param($Summary)

    $metrics = Get-OptionalPropertyObject -Object $Summary -Name "governance_metrics"
    if ($null -eq $metrics) {
        return @()
    }

    return @($metrics)
}

function Get-GovernanceMetricByContract {
    param(
        $GovernanceMetrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($GovernanceMetrics | Where-Object {
        (Get-OptionalPropertyValue -Object $_ -Name "metric") -eq $Metric -and
        (Get-OptionalPropertyValue -Object $_ -Name "id") -eq $Id -and
        (Get-OptionalPropertyValue -Object $_ -Name "report_id") -eq $ReportId -and
        (Get-OptionalPropertyValue -Object $_ -Name "source_schema") -eq $SourceSchema
    }) | Select-Object -First 1
}

function Get-TableLayoutDeliveryQuality {
    param($GovernanceMetrics)

    $deliveryMetric = Get-GovernanceMetricByContract `
        -GovernanceMetrics $GovernanceMetrics `
        -Metric "delivery_quality" `
        -Id "table_layout_delivery_governance.delivery_quality" `
        -ReportId "table_layout_delivery_governance" `
        -SourceSchema "featherdoc.table_layout_delivery_governance_report.v1"

    if ($null -eq $deliveryMetric) {
        return [ordered]@{}
    }

    return [ordered]@{
        id = Get-OptionalPropertyValue -Object $deliveryMetric -Name "id"
        metric = Get-OptionalPropertyValue -Object $deliveryMetric -Name "metric"
        report_id = Get-OptionalPropertyValue -Object $deliveryMetric -Name "report_id"
        source_schema = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_schema"
        source_report = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_report"
        source_report_display = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_report_display"
        source_json = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_json"
        source_json_display = Get-OptionalPropertyValue -Object $deliveryMetric -Name "source_json_display"
        score = Get-OptionalPropertyObject -Object $deliveryMetric -Name "score"
        level = Get-OptionalPropertyValue -Object $deliveryMetric -Name "level"
        details = Get-OptionalPropertyObject -Object $deliveryMetric -Name "details"
    }
}

function Get-NumberingCatalogRealCorpusConfidence {
    param($GovernanceMetrics)

    $confidenceMetric = Get-GovernanceMetricByContract `
        -GovernanceMetrics $GovernanceMetrics `
        -Metric "real_corpus_confidence" `
        -Id "numbering_catalog_governance.real_corpus_confidence" `
        -ReportId "numbering_catalog_governance" `
        -SourceSchema "featherdoc.numbering_catalog_governance_report.v1"

    if ($null -eq $confidenceMetric) {
        return [ordered]@{}
    }

    return [ordered]@{
        id = Get-OptionalPropertyValue -Object $confidenceMetric -Name "id"
        metric = Get-OptionalPropertyValue -Object $confidenceMetric -Name "metric"
        report_id = Get-OptionalPropertyValue -Object $confidenceMetric -Name "report_id"
        source_schema = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_schema"
        source_report = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_report"
        source_report_display = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_report_display"
        source_json = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_json"
        source_json_display = Get-OptionalPropertyValue -Object $confidenceMetric -Name "source_json_display"
        score = Get-OptionalPropertyObject -Object $confidenceMetric -Name "score"
        level = Get-OptionalPropertyValue -Object $confidenceMetric -Name "level"
        details = Get-OptionalPropertyObject -Object $confidenceMetric -Name "details"
    }
}

function Get-ContentControlRepairContracts {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $contentControlSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "content_control_data_binding_governance"
    if ([string]::IsNullOrWhiteSpace($contentControlSummaryPath)) {
        return @()
    }

    $resolvedContentControlSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $contentControlSummaryPath
    Assert-PathExists -Path $resolvedContentControlSummaryPath -Label "content-control data-binding governance summary"

    $contentControlSummary = Get-Content -Raw -LiteralPath $resolvedContentControlSummaryPath | ConvertFrom-Json
    $schema = Get-OptionalPropertyValue -Object $contentControlSummary -Name "schema"
    $releaseBlockers = Get-OptionalPropertyObject -Object $contentControlSummary -Name "release_blockers"
    if ($null -eq $releaseBlockers) {
        return @()
    }

    $contracts = [System.Collections.Generic.List[object]]::new()
    foreach ($blocker in @($releaseBlockers)) {
        $blockerId = Get-OptionalPropertyValue -Object $blocker -Name "id"
        if ($blockerId -ne "content_control_data_binding.bound_placeholder") {
            continue
        }

        [void]$contracts.Add([ordered]@{
            id = $blockerId
            source_schema = $schema
            source_json_display = Convert-EvidencePathToPublicDisplay -Value $resolvedContentControlSummaryPath -RepoRoot $RepoRoot
            input_docx = Get-OptionalPropertyValue -Object $blocker -Name "input_docx"
            input_docx_display = Get-OptionalPropertyValue -Object $blocker -Name "input_docx_display"
            template_name = Get-OptionalPropertyValue -Object $blocker -Name "template_name"
            schema_target = Get-OptionalPropertyValue -Object $blocker -Name "schema_target"
            target_mode = Get-OptionalPropertyValue -Object $blocker -Name "target_mode"
            repair_strategy = Get-OptionalPropertyValue -Object $blocker -Name "repair_strategy"
            repair_hint = Get-OptionalPropertyValue -Object $blocker -Name "repair_hint"
            command_template = Get-OptionalPropertyValue -Object $blocker -Name "command_template"
        })
    }

    return @($contracts.ToArray())
}

function Get-ProjectTemplateDeliveryReadinessContract {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $readinessSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "project_template_delivery_readiness"
    if ([string]::IsNullOrWhiteSpace($readinessSummaryPath)) {
        return [ordered]@{}
    }

    $resolvedReadinessSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $readinessSummaryPath
    Assert-PathExists -Path $resolvedReadinessSummaryPath -Label "project template delivery readiness summary"

    $readinessSummary = Get-Content -Raw -LiteralPath $resolvedReadinessSummaryPath | ConvertFrom-Json
    $readinessSummaryDisplay = Convert-EvidencePathToPublicDisplay -Value $resolvedReadinessSummaryPath -RepoRoot $RepoRoot
    return [ordered]@{
        schema = Get-OptionalPropertyValue -Object $readinessSummary -Name "schema"
        source_schema = Get-OptionalPropertyValue -Object $readinessSummary -Name "schema"
        status = Get-OptionalPropertyValue -Object $readinessSummary -Name "status"
        release_ready = Get-OptionalPropertyObject -Object $readinessSummary -Name "release_ready"
        latest_schema_approval_gate_status = Get-OptionalPropertyValue -Object $readinessSummary -Name "latest_schema_approval_gate_status"
        schema_approval_status_summary = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_approval_status_summary"
        schema_history_blocked_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_blocked_run_count"
        schema_history_pending_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_pending_run_count"
        schema_history_passed_run_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "schema_history_passed_run_count"
        template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "template_count"
        ready_template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "ready_template_count"
        blocked_template_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "blocked_template_count"
        release_blocker_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "release_blocker_count"
        action_item_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "action_item_count"
        warning_count = Get-OptionalPropertyObject -Object $readinessSummary -Name "warning_count"
        source_report_display = $readinessSummaryDisplay
        source_json_display = $readinessSummaryDisplay
    }
}

function Get-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $onboardingSummaryPath = Get-OptionalPropertyValue -Object $Summary -Name "project_template_onboarding_governance"
    if ([string]::IsNullOrWhiteSpace($onboardingSummaryPath)) {
        return [ordered]@{}
    }

    $resolvedOnboardingSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $onboardingSummaryPath
    Assert-PathExists -Path $resolvedOnboardingSummaryPath -Label "project template onboarding governance summary"

    $onboardingSummary = Get-Content -Raw -LiteralPath $resolvedOnboardingSummaryPath | ConvertFrom-Json
    $onboardingSummaryDisplay = Convert-EvidencePathToPublicDisplay -Value $resolvedOnboardingSummaryPath -RepoRoot $RepoRoot
    return [ordered]@{
        schema = Get-OptionalPropertyValue -Object $onboardingSummary -Name "schema"
        source_schema = Get-OptionalPropertyValue -Object $onboardingSummary -Name "schema"
        status = Get-OptionalPropertyValue -Object $onboardingSummary -Name "status"
        release_ready = Get-OptionalPropertyObject -Object $onboardingSummary -Name "release_ready"
        source_file_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "source_file_count"
        source_failure_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "source_failure_count"
        entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "entry_count"
        schema_approval_status_summary = Get-OptionalPropertyObject -Object $onboardingSummary -Name "schema_approval_status_summary"
        blocked_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "blocked_entry_count"
        pending_review_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "pending_review_entry_count"
        not_evaluated_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "not_evaluated_entry_count"
        approved_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "approved_entry_count"
        not_required_entry_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "not_required_entry_count"
        release_blocker_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "release_blocker_count"
        action_item_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "action_item_count"
        manual_review_recommendation_count = Get-OptionalPropertyObject -Object $onboardingSummary -Name "manual_review_recommendation_count"
        source_report_display = $onboardingSummaryDisplay
        source_json_display = $onboardingSummaryDisplay
    }
}

function Get-AssetDescriptor {
    param(
        [string]$Path,
        [string]$Label
    )

    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()

    return [ordered]@{
        label = $Label
        path = $item.FullName
        size_bytes = $item.Length
        sha256 = $hash
    }
}

function Get-TextLikeFiles {
    param([string[]]$RootPaths)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    $results = [System.Collections.Generic.List[string]]::new()
    foreach ($rootPath in $RootPaths) {
        if ([string]::IsNullOrWhiteSpace($rootPath) -or -not (Test-Path -LiteralPath $rootPath)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $rootPath -Recurse -File) {
            if ($textExtensions -contains $file.Extension.ToLowerInvariant()) {
                [void]$results.Add($file.FullName)
            }
        }
    }

    return @($results | Sort-Object -Unique)
}

function Convert-RepoPathToRelative {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $normalizedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $normalizedValue = $Value -replace '/', '\'
    try {
        if (-not [System.IO.Path]::IsPathRooted($normalizedValue)) {
            return $Value
        }

        $resolvedValue = [System.IO.Path]::GetFullPath($normalizedValue)
    } catch [System.ArgumentException] {
        return $Value
    } catch [System.NotSupportedException] {
        return $Value
    }

    if (-not $resolvedValue.StartsWith($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Value
    }

    $relative = $resolvedValue.Substring($normalizedRepoRoot.Length).TrimStart('\', '/')
    if ([string]::IsNullOrWhiteSpace($relative)) {
        return "."
    }

    return ".\" + ($relative -replace '/', '\')
}

function Convert-EvidencePathToPublicDisplay {
    param(
        [string]$Value,
        [string]$RepoRoot,
        [switch]$PreferEvidenceAnchor
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $normalized = $Value -replace '/', '\'
    if ($PreferEvidenceAnchor) {
        foreach ($anchor in @("\output\", "\release-assets\", "\release-assets-ci\")) {
            $index = $normalized.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
            if ($index -ge 0) {
                $relative = $normalized.Substring($index + 1)
                if (-not [string]::IsNullOrWhiteSpace($relative)) {
                    return ".\" + $relative
                }
            }
        }
    }

    $repoDisplay = Convert-RepoPathToRelative -Value $Value -RepoRoot $RepoRoot
    if ($repoDisplay -ne $Value) {
        return $repoDisplay
    }

    foreach ($anchor in @("\output\", "\release-assets\", "\release-assets-ci\")) {
        $index = $normalized.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            $relative = $normalized.Substring($index + 1)
            if (-not [string]::IsNullOrWhiteSpace($relative)) {
                return ".\" + $relative
            }
        }
    }

    return $Value
}

function Get-EvidenceObjectProperty {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }

        return $null
    }

    return Get-OptionalPropertyObject -Object $Object -Name $Name
}

function Set-EvidenceObjectProperty {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($null -eq $Object) {
        return
    }

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value
        return
    }

    $property.Value = $Value
}

function Convert-EvidenceEntrypointsToPublicDisplay {
    param(
        $Contract,
        $SourceContract = $null,
        [string]$RepoRoot
    )

    if ($null -eq $Contract) {
        return
    }

    $source = if ($null -ne $SourceContract) { $SourceContract } else { $Contract }
    $sourceEntrypoints = @(Get-EvidenceObjectProperty -Object $source -Name "entrypoints")
    $targetEntrypoints = @(Get-EvidenceObjectProperty -Object $Contract -Name "entrypoints")
    $targetById = @{}
    foreach ($targetEntrypoint in $targetEntrypoints) {
        $targetId = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($targetId)) {
            $targetById[$targetId] = $targetEntrypoint
        }
    }

    $convertedEntrypoints = New-Object 'System.Collections.Generic.List[object]'
    $index = 0
    foreach ($sourceEntrypoint in $sourceEntrypoints) {
        if ($null -eq $sourceEntrypoint) {
            $index++
            continue
        }

        $sourceId = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "id")
        $targetEntrypoint = if (-not [string]::IsNullOrWhiteSpace($sourceId) -and $targetById.ContainsKey($sourceId)) {
            $targetById[$sourceId]
        } elseif ($index -lt $targetEntrypoints.Count) {
            $targetEntrypoints[$index]
        } else {
            $null
        }

        if ($null -eq $targetEntrypoint) {
            $targetEntrypoint = $sourceEntrypoint
        }

        $publicEntrypoint = [ordered]@{}

        foreach ($fieldName in @("id", "required")) {
            $fieldValue = Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name $fieldName
            if ($null -eq $fieldValue) {
                $fieldValue = Get-EvidenceObjectProperty -Object $targetEntrypoint -Name $fieldName
            }
            if ($null -ne $fieldValue) {
                $publicEntrypoint[$fieldName] = $fieldValue
            }
        }

        $path = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path")
        if ([string]::IsNullOrWhiteSpace($path)) {
            $path = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path")
        }
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $publicEntrypoint["path"] = Convert-EvidencePathToPublicDisplay -Value $path -RepoRoot $RepoRoot
        }

        $pathDisplay = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path_display")
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path_display")
        }
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $sourceEntrypoint -Name "path")
        }
        if ([string]::IsNullOrWhiteSpace($pathDisplay)) {
            $pathDisplay = [string](Get-EvidenceObjectProperty -Object $targetEntrypoint -Name "path")
        }
        if (-not [string]::IsNullOrWhiteSpace($pathDisplay)) {
            $publicEntrypoint["path_display"] = Convert-EvidencePathToPublicDisplay -Value $pathDisplay -RepoRoot $RepoRoot -PreferEvidenceAnchor
        }

        $convertedEntrypoints.Add($publicEntrypoint) | Out-Null
        $index++
    }

    if ($convertedEntrypoints.Count -gt 0) {
        Set-EvidenceObjectProperty -Object $Contract -Name "entrypoints" -Value @($convertedEntrypoints.ToArray())
    }
}

function Convert-ReleaseTextToPublic {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $result = $Value
    $result = $result.Replace($RepoRoot, ".")
    $result = $result.Replace(($RepoRoot -replace '\\', '/'), ".")
    $result = [regex]::Replace($result, '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+', '<windows-absolute-path>')
    $result = [regex]::Replace($result, '(?<!\w)/(?:Users|home)/[^\s"''`<>|]+', '<unix-absolute-path>')

    $zhReleaseNoteDraft = [string]::Concat(([char[]](0x53D1, 0x5E03, 0x8BF4, 0x660E, 0x8349, 0x7A3F)))
    $zhReleaseNotePreview = [string]::Concat(([char[]](0x53D1, 0x5E03, 0x8BF4, 0x660E, 0x9884, 0x89C8, 0x7248)))
    $zhFillBeforeRelease = [string]::Concat(([char[]](0x8BF7, 0x5728, 0x53D1, 0x5E03, 0x524D, 0x8865, 0x9F50)))
    $zhFillBeforePublicRelease = [string]::Concat(([char[]](0x8BF7, 0x5728, 0x516C, 0x5F00, 0x53D1, 0x5E03, 0x524D, 0x5B8C, 0x5584)))
    $zhDraft = [string]::Concat(([char[]](0x8349, 0x7A3F)))
    $zhPreview = [string]::Concat(([char[]](0x9884, 0x89C8, 0x7248)))

    $replacements = @(
        @{ Pattern = "(?i)$zhReleaseNoteDraft"; Replacement = $zhReleaseNotePreview }
        @{ Pattern = "(?i)$zhFillBeforeRelease"; Replacement = $zhFillBeforePublicRelease }
        @{ Pattern = '(?i)\brelease body draft\b'; Replacement = 'release body preview' }
        @{ Pattern = '(?i)\brelease-note drafts\b'; Replacement = 'release-note previews' }
        @{ Pattern = '(?i)\bpublic release drafts\b'; Replacement = 'public release previews' }
        @{ Pattern = '(?i)\brelease drafts\b'; Replacement = 'release previews' }
        @{ Pattern = '(?i)\bdraft review\b'; Replacement = 'release-note review' }
        @{ Pattern = '(?i)\bdraft boilerplate\b'; Replacement = 'placeholder boilerplate' }
        @{ Pattern = '(?i)\brelease-note drafting\b'; Replacement = 'release-note preparation' }
        @{ Pattern = '(?i)\bdraft=false\b'; Replacement = 'public-release-enabled' }
        @{ Pattern = '(?i)\bdrafts\b'; Replacement = 'previews' }
        @{ Pattern = '(?i)\bdrafting\b'; Replacement = 'preparation' }
        @{ Pattern = '(?i)\bdraft\b'; Replacement = 'preview' }
        @{ Pattern = $zhDraft; Replacement = $zhPreview }
    )

    foreach ($replacement in $replacements) {
        $result = [regex]::Replace($result, $replacement.Pattern, $replacement.Replacement)
    }

    return $result
}

function Convert-StructuredValueToPublic {
    param(
        $Value,
        [string]$RepoRoot
    )

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        $relativeValue = Convert-RepoPathToRelative -Value $Value -RepoRoot $RepoRoot
        return Convert-ReleaseTextToPublic -Value $relativeValue -RepoRoot $RepoRoot
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $result[$key] = Convert-StructuredValueToPublic -Value $Value[$key] -RepoRoot $RepoRoot
        }
        return $result
    }

    if ($Value -is [pscustomobject]) {
        $result = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $result[$property.Name] = Convert-StructuredValueToPublic -Value $property.Value -RepoRoot $RepoRoot
        }
        return $result
    }

    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        $result = @()
        foreach ($item in $Value) {
            $result += ,(Convert-StructuredValueToPublic -Value $item -RepoRoot $RepoRoot)
        }
        return $result
    }

    return $Value
}

function Sanitize-StagedReleaseMaterials {
    param(
        [string]$RepoRoot,
        [string[]]$RootPaths
    )

    foreach ($filePath in Get-TextLikeFiles -RootPaths $RootPaths) {
        if ([System.IO.Path]::GetExtension($filePath).Equals(".json", [System.StringComparison]::OrdinalIgnoreCase)) {
            $content = Get-Content -Raw -LiteralPath $filePath | ConvertFrom-Json
            $sanitized = Convert-StructuredValueToPublic -Value $content -RepoRoot $RepoRoot
            ($sanitized | ConvertTo-Json -Depth 100) | Set-Content -LiteralPath $filePath -Encoding UTF8
            continue
        }

        $content = Get-Content -Raw -LiteralPath $filePath
        $content = Convert-ReleaseTextToPublic -Value $content -RepoRoot $RepoRoot
        Set-Content -LiteralPath $filePath -Encoding UTF8 -Value $content
    }
}

$repoRoot = Resolve-RepoRoot
$releaseMaterialAuditScript = Join-Path $repoRoot "scripts\assert_release_material_safety.ps1"
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputRoot

Assert-PathExists -Path $resolvedSummaryPath -Label "release summary JSON"

$summary = Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
$releaseVersion = Get-ResolvedReleaseVersion -ExplicitVersion $ReleaseVersion -Summary $summary -RepoRoot $repoRoot
if ([string]::IsNullOrWhiteSpace($releaseVersion)) {
    throw "Could not resolve the release version from the summary or CMakeLists.txt."
}

$executionStatus = Get-OptionalPropertyValue -Object $summary -Name "execution_status"
$visualVerdict = Get-VisualVerdict -RepoRoot $repoRoot -Summary $summary
$visualGateStatus = Get-VisualGateStatus -Summary $summary
$resolvedPdfVisualGateSummaryPath = Resolve-PdfVisualGateSummaryJson -RepoRoot $repoRoot -Summary $summary
$resolvedPdfVisualGateRoot = Resolve-PdfVisualGateRoot -SummaryJson $resolvedPdfVisualGateSummaryPath
$pdfVisualGateEvidence = Get-PdfVisualGateEvidence -SummaryPath $resolvedPdfVisualGateSummaryPath
$pdfVisualGateStatus = Get-OptionalPropertyValue -Object $pdfVisualGateEvidence -Name "status"
$hasPdfVisualGateEvidence = (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualGateRoot)) -and
    (Test-Path -LiteralPath $resolvedPdfVisualGateRoot) -and
    $pdfVisualGateStatus -eq "loaded"
$pdfVisualGateManifestEvidence = Convert-StructuredValueToPublic -Value $pdfVisualGateEvidence -RepoRoot $repoRoot
$pdfBoundedCtestEvidence = Get-PdfBoundedCtestEvidence -Summary $summary
$pdfBoundedCtestStatus = Get-OptionalPropertyValue -Object $pdfBoundedCtestEvidence -Name "status"
$hasPdfBoundedCtestEvidence = $pdfBoundedCtestStatus -ne "not_available"
$pdfBoundedCtestManifestEvidence = Convert-StructuredValueToPublic -Value $pdfBoundedCtestEvidence -RepoRoot $repoRoot
$wordVisualStandardReviewMetadata = @(Get-WordVisualStandardReviewMetadata -RepoRoot $repoRoot -Summary $summary)
$governanceMetrics = @(Get-GovernanceMetrics -Summary $summary)
$releaseGovernanceHandoff = Get-OptionalPropertyObject -Object $summary -Name "release_governance_handoff"
$releaseGovernanceHandoffStatus = Get-OptionalPropertyValue -Object $releaseGovernanceHandoff -Name "status"
$numberingCatalogRealCorpusConfidence = Get-NumberingCatalogRealCorpusConfidence -GovernanceMetrics $governanceMetrics
$tableLayoutDeliveryQuality = Get-TableLayoutDeliveryQuality -GovernanceMetrics $governanceMetrics
$contentControlRepairContracts = @(Get-ContentControlRepairContracts -RepoRoot $repoRoot -Summary $summary)
$projectTemplateDeliveryReadinessContract = Get-ProjectTemplateDeliveryReadinessContract -RepoRoot $repoRoot -Summary $summary
$projectTemplateOnboardingGovernanceContract = Get-ProjectTemplateOnboardingGovernanceContract -RepoRoot $repoRoot -Summary $summary
$manifestSignoffEntrypoints = Get-OptionalPropertyObject -Object $summary -Name "manifest_signoff_entrypoints"
$manifestSignoffEntrypointsPublic = Convert-StructuredValueToPublic -Value $manifestSignoffEntrypoints -RepoRoot $repoRoot
$projectTemplateReadinessChecklistEntrypoints = Get-OptionalPropertyObject -Object $summary -Name "project_template_readiness_checklist_entrypoints"
$projectTemplateReadinessChecklistEntrypointsPublic = Convert-StructuredValueToPublic -Value $projectTemplateReadinessChecklistEntrypoints -RepoRoot $repoRoot
$summaryGovernanceMetricCount = Get-OptionalPropertyValue -Object $summary -Name "governance_metric_count"
$governanceMetricCount = if (-not [string]::IsNullOrWhiteSpace($summaryGovernanceMetricCount)) {
    [int]$summaryGovernanceMetricCount
} else {
    $governanceMetrics.Count
}
$allowIncompleteCiPreview = $AllowIncomplete -and
    $visualGateStatus -eq "skipped" -and
    $releaseGovernanceHandoffStatus -eq "not_requested"
$runStrictReleaseMaterialAudit = -not $allowIncompleteCiPreview

if (-not $AllowIncomplete) {
    if ($executionStatus -ne "pass") {
        throw "Refusing to package release assets because execution_status is '$executionStatus'. Use -AllowIncomplete to override."
    }

    if (-not [string]::IsNullOrWhiteSpace($visualVerdict) -and $visualVerdict -ne "pass") {
        throw "Refusing to package release assets because visual_verdict is '$visualVerdict'. Use -AllowIncomplete to override."
    }
}

$installSmoke = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $summary -Name "steps") -Name "install_smoke"
$resolvedInstallPrefix = if (-not [string]::IsNullOrWhiteSpace($InstallPrefix)) {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $InstallPrefix
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath (Get-OptionalPropertyValue -Object $installSmoke -Name "install_prefix")
}
Assert-PathExists -Path $resolvedInstallPrefix -Label "install prefix"

$resolvedReadmeAssetsDir = if (-not [string]::IsNullOrWhiteSpace($ReadmeAssetsDir)) {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReadmeAssetsDir
} else {
    $summaryReadmeAssetsDir = Get-OptionalPropertyValue -Object (Get-OptionalPropertyObject -Object $summary -Name "readme_gallery") -Name "assets_dir"
    if (-not [string]::IsNullOrWhiteSpace($summaryReadmeAssetsDir)) {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $summaryReadmeAssetsDir
    } else {
        Join-Path $repoRoot "docs\assets\readme"
    }
}
Assert-PathExists -Path $resolvedReadmeAssetsDir -Label "README gallery asset directory"

$reportDir = Split-Path -Parent $resolvedSummaryPath
$startHerePath = Get-OptionalPropertyValue -Object $summary -Name "start_here"
if ([string]::IsNullOrWhiteSpace($startHerePath)) {
    $startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
}
$resolvedStartHerePath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $startHerePath
Assert-PathExists -Path $resolvedStartHerePath -Label "release START_HERE"

$resolvedGateRoot = Resolve-GateRoot -RepoRoot $repoRoot -Summary $summary
$resolvedGateReportDir = ""
$resolvedSmokeEvidenceDir = ""
$resolvedFixedGridAggregateDir = ""
$resolvedSectionPageSetupAggregateDir = ""
$hasVisualGateEvidence = $false
if (-not [string]::IsNullOrWhiteSpace($resolvedGateRoot) -and (Test-Path -LiteralPath $resolvedGateRoot)) {
    $resolvedGateReportDir = Join-Path $resolvedGateRoot "report"
    $resolvedSmokeEvidenceDir = Join-Path $resolvedGateRoot "smoke\evidence"
    $resolvedFixedGridAggregateDir = Join-Path $resolvedGateRoot "fixed-grid\aggregate-evidence"
    $resolvedSectionPageSetupAggregateDir = Join-Path $resolvedGateRoot "section-page-setup\aggregate-evidence"
    $hasVisualGateEvidence = `
        (Test-Path -LiteralPath $resolvedGateReportDir) -and `
        (Test-Path -LiteralPath $resolvedSmokeEvidenceDir) -and `
        (Test-Path -LiteralPath $resolvedFixedGridAggregateDir) -and `
        (Test-Path -LiteralPath $resolvedSectionPageSetupAggregateDir)
}

if (-not $hasVisualGateEvidence) {
    if (-not $AllowIncomplete -or $visualGateStatus -ne "skipped") {
        Assert-PathExists -Path $resolvedGateRoot -Label "Word visual gate output directory"
        Assert-PathExists -Path $resolvedGateReportDir -Label "gate report directory"
        Assert-PathExists -Path $resolvedSmokeEvidenceDir -Label "smoke evidence directory"
        Assert-PathExists -Path $resolvedFixedGridAggregateDir -Label "fixed-grid aggregate evidence directory"
        Assert-PathExists -Path $resolvedSectionPageSetupAggregateDir -Label "section page setup aggregate evidence directory"
    }
}

$versionOutputDir = Join-Path $resolvedOutputRoot ("v{0}" -f $releaseVersion)
$stagingRoot = Join-Path $versionOutputDir "staging"
$installLeaf = (Get-Item -LiteralPath $resolvedInstallPrefix).Name
$stageInstallRoot = Join-Path $stagingRoot $installLeaf
$stageInstallGalleryDir = Join-Path $stageInstallRoot "share\FeatherDoc\visual-validation"
$stageGalleryRoot = Join-Path $stagingRoot "visual-validation"
$stageReleaseCandidateRoot = Join-Path $stagingRoot "release-candidate-checks"
$stageWordVisualRoot = Join-Path $stagingRoot "word-visual-release-gate"
$stagePdfVisualRoot = Join-Path $stagingRoot "pdf-visual-release-gate"

Write-Step "Preparing staging directory"
New-Item -ItemType Directory -Path $versionOutputDir -Force | Out-Null
New-CleanDirectory -Path $stagingRoot

Write-Step "Staging install prefix"
Copy-PathTree -Source $resolvedInstallPrefix -Destination $stageInstallRoot
New-Item -ItemType Directory -Path $stageInstallGalleryDir -Force | Out-Null
New-Item -ItemType Directory -Path $stageGalleryRoot -Force | Out-Null

foreach ($fileName in Get-GalleryFileNames) {
    $resolvedSource = Resolve-GallerySourceFile `
        -ReadmeAssetsDir $resolvedReadmeAssetsDir `
        -InstallPrefix $resolvedInstallPrefix `
        -FileName $fileName
    Copy-FileToPath -Source $resolvedSource -Destination (Join-Path $stageInstallGalleryDir $fileName)
    Copy-FileToPath -Source $resolvedSource -Destination (Join-Path $stageGalleryRoot $fileName)
}

Write-Step "Staging release evidence bundle"
New-Item -ItemType Directory -Path $stageReleaseCandidateRoot -Force | Out-Null
Copy-FileToPath -Source $resolvedStartHerePath -Destination (Join-Path $stageReleaseCandidateRoot "START_HERE.md")
Copy-PathTree -Source $reportDir -Destination (Join-Path $stageReleaseCandidateRoot "report")

New-Item -ItemType Directory -Path $stageWordVisualRoot -Force | Out-Null
if ($hasVisualGateEvidence) {
    Copy-PathTree -Source $resolvedGateReportDir -Destination (Join-Path $stageWordVisualRoot "report")
    Copy-PathTree -Source $resolvedSmokeEvidenceDir -Destination (Join-Path $stageWordVisualRoot "smoke\evidence")
    Copy-PathTree -Source $resolvedFixedGridAggregateDir -Destination (Join-Path $stageWordVisualRoot "fixed-grid\aggregate-evidence")
    Copy-PathTree -Source $resolvedSectionPageSetupAggregateDir -Destination (Join-Path $stageWordVisualRoot "section-page-setup\aggregate-evidence")
} elseif ($AllowIncomplete -and $visualGateStatus -eq "skipped") {
    $incompleteNote = @'
# Word Visual Gate Skipped

- This release-evidence preview was packaged from a CI-style summary with `visual_gate=skipped`.
- Screenshot-backed Word evidence was not available in this environment, so this bundle only keeps the release-candidate report set.
- Run the local Windows preflight with Microsoft Word before publishing public release assets.
'@
    Set-Content -LiteralPath (Join-Path $stageWordVisualRoot "README.md") -Encoding UTF8 -Value $incompleteNote
} else {
    throw "Visual gate evidence is unavailable for packaging."
}

$releaseMaterialRoots = @(
    $stageInstallRoot,
    $stageReleaseCandidateRoot,
    $stageWordVisualRoot
)

if ($hasPdfVisualGateEvidence) {
    Copy-PathTree -Source $resolvedPdfVisualGateRoot -Destination $stagePdfVisualRoot
    $releaseMaterialRoots += $stagePdfVisualRoot
}

Write-Step "Sanitizing staged release materials"
Sanitize-StagedReleaseMaterials -RepoRoot $repoRoot -RootPaths $releaseMaterialRoots

if ($runStrictReleaseMaterialAudit) {
    Write-Step "Checking staged project-template checklist handoff evidence"
    Assert-StagedProjectTemplateChecklistHandoffEvidence -ReleaseCandidateRoot $stageReleaseCandidateRoot
} else {
    Write-Step "Skipping staged project-template checklist handoff evidence check for incomplete CI preview"
}

if ($wordVisualStandardReviewMetadata.Count -gt 0) {
    Write-Step "Checking staged Word visual metadata handoff evidence"
    Assert-StagedWordVisualStandardReviewMetadataHandoffEvidence `
        -ReleaseCandidateRoot $stageReleaseCandidateRoot `
        -ExpectedMetadataCount $wordVisualStandardReviewMetadata.Count
}

if ($runStrictReleaseMaterialAudit) {
    Write-Step "Auditing staged release materials"
    & $releaseMaterialAuditScript -Path $releaseMaterialRoots
} else {
    Write-Step "Skipping staged release material safety audit for incomplete CI preview"
}

$releaseEntryProjectTemplateChecklistMaterialSafetyAudit = [ordered]@{
    status = if ($runStrictReleaseMaterialAudit) { "passed" } else { "skipped_allow_incomplete" }
    audit_script = ".\scripts\assert_release_material_safety.ps1"
    audited_entrypoint_count = if ($runStrictReleaseMaterialAudit) { 3 } else { 0 }
    audited_entrypoints = if ($runStrictReleaseMaterialAudit) { @("start_here", "artifact_guide", "reviewer_checklist") } else { @() }
    compact_evidence_label = "Project-template readiness checklist handoff evidence"
    compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
    compact_evidence_source_schema = "featherdoc.release_candidate_summary"
    checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
    checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
}

$installZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-msvc-install.zip" -f $releaseVersion)
$galleryZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-visual-validation-gallery.zip" -f $releaseVersion)
$evidenceZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-release-evidence.zip" -f $releaseVersion)

Write-Step "Creating ZIP archives"
New-ZipArchive -SourcePaths @($stageInstallRoot) -ZipPath $installZipPath
New-ZipArchive -SourcePaths @($stageGalleryRoot) -ZipPath $galleryZipPath
$releaseEvidenceZipSources = @($stageReleaseCandidateRoot, $stageWordVisualRoot)
if ($hasPdfVisualGateEvidence) {
    $releaseEvidenceZipSources += $stagePdfVisualRoot
}
New-ZipArchive -SourcePaths $releaseEvidenceZipSources -ZipPath $evidenceZipPath

$releaseView = $null
if (-not [string]::IsNullOrWhiteSpace($UploadReleaseTag)) {
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw "GitHub CLI 'gh' is required when -UploadReleaseTag is used."
    }

    $assetPaths = @($installZipPath, $galleryZipPath, $evidenceZipPath)
    $existingReleaseViewJson = & gh release view $UploadReleaseTag --json assets
    if ($LASTEXITCODE -ne 0) {
        throw "gh release view failed before upload."
    }

    $existingReleaseView = $existingReleaseViewJson | ConvertFrom-Json
    $assetNames = @($assetPaths | ForEach-Object { Split-Path -Leaf $_ })
    foreach ($assetPath in $assetPaths) {
        $assetName = Split-Path -Leaf $assetPath
        $existingAsset = @($existingReleaseView.assets | Where-Object { $_.name -eq $assetName }) | Select-Object -First 1
        if ($null -ne $existingAsset) {
            Write-Step "Deleting existing GitHub release asset $assetName"
            & gh release delete-asset $UploadReleaseTag $assetName -y
            if ($LASTEXITCODE -ne 0) {
                throw "gh release delete-asset failed for $assetName."
            }
        }
    }

    for ($deleteWaitAttempt = 1; $deleteWaitAttempt -le 10; $deleteWaitAttempt++) {
        $postDeleteViewJson = & gh release view $UploadReleaseTag --json assets
        if ($LASTEXITCODE -ne 0) {
            throw "gh release view failed while waiting for deleted assets."
        }
        $postDeleteView = $postDeleteViewJson | ConvertFrom-Json
        $remainingAssetNames = @($postDeleteView.assets | Where-Object { $assetNames -contains $_.name } | ForEach-Object { $_.name })
        if ($remainingAssetNames.Count -eq 0) {
            break
        }
        if ($deleteWaitAttempt -eq 10) {
            throw "GitHub release assets were still present after delete: $($remainingAssetNames -join ', ')"
        }
        Start-Sleep -Seconds 2
    }

    Write-Step "Uploading ZIP archives to GitHub release $UploadReleaseTag"
    foreach ($assetPath in $assetPaths) {
        $assetName = Split-Path -Leaf $assetPath
        $uploadedAsset = $false
        for ($uploadAttempt = 1; $uploadAttempt -le 3; $uploadAttempt++) {
            & gh release upload $UploadReleaseTag $assetPath
            if ($LASTEXITCODE -eq 0) {
                $uploadedAsset = $true
                break
            }

            $retryViewJson = & gh release view $UploadReleaseTag --json assets
            if ($LASTEXITCODE -eq 0) {
                $retryView = $retryViewJson | ConvertFrom-Json
                $duplicateAsset = @($retryView.assets | Where-Object { $_.name -eq $assetName }) | Select-Object -First 1
                if ($null -ne $duplicateAsset) {
                    Write-Step "Deleting duplicate GitHub release asset $assetName before retry"
                    & gh release delete-asset $UploadReleaseTag $assetName -y
                    if ($LASTEXITCODE -ne 0) {
                        throw "gh release delete-asset failed for duplicate $assetName."
                    }
                    Start-Sleep -Seconds 2
                }
            }
        }
        if (-not $uploadedAsset) {
            throw "gh release upload failed for $assetName."
        }
    }

    $releaseViewJson = & gh release view $UploadReleaseTag --json tagName,url,assets
    if ($LASTEXITCODE -ne 0) {
        throw "gh release view failed after upload."
    }

    $releaseView = $releaseViewJson | ConvertFrom-Json
}

$manifestPath = Join-Path $versionOutputDir "release_assets_manifest.json"
if ($null -ne $manifestSignoffEntrypointsPublic) {
    $manifestSignoffEntrypointsPublic["release_assets_manifest"] = Convert-EvidencePathToPublicDisplay `
        -Value $manifestPath `
        -RepoRoot $repoRoot
    Convert-EvidenceEntrypointsToPublicDisplay `
        -Contract $manifestSignoffEntrypointsPublic `
        -SourceContract $manifestSignoffEntrypoints `
        -RepoRoot $repoRoot
}
if ($null -ne $projectTemplateReadinessChecklistEntrypointsPublic) {
    Convert-EvidenceEntrypointsToPublicDisplay `
        -Contract $projectTemplateReadinessChecklistEntrypointsPublic `
        -SourceContract $projectTemplateReadinessChecklistEntrypoints `
        -RepoRoot $repoRoot
}
$manifest = [ordered]@{
    generated_at = Get-Date -Format s
    workspace = $repoRoot
    summary_json = $resolvedSummaryPath
    release_version = $releaseVersion
    execution_status = $executionStatus
    visual_verdict = $visualVerdict
    install_prefix = $resolvedInstallPrefix
    readme_assets_dir = $resolvedReadmeAssetsDir
    gate_output_dir = $resolvedGateRoot
    output_dir = $versionOutputDir
    visual_gate_status = $visualGateStatus
    visual_gate_evidence_included = $hasVisualGateEvidence
    pdf_visual_gate_status = $pdfVisualGateStatus
    pdf_visual_gate_summary_json = $resolvedPdfVisualGateSummaryPath
    pdf_visual_gate_output_dir = $resolvedPdfVisualGateRoot
    pdf_visual_gate_evidence_included = $hasPdfVisualGateEvidence
    pdf_visual_gate_evidence = $pdfVisualGateManifestEvidence
    pdf_bounded_ctest_evidence_included = $hasPdfBoundedCtestEvidence
    pdf_bounded_ctest_evidence = $pdfBoundedCtestManifestEvidence
    word_visual_standard_review_metadata_count = $wordVisualStandardReviewMetadata.Count
    word_visual_standard_review_metadata = $wordVisualStandardReviewMetadata
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = $numberingCatalogRealCorpusConfidence
    table_layout_delivery_quality = $tableLayoutDeliveryQuality
    content_control_repair_contract_count = $contentControlRepairContracts.Count
    content_control_repair_contracts = $contentControlRepairContracts
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
    manifest_signoff_entrypoints = $manifestSignoffEntrypointsPublic
    project_template_readiness_checklist_entrypoints = $projectTemplateReadinessChecklistEntrypointsPublic
    release_entry_project_template_readiness_checklist_material_safety_audit = $releaseEntryProjectTemplateChecklistMaterialSafetyAudit
    assets = @(
        (Get-AssetDescriptor -Path $installZipPath -Label "msvc_install")
        (Get-AssetDescriptor -Path $galleryZipPath -Label "visual_validation_gallery")
        (Get-AssetDescriptor -Path $evidenceZipPath -Label "release_evidence")
    )
    upload = [ordered]@{
        requested_tag = $UploadReleaseTag
        uploaded = ($null -ne $releaseView)
        release_url = if ($null -ne $releaseView) { Get-OptionalPropertyValue -Object $releaseView -Name "url" } else { "" }
    }
}

if ($null -ne $releaseView) {
    $releaseAssets = @()
    foreach ($asset in $manifest.assets) {
        $remote = @($releaseView.assets | Where-Object { $_.name -eq (Split-Path -Leaf $asset.path) }) | Select-Object -First 1
        if ($null -ne $remote) {
            $releaseAssets += [ordered]@{
                name = $remote.name
                url = $remote.url
                size_bytes = $remote.size
                download_count = $remote.downloadCount
            }
        }
    }

    $manifest.upload.remote_assets = $releaseAssets
}

$publicManifest = Convert-StructuredValueToPublic -Value $manifest -RepoRoot $repoRoot
($publicManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $manifestPath -Encoding UTF8

if ($runStrictReleaseMaterialAudit) {
    Write-Step "Auditing release asset manifest"
    & $releaseMaterialAuditScript -Path $manifestPath
} else {
    Write-Step "Skipping release asset manifest safety audit for incomplete CI preview"
}

if (-not $KeepStaging) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

Write-Host "Release assets directory: $versionOutputDir"
Write-Host "Asset manifest: $manifestPath"
Write-Host "MSVC install ZIP: $installZipPath"
Write-Host "Visual gallery ZIP: $galleryZipPath"
Write-Host "Release evidence ZIP: $evidenceZipPath"
if ($null -ne $releaseView) {
    Write-Host "Uploaded release: $(Get-OptionalPropertyValue -Object $releaseView -Name 'url')"
}
