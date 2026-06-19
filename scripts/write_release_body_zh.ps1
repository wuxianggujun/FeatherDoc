param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputPath = "",
    [string]$ShortOutputPath = "",
    [string]$ReleaseVersion = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "write_release_body_zh_core.ps1")
. (Join-Path $PSScriptRoot "write_release_body_zh_summary.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
if (-not (Test-Path -LiteralPath $resolvedSummaryPath)) {
    throw "Summary JSON does not exist: $resolvedSummaryPath"
}

$resolvedOutputPath = if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_body.zh-CN.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputPath
}

$resolvedShortOutputPath = if ([string]::IsNullOrWhiteSpace($ShortOutputPath)) {
    Join-Path (Split-Path -Parent $resolvedSummaryPath) "release_summary.zh-CN.md"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ShortOutputPath
}

$summary = Get-Content -Raw $resolvedSummaryPath | ConvertFrom-Json
$reportDir = Split-Path -Parent $resolvedSummaryPath
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseHandoffPath = Get-OptionalPropertyValue -Object $summary -Name "release_handoff"
if ([string]::IsNullOrWhiteSpace($releaseHandoffPath)) {
    $releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
}
$artifactGuidePath = Get-OptionalPropertyValue -Object $summary -Name "artifact_guide"
if ([string]::IsNullOrWhiteSpace($artifactGuidePath)) {
    $artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
}
$reviewerChecklistPath = Get-OptionalPropertyValue -Object $summary -Name "reviewer_checklist"
if ([string]::IsNullOrWhiteSpace($reviewerChecklistPath)) {
    $reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
}
$resolvedReleaseVersion = Resolve-ReleaseVersion `
    -RepoRoot $repoRoot `
    -Summary $summary `
    -ExplicitVersion $ReleaseVersion `
    -ExistingHandoffPath $releaseHandoffPath

