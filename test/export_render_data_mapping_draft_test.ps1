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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\export_render_data_mapping_draft.ps1"
$samplePlanPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_plan.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$sampleOutput = Join-Path $resolvedWorkingDir "invoice.render_data_mapping.draft.json"
$sampleSummary = Join-Path $resolvedWorkingDir "invoice.render_data_mapping.draft.summary.json"

& $scriptPath `
    -InputPlan $samplePlanPath `
    -OutputMapping $sampleOutput `
    -SummaryJson $sampleSummary `
    -SourceRoot "data"

if ($LASTEXITCODE -ne 0) {
    throw "export_render_data_mapping_draft.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $sampleOutput) `
    -Message "Invoice mapping draft was not created."
Assert-True -Condition (Test-Path -LiteralPath $sampleSummary) `
    -Message "Invoice mapping draft summary was not created."

$sampleMapping = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleOutput | ConvertFrom-Json
$sampleSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleSummary | ConvertFrom-Json

Assert-Equal -Actual $sampleSummaryObject.status -Expected "completed" `
    -Message "Invoice mapping draft summary did not report status=completed."
Assert-Equal -Actual $sampleSummaryObject.source_root -Expected "data" `
    -Message "Invoice mapping draft summary did not preserve source_root."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_text -Expected 3 `
    -Message "Invoice mapping draft summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_paragraphs -Expected 1 `
    -Message "Invoice mapping draft summary reported an unexpected bookmark_paragraphs count."
Assert-Equal -Actual $sampleSummaryObject.mapping_counts.bookmark_table_rows -Expected 1 `
    -Message "Invoice mapping draft summary reported an unexpected bookmark_table_rows count."
Assert-Equal -Actual $sampleSummaryObject.inferred_table_column_count -Expected 3 `
    -Message "Invoice mapping draft should infer three table columns."
Assert-True -Condition ([string]$sampleMapping.'$schema' -match 'template_render_data_mapping\.schema\.json$') `
    -Message "Invoice mapping draft did not reference the mapping schema."

$customerEntry = @($sampleMapping.bookmark_text | Where-Object { $_.bookmark_name -eq "customer_name" })[0]
$tableEntry = @($sampleMapping.bookmark_table_rows | Where-Object { $_.bookmark_name -eq "line_item_row" })[0]
$notesEntry = @($sampleMapping.bookmark_paragraphs | Where-Object { $_.bookmark_name -eq "note_lines" })[0]

Assert-Equal -Actual $customerEntry.source -Expected "data.customer_name" `
    -Message "customer_name source path was not generated from the bookmark name."
Assert-Equal -Actual $notesEntry.source -Expected "data.note_lines" `
    -Message "note_lines source path was not generated from the bookmark name."
Assert-Equal -Actual $tableEntry.source -Expected "data.line_item_row" `
    -Message "line_item_row source path was not generated from the bookmark name."
Assert-Equal -Actual @($tableEntry.columns).Count -Expected 3 `
    -Message "line_item_row should include three inferred column paths."
Assert-Equal -Actual $tableEntry.columns[0] -Expected "column_1" `
    -Message "line_item_row first inferred column path is incorrect."
Assert-Equal -Actual $tableEntry.columns[2] -Expected "column_3" `
    -Message "line_item_row third inferred column path is incorrect."

$selectorPlan = Join-Path $resolvedWorkingDir "selector.render-plan.json"
$selectorOutput = Join-Path $resolvedWorkingDir "selector.render_data_mapping.draft.json"
$selectorSummary = Join-Path $resolvedWorkingDir "selector.render_data_mapping.draft.summary.json"

Set-Content -LiteralPath $selectorPlan -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "header_title",
      "part": "header",
      "index": 1,
      "text": "TODO: header_title"
    }
  ],
  "bookmark_table_rows": [
    {
      "bookmark_name": "empty_table",
      "part": "section-footer",
      "section": 2,
      "kind": "first",
      "rows": []
    }
  ],
  "bookmark_block_visibility": [
    {
      "bookmark_name": "optional_block",
      "part": "section-header",
      "section": 0,
      "kind": "even",
      "visible": true
    }
  ]
}
'@

& $scriptPath `
    -InputPlan $selectorPlan `
    -OutputMapping $selectorOutput `
    -SummaryJson $selectorSummary

if ($LASTEXITCODE -ne 0) {
    throw "export_render_data_mapping_draft.ps1 failed for selector fixture."
}

$selectorMapping = Get-Content -Raw -Encoding UTF8 -LiteralPath $selectorOutput | ConvertFrom-Json
$selectorSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $selectorSummary | ConvertFrom-Json

Assert-Equal -Actual $selectorSummaryObject.status -Expected "completed" `
    -Message "Selector mapping draft summary did not report status=completed."
Assert-Equal -Actual $selectorSummaryObject.mapping_counts.bookmark_text -Expected 1 `
    -Message "Selector mapping draft summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $selectorSummaryObject.mapping_counts.bookmark_table_rows -Expected 1 `
    -Message "Selector mapping draft summary reported an unexpected bookmark_table_rows count."
Assert-Equal -Actual $selectorSummaryObject.mapping_counts.bookmark_block_visibility -Expected 1 `
    -Message "Selector mapping draft summary reported an unexpected visibility count."
Assert-Equal -Actual $selectorSummaryObject.inferred_table_column_count -Expected 0 `
    -Message "Empty table-row draft should not infer columns."

$headerEntry = $selectorMapping.bookmark_text[0]
$emptyTableEntry = $selectorMapping.bookmark_table_rows[0]
$visibilityEntry = $selectorMapping.bookmark_block_visibility[0]

Assert-Equal -Actual $headerEntry.part -Expected "header" `
    -Message "Header selector part was not preserved."
Assert-Equal -Actual $headerEntry.index -Expected 1 `
    -Message "Header selector index was not preserved."
Assert-Equal -Actual $headerEntry.source -Expected "header_title" `
    -Message "Header source path was not generated from the bookmark name."
Assert-Equal -Actual $emptyTableEntry.part -Expected "section-footer" `
    -Message "Section footer selector part was not preserved."
Assert-Equal -Actual $emptyTableEntry.section -Expected 2 `
    -Message "Section footer selector section was not preserved."
Assert-Equal -Actual $emptyTableEntry.kind -Expected "first" `
    -Message "Section footer selector kind was not preserved."
Assert-True -Condition ($null -eq $emptyTableEntry.PSObject.Properties["columns"]) `
    -Message "Empty table-row draft should not include columns."
Assert-Equal -Actual $visibilityEntry.part -Expected "section-header" `
    -Message "Visibility selector part was not preserved."
Assert-Equal -Actual $visibilityEntry.section -Expected 0 `
    -Message "Visibility selector section was not preserved."
Assert-Equal -Actual $visibilityEntry.kind -Expected "even" `
    -Message "Visibility selector kind was not preserved."

Write-Host "Render-data mapping draft export regression passed."
