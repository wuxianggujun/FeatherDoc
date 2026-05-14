param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Assert-DispatchesOperation {
    param(
        [string]$ScriptText,
        [string]$Operation
    )

    $pattern = '(?m)^\s+"' + [regex]::Escape($Operation) + '"\s*\{'
    if ($ScriptText -notmatch $pattern) {
        throw "edit_document_from_plan.ps1 does not dispatch '$Operation'."
    }
}

function Assert-TestUsesOperation {
    param(
        [string]$TestText,
        [string]$Operation
    )

    $pattern = '"op"\s*:\s*"' + [regex]::Escape($Operation) + '"'
    if ($TestText -notmatch $pattern) {
        throw "edit_document_from_plan_test.ps1 does not exercise '$Operation'."
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}

$scriptPath = Join-Path $RepoRoot "scripts\edit_document_from_plan.ps1"
$testPath = Join-Path $RepoRoot "test\edit_document_from_plan_test.ps1"
$tablePositionPlanTestPath = Join-Path $RepoRoot "test\edit_document_from_plan_table_position_plan_test.ps1"
$numberingCatalogTestPath = Join-Path $RepoRoot "test\edit_document_from_plan_numbering_catalog_test.ps1"
$contentControlSyncTestPath = Join-Path $RepoRoot "test\edit_document_from_plan_content_control_sync_test.ps1"
$readmePath = Join-Path $RepoRoot "README.md"
$readmeZhPath = Join-Path $RepoRoot "README.zh-CN.md"
$docsPath = Join-Path $RepoRoot "docs\index.rst"

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
$testText = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $testPath
    Get-Content -Raw -Encoding UTF8 -LiteralPath $tablePositionPlanTestPath
    Get-Content -Raw -Encoding UTF8 -LiteralPath $numberingCatalogTestPath
    Get-Content -Raw -Encoding UTF8 -LiteralPath $contentControlSyncTestPath
) -join "`n"
$readmeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmePath
$readmeZhText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmeZhPath
$docsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $docsPath

$advancedOperations = @(
    "accept_all_revisions",
    "reject_all_revisions",
    "set_comment_resolved",
    "set_comment_metadata",
    "append_comment",
    "append_comment_reply",
    "replace_comment",
    "remove_comment",
    "apply_review_mutation_plan",
    "append_footnote",
    "replace_footnote",
    "remove_footnote",
    "append_endnote",
    "replace_endnote",
    "remove_endnote",
    "apply_table_position_plan",
    "import_numbering_catalog",
    "replace_content_control_text",
    "replace_content_control_paragraphs",
    "replace_content_control_table",
    "replace_content_control_table_rows",
    "replace_content_control_image",
    "set_content_control_form_state",
    "sync_content_controls_from_custom_xml",
    "append_image",
    "replace_image",
    "remove_image",
    "append_page_number_field",
    "append_total_pages_field",
    "append_table_of_contents_field",
    "append_document_property_field",
    "append_date_field",
    "set_update_fields_on_open",
    "append_hyperlink",
    "replace_hyperlink",
    "remove_hyperlink",
    "append_omml",
    "replace_omml",
    "remove_omml",
    "insert_paragraph_text_revision",
    "delete_paragraph_text_revision",
    "replace_paragraph_text_revision",
    "insert_text_range_revision",
    "delete_text_range_revision",
    "replace_text_range_revision",
    "append_paragraph_text_comment",
    "append_text_range_comment",
    "set_paragraph_text_comment_range",
    "set_text_range_comment_range"
)

foreach ($operation in $advancedOperations) {
    Assert-DispatchesOperation -ScriptText $scriptText -Operation $operation
    Assert-TestUsesOperation -TestText $testText -Operation $operation
    Assert-ContainsText -Text $readmeText -ExpectedText $operation -Label "README.md"
    Assert-ContainsText -Text $readmeZhText -ExpectedText $operation -Label "README.zh-CN.md"
}

$docsRequiredTerms = @(
    "apply-review-mutation-plan",
    "apply-table-position-plan",
    "append-hyperlink",
    "append-omml",
    "append-page-number-field",
    "append-image"
)

foreach ($term in $docsRequiredTerms) {
    Assert-ContainsText -Text $docsText -ExpectedText $term -Label "docs/index.rst"
}

Write-Host "Edit-plan advanced operation coverage passed."
