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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\lint_render_data_mapping.ps1"
$sampleDataPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data.json"
$sampleMappingPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data_mapping.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$validSummary = Join-Path $resolvedWorkingDir "invoice.mapping.lint.summary.json"

& $scriptPath `
    -MappingPath $sampleMappingPath `
    -DataPath $sampleDataPath `
    -SummaryJson $validSummary

if ($LASTEXITCODE -ne 0) {
    throw "lint_render_data_mapping.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $validSummary) `
    -Message "Invoice mapping lint summary JSON was not created."

$validSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $validSummary | ConvertFrom-Json
Assert-Equal -Actual $validSummaryObject.status -Expected "completed" `
    -Message "Invoice mapping lint summary did not report status=completed."
Assert-True -Condition ([bool]$validSummaryObject.data_validation_performed) `
    -Message "Invoice mapping lint summary did not record data_validation_performed=true."
Assert-Equal -Actual $validSummaryObject.mapping_counts.bookmark_text -Expected 3 `
    -Message "Invoice mapping lint summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $validSummaryObject.mapping_counts.bookmark_paragraphs -Expected 1 `
    -Message "Invoice mapping lint summary reported an unexpected bookmark_paragraphs count."
Assert-Equal -Actual $validSummaryObject.mapping_counts.bookmark_table_rows -Expected 1 `
    -Message "Invoice mapping lint summary reported an unexpected bookmark_table_rows count."
Assert-Equal -Actual $validSummaryObject.mapping_counts.bookmark_block_visibility -Expected 0 `
    -Message "Invoice mapping lint summary reported an unexpected bookmark_block_visibility count."
Assert-Equal -Actual $validSummaryObject.resolved_source_count -Expected 5 `
    -Message "Invoice mapping lint summary reported an unexpected resolved_source_count."
Assert-Equal -Actual $validSummaryObject.invalid_source_count -Expected 0 `
    -Message "Invoice mapping lint summary reported an unexpected invalid_source_count."
Assert-Equal -Actual $validSummaryObject.duplicate_target_count -Expected 0 `
    -Message "Invoice mapping lint summary reported an unexpected duplicate_target_count."
Assert-Equal -Actual $validSummaryObject.entries.Count -Expected 5 `
    -Message "Invoice mapping lint summary reported an unexpected entry count."

$tableEntry = @($validSummaryObject.entries | Where-Object { $_.category -eq "bookmark_table_rows" })[0]
Assert-Equal -Actual $tableEntry.resolved_type -Expected "array" `
    -Message "Invoice table-row mapping should resolve to type=array."
Assert-Equal -Actual $tableEntry.column_reports[0].status -Expected "completed" `
    -Message "Invoice table-row column report should complete."

$duplicateMappingPath = Join-Path $resolvedWorkingDir "duplicate.render_data_mapping.json"
$duplicateSummary = Join-Path $resolvedWorkingDir "duplicate.mapping.lint.summary.json"

Set-Content -LiteralPath $duplicateMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "customer_name",
      "source": "customer.name"
    },
    {
      "bookmark_name": "customer_name",
      "source": "invoice.number"
    }
  ]
}
'@

$duplicateFailed = $false
try {
    & $scriptPath `
        -MappingPath $duplicateMappingPath `
        -DataPath $sampleDataPath `
        -SummaryJson $duplicateSummary

    if ($LASTEXITCODE -ne 0) {
        $duplicateFailed = $true
    }
} catch {
    $duplicateFailed = $true
}

if (-not $duplicateFailed) {
    throw "lint_render_data_mapping.ps1 should fail when duplicate targets exist."
}

$duplicateSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $duplicateSummary | ConvertFrom-Json
Assert-Equal -Actual $duplicateSummaryObject.status -Expected "failed" `
    -Message "Duplicate-target lint summary did not report status=failed."
Assert-Equal -Actual $duplicateSummaryObject.duplicate_target_count -Expected 1 `
    -Message "Duplicate-target lint summary reported an unexpected duplicate_target_count."
Assert-Equal -Actual $duplicateSummaryObject.invalid_source_count -Expected 0 `
    -Message "Duplicate-target lint summary reported an unexpected invalid_source_count."
Assert-ContainsText -Text ([string]$duplicateSummaryObject.error) `
    -ExpectedText "found" `
    -Label "Duplicate-target lint error"

$invalidSourceMappingPath = Join-Path $resolvedWorkingDir "invalid-source.render_data_mapping.json"
$invalidSourceSummary = Join-Path $resolvedWorkingDir "invalid-source.mapping.lint.summary.json"

Set-Content -LiteralPath $invalidSourceMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "customer_name",
      "source": "customer.missing_name"
    }
  ]
}
'@

