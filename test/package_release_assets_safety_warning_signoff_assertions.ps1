$warningOnlyProjectTemplateDeliveryReadinessSummaryPath = Join-Path $reportDir "project_template_delivery_readiness_warning_only_summary.json"
$warningOnlyProjectTemplateDeliveryReadinessSummary = $projectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$warningOnlyProjectTemplateDeliveryReadinessSummary.status = "needs_review"
$warningOnlyProjectTemplateDeliveryReadinessSummary.release_ready = $false
$warningOnlyProjectTemplateDeliveryReadinessSummary.template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.ready_template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.blocked_template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.release_blocker_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.action_item_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.warning_count = 1
($warningOnlyProjectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $warningOnlyProjectTemplateDeliveryReadinessSummaryPath -Encoding UTF8

$warningOnlySummaryPath = Join-Path $reportDir "summary.warning-only-project-template-readiness.json"
$warningOnlySummary = $summary | ConvertTo-Json -Depth 20 | ConvertFrom-Json
$warningOnlySummary.project_template_delivery_readiness = $warningOnlyProjectTemplateDeliveryReadinessSummaryPath
($warningOnlySummary | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $warningOnlySummaryPath -Encoding UTF8

$warningOnlyOutputRoot = Join-Path $resolvedWorkingDir "output-release-warning-only"
& $packageScript `
    -SummaryJson $warningOnlySummaryPath `
    -OutputRoot $warningOnlyOutputRoot `
    -KeepStaging

$warningOnlyManifestPath = Join-Path $warningOnlyOutputRoot "v1.6.4\release_assets_manifest.json"
$warningOnlyManifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $warningOnlyManifestPath | ConvertFrom-Json
$warningOnlyManifestReadiness = $warningOnlyManifest.project_template_delivery_readiness_contract
if ([string]$warningOnlyManifestReadiness.status -ne "needs_review") {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness status."
}
if ([bool]$warningOnlyManifestReadiness.release_ready) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness release_ready=false."
}
if ([int]$warningOnlyManifestReadiness.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness release_blocker_count."
}
if ([int]$warningOnlyManifestReadiness.warning_count -ne 1) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness warning_count."
}
$expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $warningOnlyProjectTemplateDeliveryReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$warningOnlyManifestReadiness.source_report_display -ne $expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness source_report_display."
}
if ([string]$warningOnlyManifestReadiness.source_json_display -ne $expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness source_json_display."
}

$manifestSignoffEntrypoints = $manifest.manifest_signoff_entrypoints
if ($null -eq $manifestSignoffEntrypoints) {
    throw "release_assets_manifest.json lost manifest_signoff_entrypoints."
}
if ([string]$manifestSignoffEntrypoints.status -ne "declared") {
    throw "release_assets_manifest.json lost manifest signoff status."
}
if ([string]$manifestSignoffEntrypoints.release_assets_manifest -ne "release_assets_manifest.json") {
    throw "release_assets_manifest.json lost manifest signoff packaged manifest path."
}
if ([int]$manifestSignoffEntrypoints.required_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost manifest signoff required entrypoint count."
}
foreach ($requiredContract in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_contracts | ForEach-Object { [string]$_ }) -contains $requiredContract)) {
        throw "release_assets_manifest.json lost manifest signoff required contract '$requiredContract'."
    }
}
foreach ($requiredField in @(
        "status",
        "release_ready",
        "release_blocker_count",
        "warning_count",
        "schema_approval_status_summary",
        "onboarding_governance_next_action",
        "onboarding_governance_next_action_summary",
        "onboarding_governance_next_action_group_count",
        "next_action",
        "next_action_summary",
        "next_action_group_count",
        "source_report_display",
        "source_json_display"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_fields | ForEach-Object { [string]$_ }) -contains $requiredField)) {
        throw "release_assets_manifest.json lost manifest signoff required field '$requiredField'."
    }
}
if ([string]$manifestSignoffEntrypoints.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
    throw "release_assets_manifest.json lost manifest signoff checklist marker."
}

$manifestProjectTemplateChecklistEntrypoints = $manifest.project_template_readiness_checklist_entrypoints
if ($null -eq $manifestProjectTemplateChecklistEntrypoints) {
    throw "release_assets_manifest.json lost project_template_readiness_checklist_entrypoints."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.status -ne "declared") {
    throw "release_assets_manifest.json lost project-template readiness checklist entrypoints status."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist") {
    throw "release_assets_manifest.json lost project-template readiness checklist label."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
    throw "release_assets_manifest.json lost project-template readiness checklist path."
}
if ([int]$manifestProjectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost project-template readiness checklist required entrypoint count."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
    throw "release_assets_manifest.json lost project-template readiness checklist marker."
}

$manifestProjectTemplateChecklistMaterialSafetyAudit = $manifest.release_entry_project_template_readiness_checklist_material_safety_audit
if ($null -eq $manifestProjectTemplateChecklistMaterialSafetyAudit) {
    throw "release_assets_manifest.json lost release_entry_project_template_readiness_checklist_material_safety_audit."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.status -ne "passed") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit pass status."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.audit_script -ne ".\scripts\assert_release_material_safety.ps1") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit script."
}
if ([int]$manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost project-template checklist material-safety audited entrypoint count."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    if (-not (@($manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoints | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "release_assets_manifest.json lost project-template checklist material-safety audited entrypoint '$entrypointId'."
    }
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_label -ne "Project-template readiness checklist handoff evidence" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_field -ne "project_template_readiness_checklist_entrypoints_source_reports" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_source_schema -ne "featherdoc.release_candidate_summary" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.material_safety_marker -ne "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit identity."
}

$manifestSignoffEntrypointsById = @{}
foreach ($entrypoint in @($manifestSignoffEntrypoints.entrypoints)) {
    $manifestSignoffEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{
            id = "start_here"
            path = $startHerePath
            path_display = ".\output\release-candidate-checks\START_HERE.md"
        },
        [ordered]@{
            id = "artifact_guide"
            path = $artifactGuidePath
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
        },
        [ordered]@{
            id = "reviewer_checklist"
            path = $reviewerChecklistPath
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
        }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestSignoffEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost manifest signoff entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestSignoffEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for manifest signoff entrypoint '$entrypointId'."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize manifest signoff entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost manifest signoff entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

$manifestProjectTemplateChecklistEntrypointsById = @{}
foreach ($entrypoint in @($manifestProjectTemplateChecklistEntrypoints.entrypoints)) {
    $manifestProjectTemplateChecklistEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks\START_HERE.md" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestProjectTemplateChecklistEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost project-template readiness checklist entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestProjectTemplateChecklistEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for project-template readiness checklist entrypoint '$entrypointId'."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize project-template readiness checklist entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost project-template readiness checklist entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

foreach ($zipPath in @($installZipPath, $galleryZipPath, $evidenceZipPath)) {
    if (-not (Test-Path -LiteralPath $zipPath)) {
        throw "Expected ZIP archive was not created: $zipPath"
    }
}

$evidenceZip = [System.IO.Compression.ZipFile]::OpenRead($evidenceZipPath)
try {
    $evidenceZipEntries = @($evidenceZip.Entries | ForEach-Object { $_.FullName -replace '\\', '/' })
    foreach ($expectedEntry in @(
            "release-candidate-checks/report/final_review.md",
            "pdf-visual-release-gate/report/summary.json",
            "pdf-visual-release-gate/report/aggregate-contact-sheet.png",
            "pdf-visual-release-gate/unicode-font/report/full-contact-sheet.png"
        )) {
        if (-not ($evidenceZipEntries -contains $expectedEntry)) {
            throw "Release evidence ZIP did not include expected PDF visual gate entry '$expectedEntry'."
        }
    }
} finally {
    $evidenceZip.Dispose()
}

Write-Host "Package release assets safety regression passed."
