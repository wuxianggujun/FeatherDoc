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
$materialSafetyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
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
    "START_HERE.md",
    "ARTIFACT_GUIDE.md",
    "REVIEWER_CHECKLIST.md",
    "block_scoped_entry_trace",
    "release_handoff.md",
    "release_body.zh-CN.md",
    "release_summary.zh-CN.md",
    "release_governance_handoff.md",
    "release_assets_manifest.json"
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
    "block_scoped_governance_handoff_trace",
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
    "block_scoped_entry_content_control_trace",
    "block_scoped_entry_governance_metric_trace",
    "block_scoped_entry_project_template_trace",
    "block_scoped_release_bundle_project_template_trace",
    "manifest_scoped_project_template_trace",
    "line_scoped_release_note_project_template_trace",
    "block_scoped_release_handoff_trace",
    "single_block_release_handoff_project_template_trace",
    "block_scoped_governance_handoff_trace",
    "single_block_governance_handoff_project_template_trace",
    "block_scoped_final_review_project_template_trace",
    "single_block_final_review_project_template_trace",
    "onboarding governance block"
)) {
    Assert-ContainsText -Text $checklistDoc -ExpectedText $marker `
        -Message "Project-template checklist should document block-scoped governance handoff safety marker '$marker'."
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
