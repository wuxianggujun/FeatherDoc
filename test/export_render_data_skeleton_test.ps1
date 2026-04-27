param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\export_render_data_skeleton.ps1"
$lintScriptPath = Join-Path $resolvedRepoRoot "scripts\lint_render_data_mapping.ps1"
$convertScriptPath = Join-Path $resolvedRepoRoot "scripts\convert_render_data_to_patch_plan.ps1"
$sampleMappingPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data_mapping.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$sampleOutput = Join-Path $resolvedWorkingDir "invoice.render_data.skeleton.json"
$sampleSummary = Join-Path $resolvedWorkingDir "invoice.render_data.skeleton.summary.json"
$sampleLintSummary = Join-Path $resolvedWorkingDir "invoice.render_data.skeleton.lint.summary.json"
$samplePatch = Join-Path $resolvedWorkingDir "invoice.render_data.skeleton.patch.json"
$samplePatchSummary = Join-Path $resolvedWorkingDir "invoice.render_data.skeleton.patch.summary.json"

& $scriptPath `
    -MappingPath $sampleMappingPath `
    -OutputData $sampleOutput `
    -SummaryJson $sampleSummary

if ($LASTEXITCODE -ne 0) {
    throw "export_render_data_skeleton.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $sampleOutput) `
    -Message "Invoice render-data skeleton was not created."
Assert-True -Condition (Test-Path -LiteralPath $sampleSummary) `
    -Message "Invoice render-data skeleton summary was not created."

$sampleData = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleOutput | ConvertFrom-Json
$sampleSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleSummary | ConvertFrom-Json

Assert-Equal -Actual $sampleSummaryObject.status -Expected "completed" `
    -Message "Invoice render-data skeleton summary did not report status=completed."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_text -Expected 3 `
    -Message "Invoice render-data skeleton summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_paragraphs -Expected 1 `
    -Message "Invoice render-data skeleton summary reported an unexpected bookmark_paragraphs count."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_table_rows -Expected 1 `
    -Message "Invoice render-data skeleton summary reported an unexpected bookmark_table_rows count."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_block_visibility -Expected 0 `
    -Message "Invoice render-data skeleton summary reported an unexpected bookmark_block_visibility count."
Assert-Equal -Actual $sampleSummaryObject.source_count -Expected 5 `
    -Message "Invoice render-data skeleton summary reported an unexpected source_count."
Assert-Equal -Actual $sampleData.customer.name -Expected "TODO: customer_name" `
    -Message "Invoice render-data skeleton did not populate customer.name."
Assert-Equal -Actual $sampleData.invoice.number -Expected "TODO: invoice_number" `
    -Message "Invoice render-data skeleton did not populate invoice.number."
Assert-Equal -Actual $sampleData.invoice.issue_date -Expected "TODO: issue_date" `
    -Message "Invoice render-data skeleton did not populate invoice.issue_date."
Assert-Equal -Actual @($sampleData.notes).Count -Expected 1 `
    -Message "Invoice render-data skeleton should create one note paragraph by default."
Assert-Equal -Actual @($sampleData.notes)[0] -Expected "TODO: note_lines" `
    -Message "Invoice render-data skeleton did not populate notes."
Assert-Equal -Actual @($sampleData.line_items).Count -Expected 1 `
    -Message "Invoice render-data skeleton should create one table row by default."
Assert-Equal -Actual @($sampleData.line_items)[0].title -Expected "TODO: line_item_row.row[0].title" `
    -Message "Invoice render-data skeleton did not populate line_items[0].title."
Assert-Equal -Actual @($sampleData.line_items)[0].description -Expected "TODO: line_item_row.row[0].description" `
    -Message "Invoice render-data skeleton did not populate line_items[0].description."
Assert-Equal -Actual @($sampleData.line_items)[0].amount -Expected "TODO: line_item_row.row[0].amount" `
    -Message "Invoice render-data skeleton did not populate line_items[0].amount."

