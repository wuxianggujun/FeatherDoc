param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-TextOrder {
    param(
        [string]$Text,
        [string[]]$ExpectedTexts,
        [string]$Message
    )

    $offset = 0
    foreach ($expectedText in $ExpectedTexts) {
        $index = $Text.IndexOf($expectedText, $offset, [System.StringComparison]::Ordinal)
        if ($index -lt 0) {
            throw "$Message Missing expected text after offset $offset. Missing='$expectedText'."
        }

        $offset = $index + $expectedText.Length
    }
}

function Assert-SourceSectionContainsAll {
    param(
        [string]$Text,
        [string]$StartText,
        [string]$EndText,
        [string[]]$ExpectedTexts,
        [string]$Message
    )

    $startIndex = $Text.IndexOf($StartText, [System.StringComparison]::Ordinal)
    if ($startIndex -lt 0) {
        throw "$Message Missing section start='$StartText'."
    }

    $endIndex = $Text.IndexOf($EndText, $startIndex + $StartText.Length, [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) {
        throw "$Message Missing section end='$EndText'."
    }

    $section = $Text.Substring($startIndex, $endIndex - $startIndex)
    foreach ($expectedText in $ExpectedTexts) {
        if ($section.IndexOf($expectedText, [System.StringComparison]::Ordinal) -lt 0) {
            throw "$Message Missing expected text in section. Missing='$expectedText'."
        }
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$scriptRoot = Join-Path $resolvedRepoRoot "scripts"

$checklistDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\project_template_release_readiness_checklist_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$templateSchemaDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\template_schema_mutation_zh.rst"
$releasePipelineDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$releasePolicyDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_policy_zh.rst"
$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$changelogDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "CHANGELOG.md"
$deliveryScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_project_template_delivery_readiness_report.ps1"
$contentControlScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_content_control_data_binding_governance_report.ps1"
$releaseChecksScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_release_candidate_checks.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "run_release_candidate_checks_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseBlockerRollupScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_blocker_rollup_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseGovernancePipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$releaseGovernanceHandoffScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_governance_handoff_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$docxReadinessScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_docx_functional_smoke_readiness.ps1"
$releaseBlockerMetadataHelpersScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "release_blocker_metadata_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseVisualMetadataHelpersScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_visual_metadata_helpers.ps1"
$packageAssetsScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "package_release_assets_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$materialSafetyScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "assert_release_material_safety_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$startHereScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_metadata_start_here.ps1"
$artifactGuideScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_artifact_guide.ps1"
$reviewerChecklistScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_reviewer_checklist.ps1"
$releaseBodyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_body_zh.ps1"
$releaseBundleVersionTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_note_bundle_version_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_note_bundle_version_*.ps1" |
        Where-Object { $_.Name -ne "release_note_bundle_version_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseCandidateVisualTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_candidate_visual_verdict_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_candidate_visual_verdict_*.ps1" |
        Where-Object { $_.Name -ne "release_candidate_visual_verdict_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseBlockerRollupTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_blocker_rollup_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_blocker_rollup_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseGovernanceHandoffTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_governance_handoff_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_governance_handoff_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
    "en/index",
    "zh-CN/index",
    "en/api/index",
    "zh-CN/api/index"
)) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Docs index should preserve bilingual public docs entry '$marker'."
}

$governanceReleasePreflightStart = $governanceRoutesDoc.IndexOf(
    "Project Template Release Readiness",
    [System.StringComparison]::Ordinal)
if ($governanceReleasePreflightStart -lt 0) {
    throw "Governance routes doc should keep the project-template release readiness guidance."
}

$governanceReleasePreflightGuidance = $governanceRoutesDoc.Substring($governanceReleasePreflightStart)

foreach ($marker in @(
    "output/release-candidate-checks/START_HERE.md",
    "release_governance_handoff.md",
    "word_visual_standard_review_metadata_source_reports",
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "final_review.md",
    "release_handoff.md",
    "Word visual standard review metadata evidence"
)) {
    Assert-ContainsText -Text $governanceReleasePreflightGuidance -ExpectedText $marker `
        -Message "Governance routes release-preflight guidance should keep Word visual metadata marker '$marker'."
}

Assert-TextOrder -Text $governanceReleasePreflightGuidance -ExpectedTexts @(
    "release_governance_handoff.md",
    "word_visual_standard_review_metadata_source_reports",
    "Word visual standard review metadata evidence"
) -Message "Governance routes release-preflight guidance should describe detailed handoff evidence before compact release reviewer evidence."

foreach ($marker in @(
    "project-template governance",
    "onboard_project_template.ps1",
    "build_project_template_onboarding_governance_report.ps1",
    "run_project_template_smoke.ps1",
    "output/project-template-smoke/summary.json",
    "featherdoc.project_template_smoke_summary.v1",
    "project_template_smoke_summary",
    "sync_project_template_schema_approval.ps1",
    "write_project_template_schema_approval_history.ps1",
    "featherdoc.project_template_schema_approval_history.v1",
    "build_project_template_delivery_readiness_report.ps1",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "release_ready",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary",
    "release_blocker_count",
    "source_json_display",
    "source_report_display",
    "input_docx",
    "template_name",
    "schema_target",
    "target_mode",
    "open_command",
    "build_content_control_data_binding_governance_report.ps1",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "content_control_data_binding.bound_placeholder",
    "sync_bound_content_control",
    "sync-content-controls-from-custom-xml",
    "featherdoc.content_control_data_binding_repair_plan.v1",
    "build_release_blocker_rollup_report.ps1",
    "build_release_governance_pipeline_report.ps1",
    "build_release_governance_handoff_report.ps1",
    "run_release_candidate_checks.ps1",
    "assert_release_material_safety.ps1",
    "release_note_bundle_version_test.ps1",
    "steps.release_governance_handoff",
    "final_review.md",
    "project_template_delivery_readiness_contract",
    "project_template_onboarding.schema_approval",
    "project_template_onboarding_governance",
    "project-template-delivery-readiness",
    "project-template-onboarding-governance",
    "content-control provenance",
    "block_scoped_entry_content_control_trace",
    "block_scoped_entry_governance_metric_trace",
    "block_scoped_entry_project_template_trace",
    "block_scoped_entry_project_template_status_trace",
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "block_scoped_entry_trace",
    "release_handoff.md",
    "release_body.zh-CN.md",
    "release_summary.zh-CN.md",
    "release_governance_handoff.md",
    "release_assets_manifest.json",
    "Project template release readiness checklist",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "release_entry_project_template_readiness_checklist_trace",
    "project_template_readiness_checklist_entrypoints",
    "13 required fields",
    "project_template_readiness_checklist_entrypoints_manifest_trace",
    "project_template_readiness_checklist_entrypoints_governance_trace",
    "project_template_readiness_checklist_entrypoints_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_rollup_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_path_display_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_notes_trace",
    "compact_evidence_source_schema",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "manifest_signoff_entrypoints",
    "manifest_signoff_entrypoints_release_trace",
    "manifest_signoff_entrypoints_rollup_material_safety_trace",
    "manifest_signoff_entrypoints_handoff_material_safety_trace",
    "manifest_signoff_entrypoints_manifest_trace",
    "-CasePattern",
    "final-review-pdf-visual",
    "release-material-safety-final-review-pdf-check"
)) {
    Assert-ContainsText -Text $checklistDoc -ExpectedText $marker `
        -Message "Project-template release readiness checklist should preserve marker '$marker'."
}

$deliveryReadinessGuidanceStart = $checklistDoc.IndexOf(
    "3. Delivery readiness",
    [System.StringComparison]::Ordinal)
if ($deliveryReadinessGuidanceStart -lt 0) {
    throw "Project-template release readiness checklist should keep the delivery readiness section."
}

$deliveryReadinessGuidanceEnd = $checklistDoc.IndexOf(
    "4. Content-control",
    $deliveryReadinessGuidanceStart,
    [System.StringComparison]::Ordinal)
if ($deliveryReadinessGuidanceEnd -lt 0) {
    throw "Project-template release readiness checklist should keep content-control guidance after delivery readiness."
}

$deliveryReadinessGuidance = $checklistDoc.Substring(
    $deliveryReadinessGuidanceStart,
    $deliveryReadinessGuidanceEnd - $deliveryReadinessGuidanceStart)

foreach ($marker in @(
    "project_template_smoke_summary_missing",
    "manifest-only / missing-evidence",
    "release_ready = false",
    "status = blocked",
    "needs_review",
    "warning_count",
    "warnings[]"
)) {
    Assert-ContainsText -Text $deliveryReadinessGuidance -ExpectedText $marker `
        -Message "Project-template delivery readiness guidance should preserve warning-only needs_review marker '$marker'."
}

Assert-TextOrder -Text $deliveryReadinessGuidance -ExpectedTexts @(
    "release_ready = false",
    "status = blocked",
    "release_blocker_count",
    "manifest-only / missing-evidence",
    "needs_review",
    "warning_count",
    "warnings[]"
) -Message "Project-template delivery readiness guidance should distinguish blocked blockers from warning-only needs_review evidence."

foreach ($marker in @(
    '} elseif ($warnings.Count -gt 0) {',
    '"needs_review"',
    'release_ready = ($status -eq "ready")',
    'warning_count = $warnings.Count',
    'warnings = @($warnings.ToArray())'
)) {
    Assert-ContainsText -Text $deliveryScript -ExpectedText $marker `
        -Message "Project-template delivery readiness script should preserve warning-only needs_review implementation marker '$marker'."
}

Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText "Warning-only evidence gaps now" `
    -Message "Governance routes doc should preserve warning-only needs_review project-template delivery readiness semantics."
Assert-TextOrder `
    -Text $governanceRoutesDoc `
    -ExpectedTexts @("Warning-only evidence gaps now", "produce", "status=needs_review", "release_ready=false") `
    -Message "Governance routes doc should preserve warning-only needs_review/release_ready=false wording."
Assert-TextOrder `
    -Text $governanceRoutesDoc `
    -ExpectedTexts @(
        "scripts/build_project_template_delivery_readiness_report.ps1",
        "status=needs_review",
        "release_ready=false",
        "onboarding_summary.json"
    ) `
    -Message "Governance routes doc should preserve needs_review/release_ready=false wording."
Assert-ContainsText -Text $changelogDoc -ExpectedText "warning-only evidence gaps report" `
    -Message "Changelog should preserve warning-only delivery readiness fix note."
Assert-TextOrder `
    -Text $changelogDoc `
    -ExpectedTexts @("warning-only evidence gaps report", "needs_review", "instead of", "ready") `
    -Message "Changelog should preserve needs_review instead of ready regression note."

foreach ($marker in @(
    "schema_patch_approval_gate_status",
    "schema_patch_approval_invalid_result_count",
    "project_template_smoke.schema_approval",
    "write_project_template_schema_approval_history.ps1"
)) {
    Assert-ContainsText -Text $templateSchemaDoc -ExpectedText $marker `
        -Message "Template schema mutation docs should keep schema approval governance marker '$marker'."
}

foreach ($marker in @(
    "featherdoc.project_template_delivery_readiness_report.v1",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "featherdoc.docx_functional_smoke_readiness.v1",
    "docx_functional_smoke_readiness",
    "content_control_data_binding.bound_placeholder",
    "sync_bound_content_control",
    "input_docx",
    "template_name",
    "schema_target",
    "target_mode",
    "steps.release_governance_handoff",
    "final_review.md",
    "Release governance handoff details",
    "source_report_display",
    "source_json_display",
    "project_template_readiness_checklist_entrypoints",
    "project_template_readiness_checklist_entrypoints_source_reports",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "release_entry_project_template_readiness_checklist_trace",
    "project_template_readiness_checklist_entrypoints_governance_trace",
    "project_template_readiness_checklist_entrypoints_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_rollup_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_path_display_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace",
    "compact_evidence_source_schema",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "manifest_signoff_entrypoints_rollup_material_safety_trace",
    "manifest_signoff_entrypoints_handoff_material_safety_trace",
    "block_scoped_governance_handoff_trace",
    "block_scoped_governance_handoff_project_template_status_trace",
    "release_blocker_count",
    "warning_count",
    "project_template_onboarding.schema_approval",
    "Markdown list block",
    "project-template-delivery-readiness",
    "project-template-onboarding-governance",
    "review_schema_update_candidate",
    "approve_project_template_schema",
    "promote_schema_update_candidate",
    "update_template_or_schema_before_retry",
    "freeze_schema_baseline",
    "review_schema_baseline",
    "review_schema_approval_history",
    "run_project_template_smoke_for_registered_manifest",
    "run_project_template_smoke_then_review_schema_patch_approval",
    "review_project_template_smoke_failure",
    "collect_project_template_onboarding_governance_evidence",
    "review_project_template_delivery_readiness_evidence"
)) {
    Assert-ContainsText -Text $releasePipelineDoc -ExpectedText $marker `
        -Message "Release metadata pipeline docs should keep governance marker '$marker'."
}

$onboardingGuidanceStart = $releasePipelineDoc.IndexOf(
    "onboarding-derived blocker / action item",
    [System.StringComparison]::Ordinal)
if ($onboardingGuidanceStart -lt 0) {
    throw "Release metadata pipeline docs should describe project-template onboarding-derived blocker/action-item guidance."
}

$onboardingGuidanceEnd = $releasePipelineDoc.IndexOf(
    "schema confidence calibration",
    $onboardingGuidanceStart,
    [System.StringComparison]::Ordinal)
if ($onboardingGuidanceEnd -lt 0) {
    throw "Release metadata pipeline docs should keep schema confidence calibration after onboarding guidance."
}

$onboardingGuidance = $releasePipelineDoc.Substring(
    $onboardingGuidanceStart,
    $onboardingGuidanceEnd - $onboardingGuidanceStart)

foreach ($marker in @(
    "onboarding-derived blocker / action item",
    "``source_schema``",
    "``source_report_display``",
    "``source_json_display``",
    "``open_command``",
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "release_handoff.md",
    "project_template_onboarding.*",
    "governance/report",
    "onboarding governance",
    "schema approval",
    "project-template provenance",
    "run_project_template_smoke.ps1",
    "sync_project_template_schema_approval.ps1",
    "write_project_template_schema_approval_history.ps1",
    "build_project_template_onboarding_governance_report.ps1",
    "build_project_template_delivery_readiness_report.ps1",
    "reviewer runbook"
)) {
    Assert-ContainsText -Text $onboardingGuidance -ExpectedText $marker `
        -Message "Release metadata pipeline docs should keep project-template onboarding source evidence guidance '$marker'."
}

Assert-TextOrder -Text $onboardingGuidance -ExpectedTexts @(
    "``source_schema``",
    "``source_report_display``",
    "``source_json_display``"
) -Message "Release metadata pipeline onboarding guidance should list source report display between schema and source JSON."

Assert-TextOrder -Text $onboardingGuidance -ExpectedTexts @(
    "``source_report_display``",
    "governance/report",
    "``source_json_display``",
    "onboarding governance",
    "``open_command``",
    "schema approval"
) -Message "Release metadata pipeline onboarding guidance should tell reviewers to open report, then source JSON, then command."

$wordVisualMetadataEvidenceIndex = $releasePipelineDoc.IndexOf(
    "Word visual standard review metadata evidence",
    [System.StringComparison]::Ordinal)
if ($wordVisualMetadataEvidenceIndex -lt 0) {
    throw "Release metadata pipeline docs should describe the Word visual metadata compact evidence line."
}

$wordVisualMetadataGuidanceStart = $releasePipelineDoc.LastIndexOf(
    "START_HERE.md",
    $wordVisualMetadataEvidenceIndex,
    [System.StringComparison]::Ordinal)
if ($wordVisualMetadataGuidanceStart -lt 0) {
    throw "Release metadata pipeline docs should list release materials before the Word visual metadata compact evidence line."
}

$wordVisualMetadataGuidanceEnd = $releasePipelineDoc.IndexOf(
    "package_release_assets.ps1",
    $wordVisualMetadataEvidenceIndex,
    [System.StringComparison]::Ordinal)
if ($wordVisualMetadataGuidanceEnd -lt 0) {
    throw "Release metadata pipeline docs should keep package_release_assets guidance after Word visual metadata compact evidence."
}

$wordVisualMetadataGuidance = $releasePipelineDoc.Substring(
    $wordVisualMetadataGuidanceStart,
    $wordVisualMetadataGuidanceEnd - $wordVisualMetadataGuidanceStart)

foreach ($marker in @(
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "final_review.md",
    "release_handoff.md",
    "release_governance_handoff.md",
    "word_visual_standard_review_metadata_source_reports",
    "task_reviews=",
    "final_review_path=",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report",
    "release-candidate-checks",
    "review_note",
    "detailed"
)) {
    Assert-ContainsText -Text $wordVisualMetadataGuidance -ExpectedText $marker `
        -Message "Release metadata pipeline docs should keep Word visual metadata guidance '$marker'."
}

Assert-TextOrder -Text $wordVisualMetadataGuidance -ExpectedTexts @(
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "final_review.md",
    "release_handoff.md",
    "release_governance_handoff.md",
    "detailed",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report",
    "release-candidate-checks",
    "review_note"
) -Message "Release metadata pipeline Word visual metadata guidance should separate compact release materials from detailed governance handoff source reports."

Assert-ContainsText -Text $releasePolicyDoc -ExpectedText "schema_patch_approval_gate_status" `
    -Message "Release policy should keep project-template schema approval as a release criterion."

foreach ($marker in @(
    "content_control_data_binding.bound_placeholder",
    "source_json_display",
    "repair_strategy",
    "command_template",
    "input_docx",
    "template_name",
    "schema_target",
    "target_mode"
)) {
    Assert-ContainsText -Text $releasePolicyDoc -ExpectedText $marker `
        -Message "Release policy should keep content-control provenance release criterion '$marker'."
}

foreach ($marker in @(
    "featherdoc.project_template_delivery_readiness_report.v1",
    "project_template_delivery_readiness.schema_approval_history_gate",
    "source_json_display",
    "source_report_display",
    "release_ready",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary"
)) {
    Assert-ContainsText -Text $deliveryScript -ExpectedText $marker `
        -Message "Delivery readiness script should keep contract marker '$marker'."
}

foreach ($marker in @(
    "featherdoc.content_control_data_binding_governance_report.v1",
    "content_control_data_binding.bound_placeholder",
    "sync_bound_content_control",
    "sync-content-controls-from-custom-xml",
    "featherdoc.content_control_data_binding_repair_plan.v1"
)) {
    Assert-ContainsText -Text $contentControlScript -ExpectedText $marker `
        -Message "Content-control governance script should keep contract marker '$marker'."
}

foreach ($marker in @(
    "Project template smoke schema approval history:",
    "project_template_schema_approval_history",
    "ProjectTemplateSmokeManifestPath",
    "project_template_smoke",
    "ReleaseBlockerRollupInputJson",
    "ReleaseGovernanceHandoffInputJson",
    "ReleaseGovernanceHandoffExpectedReportProfile",
    "ReleaseEvidenceScope",
    '$releaseManifestSignoffEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only")',
    'manifest_signoff_entrypoints = $releaseManifestSignoffEntrypoints'
)) {
    Assert-ContainsText -Text $releaseChecksScript -ExpectedText $marker `
        -Message "Release candidate checks should keep governance handoff marker '$marker'."
}

foreach ($marker in @(
    "ExpectedReportProfile",
    "explicit-only",
    '$expectedReports = @(',
    '$knownExpectedReportById',
    "docx_functional_smoke_readiness",
    "featherdoc.docx_functional_smoke_readiness.v1"
)) {
    Assert-ContainsText -Text $releaseGovernanceHandoffScript -ExpectedText $marker `
        -Message "Release governance handoff should keep expected-report profile marker '$marker'."
}

foreach ($marker in @(
    "docx_functional_smoke_readiness",
    "check_docx_functional_smoke_readiness.ps1",
    "docx-functional-smoke-readiness"
)) {
    Assert-ContainsText -Text $releaseGovernancePipelineScript -ExpectedText $marker `
        -Message "Release governance pipeline should keep DOCX readiness stage marker '$marker'."
}

foreach ($marker in @(
    "OutputDir",
    "SummaryJson",
    "ReportMarkdown",
    "release_blocker_count",
    "report_markdown_display"
)) {
    Assert-ContainsText -Text $docxReadinessScript -ExpectedText $marker `
        -Message "DOCX readiness script should keep release-governance compatible marker '$marker'."
}

foreach ($marker in @(
    "Test-ReleaseManifestSignoffRequiresProjectTemplateGovernance",
    "project_template_delivery_readiness_contract",
    "project_template_onboarding_governance_contract"
)) {
    Assert-ContainsText -Text $releaseVisualMetadataHelpersScript -ExpectedText $marker `
        -Message "Release visual metadata helpers should keep project-template signoff scope marker '$marker'."
}

foreach ($scriptInfo in @(
    [pscustomobject]@{ Name = "write_release_metadata_start_here.ps1"; Text = $startHereScript },
    [pscustomobject]@{ Name = "write_release_artifact_guide.ps1"; Text = $artifactGuideScript },
    [pscustomobject]@{ Name = "write_release_reviewer_checklist.ps1"; Text = $reviewerChecklistScript }
)) {
    Assert-ContainsText -Text $scriptInfo.Text -ExpectedText 'Test-ReleaseManifestSignoffRequiresProjectTemplateGovernance -Summary $summary' `
        -Message "$($scriptInfo.Name) should gate project-template signoff text on explicit manifest contracts."
    Assert-ContainsText -Text $scriptInfo.Text -ExpectedText 'if ($requiresProjectTemplateGovernanceSignoff -or $hasProjectTemplateReleaseEntryEvidence)' `
        -Message "$($scriptInfo.Name) should skip PDF-only placeholders while preserving materialized project-template release-entry evidence."
}

foreach ($marker in @(
    "manifest_signoff_entrypoints",
    "release_assets_manifest.json",
    "project_template_delivery_readiness_contract",
    "project_template_onboarding_governance_contract",
    "required_entrypoint_count",
    "required_contracts",
    "required_fields",
    "reviewer_manifest_scoped_project_template_trace"
)) {
    Assert-ContainsText -Text $releaseChecksScript -ExpectedText $marker `
        -Message "Release candidate checks should keep manifest signoff summary marker '$marker'."
}

foreach ($marker in @(
    "project_template_delivery_readiness_contract",
    "project_template_onboarding_governance_contract",
    "content_control_data_binding.bound_placeholder",
    "Test-ReleaseEntryContentControlTraceBlockContainsAll",
    "Test-ReleaseEntryGovernanceMetricTraceBlockContainsAll",
    "Add-ReleaseEntryGovernanceMetricDetailViolations",
    "Test-ReleaseEntryProjectTemplateTraceBlockContainsAll",
    "Add-ReleaseEntryProjectTemplateDetailViolations",
    "numbering_catalog_governance.real_corpus_confidence",
    "table_layout_delivery_governance.delivery_quality",
    "project_template_delivery_readiness",
    "project_template_onboarding.schema_approval",
    "Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations",
    "Add-ReleaseGovernanceHandoffProjectTemplateGovernanceTraceViolations",
    "Add-FinalReviewProjectTemplateGovernanceTraceViolations",
    "Add-ProjectTemplateDeliveryReadinessContractViolations",
    "Add-ProjectTemplateOnboardingGovernanceContractViolations",
    "Add-ManifestSignoffEntrypointsContractViolations",
    "manifest_signoff_entrypoints",
    "required_entrypoint_count",
    "reviewer_checklist",
    "source_json_display",
    "Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll",
    "Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll",
    "Test-FinalReviewProjectTemplateTraceBlockContainsAll",
    "Test-ReleaseNoteProjectTemplateTraceLineContainsAll",
    "Test-TextLineContainsAll",
    "Test-MarkdownListBlockContainsAll",
    "-Anchor ""project_template_delivery_readiness""",
    "-Anchor ""project_template_onboarding.schema_approval""",
    "project_template_onboarding.schema_approval",
    "schema_approval_status_summary=",
    "repair_strategy",
    "command_template",
    "source_report_display",
    "source_json_display"
)) {
    Assert-ContainsText -Text $materialSafetyScript -ExpectedText $marker `
        -Message "Material safety audit should keep public governance safety marker '$marker'."
}

foreach ($marker in @(
    "Markdown list block",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "block_scoped_entry_content_control_trace",
    "block_scoped_entry_governance_metric_trace",
    "table_layout_delivery_governance.delivery_quality",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "pdf_floating_table_metadata_only_fields",
    "pdf_floating_table_review_required_fields",
    'metadata-only ``tblpPr``',
    "block_scoped_entry_project_template_trace",
    "block_scoped_entry_project_template_status_trace",
    "entry_readiness_value_set_trace",
    "block_scoped_release_bundle_project_template_trace",
    "manifest_scoped_project_template_trace",
    "manifest_source_display_identity_trace",
    "manifest_status_release_ready_consistency_trace",
    "manifest_readiness_value_set_trace",
    "reviewer_manifest_scoped_project_template_trace",
    "manifest_signoff_entrypoints_release_trace",
    "manifest_signoff_entrypoints_manifest_trace",
    "line_scoped_release_note_project_template_trace",
    "line_scoped_release_note_source_identity_trace",
    "line_scoped_release_note_readiness_value_set_trace",
    "line_scoped_release_note_onboarding_value_set_trace",
    "block_scoped_release_handoff_trace",
    "single_block_release_handoff_project_template_trace",
    "block_scoped_release_handoff_source_identity_trace",
    "release_handoff_readiness_value_set_trace",
    "block_scoped_governance_handoff_trace",
    "block_scoped_governance_handoff_project_template_status_trace",
    "single_block_governance_handoff_project_template_trace",
    "block_scoped_governance_handoff_source_identity_trace",
    "governance_handoff_readiness_value_set_trace",
    "onboarding_contract_status_release_ready_value_set_trace",
    "block_scoped_final_review_project_template_trace",
    "block_scoped_final_review_project_template_status_trace",
    "readiness_status",
    "readiness_release_ready",
    "final_review_readiness_value_set_trace",
    "single_block_final_review_project_template_trace",
    "block_scoped_final_review_source_identity_trace",
    "onboarding governance block"
)) {
    Assert-ContainsText -Text $checklistDoc -ExpectedText $marker `
        -Message "Project-template checklist should document block-scoped governance handoff safety marker '$marker'."
}

foreach ($scriptText in @($startHereScript, $artifactGuideScript, $reviewerChecklistScript, $releaseBundleVersionTest)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Release entry generators/tests should keep the project-template readiness checklist wired."
}

Assert-ContainsText -Text $reviewerChecklistScript -ExpectedText "Confirm the fixed Project template release readiness checklist has been reviewed before publishing" `
    -Message "Reviewer checklist should require the fixed project-template readiness checklist review."

Assert-ContainsText -Text $materialSafetyScript -ExpectedText "Entry document must point reviewers at docs/project_template_release_readiness_checklist_zh.rst" `
    -Message "Material safety audit should require release entries to point at the fixed project-template readiness checklist."

foreach ($scriptText in @($releaseChecksScript, $packageAssetsScript, $materialSafetyScript, $releaseCandidateVisualTest)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints" `
        -Message "Release candidate/package/material safety flow should keep the project-template readiness checklist entrypoints machine-auditable."
}

foreach ($scriptText in @($releaseBlockerRollupScript, $releaseGovernanceHandoffScript, $releaseBlockerMetadataHelpersScript, $releaseChecksScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints" `
        -Message "Release governance flow should preserve project-template readiness checklist entrypoint fields."
}

foreach ($scriptText in @($releaseBlockerRollupScript, $releaseGovernanceHandoffScript, $releaseBlockerMetadataHelpersScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_marker" `
        -Message "Release governance flow should preserve the project-template readiness checklist marker field."
}

foreach ($marker in @(
    "release_entry_project_template_readiness_checklist_material_safety_audit",
    "release_entry_project_template_readiness_checklist_material_safety_audit_status",
    "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count",
    "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema",
    "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
)) {
    Assert-ContainsText -Text $releaseBlockerRollupScript -ExpectedText $marker `
        -Message "Release blocker rollup should expose packaged project-template checklist material-safety audit evidence."
}

Assert-ContainsText -Text $releaseBlockerRollupTest -ExpectedText 'Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "featherdoc.release_candidate_summary" -ExpectedFragments @(' `
    -Message "Release blocker rollup regression should lock release-candidate Source Report Contracts as one Markdown list block."
Assert-SourceSectionContainsAll -Text $releaseBlockerRollupTest `
    -StartText 'Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "featherdoc.release_candidate_summary" -ExpectedFragments @(' `
    -EndText 'project_template_readiness_checklist_entrypoints_status: ``declared``' `
    -ExpectedTexts @(
        'manifest_signoff_entrypoints_status: ``declared``',
        'manifest_signoff_entrypoints_release_assets_manifest_display:',
        'release_assets_manifest.json',
        'manifest_signoff_entrypoints_required_entrypoint_count: ``3``',
        'manifest_signoff_entrypoints_entrypoint_ids:',
        'start_here',
        'artifact_guide',
        'reviewer_checklist',
        'manifest_signoff_entrypoints_required_contracts:',
        'project_template_delivery_readiness_contract',
        'project_template_onboarding_governance_contract',
        'manifest_signoff_entrypoints_required_fields:',
        'status',
        'release_ready',
        'release_blocker_count',
        'warning_count',
        'schema_approval_status_summary',
        'onboarding_governance_next_action',
        'onboarding_governance_next_action_summary',
        'onboarding_governance_next_action_group_count',
        'next_action',
        'next_action_summary',
        'next_action_group_count',
        'source_report_display',
        'source_json_display',
        'manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``'
    ) `
    -Message "Release blocker rollup regression should keep manifest signoff evidence in the release-candidate Source Report Contracts block."
foreach ($marker in @(
    'project_template_readiness_checklist_entrypoints_status: ``declared``',
    'project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``',
    'project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``',
    'project_template_readiness_checklist_entrypoints_entrypoint_ids:',
    'release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``'
)) {
    Assert-ContainsText -Text $releaseBlockerRollupTest -ExpectedText $marker `
        -Message "Release blocker rollup regression should prove project-template checklist and packaged audit evidence stay in the release-candidate source-report block."
}

Assert-ContainsText -Text $releaseGovernanceHandoffTest -ExpectedText 'if (Test-Scenario -Name "include_rollup") {' `
    -Message "Release governance handoff regression should exercise the nested release blocker rollup evidence path."
Assert-ContainsText -Text $releaseGovernanceHandoffTest -ExpectedText 'Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(' `
    -Message "Release governance handoff regression should lock release-candidate source_report evidence as one Markdown list block."
Assert-SourceSectionContainsAll -Text $releaseGovernanceHandoffTest `
    -StartText 'Assert-ContainsText -Text $markdown -ExpectedText "Manifest signoff entrypoints evidence source reports: ``1``"' `
    -EndText 'Assert-ContainsText -Text $markdown -ExpectedText "Project-template readiness checklist entrypoints evidence source reports: ``1``"' `
    -ExpectedTexts @(
        'Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(',
        '"schema=``featherdoc.release_candidate_summary``"',
        '"manifest_signoff_entrypoints_status: ``declared``"',
        '"manifest_signoff_entrypoints_release_assets_manifest_display:"',
        '"release_assets_manifest.json"',
        '"manifest_signoff_entrypoints_required_entrypoint_count: ``3``"',
        '"manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``"',
        '"manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``"',
        '"manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display``"',
        '"manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``"'
    ) `
    -Message "Release governance handoff regression should keep manifest signoff evidence in one release-candidate source_report block."
foreach ($marker in @(
    'project_template_readiness_checklist_entrypoints_source_report_count) -Expected 1',
    'release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count) -Expected 1',
    'project_template_readiness_checklist_entrypoints_status: ``declared``',
    'project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``',
    'project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``',
    'project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``',
    'release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``'
)) {
    Assert-ContainsText -Text $releaseGovernanceHandoffTest -ExpectedText $marker `
        -Message "Release governance handoff regression should prove nested project-template checklist and packaged audit evidence stay in the release-candidate source_report block."
}

foreach ($scriptText in @($releaseGovernanceHandoffScript, $releaseBlockerMetadataHelpersScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema" `
        -Message "Release governance handoff/helper flow should preserve packaged checklist compact evidence source schema."
}

foreach ($scriptText in @($releaseGovernanceHandoffScript, $releaseBlockerMetadataHelpersScript, $releaseChecksScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports" `
        -Message "Release governance handoff flow should propagate packaged project-template checklist material-safety audit source reports."
    Assert-ContainsText -Text $scriptText -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count" `
        -Message "Release governance handoff flow should propagate packaged project-template checklist material-safety audit evidence counts."
}

foreach ($scriptText in @($releaseGovernanceHandoffScript, $releaseBlockerMetadataHelpersScript, $releaseChecksScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Release governance flow should propagate project-template readiness checklist entrypoints into source-report evidence."
}

Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine" `
    -Message "Release entry flow should have a compact checklist handoff evidence line helper."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Project-template readiness checklist handoff evidence" `
    -Message "Release entry flow helper should render the checklist handoff evidence label."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "project_template_readiness_checklist_entrypoints_required_entrypoint_count" `
    -Message "Compact checklist handoff evidence should render the required entrypoint count."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "project_template_readiness_checklist_entrypoints_entrypoints" `
    -Message "Compact checklist handoff evidence should render entrypoint path_display details."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "entrypoint_paths=" `
    -Message "Compact checklist handoff evidence should expose entrypoint_paths on one line."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "source_schema=" `
    -Message "Compact checklist evidence should render source_schema on the release entry evidence lines."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine" `
    -Message "Release entry flow should have a compact checklist packaged audit evidence line helper."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Project-template readiness checklist packaged audit evidence" `
    -Message "Release entry flow helper should render the packaged checklist audit evidence label."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "compact_evidence_source_schema=" `
    -Message "Release entry flow helper should render the packaged checklist compact evidence source schema."

foreach ($scriptText in @($startHereScript, $artifactGuideScript, $reviewerChecklistScript)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine" `
        -Message "Release entry generators should call the compact checklist handoff evidence helper."
    Assert-ContainsText -Text $scriptText -ExpectedText "projectTemplateChecklistHandoffEvidenceLine" `
        -Message "Release entry generators should surface the checklist handoff evidence line."
    Assert-ContainsText -Text $scriptText -ExpectedText "Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine" `
        -Message "Release entry generators should call the compact checklist packaged audit evidence helper."
    Assert-ContainsText -Text $scriptText -ExpectedText "projectTemplateChecklistMaterialSafetyAuditEvidenceLine" `
        -Message "Release entry generators should surface the checklist packaged audit evidence line."
}

foreach ($marker in @(
    "Add-ProjectTemplateReadinessChecklistEvidenceSummaryLines",
    "Add-ProjectTemplateReadinessChecklistEvidenceShortSummaryBullets",
    "Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine",
    "Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine",
    "Project template release checklist evidence",
    "Project-template readiness checklist handoff evidence",
    "Project-template readiness checklist packaged audit evidence"
)) {
    Assert-ContainsText -Text $releaseBodyScript -ExpectedText $marker `
        -Message "Release note body generator should consume project-template readiness checklist compact evidence."
}

foreach ($scriptText in @($releaseBundleVersionTest)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "Project-template readiness checklist handoff evidence" `
        -Message "Release entry tests should lock the checklist handoff evidence label."
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Release entry tests should expose checklist handoff source-report evidence."
    Assert-ContainsText -Text $scriptText -ExpectedText "required_entrypoint_count=3" `
        -Message "Release entry tests should lock checklist handoff required entrypoint count."
    Assert-ContainsText -Text $scriptText -ExpectedText "entrypoint_paths=" `
        -Message "Release entry tests should lock checklist handoff entrypoint path display trace."
    Assert-ContainsText -Text $scriptText -ExpectedText "path_display=.\output\release-candidate-checks\START_HERE.md" `
        -Message "Release entry tests should lock START_HERE path_display evidence."
    Assert-ContainsText -Text $scriptText -ExpectedText "source_report=.\output\release-candidate-checks\summary.json" `
        -Message "Release entry tests should lock checklist handoff evidence to the release-candidate summary source."
    Assert-ContainsText -Text $scriptText -ExpectedText "source_schema=featherdoc.release_candidate_summary" `
        -Message "Release entry tests should lock checklist compact evidence to the release-candidate summary schema."
    Assert-ContainsText -Text $scriptText -ExpectedText "Project-template readiness checklist packaged audit evidence" `
        -Message "Release entry tests should lock the packaged checklist audit evidence label."
    Assert-ContainsText -Text $scriptText -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports" `
        -Message "Release entry tests should expose packaged checklist material-safety audit source-report evidence."
    Assert-ContainsText -Text $scriptText -ExpectedText "compact_evidence_source_schema=featherdoc.release_candidate_summary" `
        -Message "Release entry tests should lock packaged checklist compact evidence source schema."
    Assert-ContainsText -Text $scriptText -ExpectedText "source_report=.\output\release-blocker-rollup\summary.json" `
        -Message "Release entry tests should lock packaged audit evidence to the release-blocker rollup source."
}

Assert-ContainsText -Text $releaseChecksScript -ExpectedText "Project-template release entry evidence" `
    -Message "Release candidate final_review.md should expose a dedicated project-template release entry evidence section."
Assert-ContainsText -Text $releaseChecksScript -ExpectedText "projectTemplateChecklistEvidenceMarkdown" `
    -Message "Release candidate final_review.md should render compact project-template checklist evidence from governance handoff."

foreach ($marker in @(
    "ReleaseGovernanceHandoffIncludeRollup",
    "Project-template release entry evidence",
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_report_count -ne 2",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count -ne 2",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
)) {
    Assert-ContainsText -Text $releaseCandidateVisualTest -ExpectedText $marker `
        -Message "Release candidate visual verdict test should lock final_review.md project-template source-report evidence consumption."
}

Assert-ContainsText -Text $materialSafetyScript -ExpectedText "Missing project_template_readiness_checklist_entrypoints." `
    -Message "Material safety audit should fail packaged manifests missing the project-template readiness checklist entrypoints contract."
Assert-ContainsText -Text $materialSafetyScript -ExpectedText "Missing release_entry_project_template_readiness_checklist_material_safety_audit." `
    -Message "Material safety audit should fail packaged manifests missing the release-entry checklist material-safety audit contract."

foreach ($marker in @(
    "Assert-StagedProjectTemplateChecklistHandoffEvidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit",
    "compact_evidence_source_schema",
    "source_schema=featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
)) {
    Assert-ContainsText -Text $packageAssetsScript -ExpectedText $marker `
        -Message "Package release assets should require and record release-entry project-template checklist material-safety evidence."
}

foreach ($marker in @(
    "Add-ReleaseBlockerRollupManifestSignoffEntrypointsTraceViolations",
    "Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistEntrypointsTraceViolations",
    "Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations",
    "Add-ReleaseGovernanceHandoffManifestSignoffEntrypointsTraceViolations",
    "Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistEntrypointsTraceViolations",
    "Add-FinalReviewProjectTemplateReadinessChecklistEntrypointsTraceViolations",
    "Add-ReleaseMetadataProjectTemplateReadinessChecklistEntrypointsTraceViolations",
    "Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceTraceViolations",
    "Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations",
    "Add-FinalReviewProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations",
    "Add-ReleaseMetadataProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations",
    "Add-ReleaseEntryProjectTemplateReadinessChecklistEntrypointsEvidenceTraceViolations",
    "Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditContractViolations",
    "Project-template readiness checklist handoff evidence",
    "Project-template readiness checklist packaged audit evidence",
    "Project-template readiness checklist entrypoints evidence source reports",
    "Release-entry project-template readiness checklist material-safety audit source reports",
    "Manifest signoff entrypoints evidence source reports",
    "manifest_signoff_entrypoints_rollup_material_safety_trace",
    "manifest_signoff_entrypoints_handoff_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_rollup_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace",
    "Source Report Contracts",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema",
    "release-candidate-checks",
    "release-blocker-rollup",
    "source_schema=featherdoc.release_candidate_summary",
    "featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_checklist_path",
    "required_entrypoint_count=3",
    "entrypoint_paths=",
    "project_template_readiness_checklist_entrypoints_checklist_marker",
    "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
)) {
    Assert-ContainsText -Text $materialSafetyScript -ExpectedText $marker `
        -Message "Material safety audit should keep project-template readiness checklist source-report evidence block-scoped."
}

Assert-ContainsText -Text $cmakeLists -ExpectedText "project_template_release_readiness_checklist_docs_contract" `
    -Message "CTest should register the project-template readiness checklist contract."

foreach ($marker in @(
    "release_note_bundle_version",
    "release_note_bundle_version_test.ps1"
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CTest should keep the release bundle version contract wired for project-template readiness evidence."
}

Write-Host "Project template release readiness checklist docs contract passed."
