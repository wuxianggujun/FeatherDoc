param(
    [string]$RepoRoot,
    [string]$BuildDir,
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

function Find-ExecutableByName {
    param(
        [string]$SearchRoot,
        [string]$TargetName
    )

    $candidate = Get-ChildItem -Path $SearchRoot -Recurse -File |
        Where-Object { $_.Name -ieq $TargetName -or $_.Name -ieq ($TargetName + ".exe") } |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1

    if ($null -eq $candidate) {
        throw "Could not find executable '$TargetName' under $SearchRoot."
    }

    return $candidate.FullName
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\validate_render_data_mapping.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$sampleDataPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data.json"
$sampleMappingPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data_mapping.json"
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"
$partTemplateSampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_part_template_validation"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$validDraftPlan = Join-Path $resolvedWorkingDir "invoice.validation.draft.render-plan.json"
$validGeneratedPatch = Join-Path $resolvedWorkingDir "invoice.validation.generated.render_patch.json"
$validPatchedPlan = Join-Path $resolvedWorkingDir "invoice.validation.patched.render-plan.json"
$validSummary = Join-Path $resolvedWorkingDir "invoice.validation.summary.json"

& $scriptPath `
    -InputDocx $sampleDocx `
    -MappingPath $sampleMappingPath `
    -DataPath $sampleDataPath `
    -SummaryJson $validSummary `
    -DraftPlanOutput $validDraftPlan `
    -GeneratedPatchOutput $validGeneratedPatch `
    -PatchedPlanOutput $validPatchedPlan `
    -BuildDir $resolvedBuildDir `
    -SkipBuild `
    -RequireComplete

if ($LASTEXITCODE -ne 0) {
    throw "validate_render_data_mapping.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $validSummary) `
    -Message "Invoice validation summary JSON was not created."
Assert-True -Condition (Test-Path -LiteralPath $validDraftPlan) `
    -Message "Invoice validation draft plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $validGeneratedPatch) `
    -Message "Invoice validation generated patch was not created."
Assert-True -Condition (Test-Path -LiteralPath $validPatchedPlan) `
    -Message "Invoice validation patched plan was not created."

$validSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $validSummary | ConvertFrom-Json
Assert-Equal -Actual $validSummaryObject.status -Expected "completed" `
    -Message "Invoice validation summary did not report status=completed."
Assert-True -Condition ([bool]$validSummaryObject.require_complete) `
    -Message "Invoice validation summary did not record require_complete=true."
Assert-Equal -Actual $validSummaryObject.export_target_mode -Expected "loaded-parts" `
    -Message "Invoice validation summary should default to loaded-parts export target mode."
Assert-Equal -Actual $validSummaryObject.operation_count -Expected 3 `
    -Message "Invoice validation summary reported an unexpected operation_count."
Assert-Equal -Actual $validSummaryObject.steps[0].status -Expected "completed" `
    -Message "Invoice export validation step did not complete."
Assert-Equal -Actual $validSummaryObject.steps[1].status -Expected "completed" `
    -Message "Invoice convert validation step did not complete."
Assert-Equal -Actual $validSummaryObject.steps[2].status -Expected "completed" `
    -Message "Invoice patch validation step did not complete."
Assert-Equal -Actual $validSummaryObject.remaining_placeholder_count -Expected 0 `
    -Message "Invoice validation should not leave placeholders."
Assert-Equal -Actual $validSummaryObject.patch_counts.bookmark_text.applied -Expected 3 `
    -Message "Invoice validation patch summary reported an unexpected applied bookmark_text count."

$incompleteMappingPath = Join-Path $resolvedWorkingDir "invoice.incomplete.render_data_mapping.json"
$incompleteSummary = Join-Path $resolvedWorkingDir "invoice.incomplete.validation.summary.json"

Set-Content -LiteralPath $incompleteMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_text": [
    {
      "bookmark_name": "customer_name",
      "source": "customer.name"
    },
    {
      "bookmark_name": "invoice_number",
      "source": "invoice.number"
    }
  ],
  "bookmark_table_rows": [
    {
      "bookmark_name": "line_item_row",
      "source": "line_items",
      "columns": [
        "title",
        "description",
        "amount"
      ]
    }
  ]
}
'@

$incompleteFailed = $false
try {
    & $scriptPath `
        -InputDocx $sampleDocx `
        -MappingPath $incompleteMappingPath `
        -DataPath $sampleDataPath `
        -SummaryJson $incompleteSummary `
        -BuildDir $resolvedBuildDir `
        -SkipBuild `
        -RequireComplete

    if ($LASTEXITCODE -ne 0) {
        $incompleteFailed = $true
    }
} catch {
    $incompleteFailed = $true
}

if (-not $incompleteFailed) {
    throw "validate_render_data_mapping.ps1 should fail when required mappings are missing."
}

Assert-True -Condition (Test-Path -LiteralPath $incompleteSummary) `
    -Message "Incomplete-mapping validation summary JSON was not created."

$incompleteSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $incompleteSummary | ConvertFrom-Json
Assert-Equal -Actual $incompleteSummaryObject.status -Expected "failed" `
    -Message "Incomplete-mapping validation summary did not report status=failed."
Assert-Equal -Actual $incompleteSummaryObject.steps[0].status -Expected "completed" `
    -Message "Incomplete-mapping export step did not complete."
Assert-Equal -Actual $incompleteSummaryObject.steps[1].status -Expected "completed" `
    -Message "Incomplete-mapping convert step did not complete."
Assert-Equal -Actual $incompleteSummaryObject.steps[2].status -Expected "failed" `
    -Message "Incomplete-mapping patch step should fail."
Assert-True -Condition ($incompleteSummaryObject.remaining_placeholder_count -gt 0) `
    -Message "Incomplete-mapping validation should retain placeholders."
Assert-ContainsText -Text ([string]$incompleteSummaryObject.error) `
    -ExpectedText "render plan still contains" `
    -Label "Incomplete-mapping validation error"

$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility.validation.fixture.docx"
$visibilityDataPath = Join-Path $resolvedWorkingDir "block_visibility.validation.render_data.json"
$visibilityMappingPath = Join-Path $resolvedWorkingDir "block_visibility.validation.render_data_mapping.json"
$visibilitySummary = Join-Path $resolvedWorkingDir "block_visibility.validation.summary.json"

& $blockVisibilitySampleExecutable $visibilityFixtureDocx
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_bookmark_block_visibility_visual failed."
}

Set-Content -LiteralPath $visibilityDataPath -Encoding UTF8 -Value @'
{
  "visibility": {
    "keep": true,
    "hide": false
  }
}
'@

Set-Content -LiteralPath $visibilityMappingPath -Encoding UTF8 -Value @'
{
  "bookmark_block_visibility": [
    {
      "bookmark_name": "keep_block",
      "source": "visibility.keep"
    },
    {
      "bookmark_name": "hide_block",
      "source": "visibility.hide"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $visibilityFixtureDocx `
    -MappingPath $visibilityMappingPath `
    -DataPath $visibilityDataPath `
    -SummaryJson $visibilitySummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "validate_render_data_mapping.ps1 failed for the visibility fixture."
}

$visibilitySummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilitySummary | ConvertFrom-Json
Assert-Equal -Actual $visibilitySummaryObject.status -Expected "completed" `
    -Message "Visibility validation summary did not report status=completed."
Assert-Equal -Actual $visibilitySummaryObject.patch_counts.bookmark_block_visibility.applied -Expected 2 `
    -Message "Visibility validation patch summary reported an unexpected applied count."
Assert-Equal -Actual $visibilitySummaryObject.steps[2].status -Expected "completed" `
    -Message "Visibility patch validation step did not complete."


$partTemplateDir = Join-Path $resolvedWorkingDir "part_template_validation_fixture"
$partTemplateDocx = Join-Path $partTemplateDir "part_template_validation.docx"
$partTemplateDataPath = Join-Path $resolvedWorkingDir "part_template.validation.render_data.json"
$partTemplateMappingPath = Join-Path $resolvedWorkingDir "part_template.validation.render_data_mapping.json"
$partTemplateDraftPlan = Join-Path $resolvedWorkingDir "part_template.validation.draft.render-plan.json"
$partTemplatePatchPath = Join-Path $resolvedWorkingDir "part_template.validation.generated.render_patch.json"
$partTemplatePatchedPlan = Join-Path $resolvedWorkingDir "part_template.validation.patched.render-plan.json"
$partTemplateSummary = Join-Path $resolvedWorkingDir "part_template.validation.summary.json"
$partTemplateReport = Join-Path $resolvedWorkingDir "part_template.validation.report.md"

& $partTemplateSampleExecutable $partTemplateDir
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_part_template_validation failed."
}

$partTemplateData = [ordered]@{
    header = [ordered]@{
        title = "Section Header Validated From Data"
        note_lines = @("Validated header note line 1", "Validated header note line 2")
        rows = @(
            [ordered]@{ name = "Eta"; quantity = "21" },
            [ordered]@{ name = "Theta"; quantity = "43" }
        )
    }
    footer = [ordered]@{
        company = "FeatherDoc Section Validation Ltd."
        summary = "Section footer summary validated from business data"
    }
}
($partTemplateData | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $partTemplateDataPath -Encoding UTF8

$partTemplateMapping = [ordered]@{
    bookmark_text = @(
        [ordered]@{ bookmark_name = "header_title"; part = "section-header"; section = 0; kind = "default"; source = "header.title" },
        [ordered]@{ bookmark_name = "footer_company"; part = "section-footer"; section = 0; kind = "default"; source = "footer.company" },
        [ordered]@{ bookmark_name = "footer_summary"; part = "section-footer"; section = 0; kind = "default"; source = "footer.summary" }
    )
    bookmark_paragraphs = @(
        [ordered]@{ bookmark_name = "header_note"; part = "section-header"; section = 0; kind = "default"; source = "header.note_lines" }
    )
    bookmark_table_rows = @(
        [ordered]@{ bookmark_name = "header_rows"; part = "section-header"; section = 0; kind = "default"; source = "header.rows"; columns = @("name", "quantity") }
    )
}
($partTemplateMapping | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $partTemplateMappingPath -Encoding UTF8

& $scriptPath `
    -InputDocx $partTemplateDocx `
    -MappingPath $partTemplateMappingPath `
    -DataPath $partTemplateDataPath `
    -SummaryJson $partTemplateSummary `
    -ReportMarkdown $partTemplateReport `
    -DraftPlanOutput $partTemplateDraftPlan `
    -GeneratedPatchOutput $partTemplatePatchPath `
    -PatchedPlanOutput $partTemplatePatchedPlan `
    -BuildDir $resolvedBuildDir `
    -ExportTargetMode resolved-section-targets `
    -SkipBuild `
    -RequireComplete

if ($LASTEXITCODE -ne 0) {
    throw "validate_render_data_mapping.ps1 failed for the section header/footer fixture."
}

$partTemplateSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplateSummary | ConvertFrom-Json
$partTemplatePatchObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplatePatchPath | ConvertFrom-Json
$partTemplateReportText = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplateReport
Assert-Equal -Actual $partTemplateSummaryObject.status -Expected "completed" `
    -Message "Section validation summary did not report status=completed."
Assert-Equal -Actual $partTemplateSummaryObject.export_target_mode -Expected "resolved-section-targets" `
    -Message "Section validation summary did not record export_target_mode."
Assert-Equal -Actual $partTemplateSummaryObject.steps[0].summary.target_mode -Expected "resolved-section-targets" `
    -Message "Section validation export step did not record target_mode."
Assert-Equal -Actual $partTemplateSummaryObject.remaining_placeholder_count -Expected 0 `
    -Message "Section validation should not leave placeholders."
Assert-Equal -Actual $partTemplatePatchObject.bookmark_text[0].part -Expected "section-header" `
    -Message "Section validation generated patch did not preserve section-header selectors."
Assert-ContainsText -Text $partTemplateReportText `
    -ExpectedText 'export_target_mode：`resolved-section-targets`' `
    -Label "Section validation report"

Write-Host "Render-data mapping validation regression passed."