& $lintScriptPath `
    -MappingPath $sampleMappingPath `
    -DataPath $sampleOutput `
    -SummaryJson $sampleLintSummary

if ($LASTEXITCODE -ne 0) {
    throw "Generated invoice render-data skeleton should pass lint_render_data_mapping.ps1."
}

& $convertScriptPath `
    -DataPath $sampleOutput `
    -MappingPath $sampleMappingPath `
    -OutputPatch $samplePatch `
    -SummaryJson $samplePatchSummary

if ($LASTEXITCODE -ne 0) {
    throw "Generated invoice render-data skeleton should convert to a render patch."
}

$samplePatchObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePatch | ConvertFrom-Json
$samplePatchSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePatchSummary | ConvertFrom-Json
Assert-Equal -Actual $samplePatchSummaryObject.output_patch_path -Expected $samplePatch `
    -Message "Convert summary should expose output_patch_path for automation."
Assert-Equal -Actual $samplePatchObject.bookmark_paragraphs[0].paragraphs[0] `
    -Expected "TODO: note_lines" `
    -Message "Invoice render-data skeleton patch did not preserve the note placeholder."
Assert-Equal -Actual $samplePatchObject.bookmark_table_rows[0].rows[0][0] `
    -Expected "TODO: line_item_row.row[0].title" `
    -Message "Invoice render-data skeleton patch did not preserve the table title placeholder."

$nestedMappingPath = Join-Path $resolvedWorkingDir "nested.render_data_mapping.json"
$nestedOutput = Join-Path $resolvedWorkingDir "nested.render_data.skeleton.json"
$nestedSummary = Join-Path $resolvedWorkingDir "nested.render_data.skeleton.summary.json"
$nestedLintSummary = Join-Path $resolvedWorkingDir "nested.render_data.skeleton.lint.summary.json"

Set-Content -LiteralPath $nestedMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "section_title",
      "source": "sections[0].title"
    }
  ],
  "bookmark_paragraphs": [
    {
      "bookmark_name": "section_notes",
      "source": "sections[0].notes"
    }
  ],
  "bookmark_table_rows": [
    {
      "bookmark_name": "section_line_items",
      "source": "sections[0].line_items",
      "columns": [
        "label",
        "pricing.amount"
      ]
    }
  ],
  "bookmark_block_visibility": [
    {
      "bookmark_name": "section_show_block",
      "source": "sections[0].show_block"
    }
  ]
}
'@

& $scriptPath `
    -MappingPath $nestedMappingPath `
    -OutputData $nestedOutput `
    -SummaryJson $nestedSummary `
    -DefaultParagraphCount 2 `
    -DefaultTableRowCount 2

if ($LASTEXITCODE -ne 0) {
    throw "export_render_data_skeleton.ps1 failed for the nested mapping sample."
}

$nestedData = Get-Content -Raw -Encoding UTF8 -LiteralPath $nestedOutput | ConvertFrom-Json
$nestedSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $nestedSummary | ConvertFrom-Json

Assert-Equal -Actual $nestedSummaryObject.status -Expected "completed" `
    -Message "Nested render-data skeleton summary did not report status=completed."
Assert-Equal -Actual $nestedSummaryObject.source_count -Expected 4 `
    -Message "Nested render-data skeleton summary reported an unexpected source_count."
Assert-Equal -Actual @($nestedData.sections).Count -Expected 1 `
    -Message "Nested render-data skeleton should create one section array element."
Assert-Equal -Actual @($nestedData.sections)[0].title -Expected "TODO: section_title" `
    -Message "Nested render-data skeleton did not populate sections[0].title."
Assert-Equal -Actual @(@($nestedData.sections)[0].notes).Count -Expected 2 `
    -Message "Nested render-data skeleton should create two note paragraphs."
Assert-Equal -Actual @(@($nestedData.sections)[0].notes)[1] -Expected "TODO: section_notes[1]" `
    -Message "Nested render-data skeleton did not populate sections[0].notes[1]."
Assert-Equal -Actual @(@($nestedData.sections)[0].line_items).Count -Expected 2 `
    -Message "Nested render-data skeleton should create two line-item rows."
