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

function Assert-RepoPathExists {
    param(
        [string]$Root,
        [string]$RelativePath,
        [string]$Message
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "$Message MissingPath='$RelativePath'."
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
$readmeZh = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.zh-CN.md"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$maintenanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\documentation_maintenance_zh.rst"
$scoreDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\project_score_assessment_zh.rst"
$scriptIndexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\script_task_index_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        "script_task_index_zh",
        "documentation_maintenance_zh",
        "project_score_assessment_zh",
        "scripts/check_script_task_index.ps1",
        "output/script-task-index-check/summary.json",
        "output/script-task-index-check/script_task_index_check.md",
        "script_reference_group_count",
        "script_reference_extension_count",
        "duplicate_script_reference_count",
        "occurrence_lines",
        "occurrence_groups",
        "missing_marker_count",
        "powershell_version",
        "output_encoding",
        "UTF-8 without BOM"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Sphinx index should keep script task index and maintenance docs reachable."
}

foreach ($entrypoint in @(
        @{ Label = "English README"; Text = $readme },
        @{ Label = "Chinese README"; Text = $readmeZh }
    )) {
    foreach ($marker in @(
            "docs/documentation_maintenance_zh.rst",
            "docs/script_task_index_zh.rst",
            "scripts/check_script_task_index.ps1",
            "output/script-task-index-check/summary.json",
            "output/script-task-index-check/script_task_index_check.md",
            "script_reference_group_count",
            "script_reference_extension_count",
            "duplicate_script_reference_count",
            "occurrence_lines",
            "occurrence_groups",
            "missing_marker_count",
            "powershell_version",
            "output_encoding",
            "UTF-8 without BOM"
        )) {
        Assert-ContainsText -Text $entrypoint.Text -ExpectedText $marker `
            -Message "$($entrypoint.Label) should keep documentation maintenance entrypoint '$marker' reachable."
    }
}

foreach ($marker in @(
        "docs/script_task_index_zh.rst",
        "check_word_visual_release_gate_preflight.ps1"
    )) {
    Assert-ContainsText -Text $maintenanceDoc -ExpectedText $marker `
        -Message "Documentation maintenance guide should point to the script task index marker '$marker'."
}

foreach ($marker in @(
        "docs/script_task_index_zh.rst",
        "scripts"
    )) {
    Assert-ContainsText -Text $scoreDoc -ExpectedText $marker `
        -Message "Project score assessment should route script governance improvements to the script index."
}

foreach ($marker in @(
        "check_template_schema_baseline.ps1",
        "export_template_render_plan.ps1",
        "check_project_template_smoke_manifest.ps1",
        "-CheckPaths",
        "register_project_template_smoke_manifest_entry.ps1",
        "-ReplaceExisting",
        "sync_project_template_smoke_visual_verdict.ps1",
        "visual_review_undetermined_count",
        "review_verdict=fail",
        "describe_project_template_smoke_manifest.ps1",
        "featherdoc.project_template_smoke_manifest_description.v1",
        "discover_project_template_smoke_candidates.ps1",
        "-FailOnUnregistered",
        "unregistered_candidate_count",
        "check_docx_functional_smoke_readiness.ps1",
        "check_script_task_index.ps1",
        "summary.json",
        "script_task_index_check.md",
        "powershell_version",
        "output_encoding",
        "UTF-8 without BOM",
        "duplicate_script_reference_count",
        "occurrence_lines",
        "occurrence_groups",
        "script_reference_group_count",
        "script_reference_groups",
        "script_reference_extension_count",
        "script_reference_extensions",
        "build_numbering_catalog_governance_report.ps1",
        "build_table_layout_delivery_report.ps1",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "pdf_floating_table_metadata_only_fields",
        "pdf_floating_table_review_required_fields",
        "metadata_only_fields",
        "review_required_fields",
        "metadata-only tblpPr",
        "check_word_visual_release_gate_preflight.ps1",
        "build_release_governance_pipeline_report.ps1",
        "check_pdf_release_readiness.ps1",
        "TIMEOUT 60",
        "ReportMarkdown",
        "-SkipBuild",
        "schema_version"
    )) {
    Assert-ContainsText -Text $scriptIndexDoc -ExpectedText $marker `
        -Message "Script task index should preserve category or maintenance marker '$marker'."
}

$requiredScriptPaths = @(
    "scripts\check_template_schema_baseline.ps1",
    "scripts\check_script_task_index.ps1",
    "scripts\check_template_schema_manifest.ps1",
    "scripts\run_project_template_smoke.ps1",
    "scripts\check_project_template_smoke_manifest.ps1",
    "scripts\register_project_template_smoke_manifest_entry.ps1",
    "scripts\sync_project_template_smoke_visual_verdict.ps1",
    "scripts\describe_project_template_smoke_manifest.ps1",
    "scripts\discover_project_template_smoke_candidates.ps1",
    "scripts\new_project_template_smoke_onboarding_plan.ps1",
    "scripts\sync_project_template_schema_approval.ps1",
    "scripts\write_project_template_schema_approval_history.ps1",
    "scripts\build_project_template_onboarding_governance_report.ps1",
    "scripts\export_template_render_plan.ps1",
    "scripts\prepare_template_render_data_workspace.ps1",
    "scripts\render_template_document_from_workspace.ps1",
    "scripts\edit_document_from_plan.ps1",
    "scripts\build_content_control_data_binding_governance_report.ps1",
    "scripts\check_docx_functional_smoke_readiness.ps1",
    "scripts\check_numbering_catalog_baseline.ps1",
    "scripts\build_numbering_catalog_governance_report.ps1",
    "scripts\build_document_skeleton_governance_report.ps1",
    "scripts\build_document_skeleton_governance_rollup_report.ps1",
    "scripts\write_schema_patch_confidence_calibration_report.ps1",
    "scripts\write_style_merge_suggestion_review.ps1",
    "scripts\build_table_layout_delivery_report.ps1",
    "scripts\build_table_layout_delivery_rollup_report.ps1",
    "scripts\build_table_layout_delivery_governance_report.ps1",
    "scripts\run_word_visual_release_gate.ps1",
    "scripts\check_word_visual_release_gate_preflight.ps1",
    "scripts\sync_latest_visual_review_verdict.ps1",
    "scripts\run_release_candidate_checks.ps1",
    "scripts\build_release_blocker_rollup_report.ps1",
    "scripts\build_release_governance_pipeline_report.ps1",
    "scripts\build_release_governance_handoff_report.ps1",
    "scripts\check_release_metadata_docs.ps1",
    "scripts\assert_release_material_safety.ps1",
    "scripts\package_release_assets.ps1",
    "scripts\write_release_metadata_start_here.ps1",
    "scripts\write_release_body_zh.ps1",
    "scripts\check_pdf_dependency_inputs.ps1",
    "scripts\check_pdf_release_readiness.ps1",
    "scripts\check_pdf_visual_release_gate_preflight.ps1",
    "scripts\run_pdf_ctest_bounded_subset.ps1"
)

foreach ($relativePath in $requiredScriptPaths) {
    Assert-RepoPathExists -Root $resolvedRepoRoot -RelativePath $relativePath `
        -Message "Script task index should only list maintained scripts that exist."
    Assert-ContainsText -Text $scriptIndexDoc -ExpectedText ($relativePath -replace "\\", "/") `
        -Message "Script task index should mention maintained script '$relativePath'."
}

foreach ($marker in @(
        "script_task_index_docs_contract",
        "script_task_index_docs_contract_test.ps1",
        "check_project_template_smoke_manifest",
        "check_project_template_smoke_manifest_test.ps1",
        "register_project_template_smoke_manifest_entry",
        "register_project_template_smoke_manifest_entry_test.ps1",
        "sync_project_template_smoke_visual_verdict",
        "sync_project_template_smoke_visual_verdict_test.ps1",
        "sync_project_template_smoke_visual_verdict_failure",
        "sync_project_template_smoke_visual_verdict_failure_test.ps1",
        "describe_project_template_smoke_manifest",
        "describe_project_template_smoke_manifest_test.ps1",
        "new_project_template_smoke_onboarding_plan",
        "onboard_project_template",
        "discover_project_template_smoke_candidates_registered",
        "discover_project_template_smoke_candidates_fail_on_unregistered",
        "ReportMarkdown",
        "script_task_index_check.md",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;scripts"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep script task index docs contract wired."
}

Write-Host "Script task index docs contract passed."
