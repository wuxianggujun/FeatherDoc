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

function Get-TestCMakeRegistrationText {
    param(
        [string]$Root
    )

    $cmakePaths = @(
        (Join-Path $Root "test\CMakeLists.txt")
    )
    $cmakeModuleDir = Join-Path $Root "test\cmake"
    if (Test-Path -LiteralPath $cmakeModuleDir) {
        $cmakePaths += Get-ChildItem -LiteralPath $cmakeModuleDir -Filter "*.cmake" |
            Sort-Object Name |
            ForEach-Object { $_.FullName }
    }

    return ($cmakePaths | ForEach-Object {
            Get-Content -Raw -Encoding UTF8 -LiteralPath $_
        }) -join "`n"
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$schemaJson = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "samples\template_render_data_mapping.schema.json"
$sampleMappingJson = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "samples\chinese_invoice_template.render_data_mapping.json"
$cmakeLists = Get-TestCMakeRegistrationText -Root $resolvedRepoRoot

$convertScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\convert_render_data_to_patch_plan.ps1"
$lintScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\lint_render_data_mapping.ps1"
$mappingDraftScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\export_render_data_mapping_draft.ps1"
$skeletonScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\export_render_data_skeleton.ps1"
$validateScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\validate_render_data_mapping.ps1"
$workspacePrepScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\prepare_template_render_data_workspace.ps1"
$workspaceRenderScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\render_template_document_from_workspace.ps1"
$dataRenderScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\render_template_document_from_data.ps1"

$convertTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\convert_render_data_to_patch_plan_test.ps1"
$lintTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\lint_render_data_mapping_test.ps1"
$mappingDraftTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\export_render_data_mapping_draft_test.ps1"
$skeletonTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\export_render_data_skeleton_test.ps1"
$validateTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\validate_render_data_mapping_test.ps1"
$workspacePrepTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\prepare_template_render_data_workspace_test.ps1"
$workspaceRenderTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\render_template_document_from_workspace_test.ps1"
$dataRenderTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\render_template_document_from_data_test.ps1"

foreach ($marker in @(
        "scripts/convert_render_data_to_patch_plan.ps1",
        "samples/template_render_data_mapping.schema.json",
        "scripts/export_render_data_mapping_draft.ps1",
        "scripts/prepare_template_render_data_workspace.ps1",
        "scripts/validate_render_data_mapping.ps1",
        "scripts/render_template_document_from_workspace.ps1",
        "scripts/render_template_document_from_data.ps1",
        "TODO: <bookmark_name>",
        "output_patch_path",
        "remaining_placeholder_count=0",
        "resolved-section-targets",
        "export_target_mode"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve render-data mapping route marker '$marker'."
}

foreach ($marker in @(
        "scripts/convert_render_data_to_patch_plan.ps1",
        "samples/template_render_data_mapping.schema.json",
        "scripts/export_render_data_mapping_draft.ps1",
        "scripts/lint_render_data_mapping.ps1",
        "scripts/validate_render_data_mapping.ps1",
        "scripts/render_template_document_from_data.ps1",
        "TODO: <bookmark_name>",
        "output_patch_path",
        "resolved-section-targets",
        "export_target_mode"
    )) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should preserve render-data mapping route marker '$marker'."
}

foreach ($marker in @(
        "template_render_data_mapping.schema.json",
        "bookmark_text",
        "bookmark_paragraphs",
        "bookmark_table_rows",
        "bookmark_block_visibility",
        "section-header",
        "section-footer",
        "source",
        "part",
        "index",
        "section",
        "kind",
        "required"
    )) {
    Assert-ContainsText -Text $schemaJson -ExpectedText $marker `
        -Message "Render-data mapping schema should preserve marker '$marker'."
}

foreach ($marker in @(
        "./template_render_data_mapping.schema.json",
        "bookmark_text",
        "bookmark_paragraphs",
        "bookmark_table_rows",
        "source",
        "customer.name",
        "invoice.number",
        "line_items"
    )) {
    Assert-ContainsText -Text $sampleMappingJson -ExpectedText $marker `
        -Message "Sample render-data mapping should preserve marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $convertScript
            label = "convert render-data mapping script"
            markers = @(
                "bookmark_text",
                "bookmark_paragraphs",
                "bookmark_table_rows",
                "bookmark_block_visibility",
                "output_patch_path",
                "part",
                "index",
                "section",
                "kind"
            )
        },
        [ordered]@{
            text = $lintScript
            label = "lint render-data mapping script"
            markers = @(
                "mapping_counts",
                "resolved_source_count",
                "invalid_source_count",
                "duplicate",
                "section-header",
                "section-footer",
                "bookmark_table_rows"
            )
        },
        [ordered]@{
            text = $mappingDraftScript
            label = "export render-data mapping draft script"
            markers = @(
                "template_render_data_mapping.schema.json",
                "source_root",
                "bookmark_text",
                "bookmark_paragraphs",
                "bookmark_table_rows",
                "bookmark_block_visibility",
                "part",
                "index",
                "section",
                "kind"
            )
        },
        [ordered]@{
            text = $skeletonScript
            label = "export render-data skeleton script"
            markers = @(
                "TODO:",
                "source_count",
                "bookmark_text",
                "bookmark_paragraphs",
                "bookmark_table_rows",
                "bookmark_block_visibility",
                "Mapping file does not contain any supported source entries"
            )
        },
        [ordered]@{
            text = $validateScript
            label = "validate render-data mapping script"
            markers = @(
                "WorkspaceDir",
                "RequireComplete",
                "remaining_placeholder_count",
                "export_target_mode",
                "resolved-section-targets",
                "convert_render_data_to_patch_plan.ps1",
                "status",
                "completed"
            )
        },
        [ordered]@{
            text = $workspacePrepScript
            label = "prepare template render-data workspace script"
            markers = @(
                "START_HERE.zh-CN.md",
                "export_render_data_mapping_draft.ps1",
                "export_render_data_skeleton.ps1",
                "lint_render_data_mapping.ps1",
                "validate_render_data_mapping.ps1",
                "render_template_document_from_workspace.ps1",
                "export_target_mode",
                "remaining_placeholder_count=0"
            )
        },
        [ordered]@{
            text = $workspaceRenderScript
            label = "render template document from workspace script"
            markers = @(
                "WorkspaceDir",
                "export_target_mode",
                "remaining_placeholder_count",
                "render_template_document_from_data.ps1",
                "status",
                "completed"
            )
        },
        [ordered]@{
            text = $dataRenderScript
            label = "render template document from data script"
            markers = @(
                "convert_render_data_to_patch_plan.ps1",
                "RequireComplete",
                "remaining_placeholder_count",
                "export_target_mode",
                "resolved-section-targets",
                "PatchPlanOutput",
                "DraftPlanOutput",
                "PatchedPlanOutput"
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
            text = $convertTest
            label = "convert render-data mapping regression"
            markers = @(
                "convert_render_data_to_patch_plan.ps1",
                "patch_counts",
                "bookmark_text",
                "bookmark_table_rows",
                "visibility.render_data_mapping.json"
            )
        },
        [ordered]@{
            text = $lintTest
            label = "lint render-data mapping regression"
            markers = @(
                "mapping_counts",
                "resolved_source_count",
                "invalid_source_count",
                "duplicate.render_data_mapping.json",
                "invalid-source.render_data_mapping.json"
            )
        },
        [ordered]@{
            text = $mappingDraftTest
            label = "export render-data mapping draft regression"
            markers = @(
                "source_root",
                "template_render_data_mapping\.schema\.json",
                "bookmark_text",
                "bookmark_table_rows",
                "section-footer",
                "section-header"
            )
        },
        [ordered]@{
            text = $skeletonTest
            label = "export render-data skeleton regression"
            markers = @(
                "TODO: customer_name",
                "source_count",
                "nested.render_data_mapping.json",
                "conflict.render_data_mapping.json",
                "invalid-selector.render_data_mapping.json"
            )
        },
        [ordered]@{
            text = $validateTest
            label = "validate render-data mapping regression"
            markers = @(
                "RequireComplete",
                "status",
                "completed",
                "remaining_placeholder_count",
                "resolved-section-targets",
                "export_target_mode"
            )
        },
        [ordered]@{
            text = $workspacePrepTest
            label = "prepare render-data workspace regression"
            markers = @(
                "start_here",
                "validate_render_data_mapping.ps1",
                "data_skeleton",
                "TODO: customer_name",
                "resolved-section-targets"
            )
        },
        [ordered]@{
            text = $workspaceRenderTest
            label = "render from workspace regression"
            markers = @(
                "WorkspaceDir",
                "RequireComplete",
                "remaining_placeholder_count",
                "loaded-parts",
                "resolved-section-targets"
            )
        },
        [ordered]@{
            text = $dataRenderTest
            label = "render from data regression"
            markers = @(
                "convert_render_data_to_patch_plan",
                "status",
                "completed",
                "TODO:",
                "resolved-section-targets"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "convert_render_data_to_patch_plan",
        "convert_render_data_to_patch_plan_test.ps1",
        "lint_render_data_mapping",
        "lint_render_data_mapping_test.ps1",
        "export_render_data_mapping_draft",
        "export_render_data_mapping_draft_test.ps1",
        "export_render_data_skeleton",
        "export_render_data_skeleton_test.ps1",
        "validate_render_data_mapping",
        "validate_render_data_mapping_test.ps1",
        "render_data_mapping_route_docs_contract",
        "render_data_mapping_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;template"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep render-data mapping route wired for marker '$marker'."
}

Write-Host "Render-data mapping route docs contract passed."
