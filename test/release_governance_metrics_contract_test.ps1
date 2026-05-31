param(
    [string]$RepoRoot,
    [string]$WorkingDir
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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
[void][System.IO.Path]::GetFullPath($WorkingDir)

$requiredMetrics = @(
    "real_corpus_confidence",
    "delivery_quality"
)
$reviewFocusText = @(
    "Governance Metric Review Focus",
    "Numbering real-corpus confidence",
    "Table/layout delivery quality"
)

$handoffScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
$rollupScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
$packageScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
$safetyAuditScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"

$handoffTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"
$rollupTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
$packageSafetyTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\package_release_assets_safety_test.ps1"
$packageAllowIncompleteTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\package_release_assets_allow_incomplete_test.ps1"
$safetyAuditTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\assert_release_material_safety_test.ps1"
$contentControlReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_content_control_data_binding_governance_report.ps1"
$numberingCatalogReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_numbering_catalog_governance_report.ps1"
$tableLayoutDeliveryReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_table_layout_delivery_governance_report.ps1"
$projectTemplateReadinessReportScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_project_template_delivery_readiness_report.ps1"
$acceptanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\document_governance_acceptance_zh.rst"

foreach ($text in @($reviewFocusText)) {
    Assert-ContainsText -Text $handoffScript -ExpectedText $text `
        -Message "Handoff Markdown generator should keep the governance metric review focus contract."
    Assert-ContainsText -Text $rollupScript -ExpectedText $text `
        -Message "Release blocker rollup Markdown generator should keep the governance metric review focus contract."
    Assert-ContainsText -Text $handoffTest -ExpectedText $text `
        -Message "Handoff regression test should lock the governance metric review focus contract."
    Assert-ContainsText -Text $rollupTest -ExpectedText $text `
        -Message "Release blocker rollup regression test should lock the governance metric review focus contract."
}

foreach ($metric in @($requiredMetrics)) {
    foreach ($pathText in @(
        [ordered]@{ name = "handoff script"; text = $handoffScript },
        [ordered]@{ name = "rollup script"; text = $rollupScript },
        [ordered]@{ name = "safety audit script"; text = $safetyAuditScript },
        [ordered]@{ name = "handoff test"; text = $handoffTest },
        [ordered]@{ name = "rollup test"; text = $rollupTest },
        [ordered]@{ name = "package safety test"; text = $packageSafetyTest },
        [ordered]@{ name = "package allow-incomplete test"; text = $packageAllowIncompleteTest },
        [ordered]@{ name = "safety audit test"; text = $safetyAuditTest }
    )) {
        Assert-ContainsText -Text $pathText.text -ExpectedText $metric `
            -Message "$($pathText.name) should keep required governance metric '$metric'."
    }
}

foreach ($text in @("governance_metric_count", "governance_metrics")) {
    foreach ($pathText in @(
        [ordered]@{ name = "handoff script"; text = $handoffScript },
        [ordered]@{ name = "rollup script"; text = $rollupScript },
        [ordered]@{ name = "package manifest script"; text = $packageScript },
        [ordered]@{ name = "safety audit script"; text = $safetyAuditScript },
        [ordered]@{ name = "package safety test"; text = $packageSafetyTest },
        [ordered]@{ name = "package allow-incomplete test"; text = $packageAllowIncompleteTest },
        [ordered]@{ name = "safety audit test"; text = $safetyAuditTest }
    )) {
        Assert-ContainsText -Text $pathText.text -ExpectedText $text `
            -Message "$($pathText.name) should keep '$text' in the release governance metric contract."
    }
}

foreach ($text in @("Missing governance_metrics.", "Missing governance_metric_count.", "governance_metric_count does not match governance_metrics length.")) {
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $text `
        -Message "Release material safety audit should enforce governance metric manifest contract."
}

foreach ($marker in @(
    "featherdoc.project_template_delivery_readiness_report.v1",
    "latest_schema_approval_gate_status",
    "source_json_display"
)) {
    Assert-ContainsText -Text $projectTemplateReadinessReportScript -ExpectedText $marker `
        -Message "Project-template readiness report should keep the template contract marker '$marker'."
}

foreach ($marker in @(
    "project_template_delivery_readiness_contract",
    "latest_schema_approval_gate_status",
    "source_json_display"
)) {
    Assert-ContainsText -Text $packageScript -ExpectedText $marker `
        -Message "Release asset packaging should keep the template contract marker '$marker'."
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $marker `
        -Message "Release material safety audit should keep the template contract marker '$marker'."
    Assert-ContainsText -Text $safetyAuditTest -ExpectedText $marker `
        -Message "Release material safety regression should lock the template contract marker '$marker'."
}

foreach ($marker in @(
    "featherdoc.project_template_delivery_readiness_report.v1",
    "project_template_delivery_readiness_contract",
    "latest_schema_approval_gate_status",
    "source_json_display"
)) {
    Assert-ContainsText -Text $packageSafetyTest -ExpectedText $marker `
        -Message "Release asset packaging regression should lock the template contract marker '$marker'."
    Assert-ContainsText -Text $packageAllowIncompleteTest -ExpectedText $marker `
        -Message "Release asset allow-incomplete regression should lock the template contract marker '$marker'."
    Assert-ContainsText -Text $safetyAuditTest -ExpectedText $marker `
        -Message "Release material safety regression should lock the template contract marker '$marker'."
}

foreach ($marker in @(
    "content_control_data_binding.bound_placeholder",
    "sync_bound_content_control",
    "repair_strategy",
    "repair_hint",
    "command_template",
    "sync-content-controls-from-custom-xml",
    "source_json_display"
)) {
    Assert-ContainsText -Text $contentControlReportScript -ExpectedText $marker `
        -Message "Content-control governance report should keep the repair workflow marker '$marker'."
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $marker `
        -Message "Release material safety audit should keep the content-control repair marker '$marker'."
    Assert-ContainsText -Text $safetyAuditTest -ExpectedText $marker `
        -Message "Release material safety regression should lock the content-control repair marker '$marker'."
}

foreach ($marker in @(
    "matched_document_count",
    "unmatched_catalog_document_count",
    "unmatched_baseline_document_count",
    "alignment_gap_count",
    "catalog_document_keys",
    "baseline_document_keys",
    "matched_document_keys"
)) {
    Assert-ContainsText -Text $numberingCatalogReportScript -ExpectedText $marker `
        -Message "Numbering catalog governance report should keep the real-corpus alignment marker '$marker'."
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $marker `
        -Message "Release material safety audit should keep the numbering real-corpus alignment marker '$marker'."
    Assert-ContainsText -Text $safetyAuditTest -ExpectedText $marker `
        -Message "Release material safety regression should lock the numbering real-corpus alignment marker '$marker'."
}

Assert-ContainsText -Text $numberingCatalogReportScript -ExpectedText "numbering_catalog_governance.real_corpus_alignment_gap" `
    -Message "Numbering catalog governance report should keep the real-corpus alignment blocker marker."

foreach ($marker in @(
    "table_style_issue_count",
    "automatic_tblLook_fix_count",
    "manual_table_style_fix_count",
    "table_position_automatic_count",
    "table_position_review_count",
    "pdf_floating_table_supported_geometry_percent",
    "command_failure_count",
    "floating_table_plans_pending"
)) {
    Assert-ContainsText -Text $tableLayoutDeliveryReportScript -ExpectedText $marker `
        -Message "Table layout delivery governance report should keep the delivery quality marker '$marker'."
}