$invalidSourceFailed = $false
try {
    & $scriptPath `
        -MappingPath $invalidSourceMappingPath `
        -DataPath $sampleDataPath `
        -SummaryJson $invalidSourceSummary

    if ($LASTEXITCODE -ne 0) {
        $invalidSourceFailed = $true
    }
} catch {
    $invalidSourceFailed = $true
}

if (-not $invalidSourceFailed) {
    throw "lint_render_data_mapping.ps1 should fail when a source path is invalid."
}

$invalidSourceSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $invalidSourceSummary | ConvertFrom-Json
Assert-Equal -Actual $invalidSourceSummaryObject.status -Expected "failed" `
    -Message "Invalid-source lint summary did not report status=failed."
Assert-Equal -Actual $invalidSourceSummaryObject.resolved_source_count -Expected 0 `
    -Message "Invalid-source lint summary reported an unexpected resolved_source_count."
Assert-Equal -Actual $invalidSourceSummaryObject.invalid_source_count -Expected 1 `
    -Message "Invalid-source lint summary reported an unexpected invalid_source_count."
Assert-Equal -Actual $invalidSourceSummaryObject.entries[0].data_status -Expected "failed" `
    -Message "Invalid-source lint entry should report data_status=failed."
Assert-ContainsText -Text ([string]$invalidSourceSummaryObject.entries[0].issues[0].message) `
    -ExpectedText "could not resolve property" `
    -Label "Invalid-source lint issue"

$invalidColumnMappingPath = Join-Path $resolvedWorkingDir "invalid-column.render_data_mapping.json"
$invalidColumnSummary = Join-Path $resolvedWorkingDir "invalid-column.mapping.lint.summary.json"

Set-Content -LiteralPath $invalidColumnMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_table_rows": [
    {
      "bookmark_name": "line_item_row",
      "source": "line_items",
      "columns": [
        "title",
        "missing_field"
      ]
    }
  ]
}
'@

$invalidColumnFailed = $false
try {
    & $scriptPath `
        -MappingPath $invalidColumnMappingPath `
        -DataPath $sampleDataPath `
        -SummaryJson $invalidColumnSummary

    if ($LASTEXITCODE -ne 0) {
        $invalidColumnFailed = $true
    }
} catch {
    $invalidColumnFailed = $true
}

if (-not $invalidColumnFailed) {
    throw "lint_render_data_mapping.ps1 should fail when a table column path is invalid."
}

$invalidColumnSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $invalidColumnSummary | ConvertFrom-Json
Assert-Equal -Actual $invalidColumnSummaryObject.status -Expected "failed" `
    -Message "Invalid-column lint summary did not report status=failed."
Assert-Equal -Actual $invalidColumnSummaryObject.invalid_source_count -Expected 1 `
    -Message "Invalid-column lint summary reported an unexpected invalid_source_count."
Assert-Equal -Actual $invalidColumnSummaryObject.entries[0].resolved_type -Expected "array" `
    -Message "Invalid-column lint entry should still record resolved_type=array."
Assert-Equal -Actual $invalidColumnSummaryObject.entries[0].column_reports[1].status -Expected "failed" `
    -Message "Invalid-column lint entry should mark the broken column as failed."
Assert-ContainsText -Text ([string]$invalidColumnSummaryObject.entries[0].issues[0].message) `
    -ExpectedText "invalid table column mappings" `
    -Label "Invalid-column lint issue"

Write-Host "Render-data mapping lint regression passed."
