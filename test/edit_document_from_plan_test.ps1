param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir,
    [ValidateSet("all", "core", "core_invoice", "core_content", "core_fields_style", "core_fields", "core_style_page")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\edit_document_from_plan_test"
}

if (-not $WorkingDir) {
    $WorkingDir = $BuildDir
}

New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

. (Join-Path $PSScriptRoot "edit_document_from_plan_test_assertions.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_content_fixtures.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_style_fixtures.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_review_section_fixtures.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $PSScriptRoot "edit_document_from_plan_test_invoice_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_content_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_field_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_style_page_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_section_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_style_metadata_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_table_workflow_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_table_structure_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_table_property_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_review_cases.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_test_link_revision_cases.ps1")
