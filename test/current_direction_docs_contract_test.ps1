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

$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$currentDirectionDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\current_direction_zh.rst"
$maintenanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\documentation_maintenance_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        "project_identity_zh",
        "current_direction_zh",
        "document_governance_acceptance_zh",
        "docx_functional_smoke_readiness_zh",
        "documentation_maintenance_zh",
        "automation/word_visual_workflow_zh",
        "libreoffice_pdf/index_zh"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Documentation index should preserve current product direction and maintenance entry '$marker'."
}

foreach ($marker in @(
        "WordprocessingML",
        "project_identity_zh",
        "v1_7_roadmap_zh",
        "open()",
        "create_empty()",
        "typed API",
        "Microsoft Word",
        "visual validation",
        "template_schema_patch",
        "project-template onboarding governance",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "schema-patch-confidence-calibration/summary.json",
        "release blocker rollup",
        "content-control data-binding governance",
        "check_docx_functional_smoke_readiness.ps1",
        "run_project_template_smoke.ps1",
        "numbering catalog",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "build_table_layout_delivery_rollup_report.ps1",
        "featherdoc.table_layout_delivery_rollup_report.v1",
        "OMML"
    )) {
    Assert-ContainsText -Text $currentDirectionDoc -ExpectedText $marker `
        -Message "Current direction document should preserve product-scope marker '$marker'."
}

foreach ($marker in @(
        "docs/current_direction_zh.rst",
        "docs/document_governance_acceptance_zh.rst",
        "docs/release_metadata_pipeline_zh.rst",
        "docs/release_metadata_maintenance_checklist_zh.rst",
        "docs/automation/word_visual_workflow_zh.rst",
        "BUILDING_PDF.md",
        "design/04-pdf-execution-plan.md",
        "docs/stale_codex_branch_inventory_zh.rst",
        "PDF CJK",
        "git status --short --branch",
        "git rev-list --left-right --count dev...origin/dev",
        "git diff --check",
        "PowerShell",
        "60",
        "CMake",
        "Ninja",
        "MSBuild",
        "CTest",
        "Word",
        "LibreOffice",
        "PDF visual gate",
        "run_word_visual_release_gate.ps1",
        "origin/dev"
    )) {
    Assert-ContainsText -Text $maintenanceDoc -ExpectedText $marker `
        -Message "Documentation maintenance guide should preserve low-resource governance marker '$marker'."
}

foreach ($marker in @(
        "current_direction_docs_contract",
        "current_direction_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep current direction docs contract wired."
}

Write-Host "Current direction docs contract passed."