$installPrefix = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "install_prefix"
$consumerDocument = Get-OptionalPropertyValue -Object $summary.steps.install_smoke -Name "consumer_document"
$gateSummaryPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "summary_json"
$gateFinalReviewPath = Get-OptionalPropertyValue -Object $summary.steps.visual_gate -Name "final_review"
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
$templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($templateSchemaManifestSummaryPath)) {
    $templateSchemaManifestSummaryPath = Get-OptionalPropertyValue -Object $templateSchemaManifestStep -Name "summary_json"
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
$projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeSummary -Name "summary_json"
if ([string]::IsNullOrWhiteSpace($projectTemplateSmokeSummaryPath)) {
    $projectTemplateSmokeSummaryPath = Get-OptionalPropertyValue -Object $projectTemplateSmokeStep -Name "summary_json"
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
$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "page_number_fields"
$smokeReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "smoke"
$fixedGridReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "fixed_grid"
$sectionPageSetupReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "section_page_setup"
$pageNumberFieldsReviewStatus = Get-VisualTaskReviewStatus -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "page_number_fields"
$curatedVisualReviewEntries = @(Get-CuratedVisualReviewEntries -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary)
$pdfVisualGateSummaryPath = Get-PdfVisualGateSummaryPath -Summary $summary
$pdfVisualGateEvidence = Get-PdfVisualGateEvidence -SummaryPath $pdfVisualGateSummaryPath -RepoRoot $repoRoot
$pdfBoundedCtestEvidence = Get-PdfBoundedCtestEvidence -Summary $summary
$pdfBoundedCtestImportDiagnostics = Get-PdfBoundedCtestImportDiagnosticsDisplay -Evidence $pdfBoundedCtestEvidence

$installedDataDir = ""
$installedQuickstartZh = ""
$installedTemplateZh = ""
$installedVisualDir = ""
if (-not [string]::IsNullOrWhiteSpace($installPrefix)) {
    $installedDataDir = Join-Path $installPrefix "share\FeatherDoc"
    $installedQuickstartZh = Join-Path $installedDataDir "VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
    $installedTemplateZh = Join-Path $installedDataDir "RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
    $installedVisualDir = Join-Path $installedDataDir "visual-validation"
}

$publicInstalledQuickstartZh = if ([string]::IsNullOrWhiteSpace($installedQuickstartZh)) {
    ""
} else {
    "share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
}
$publicInstalledTemplateZh = if ([string]::IsNullOrWhiteSpace($installedTemplateZh)) {
    ""
} else {
    "share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
}
$publicInstalledVisualDir = if ([string]::IsNullOrWhiteSpace($installedVisualDir)) {
    ""
} else {
    "share\FeatherDoc\visual-validation"
}

$publicSummaryPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedSummaryPath
$publicShortOutputPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedShortOutputPath
$publicFinalReviewPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $finalReviewPath
$publicReleaseHandoffPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $releaseHandoffPath
$publicArtifactGuidePath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $artifactGuidePath
$publicReviewerChecklistPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $reviewerChecklistPath
$publicGateSummaryPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $gateSummaryPath
$publicGateFinalReviewPath = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $gateFinalReviewPath
$publicReadmeGalleryAssetsDir = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $readmeGalleryAssetsDir
$publicConsumerDocument = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $consumerDocument
$publicProjectTemplateSmokeCandidateDiscoveryJson = Get-PublicArtifactPath -RepoRoot $repoRoot -Value $projectTemplateSmokeCandidateDiscoveryJson

$validationNote = Get-ValidationNote `
    -ExecutionStatus $summary.execution_status `
    -VisualGateStatus $summary.steps.visual_gate.status `
    -VisualVerdict $visualVerdict
$changelogSelection = Resolve-ChangelogSelection -RepoRoot $repoRoot -ReleaseVersion $resolvedReleaseVersion
$changelogSections = $changelogSelection.Sections
$changelogSourceLabel = $changelogSelection.SourceLabel
$shortSummaryBulletResults = @(Get-ShortSummaryBullets `
    -Sections $changelogSections `
    -ChangelogSourceLabel $changelogSourceLabel `
    -ExecutionStatus $summary.execution_status `
    -ConfigureStatus $summary.steps.configure.status `
    -BuildStatus $summary.steps.build.status `
    -TestsStatus $summary.steps.tests.status `
    -InstallSmokeStatus $summary.steps.install_smoke.status `
    -VisualGateStatus $summary.steps.visual_gate.status `
    -VisualVerdict $visualVerdict `
    -InstalledDataDir $installedDataDir `
    -TemplateSchemaManifestStatus $templateSchemaManifestStatus `
    -TemplateSchemaManifestPassed $templateSchemaManifestPassed `
    -TemplateSchemaManifestEntryCount $templateSchemaManifestEntryCount `
    -TemplateSchemaManifestDriftCount $templateSchemaManifestDriftCount `
    -ProjectTemplateSmokeDirtySchemaBaselineCount $projectTemplateSmokeDirtySchemaBaselineCount `
    -SmokeVerdict $smokeVerdict `
    -FixedGridVerdict $fixedGridVerdict `
    -SectionPageSetupVerdict $sectionPageSetupVerdict `
    -PageNumberFieldsVerdict $pageNumberFieldsVerdict `
    -CuratedVisualReviewEntries $curatedVisualReviewEntries)
$shortSummaryBullets = New-Object 'System.Collections.Generic.List[string]'
foreach ($shortSummaryBullet in $shortSummaryBulletResults) {
    Add-UniqueLine -Lines $shortSummaryBullets -Line ([string]$shortSummaryBullet)
}
Add-ProjectTemplateGovernanceContractShortSummaryBullets -Lines $shortSummaryBullets -Summary $summary
Add-ProjectTemplateReadinessChecklistEvidenceShortSummaryBullets -Lines $shortSummaryBullets -Summary $summary
Add-PdfVisualGateEvidenceShortSummaryBullets -Lines $shortSummaryBullets -PdfVisualGateEvidence $pdfVisualGateEvidence -RepoRoot $repoRoot
Add-PdfBoundedCtestEvidenceShortSummaryBullets -Lines $shortSummaryBullets -PdfBoundedCtestEvidence $pdfBoundedCtestEvidence

$releaseChecksCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1"
$releaseGateCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1"
$refreshCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\write_release_note_bundle.ps1 -SummaryJson "{0}" -HandoffOutputPath "{1}" -BodyOutputPath "{2}" -ShortOutputPath "{3}"' -f `
    (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedSummaryPath),
    (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $releaseHandoffPath),
    (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedOutputPath),
    (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $resolvedShortOutputPath)
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseVersion)) {
    $refreshCommand += (' -ReleaseVersion "{0}"' -f $resolvedReleaseVersion)
}

