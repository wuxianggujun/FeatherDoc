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

$checklistDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\project_template_release_readiness_checklist_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$templateSchemaDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\template_schema_mutation_zh.rst"
$releasePipelineDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$releasePolicyDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_policy_zh.rst"
$deliveryScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_project_template_delivery_readiness_report.ps1"
$contentControlScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_content_control_data_binding_governance_report.ps1"
$releaseChecksScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_release_candidate_checks.ps1"
$releaseBlockerRollupScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
$releaseGovernanceHandoffScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
$releaseBlockerMetadataHelpersScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
$packageAssetsScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
$materialSafetyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
$startHereScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_metadata_start_here.ps1"
$artifactGuideScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_artifact_guide.ps1"
$reviewerChecklistScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_reviewer_checklist.ps1"
$releaseBundleVersionTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_note_bundle_version_test.ps1"
$releaseCandidateVisualTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_candidate_visual_verdict_test.ps1"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
    "project_template_release_readiness_checklist_zh",
    "template_schema_mutation_zh",
    "release_metadata_pipeline_zh"
)) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Docs index should expose the project-template release readiness entry."
}

foreach ($marker in @(
    "project-template governance",
    "onboard_project_template.ps1",
    "build_project_template_onboarding_governance_report.ps1",
    "run_project_template_smoke.ps1",
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
    "project_template_readiness_checklist_entrypoints_manifest_trace",
    "project_template_readiness_checklist_entrypoints_governance_trace",
    "project_template_readiness_checklist_entrypoints_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace",
    "manifest_signoff_entrypoints",
    "manifest_signoff_entrypoints_release_trace",
    "manifest_signoff_entrypoints_manifest_trace"
)) {
    Assert-ContainsText -Text $checklistDoc -ExpectedText $marker `
        -Message "Project-template release readiness checklist should preserve marker '$marker'."
}

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
    "project_template_readiness_checklist_entrypoints_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_release_entry_source_report_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_handoff_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_final_review_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_metadata_details_source_schema_identity_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_material_safety_trace",
    "project_template_readiness_checklist_entrypoints_packaged_audit_release_entry_source_report_identity_trace",
    "block_scoped_governance_handoff_trace",
    "block_scoped_governance_handoff_project_template_status_trace",
    "project_template_onboarding.schema_approval",
    "Markdown list block",
    "project-template-delivery-readiness",
    "project-template-onboarding-governance"
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
    "schema approval"
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
    "ReleaseGovernanceHandoffInputJson"
)) {
    Assert-ContainsText -Text $releaseChecksScript -ExpectedText $marker `
        -Message "Release candidate checks should keep governance handoff marker '$marker'."
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
    "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
)) {
    Assert-ContainsText -Text $releaseBlockerRollupScript -ExpectedText $marker `
        -Message "Release blocker rollup should expose packaged project-template checklist material-safety audit evidence."
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
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine" `
    -Message "Release entry flow should have a compact checklist packaged audit evidence line helper."
Assert-ContainsText -Text $releaseBlockerMetadataHelpersScript -ExpectedText "Project-template readiness checklist packaged audit evidence" `
    -Message "Release entry flow helper should render the packaged checklist audit evidence label."

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

foreach ($scriptText in @($releaseBundleVersionTest)) {
    Assert-ContainsText -Text $scriptText -ExpectedText "Project-template readiness checklist handoff evidence" `
        -Message "Release entry tests should lock the checklist handoff evidence label."
    Assert-ContainsText -Text $scriptText -ExpectedText "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Release entry tests should expose checklist handoff source-report evidence."
    Assert-ContainsText -Text $scriptText -ExpectedText "source_report=.\output\release-candidate-checks\summary.json" `
        -Message "Release entry tests should lock checklist handoff evidence to the release-candidate summary source."
    Assert-ContainsText -Text $scriptText -ExpectedText "Project-template readiness checklist packaged audit evidence" `
        -Message "Release entry tests should lock the packaged checklist audit evidence label."
    Assert-ContainsText -Text $scriptText -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports" `
        -Message "Release entry tests should expose packaged checklist material-safety audit source-report evidence."
    Assert-ContainsText -Text $scriptText -ExpectedText "source_report=.\output\release-blocker-rollup\summary.json" `
        -Message "Release entry tests should lock packaged audit evidence to the release-blocker rollup source."
}

Assert-ContainsText -Text $materialSafetyScript -ExpectedText "Missing project_template_readiness_checklist_entrypoints." `
    -Message "Material safety audit should fail packaged manifests missing the project-template readiness checklist entrypoints contract."
Assert-ContainsText -Text $materialSafetyScript -ExpectedText "Missing release_entry_project_template_readiness_checklist_material_safety_audit." `
    -Message "Material safety audit should fail packaged manifests missing the release-entry checklist material-safety audit contract."

foreach ($marker in @(
    "Assert-StagedProjectTemplateChecklistHandoffEvidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
)) {
    Assert-ContainsText -Text $packageAssetsScript -ExpectedText $marker `
        -Message "Package release assets should require and record release-entry project-template checklist material-safety evidence."
}

foreach ($marker in @(
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
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=",
    "release-candidate-checks",
    "release-blocker-rollup",
    "featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_checklist_path",
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
