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

$statusDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\document_api_mainline_status_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        "document_api_mainline_status.v1",
        "document_api_status_current_dev",
        "codex_branch_reference_only",
        "pdf_cjk_branch_deferred",
        "document_api_lightweight_validation_checklist",
        "git diff --check",
        "PowerShell parser",
        "test/document_api_lightweight_parser_check_test.ps1",
        "test/check_release_metadata_docs_test.ps1",
        "test/release_governance_warning_helper_contract_test.ps1",
        "test/build_release_blocker_rollup_report_test.ps1 -Scenario passing",
        "test/build_release_governance_handoff_report_test.ps1 -Scenario aggregate",
        "test/build_release_governance_pipeline_report_test.ps1 -Scenario aggregate",
        "test/write_schema_patch_confidence_calibration_report_test.ps1 -Scenario aggregate",
        "test/build_content_control_data_binding_governance_report_test.ps1",
        "test/build_project_template_delivery_readiness_report_test.ps1",
        "test/build_table_layout_delivery_governance_report_test.ps1",
        "Word",
        "LibreOffice",
        "CMake",
        "CTest",
        "Ninja",
        "MSBuild",
        "PDF",
        "visual validation",
        "origin/dev"
    )) {
    Assert-ContainsText -Text $statusDoc -ExpectedText $marker `
        -Message "Document API mainline status should preserve lightweight validation marker '$marker'."
}

foreach ($marker in @(
        "document_api_mainline_status_docs_contract",
        "document_api_mainline_status_docs_contract_test.ps1",
        "document_api_lightweight_parser_check",
        "document_api_lightweight_parser_check_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep document API mainline status docs contract wired."
}

Write-Host "Document API mainline status docs contract passed."