foreach ($marker in @(
    "table_layout_delivery_governance.delivery_quality",
    "table_style_issue_count",
    "automatic_tblLook_fix_count",
    "manual_table_style_fix_count",
    "table_position_automatic_count",
    "table_position_review_count",
    "command_failure_count",
    "penalty_summary"
)) {
    Assert-ContainsText -Text $handoffScript -ExpectedText $marker `
        -Message "Release governance handoff should keep the table delivery marker '$marker'."
    Assert-ContainsText -Text $rollupScript -ExpectedText $marker `
        -Message "Release blocker rollup should keep the table delivery marker '$marker'."
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $marker `
        -Message "Release material safety audit should keep the table delivery marker '$marker'."
    Assert-ContainsText -Text $safetyAuditTest -ExpectedText $marker `
        -Message "Release material safety regression should lock the table delivery marker '$marker'."
}

foreach ($marker in @("floating_table_plans_pending")) {
    Assert-ContainsText -Text $safetyAuditScript -ExpectedText $marker `
        -Message "Release material safety audit should keep the table delivery compatibility marker '$marker'."
    Assert-ContainsText -Text $rollupTest -ExpectedText $marker `
        -Message "Release blocker rollup regression should lock the table delivery compatibility marker '$marker'."
    Assert-ContainsText -Text $packageSafetyTest -ExpectedText $marker `
        -Message "Release asset packaging regression should lock the table delivery compatibility marker '$marker'."
    Assert-ContainsText -Text $packageAllowIncompleteTest -ExpectedText $marker `
        -Message "Release asset allow-incomplete regression should lock the table delivery compatibility marker '$marker'."
}

foreach ($marker in @(
    "document_governance_acceptance.v1",
    "document_governance_primary_track",
    "pdf_conservative_maintenance",
    "no_pdf_rendering_in_low_resource_stage",
    "long_task_document_governance_closure"
)) {
    Assert-ContainsText -Text $acceptanceDoc -ExpectedText $marker `
        -Message "Document governance acceptance note should keep the PDF-conservative planning marker '$marker'."
}

Write-Host "Release governance metrics contract regression passed."