$lines = New-Object 'System.Collections.Generic.List[string]'
[void]$lines.Add("# FeatherDoc v$(if ($resolvedReleaseVersion) { $resolvedReleaseVersion } else { '<版本号>' }) 发布说明")
[void]$lines.Add("")
[void]$lines.Add("## 核心变化")
Add-ChangelogSummaryLines -Lines $lines -Sections $changelogSections -SourceLabel $changelogSourceLabel
[void]$lines.Add("")
[void]$lines.Add("## 验证结论")
[void]$lines.Add("- 执行状态：$($summary.execution_status)")
[void]$lines.Add("- Release blockers：$(Get-ReleaseBlockerCount -Summary $summary)")
[void]$lines.Add("- MSVC configure/build：$($summary.steps.configure.status) / $($summary.steps.build.status)")
[void]$lines.Add("- ctest：$($summary.steps.tests.status)")
[void]$lines.Add("- template schema manifest gate：$(Get-DisplayValue -Value $templateSchemaManifestStatus)")
[void]$lines.Add("- template schema manifest passed：$(Get-DisplayValue -Value $templateSchemaManifestPassed)")
[void]$lines.Add("- template schema manifest entries / drifts：$(Get-DisplayValue -Value ('{0}/{1}' -f $templateSchemaManifestEntryCount, $templateSchemaManifestDriftCount))")
[void]$lines.Add("- project template smoke gate：$(Get-DisplayValue -Value $projectTemplateSmokeStatus)")
[void]$lines.Add("- project template smoke passed：$(Get-DisplayValue -Value $projectTemplateSmokePassed)")
[void]$lines.Add("- project template smoke entries / failed：$(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeEntryCount, $projectTemplateSmokeFailedEntryCount))")
[void]$lines.Add("- project template smoke schema baseline dirty / drift：$(Get-DisplayValue -Value ('{0}/{1}' -f $projectTemplateSmokeDirtySchemaBaselineCount, $projectTemplateSmokeSchemaBaselineDriftCount))")
[void]$lines.Add("- project template smoke candidates registered / unregistered / excluded：$(Get-DisplayValue -Value ('{0}/{1}/{2}' -f $projectTemplateSmokeRegisteredCandidateCount, $projectTemplateSmokeUnregisteredCandidateCount, $projectTemplateSmokeExcludedCandidateCount))")
[void]$lines.Add("- project template smoke full coverage required：$(Get-DisplayValue -Value $projectTemplateSmokeRequireFullCoverage)")
[void]$lines.Add("- project template smoke visual verdict：$(Get-DisplayValue -Value $projectTemplateSmokeVisualVerdict)")
[void]$lines.Add("- project template smoke pending reviews：$(Get-DisplayValue -Value $projectTemplateSmokePendingReviewCount)")
[void]$lines.Add("- install + find_package smoke：$($summary.steps.install_smoke.status)")
[void]$lines.Add("- Word visual release gate：$($summary.steps.visual_gate.status)")
[void]$lines.Add("- Visual verdict：$(if ($visualVerdict) { $visualVerdict } else { 'pending_manual_review' })")
if (-not [string]::IsNullOrWhiteSpace($pdfVisualGateEvidence.status)) {
    [void]$lines.Add("- PDF visual gate verdict：verdict=$(Get-DisplayValue -Value $pdfVisualGateEvidence.verdict) summary=$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $pdfVisualGateEvidence.summary_json)) aggregate_contact_sheet=$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $pdfVisualGateEvidence.aggregate_contact_sheet)) cjk_manifest_count=$(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_manifest_count) cjk_copy_search_count=$(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_copy_search_count) visual_baseline_manifest_count=$(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_manifest_count) visual_baseline_count=$(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_count)")
    [void]$lines.Add("- PDF visual gate summary：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $pdfVisualGateEvidence.summary_json))")
    [void]$lines.Add("- PDF visual gate evidence status：$(Get-DisplayValue -Value $pdfVisualGateEvidence.status)")
    if ($pdfVisualGateEvidence.status -eq "loaded") {
        [void]$lines.Add("- PDF visual gate verdict：$(Get-DisplayValue -Value $pdfVisualGateEvidence.verdict)")
        [void]$lines.Add("- PDF visual gate aggregate contact sheet：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $pdfVisualGateEvidence.aggregate_contact_sheet))")
        [void]$lines.Add("- PDF CJK manifest samples：$(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_manifest_count)")
        [void]$lines.Add("- PDF CJK copy/search samples：$(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_copy_search_count)")
        [void]$lines.Add("- PDF CJK missing text count：$(Get-DisplayValue -Value $pdfVisualGateEvidence.cjk_missing_text_count)")
        [void]$lines.Add("- PDF visual baseline manifest samples：$(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_manifest_count)")
        [void]$lines.Add("- PDF visual baselines：$(Get-DisplayValue -Value $pdfVisualGateEvidence.visual_baseline_count)")
    } elseif (-not [string]::IsNullOrWhiteSpace($pdfVisualGateEvidence.error)) {
        [void]$lines.Add("- PDF visual gate evidence error：$($pdfVisualGateEvidence.error)")
    }
}
if ($pdfBoundedCtestEvidence.status -ne "not_available") {
    [void]$lines.Add("- PDF bounded CTest auxiliary evidence：status=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.status), summaries=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.summary_count), pass=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.pass_count), selected_tests=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.selected_test_count), skipped_tests=$(Get-DisplayValue -Value $pdfBoundedCtestEvidence.skipped_test_count)")
    [void]$lines.Add("- PDF bounded CTest auxiliary subsets：$(Get-DisplayValue -Value (@($pdfBoundedCtestEvidence.subsets) -join ', '))")
    [void]$lines.Add("- PDF bounded CTest auxiliary summaries：$(Get-DisplayValue -Value (@($pdfBoundedCtestEvidence.summary_json_display) -join ', '))")
    if ($pdfBoundedCtestImportDiagnostics.has_evidence) {
        [void]$lines.Add("- PDF bounded CTest import diagnostics contract tests：$(Get-DisplayValue -Value $pdfBoundedCtestImportDiagnostics.tests)")
        [void]$lines.Add("- PDF bounded CTest import diagnostics contract fields：$(Get-DisplayValue -Value $pdfBoundedCtestImportDiagnostics.fields)")
        [void]$lines.Add("- PDF bounded CTest import negative boundary cases：$(Get-DisplayValue -Value $pdfBoundedCtestImportDiagnostics.negative_boundary_cases)")
    }
    [void]$lines.Add("- PDF bounded CTest boundary：辅助证据不能替代 full visual gate verdict。")
}
[void]$lines.Add("- Smoke verdict：$(Get-DisplayValue -Value $smokeVerdict)")
[void]$lines.Add("- Smoke review status：$(Get-DisplayValue -Value $smokeReviewStatus)")
[void]$lines.Add("- Fixed-grid verdict：$(Get-DisplayValue -Value $fixedGridVerdict)")
[void]$lines.Add("- Fixed-grid review status：$(Get-DisplayValue -Value $fixedGridReviewStatus)")
[void]$lines.Add("- Section page setup verdict：$(Get-DisplayValue -Value $sectionPageSetupVerdict)")
[void]$lines.Add("- Section page setup review status：$(Get-DisplayValue -Value $sectionPageSetupReviewStatus)")
[void]$lines.Add("- Page number fields verdict：$(Get-DisplayValue -Value $pageNumberFieldsVerdict)")
[void]$lines.Add("- Page number fields review status：$(Get-DisplayValue -Value $pageNumberFieldsReviewStatus)")
[void]$lines.Add("- Curated visual regression bundles：$($curatedVisualReviewEntries.Count)")
foreach ($curatedVisualReview in $curatedVisualReviewEntries) {
    [void]$lines.Add("- $($curatedVisualReview.label) verdict：$(Get-DisplayValue -Value $curatedVisualReview.verdict)")
    [void]$lines.Add("- $($curatedVisualReview.label) review status：$(Get-DisplayValue -Value $curatedVisualReview.review_status)")
}
[void]$lines.Add("- README 展示图刷新：$(if ($readmeGalleryStatus) { $readmeGalleryStatus } else { 'unknown' })")
[void]$lines.Add("- 说明：$validationNote")
Add-ReleaseBlockerMarkdownSection -Lines $lines -Summary $summary -RepoRoot $repoRoot -Heading "## 发布阻断项" -PublicArtifactPaths
Add-ProjectTemplateGovernanceContractSummaryLines -Lines $lines -Summary $summary
Add-ProjectTemplateReadinessChecklistEvidenceSummaryLines -Lines $lines -Summary $summary
[void]$lines.Add("")
[void]$lines.Add("## 安装包入口")
[void]$lines.Add("- 以下路径使用安装树相对位置，不包含本机绝对目录。")
[void]$lines.Add("- Quickstart：$(Get-DisplayValue -Value $publicInstalledQuickstartZh)")
[void]$lines.Add("- Release 模板：$(Get-DisplayValue -Value $publicInstalledTemplateZh)")
[void]$lines.Add("- 预览图目录：$(Get-DisplayValue -Value $publicInstalledVisualDir)")
[void]$lines.Add("")
[void]$lines.Add("## 复现与复核命令")
[void]$lines.Add('```powershell')
[void]$lines.Add($releaseChecksCommand)
[void]$lines.Add($releaseGateCommand)
[void]$lines.Add($refreshCommand)
[void]$lines.Add('```')
[void]$lines.Add("")
[void]$lines.Add("## 证据文件")
[void]$lines.Add("- 以下路径默认使用仓库相对位置，不包含本机绝对目录。")
[void]$lines.Add("- Release candidate summary JSON：$(Get-DisplayValue -Value $publicSummaryPath)")
[void]$lines.Add("- Final review：$(Get-DisplayValue -Value $publicFinalReviewPath)")
[void]$lines.Add("- Release handoff：$(Get-DisplayValue -Value $publicReleaseHandoffPath)")
[void]$lines.Add("- Release short summary：$(Get-DisplayValue -Value $publicShortOutputPath)")
[void]$lines.Add("- Artifact guide：$(Get-DisplayValue -Value $publicArtifactGuidePath)")
[void]$lines.Add("- Reviewer checklist：$(Get-DisplayValue -Value $publicReviewerChecklistPath)")
[void]$lines.Add("- Template schema manifest summary：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $templateSchemaManifestSummaryPath))")
[void]$lines.Add("- Project template smoke summary：$(Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $repoRoot -Value $projectTemplateSmokeSummaryPath))")
[void]$lines.Add("- Project template smoke candidate discovery：$(Get-DisplayValue -Value $publicProjectTemplateSmokeCandidateDiscoveryJson)")
[void]$lines.Add("- Visual gate summary：$(Get-DisplayValue -Value $publicGateSummaryPath)")
[void]$lines.Add("- Visual gate final review：$(Get-DisplayValue -Value $publicGateFinalReviewPath)")
[void]$lines.Add("- README 展示图目录：$(Get-DisplayValue -Value $publicReadmeGalleryAssetsDir)")
[void]$lines.Add("- install smoke consumer docx：$(Get-DisplayValue -Value $publicConsumerDocument)")