Assert-Equal -Actual @(@($nestedData.sections)[0].line_items)[0].label -Expected "TODO: section_line_items.row[0].label" `
    -Message "Nested render-data skeleton did not populate sections[0].line_items[0].label."
Assert-Equal -Actual @(@($nestedData.sections)[0].line_items)[0].pricing.amount -Expected "TODO: section_line_items.row[0].pricing.amount" `
    -Message "Nested render-data skeleton did not populate nested table column paths."
Assert-True -Condition ([bool]@($nestedData.sections)[0].show_block) `
    -Message "Nested render-data skeleton should default block visibility to true."

& $lintScriptPath `
    -MappingPath $nestedMappingPath `
    -DataPath $nestedOutput `
    -SummaryJson $nestedLintSummary

if ($LASTEXITCODE -ne 0) {
    throw "Generated nested render-data skeleton should pass lint_render_data_mapping.ps1."
}

$conflictMappingPath = Join-Path $resolvedWorkingDir "conflict.render_data_mapping.json"
$conflictSummary = Join-Path $resolvedWorkingDir "conflict.render_data.skeleton.summary.json"

Set-Content -LiteralPath $conflictMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "customer_root",
      "source": "customer"
    },
    {
      "bookmark_name": "customer_name",
      "source": "customer.name"
    }
  ]
}
'@

$conflictFailed = $false
try {
    & $scriptPath `
        -MappingPath $conflictMappingPath `
        -SummaryJson $conflictSummary

    if ($LASTEXITCODE -ne 0) {
        $conflictFailed = $true
    }
} catch {
    $conflictFailed = $true
}

if (-not $conflictFailed) {
    throw "export_render_data_skeleton.ps1 should fail when source paths conflict."
}

$conflictSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $conflictSummary | ConvertFrom-Json
Assert-Equal -Actual $conflictSummaryObject.status -Expected "failed" `
    -Message "Conflict render-data skeleton summary did not report status=failed."
Assert-Equal -Actual $conflictSummaryObject.conflict_count -Expected 1 `
    -Message "Conflict render-data skeleton summary reported an unexpected conflict_count."
Assert-ContainsText -Text ([string]$conflictSummaryObject.error) `
    -ExpectedText "conflicts" `
    -Label "Conflict render-data skeleton error"

$invalidSelectorMappingPath = Join-Path $resolvedWorkingDir "invalid-selector.render_data_mapping.json"
$invalidSelectorPatch = Join-Path $resolvedWorkingDir "invalid-selector.render_patch.json"
$invalidSelectorSummary = Join-Path $resolvedWorkingDir "invalid-selector.render_patch.summary.json"

Set-Content -LiteralPath $invalidSelectorMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "customer_name",
      "part": "header",
      "index": -1,
      "source": "customer.name"
    }
  ]
}
'@

$invalidSelectorFailed = $false
try {
    & $convertScriptPath `
        -DataPath $sampleOutput `
        -MappingPath $invalidSelectorMappingPath `
        -OutputPatch $invalidSelectorPatch `
        -SummaryJson $invalidSelectorSummary

    if ($LASTEXITCODE -ne 0) {
        $invalidSelectorFailed = $true
    }
} catch {
    $invalidSelectorFailed = $true
}

if (-not $invalidSelectorFailed) {
    throw "convert_render_data_to_patch_plan.ps1 should fail when selector indexes are negative."
}

$invalidSelectorSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $invalidSelectorSummary | ConvertFrom-Json
Assert-Equal -Actual $invalidSelectorSummaryObject.status -Expected "failed" `
    -Message "Invalid selector convert summary did not report status=failed."
Assert-Equal -Actual $invalidSelectorSummaryObject.output_patch_path -Expected $invalidSelectorPatch `
    -Message "Invalid selector convert summary should still expose output_patch_path."
Assert-ContainsText -Text ([string]$invalidSelectorSummaryObject.error) `
    -ExpectedText "non-negative integer" `
    -Label "Invalid selector convert error"

Write-Host "Render-data skeleton export regression passed."
