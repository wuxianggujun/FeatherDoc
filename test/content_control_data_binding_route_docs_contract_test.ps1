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

$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$featureGapDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\feature_gap_analysis_zh.rst"
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$documentApiStatusDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\document_api_mainline_status_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

$governanceScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_content_control_data_binding_governance_report.ps1"
$releaseSafetyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
$pipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$handoffScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"

$governanceTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_content_control_data_binding_governance_report_test.ps1"
$releaseSafetyTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"
$detailRollupTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_governance_detail_rollup_static_contract_test.ps1"
$metricsContractTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_governance_metrics_contract_test.ps1"

foreach ($marker in @(
        "inspect-content-controls",
        "set-content-control-form-state",
        "sync_content_controls_from_custom_xml()",
        "sync-content-controls-from-custom-xml",
        "content_control_summary",
        "dataBinding",
        "content_control_tag",
        "content_control_alias"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Sphinx index should preserve content-control route marker '$marker'."
}

foreach ($marker in @(
        "build_content_control_data_binding_governance_report.ps1",
        "output/content-control-data-binding-governance/summary.json",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "content_control_data_binding.custom_xml_sync_issue",
        "content_control_data_binding.bound_placeholder",
        "sync_bound_content_control",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "input_docx",
        "template_name",
        "schema_target",
        "target_mode"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Sphinx index should preserve content-control governance route marker '$marker'."
}

foreach ($marker in @(
        "content-control data-binding governance",
        "output/content-control-data-binding-governance/summary.json",
        "ReleaseBlockerRollupAutoDiscover",
        "ReleaseBlockerRollupInputRoot"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "README should preserve content-control governance release route marker '$marker'."
}

foreach ($marker in @(
        "build_content_control_data_binding_governance_report.ps1",
        "inspect-content-controls",
        "sync-content-controls-from-custom-xml",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "source_schema",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve content-control governance marker '$marker'."
}

foreach ($marker in @(
        "content-control data-binding governance",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "content_control_data_binding_governance",
        "content_control_data_binding.bound_placeholder",
        "sync_bound_content_control",
        "fix_custom_xml_data_binding_source",
        "sync_or_fill_bound_content_control",
        "review_content_control_lock_strategy",
        "review_unbound_form_content_control",
        "review_duplicate_content_control_binding",
        "collect_content_control_data_binding_evidence",
        "review_content_control_data_binding_evidence",
        "run_content_control_custom_xml_sync",
        "source_report_display",
        "source_json_display",
        "input_docx",
        "template_name",
        "schema_target",
        "target_mode",
        "release_governance_handoff"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata document should preserve content-control governance marker '$marker'."
}

foreach ($marker in @(
        "sync_content_controls_from_custom_xml",
        "build_content_control_data_binding_governance_report.ps1",
        "build_content_control_data_binding_governance_report_test.ps1"
    )) {
    Assert-ContainsText -Text $documentApiStatusDoc -ExpectedText $marker `
        -Message "Document API status should preserve content-control route entry '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $governanceScript
            label = "content-control data-binding governance script"
            markers = @(
                "featherdoc.content_control_data_binding_governance_report.v1",
                "featherdoc.content_control_data_binding_repair_plan.v1",
                "content_control_data_binding.custom_xml_sync_issue",
                "content_control_data_binding.bound_placeholder",
                "sync_bound_content_control",
                "fix_custom_xml_source",
                "review_lock_state",
                "bind_or_exempt_form_control",
                "deduplicate_or_confirm_shared_binding",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "input_docx",
                "template_name",
                "schema_target",
                "target_mode",
                "open_command",
                "command_template",
                "sync-content-controls-from-custom-xml",
                "inspect-content-controls"
            )
        },
        [ordered]@{
            text = $releaseSafetyScript
            label = "release material safety audit"
            markers = @(
                "featherdoc.content_control_data_binding_governance_report.v1",
                "content_control_data_binding.bound_placeholder",
                "sync_bound_content_control",
                "source_json_display",
                "input_docx",
                "template_name",
                "schema_target",
                "target_mode",
                "command_template"
            )
        },
        [ordered]@{
            text = $pipelineScript
            label = "release governance pipeline"
            markers = @(
                "content_control_data_binding_governance",
                "content-control-data-binding-governance",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        },
        [ordered]@{
            text = $handoffScript
            label = "release governance handoff"
            markers = @(
                "content_control_data_binding_governance",
                "content-control-data-binding-governance",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($assertion in @(
        [ordered]@{
            text = $governanceTest
            label = "content-control governance regression"
            markers = @(
                "featherdoc.content_control_data_binding_governance_report.v1",
                "featherdoc.content_control_data_binding_repair_plan.v1",
                "content_control_data_binding.custom_xml_sync_issue",
                "content_control_data_binding.bound_placeholder",
                "sync_bound_content_control",
                "custom_xml_sync_evidence_missing",
                "content_control_binding_evidence_missing",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "input_docx",
                "template_name",
                "schema_target",
                "target_mode",
                "open_command",
                "command_template",
                "requires_visual_verification"
            )
        },
        [ordered]@{
            text = $releaseSafetyTest
            label = "release material safety regression"
            markers = @(
                "content_control_data_binding.bound_placeholder",
                "source_schema=featherdoc.content_control_data_binding_governance_report.v1",
                "source_json_display",
                "repair_strategy=sync_bound_content_control",
                "input_docx",
                "template_name",
                "schema_target",
                "target_mode"
            )
        },
        [ordered]@{
            text = $detailRollupTest
            label = "release governance detail rollup contract"
            markers = @(
                "featherdoc.content_control_data_binding_governance_report.v1",
                "content-control-data-binding",
                "source_schema",
                "source_json_display"
            )
        },
        [ordered]@{
            text = $metricsContractTest
            label = "release governance metrics contract"
            markers = @(
                "content_control_data_binding.bound_placeholder",
                "sync_bound_content_control",
                "repair_strategy",
                "sync-content-controls-from-custom-xml"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "content_control_data_binding_route_docs_contract",
        "content_control_data_binding_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;content-control"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep content-control data-binding route contract wired."
}

Write-Host "Content-control data-binding route docs contract passed."
