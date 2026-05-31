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
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

$deliveryReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_table_layout_delivery_report.ps1"
$rollupScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_table_layout_delivery_rollup_report.ps1"
$governanceScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_table_layout_delivery_governance_report.ps1"
$releaseSafetyScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
$pipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$handoffScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"

$deliveryReportTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_table_layout_delivery_report_test.ps1"
$deliveryRunTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\table_layout_delivery_report_test.ps1"
$rollupTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_table_layout_delivery_rollup_report_test.ps1"
$governanceTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_table_layout_delivery_governance_report_test.ps1"
$metricsContractTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_governance_metrics_contract_test.ps1"
$releaseSafetyTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"

foreach ($marker in @(
        "scripts/build_table_layout_delivery_report.ps1",
        "scripts/build_table_layout_delivery_rollup_report.ps1",
        "featherdoc.table_layout_delivery_rollup_report.v1",
        "scripts/build_table_layout_delivery_governance_report.ps1",
        "output/table-layout-delivery-governance/summary.json",
        "featherdoc.table_layout_delivery_governance_report.v1",
        "table_layout_delivery_governance.delivery_quality",
        "delivery_quality",
        "safe_tblLook_fixes_pending",
        "floating_table_plans_pending",
        "pdf_floating_table_support",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Sphinx index should preserve table-layout delivery route marker '$marker'."
}

foreach ($marker in @(
        "build_table_layout_delivery_governance_report.ps1",
        "featherdoc.table_layout_delivery_governance_report.v1",
        "table_layout_delivery_governance",
        "table-layout-delivery-governance",
        "tblLook",
        "floating table",
        "release blockers",
        "action items"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "README should preserve table-layout governance route marker '$marker'."
}

foreach ($marker in @(
        "build_table_layout_delivery_report.ps1",
        "build_table_layout_delivery_rollup_report.ps1",
        "table style quality",
        "tblLook",
        "floating table",
        "release blocker rollup"
    )) {
    Assert-ContainsText -Text $featureGapDoc -ExpectedText $marker `
        -Message "Feature gap document should preserve table-layout delivery marker '$marker'."
}

foreach ($marker in @(
        "featherdoc.table_layout_delivery_governance_report.v1",
        "delivery_quality",
        "content_control_data_binding_governance",
        "source_schema",
        "source_report_display",
        "source_json_display",
        "open_command",
        "table_layout_delivery_governance",
        "apply_safe_tblLook_fixes_then_visual_regression",
        "apply_safe_tblLook_fixes",
        "review_table_style_quality_plan",
        "review_manual_table_style_definition_work",
        "review_table_position_plan",
        "review_floating_table_position_plans",
        "dry_run_apply_table_position_plans",
        "run_table_style_quality_visual_regression",
        "review_table_layout_delivery_governance_sources",
        "rebuild_table_layout_delivery_rollup"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata document should preserve table-layout governance marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $deliveryReportScript
            label = "table layout delivery report script"
            markers = @(
                "featherdoc.table_layout_delivery_report.v1",
                "tblLook",
                "table_style_quality",
                "table_position_plan",
                "run_table_style_quality_visual_regression.ps1",
                "pdf_floating_table_support"
            )
        },
        [ordered]@{
            text = $rollupScript
            label = "table layout delivery rollup script"
            markers = @(
                "featherdoc.table_layout_delivery_rollup_report.v1",
                "release_blockers",
                "action_items",
                "table_position_plans",
                "pdf_floating_table_support",
                "pdf_floating_table_supported_geometry_percent"
            )
        },
        [ordered]@{
            text = $governanceScript
            label = "table layout delivery governance script"
            markers = @(
                "featherdoc.table_layout_delivery_governance_report.v1",
                "delivery_quality",
                "table_layout_delivery_governance.delivery_quality",
                "table_layout_delivery.safe_tblLook_fixes_pending",
                "table_layout_delivery.floating_table_review_pending",
                "apply_safe_tblLook_fixes",
                "review_table_position_plan",
                "dry_run_table_position_plan",
                "generate_table_layout_visual_evidence",
                "safe_tblLook_fixes_pending",
                "floating_table_plans_pending",
                "pdf_floating_table_support",
                "pdf_floating_table_supported_geometry_percent",
                "pdf_floating_table_tracked_geometry_count",
                "pdf_floating_table_layout_boundary",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command",
                "repair_strategy",
                "repair_hint",
                "command_template"
            )
        },
        [ordered]@{
            text = $releaseSafetyScript
            label = "release material safety audit"
            markers = @(
                "table_layout_delivery_governance.delivery_quality",
                "featherdoc.table_layout_delivery_governance_report.v1",
                "table_layout_delivery_quality",
                "automatic_tblLook_fix_count",
                "manual_table_style_fix_count",
                "table_position_automatic_count",
                "table_position_review_count",
                "floating_table_plans_pending"
            )
        },
        [ordered]@{
            text = $pipelineScript
            label = "release governance pipeline"
            markers = @(
                "table_layout_delivery_governance",
                "table-layout-delivery-governance",
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
                "table_layout_delivery_governance",
                "table-layout-delivery-governance",
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
            text = $deliveryReportTest
            label = "table layout delivery report regression"
            markers = @(
                "featherdoc.table_layout_delivery_report.v1",
                "tblLook",
                "pdf_floating_table_layout_boundary"
            )
        },
        [ordered]@{
            text = $deliveryRunTest
            label = "table layout delivery runner regression"
            markers = @(
                "Apply safe tblLook quality fixes",
                "Apply floating table position preset",
                "run_table_style_quality_visual_regression.ps1",
                "table_style_quality_automatic_fix_count"
            )
        },
        [ordered]@{
            text = $rollupTest
            label = "table layout delivery rollup regression"
            markers = @(
                "featherdoc.table_layout_delivery_rollup_report.v1",
                "release_blockers",
                "action_items",
                "table_position_plans",
                "pdf_floating_table_support",
                "pdf_floating_table_supported_geometry_percent"
            )
        },
        [ordered]@{
            text = $governanceTest
            label = "table layout delivery governance regression"
            markers = @(
                "featherdoc.table_layout_delivery_governance_report.v1",
                "delivery_quality",
                "table_layout_delivery.safe_tblLook_fixes_pending",
                "table_layout_delivery.floating_table_review_pending",
                "safe_tblLook_fixes_pending",
                "floating_table_plans_pending",
                "pdf_floating_table_support",
                "pdf_floating_table_supported_geometry_percent",
                "stable_pdf_geometry_subset_not_full_word_wrapping",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command",
                "repair_strategy",
                "repair_hint",
                "command_template"
            )
        },
        [ordered]@{
            text = $metricsContractTest
            label = "release governance metrics contract"
            markers = @(
                "delivery_quality",
                "table_layout_delivery_governance.delivery_quality",
                "floating_table_plans_pending",
                "source_json_display"
            )
        },
        [ordered]@{
            text = $releaseSafetyTest
            label = "release material safety regression"
            markers = @(
                "table_layout_delivery_governance.delivery_quality",
                'source_schema = "featherdoc.table_layout_delivery_governance_report.v1"',
                "automatic_tblLook_fix_count",
                "manual_table_style_fix_count",
                "table_position_automatic_count",
                "table_position_review_count",
                "floating_table_plans_pending"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "table_layout_delivery_route_docs_contract",
        "table_layout_delivery_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;table-layout"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep table-layout delivery route contract wired."
}

Write-Host "Table layout delivery route docs contract passed."
