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

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
    }
}

function Assert-RepoFileMissing {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (Test-Path -LiteralPath $path) {
        throw "Legacy current-direction file should have been removed: $RelativePath"
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
$nextTasksDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\next_tasks_zh.rst"
$longTaskBoardDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\long_task_board_zh.rst"
$maintenanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\documentation_maintenance_zh.rst"
$scriptTaskIndexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\script_task_index_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

foreach ($marker in @(
        "Choose a language entry point",
        "English documentation",
        "FDOC_DOCS_ROOT_ZH_CN_DOCUMENTATION_LABEL",
        "English API reference",
        "FDOC_DOCS_ROOT_ZH_CN_API_LABEL",
        "en/index",
        "zh-CN/index",
        "en/api/index",
        "zh-CN/api/index"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Documentation index should preserve bilingual public entry '$marker'."
}

foreach ($marker in @(
        "project_identity_zh",
        "current_direction_zh",
        "document_governance_acceptance_zh",
        "docx_functional_smoke_readiness_zh",
        "documentation_maintenance_zh",
        "automation/word_visual_workflow_zh",
        "libreoffice_pdf/index_zh"
    )) {
    Assert-DoesNotContainText -Text $indexDoc -UnexpectedText $marker `
        -Message "Documentation index should not expose legacy root-level governance entry '$marker'."
}

Assert-RepoFileMissing -Root $resolvedRepoRoot -RelativePath "docs\libreoffice_pdf\index_zh.rst"

foreach ($marker in @(
        "WordprocessingML",
        "project_identity_zh",
        "next_tasks_zh",
        "long_task_board_zh",
        "v1_7_roadmap_zh",
        "open()",
        "create_empty()",
        "typed API",
        "Microsoft Word",
        "visual validation",
        "template_schema_patch",
        "project-template onboarding governance",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "featherdoc.project_template_workflow_dashboard.v1",
        "project_template_workflow_dashboard_report",
        "schema-patch-confidence-calibration/summary.json",
        "release blocker rollup",
        "content-control data-binding governance",
        "check_docx_functional_smoke_readiness.ps1",
        "run_project_template_smoke.ps1",
        "numbering catalog",
        "featherdoc.document_skeleton_governance_rollup_report.v1",
        "build_table_layout_delivery_rollup_report.ps1",
        "featherdoc.table_layout_delivery_rollup_report.v1",
        "current_direction_guardrails",
        "template_contract_project_template_workflow",
        "style_numbering_governance_workflow",
        "table_layout_delivery_workflow",
        "current_non_priority_guardrails",
        "current_feature_admission_criteria",
        "current_maintenance_cadence",
        "docs/script_task_index_zh.rst",
        "reopen-save",
        "gate",
        "OMML"
    )) {
    Assert-ContainsText -Text $currentDirectionDoc -ExpectedText $marker `
        -Message "Current direction document should preserve product-scope marker '$marker'."
}

foreach ($marker in @(
        "docs/current_direction_zh.rst",
        "docs/next_tasks_zh.rst",
        "docs/long_task_board_zh.rst",
        "docs/document_governance_acceptance_zh.rst",
        "docs/release_metadata_pipeline_zh.rst",
        "docs/release_metadata_maintenance_checklist_zh.rst",
        "docs/automation/word_visual_workflow_zh.rst",
        "BUILDING_PDF.md",
        "design/04-pdf-execution-plan.md",
        "docs/stale_codex_branch_inventory_zh.rst",
        "docs/pdf_release_readiness_checklist_zh.rst",
        "FDOC_DOCS_LIBREOFFICE_PDF_LEGACY_REMOVED",
        "FDOC_DOCS_LIBREOFFICE_PDF_LEGACY_DO_NOT_RESTORE",
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
        "next_tasks_zh",
        "long_task_board_zh",
        "P1-SCHEMA-01",
        "P0",
        "P1",
        "P2",
        "P3",
        "dev",
        "codex/*",
        "Release governance",
        "DOCX smoke",
        "PDF",
        "project-template workflow dashboard",
        "next_action_summary",
        "release material safety",
        "schema approval",
        "release blocker rollup",
        "Word visual gate",
        "git diff --check"
    )) {
    Assert-ContainsText -Text $nextTasksDoc -ExpectedText $marker `
        -Message "Next tasks document should preserve actionable backlog marker '$marker'."
}

foreach ($marker in @(
        "long_task_board_zh",
        "active goal",
        "dev",
        "codex/*",
        "P0-CI-01",
        "P1-SCHEMA-01",
        "P1-TEMPLATE-01",
        "P1-DASHBOARD-01",
        "P1-APPROVAL-01",
        "P1-CONTENT-01",
        "P1-RELEASE-01",
        "P2-STYLE-01",
        "P2-NUMBERING-01",
        "P2-TABLE-01",
        "P2-WORD-01",
        "P3-PDF-01",
        "P3-DOCS-01",
        "git status --short --branch",
        "git diff --check",
        "git push origin dev"
    )) {
    Assert-ContainsText -Text $longTaskBoardDoc -ExpectedText $marker `
        -Message "Long task board should preserve execution ledger marker '$marker'."
}

foreach ($marker in @(
        "template_contract_project_template_workflow",
        "style_numbering_governance_workflow",
        "table_layout_delivery_workflow",
        "build_release_governance_pipeline_report.ps1",
        "check_pdf_visual_release_gate_preflight.ps1",
        "schema_version",
        "TIMEOUT 60"
    )) {
    Assert-ContainsText -Text $scriptTaskIndexDoc -ExpectedText $marker `
        -Message "Script task index should preserve current-direction guardrail marker '$marker'."
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
