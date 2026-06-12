param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Resolve-RepoFile {
    param([string]$Root, [string]$RelativePath)

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected parser-check file was not found: $RelativePath"
    }

    return (Resolve-Path -LiteralPath $path).Path
}

function Test-PowerShellSyntax {
    param([string]$Path, [string]$Label)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if (@($parseErrors).Count -gt 0) {
        $errorText = (@($parseErrors) | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine
        throw "PowerShell parser check failed for $Label. $errorText"
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
if (-not [string]::IsNullOrWhiteSpace($WorkingDir)) {
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetFullPath($WorkingDir)) -Force | Out-Null
}

$releaseGovernanceFiles = @(
    "scripts\release_blocker_metadata_helpers.ps1",
    "scripts\build_release_blocker_rollup_report.ps1",
    "scripts\build_release_governance_handoff_report.ps1",
    "scripts\build_release_governance_handoff_report_core.ps1",
    "scripts\build_release_governance_handoff_report_governance_metrics.ps1",
    "scripts\build_release_governance_handoff_report_input_contracts.ps1",
    "scripts\build_release_governance_handoff_report_normalization.ps1",
    "scripts\build_release_governance_handoff_report_report_markdown.ps1",
    "scripts\build_release_governance_handoff_report_rollup_evidence.ps1",
    "scripts\build_release_governance_pipeline_report.ps1",
    "scripts\write_release_note_bundle.ps1",
    "scripts\write_release_metadata_start_here.ps1",
    "scripts\write_release_artifact_guide.ps1",
    "scripts\write_release_reviewer_checklist.ps1",
    "scripts\assert_release_material_safety.ps1",
    "scripts\write_schema_patch_confidence_calibration_report.ps1",
    "scripts\check_release_metadata_docs.ps1",
    "scripts\check_script_task_index.ps1",
    "scripts\check_word_visual_release_gate_preflight.ps1",
    "scripts\run_word_visual_release_gate.ps1",
    "scripts\run_word_visual_release_gate_helpers.ps1",
    "scripts\run_word_visual_release_gate_setup.ps1",
    "scripts\run_word_visual_release_gate_descriptors.ps1",
    "scripts\run_word_visual_release_gate_standard_flows.ps1",
    "scripts\run_word_visual_release_gate_curated_report.ps1",
    "scripts\check_docx_functional_smoke_readiness.ps1",
    "test\check_release_metadata_docs_test.ps1",
    "test\release_governance_warning_helper_contract_test.ps1",
    "test\build_release_blocker_rollup_report_test.ps1",
    "test\build_release_governance_handoff_report_test.ps1",
    "test\build_release_governance_pipeline_report_test.ps1",
    "test\write_schema_patch_confidence_calibration_report_test.ps1"
)

$documentGovernanceFiles = @(
    "scripts\build_content_control_data_binding_governance_report.ps1",
    "scripts\build_project_template_delivery_readiness_report.ps1",
    "scripts\build_table_layout_delivery_governance_report.ps1",
    "scripts\build_numbering_catalog_governance_report.ps1",
    "scripts\build_document_skeleton_governance_rollup_report.ps1",
    "scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1",
    "test\build_content_control_data_binding_governance_report_test.ps1",
    "test\build_project_template_delivery_readiness_report_test.ps1",
    "test\build_table_layout_delivery_governance_report_test.ps1",
    "test\build_numbering_catalog_governance_report_test.ps1",
    "test\build_document_skeleton_governance_rollup_report_test.ps1",
    "test\pdf_visual_release_gate_preflight_governance_report_test.ps1"
)

$filesToParse = @(
    $releaseGovernanceFiles
    $documentGovernanceFiles
) | Select-Object -Unique

Assert-True -Condition (@($filesToParse).Count -ge 20) `
    -Message "Lightweight parser checklist should cover release and document governance entrypoints."

foreach ($relativePath in @($filesToParse)) {
    $path = Resolve-RepoFile -Root $resolvedRepoRoot -RelativePath $relativePath
    Test-PowerShellSyntax -Path $path -Label $relativePath
}

Write-Host "Document API lightweight parser check passed for $(@($filesToParse).Count) file(s)."
