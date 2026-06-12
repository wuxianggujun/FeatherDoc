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

. (Join-Path $PSScriptRoot "package_release_assets_core.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_governance_contracts.ps1")
. (Join-Path $PSScriptRoot "package_release_assets_public_materials.ps1")

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
$pdfVisualGateManifestEvidence = Convert-StructuredValueToPublic `
    -Value $pdfVisualGateEvidence `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
if ($pdfVisualGateManifestEvidence -is [System.Collections.IDictionary]) {
    $pdfVisualGateManifestEvidence["summary_json_display"] = Convert-EvidencePathToPublicDisplay `
        -Value $resolvedPdfVisualGateSummaryPath `
        -RepoRoot $repoRoot `
        -PreferEvidenceAnchor
    $pdfVisualGateAggregateContactSheet = Get-OptionalPropertyValue `
        -Object $pdfVisualGateEvidence `
        -Name "aggregate_contact_sheet"
    if (-not [string]::IsNullOrWhiteSpace($pdfVisualGateAggregateContactSheet)) {
        $pdfVisualGateManifestEvidence["aggregate_contact_sheet_display"] = Convert-EvidencePathToPublicDisplay `
            -Value $pdfVisualGateAggregateContactSheet `
            -RepoRoot $repoRoot `
            -PreferEvidenceAnchor
    }
}
$pdfBoundedCtestEvidence = Get-PdfBoundedCtestEvidence -Summary $summary
$pdfBoundedCtestStatus = Get-OptionalPropertyValue -Object $pdfBoundedCtestEvidence -Name "status"
$hasPdfBoundedCtestEvidence = $pdfBoundedCtestStatus -ne "not_available"
$pdfBoundedCtestManifestEvidence = Convert-StructuredValueToPublic `
    -Value $pdfBoundedCtestEvidence `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
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
$manifestSignoffEntrypointsPublic = Convert-StructuredValueToPublic `
    -Value $manifestSignoffEntrypoints `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
$projectTemplateReadinessChecklistEntrypoints = Get-OptionalPropertyObject -Object $summary -Name "project_template_readiness_checklist_entrypoints"
$projectTemplateReadinessChecklistEntrypointsPublic = Convert-StructuredValueToPublic `
    -Value $projectTemplateReadinessChecklistEntrypoints `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
$releaseNoteBundle = Get-OptionalPropertyObject -Object $summary -Name "release_note_bundle"
$releaseNoteBundlePublic = Convert-StructuredValueToPublic `
    -Value $releaseNoteBundle `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
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
    $manifestSignoffEntrypointsPublic["release_assets_manifest"] = "release_assets_manifest.json"
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
if ($null -ne $releaseNoteBundlePublic) {
    Convert-EvidenceEntrypointsToPublicDisplay `
        -Contract $releaseNoteBundlePublic `
        -SourceContract $releaseNoteBundle `
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
    release_note_bundle = $releaseNoteBundlePublic
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

$publicManifest = Convert-StructuredValueToPublic `
    -Value $manifest `
    -RepoRoot $repoRoot `
    -PreferEvidenceAnchor
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
