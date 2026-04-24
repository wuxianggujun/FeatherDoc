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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\convert_render_data_to_patch_plan.ps1"
$sampleDataPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data.json"
$sampleMappingPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data_mapping.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$sampleOutput = Join-Path $resolvedWorkingDir "invoice.render_patch.generated.json"
$sampleSummary = Join-Path $resolvedWorkingDir "invoice.render_patch.generated.summary.json"

& $scriptPath `
    -DataPath $sampleDataPath `
    -MappingPath $sampleMappingPath `
    -OutputPatch $sampleOutput `
    -SummaryJson $sampleSummary

if ($LASTEXITCODE -ne 0) {
    throw "convert_render_data_to_patch_plan.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $sampleOutput) `
    -Message "Invoice render patch was not created."
Assert-True -Condition (Test-Path -LiteralPath $sampleSummary) `
    -Message "Invoice conversion summary JSON was not created."

$samplePatch = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleOutput | ConvertFrom-Json
$sampleSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleSummary | ConvertFrom-Json

Assert-Equal -Actual $sampleSummaryObject.status -Expected "completed" `
    -Message "Invoice conversion summary did not report status=completed."
Assert-Equal -Actual $sampleSummaryObject.patch_counts.bookmark_text -Expected 3 `
    -Message "Invoice conversion summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $sampleSummaryObject.patch_counts.bookmark_paragraphs -Expected 1 `
    -Message "Invoice conversion summary reported an unexpected bookmark_paragraphs count."
Assert-Equal -Actual $sampleSummaryObject.patch_counts.bookmark_table_rows -Expected 1 `
    -Message "Invoice conversion summary reported an unexpected bookmark_table_rows count."
Assert-Equal -Actual $sampleSummaryObject.patch_counts.bookmark_block_visibility -Expected 0 `
    -Message "Invoice conversion summary reported an unexpected bookmark_block_visibility count."
Assert-True -Condition ([string]$samplePatch.'$schema' -match 'template_render_plan\.schema\.json$') `
    -Message "Invoice render patch did not reference the render-plan schema."
Assert-Equal -Actual $samplePatch.bookmark_text[0].text -Expected "上海羽文档科技有限公司" `
    -Message "Invoice render patch did not resolve customer_name."
Assert-Equal -Actual $samplePatch.bookmark_text[1].text -Expected "报价单-2026-0410" `
    -Message "Invoice render patch did not resolve invoice_number."
Assert-Equal -Actual $samplePatch.bookmark_paragraphs[0].paragraphs[2] `
    -Expected "3. 后续如需扩展审批意见、页眉页脚或附加说明，可以继续沿用同一份渲染计划。" `
    -Message "Invoice render patch did not resolve note_lines."
Assert-Equal -Actual $samplePatch.bookmark_table_rows[0].rows[1][0] -Expected "文档生成" `
    -Message "Invoice render patch did not resolve the second line-item row."
Assert-Equal -Actual $samplePatch.bookmark_table_rows[0].rows[2][2] -Expected "2,800.00" `
    -Message "Invoice render patch did not resolve the third line-item amount."

$visibilityDataPath = Join-Path $resolvedWorkingDir "visibility.render_data.json"
$visibilityMappingPath = Join-Path $resolvedWorkingDir "visibility.render_data_mapping.json"
$visibilityOutput = Join-Path $resolvedWorkingDir "visibility.render_patch.generated.json"
$visibilitySummary = Join-Path $resolvedWorkingDir "visibility.render_patch.generated.summary.json"

Set-Content -LiteralPath $visibilityDataPath -Encoding UTF8 -Value @'
{
  "visibility": {
    "show_keep_block": true,
    "show_hide_block": false
  }
}
'@

Set-Content -LiteralPath $visibilityMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_block_visibility": [
    {
      "bookmark_name": "keep_block",
      "source": "visibility.show_keep_block"
    },
    {
      "bookmark_name": "hide_block",
      "source": "visibility.show_hide_block"
    }
  ]
}
'@

& $scriptPath `
    -DataPath $visibilityDataPath `
    -MappingPath $visibilityMappingPath `
    -OutputPatch $visibilityOutput `
    -SummaryJson $visibilitySummary

if ($LASTEXITCODE -ne 0) {
    throw "convert_render_data_to_patch_plan.ps1 failed for the visibility sample."
}

$visibilityPatch = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilityOutput | ConvertFrom-Json
$keepEntry = @($visibilityPatch.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "keep_block" })[0]
$hideEntry = @($visibilityPatch.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "hide_block" })[0]

Assert-True -Condition ($null -ne $keepEntry) -Message "Visibility render patch is missing keep_block."
Assert-True -Condition ($null -ne $hideEntry) -Message "Visibility render patch is missing hide_block."
Assert-True -Condition ([bool]$keepEntry.visible) -Message "keep_block should resolve to visible=true."
Assert-True -Condition (-not [bool]$hideEntry.visible) -Message "hide_block should resolve to visible=false."

Write-Host "Render-data to patch conversion regression passed."