$pdfReleaseBlockerRollup = Get-ReleaseGovernanceRollup -Summary $summary
$pdfPreflightEvidenceBlocker = @(
    Get-ReleaseBlockerArrayProperty -Object $pdfReleaseBlockerRollup -Name "action_items" |
        Where-Object {
            $actionName = Get-ReleaseBlockerPropertyValue -Object $_ -Name "action"
            $actionName -in @(
                "prepare_pdf_visual_release_gate_build_outputs",
                "rerun_pdf_visual_release_gate_preflight"
            )
        } |
        Select-Object -First 1
)
if ($pdfPreflightEvidenceBlocker.Count -gt 0) {
    $pdfPreflightEvidenceLine = Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $pdfPreflightEvidenceBlocker[0]
    if (-not [string]::IsNullOrWhiteSpace($pdfPreflightEvidenceLine)) {
        [void]$lines.Add("")
        [void]$lines.Add("## PDF 预检行动证据")
        [void]$lines.Add("")
        [void]$lines.Add("- $pdfPreflightEvidenceLine")
    }
}

New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputPath) -Force | Out-Null
New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedShortOutputPath) -Force | Out-Null
Write-Utf8NoBomFile -Path $resolvedOutputPath -Text ($lines -join [Environment]::NewLine)

$shortLines = New-Object 'System.Collections.Generic.List[string]'
[void]$shortLines.Add("# FeatherDoc v$(if ($resolvedReleaseVersion) { $resolvedReleaseVersion } else { '<版本号>' }) 发布摘要")
[void]$shortLines.Add("")

if ($shortSummaryBullets.Count -eq 0) {
    [void]$shortLines.Add('- 未能自动提取短摘要，请改为参考 `release_body.zh-CN.md` 手工压缩。')
} else {
    foreach ($bullet in $shortSummaryBullets) {
        [void]$shortLines.Add("- $bullet")
    }
}

Write-Utf8NoBomFile -Path $resolvedShortOutputPath -Text ($shortLines -join [Environment]::NewLine)

Write-Host "Release body output: $resolvedOutputPath"
Write-Host "Release summary output: $resolvedShortOutputPath"
