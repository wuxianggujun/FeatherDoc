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

function Get-CMakeAddTestBlock {
    param(
        [string]$CMakeLists,
        [string]$TestName
    )

    $escapedTestName = [regex]::Escape($TestName)
    $testMatch = [regex]::Match(
        $CMakeLists,
        "(?ms)^[ \t]*add_test\(\s*NAME\s+$escapedTestName(?:\s|$)"
    )

    if (-not $testMatch.Success) {
        throw "CMake test registration is missing add_test block for '$TestName'."
    }

    $remaining = $CMakeLists.Substring($testMatch.Index + $testMatch.Length)
    $nextTestMatch = [regex]::Match($remaining, "(?m)^[ \t]*add_test\(")
    if ($nextTestMatch.Success) {
        return $CMakeLists.Substring($testMatch.Index, $testMatch.Length + $nextTestMatch.Index)
    }

    return $CMakeLists.Substring($testMatch.Index)
}

function Assert-CMakeScenarioRegistration {
    param(
        [string]$CMakeLists,
        [string]$TestName,
        [string]$ScriptName,
        [string]$Scenario
    )

    $testBlock = Get-CMakeAddTestBlock -CMakeLists $CMakeLists -TestName $TestName
    foreach ($marker in @(
            $ScriptName,
            "-RepoRoot",
            "-WorkingDir"
        )) {
        Assert-ContainsText -Text $testBlock -ExpectedText $marker `
            -Message "CMake table-layout test '$TestName' should preserve marker '$marker'."
    }

    $scenarioPattern = "(?ms)-Scenario\s+$([regex]::Escape($Scenario))(?:\s|$)"
    if ($testBlock -notmatch $scenarioPattern) {
        throw "CMake table-layout test '$TestName' should pass scenario '$Scenario'."
    }
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
        "pdf_floating_table_supported_geometry_percent",
        "pdf_floating_table_tracked_geometry_count",
        "pdf_floating_table_reviewer_focus",
        "pdf_floating_table_metadata_only_fields",
        "pdf_floating_table_review_required_fields",
        "metadata-only",
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
        "pdf_floating_table_support",
        "pdf_floating_table_supported_geometry_percent",
        "pdf_floating_table_reviewer_focus",
        "metadata-only",
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
        "stable_pdf_geometry_subset_not_full_word_wrapping",
        "metadata-only tblpPr",
        "review_required_fields",
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
        "pdf_floating_table_supported_geometry_percent",
        "pdf_floating_table_tracked_geometry_count",
        "pdf_floating_table_reviewer_focus",
        "pdf_floating_table_metadata_only_fields",
        "pdf_floating_table_review_required_fields",
        "metadata-only",
        "tblpPr",
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
                "pdf_floating_table_support",
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus"
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
                "pdf_floating_table_supported_geometry_percent",
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus",
                "pdf_floating_table_metadata_only_fields",
                "pdf_floating_table_review_required_fields"
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
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus",
                "pdf_floating_table_metadata_only_fields",
                "pdf_floating_table_review_required_fields",
                "metadata-only tblpPr",
                "pdf_floating_table_layout_boundary",
                "source_schema",
                "source_report_display",
                "source_json_display",
                "open_command",
                "repair_strategy",
                "repair_hint",
                "command_template",
                "FailOnIssue"
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
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus",
                "metadata-only tblpPr",
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
                "open_command",
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus",
                "pdf_floating_table_metadata_only_fields",
                "pdf_floating_table_review_required_fields"
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
                "pdf_floating_table_supported_geometry_percent",
                "pdf_floating_table_metadata_only_fields",
                "pdf_floating_table_review_required_fields"
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
                "pdf_floating_table_metadata_only_fields",
                "pdf_floating_table_review_required_fields",
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
                "pdf_floating_table_support_coverage",
                "pdf_floating_table_reviewer_focus",
                "metadata-only tblpPr",
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
        "build_table_layout_delivery_governance_report_fail_on_issue",
        "fail_on_issue",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;table-layout"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep table-layout delivery route contract wired."
}

foreach ($registration in @(
        [ordered]@{
            name = "build_table_layout_delivery_report_passing"
            script = "build_table_layout_delivery_report_test.ps1"
            scenario = "passing"
        },
        [ordered]@{
            name = "build_table_layout_delivery_report_failing"
            script = "build_table_layout_delivery_report_test.ps1"
            scenario = "failing"
        },
        [ordered]@{
            name = "build_table_layout_delivery_report_fail_on_issue"
            script = "build_table_layout_delivery_report_test.ps1"
            scenario = "fail_on_issue"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_aggregate"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "aggregate"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_empty"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "empty"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_malformed"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "malformed"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_failed_source_report"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "failed_source_report"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_fail_on_issue"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "fail_on_issue"
        },
        [ordered]@{
            name = "build_table_layout_delivery_rollup_report_fail_on_blocker"
            script = "build_table_layout_delivery_rollup_report_test.ps1"
            scenario = "fail_on_blocker"
        },
        [ordered]@{
            name = "build_table_layout_delivery_governance_report_aggregate"
            script = "build_table_layout_delivery_governance_report_test.ps1"
            scenario = "aggregate"
        },
        [ordered]@{
            name = "build_table_layout_delivery_governance_report_ready"
            script = "build_table_layout_delivery_governance_report_test.ps1"
            scenario = "ready"
        },
        [ordered]@{
            name = "build_table_layout_delivery_governance_report_malformed"
            script = "build_table_layout_delivery_governance_report_test.ps1"
            scenario = "malformed"
        },
        [ordered]@{
            name = "build_table_layout_delivery_governance_report_fail_on_issue"
            script = "build_table_layout_delivery_governance_report_test.ps1"
            scenario = "fail_on_issue"
        },
        [ordered]@{
            name = "build_table_layout_delivery_governance_report_fail_on_blocker"
            script = "build_table_layout_delivery_governance_report_test.ps1"
            scenario = "fail_on_blocker"
        }
    )) {
    Assert-CMakeScenarioRegistration `
        -CMakeLists $cmakeLists `
        -TestName ([string]$registration.name) `
        -ScriptName ([string]$registration.script) `
        -Scenario ([string]$registration.scenario)
}

Write-Host "Table layout delivery route docs contract passed."
