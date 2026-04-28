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

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText'."
    }
}

function Read-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($null -eq $entry) {
            throw "Entry '$EntryName' was not found in $DocxPath."
        }

        $reader = New-Object System.IO.StreamReader($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $archive.Dispose()
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\render_template_document_from_data.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$sampleDataPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data.json"
$sampleMappingPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_data_mapping.json"
$sampleDataObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleDataPath | ConvertFrom-Json
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"
$partTemplateSampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_part_template_validation"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$generatedPatch = Join-Path $resolvedWorkingDir "invoice.render_patch.generated.json"
$draftPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.draft.json"
$patchedPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.filled.json"
$renderedDocx = Join-Path $resolvedWorkingDir "invoice.rendered.from-data.docx"
$summaryPath = Join-Path $resolvedWorkingDir "invoice.rendered.from-data.summary.json"

& $scriptPath `
    -InputDocx $sampleDocx `
    -DataPath $sampleDataPath `
    -MappingPath $sampleMappingPath `
    -OutputDocx $renderedDocx `
    -SummaryJson $summaryPath `
    -PatchPlanOutput $generatedPatch `
    -DraftPlanOutput $draftPlan `
    -PatchedPlanOutput $patchedPlan `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_data.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $generatedPatch) `
    -Message "Invoice generated render patch was not created."
Assert-True -Condition (Test-Path -LiteralPath $draftPlan) `
    -Message "Invoice draft render plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $patchedPlan) `
    -Message "Invoice patched render plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $renderedDocx) `
    -Message "Invoice rendered DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Invoice orchestration summary JSON was not created."

$summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$generatedPatchObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $generatedPatch | ConvertFrom-Json
$invoiceDocumentXml = Read-DocxEntryText -DocxPath $renderedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summaryObject.status -Expected "completed" `
    -Message "Invoice data-render summary did not report status=completed."
Assert-Equal -Actual $summaryObject.operation_count -Expected 4 `
    -Message "Invoice data-render summary reported an unexpected operation_count."
Assert-Equal -Actual $summaryObject.steps.Count -Expected 4 `
    -Message "Invoice data-render summary reported an unexpected step count."
Assert-True -Condition ([bool]$summaryObject.require_complete) `
    -Message "Invoice data-render summary did not record require_complete=true."
Assert-Equal -Actual $summaryObject.steps[0].name -Expected "export_template_render_plan" `
    -Message "Invoice data-render summary did not record the export step first."
Assert-Equal -Actual $summaryObject.steps[1].name -Expected "convert_render_data_to_patch_plan" `
    -Message "Invoice data-render summary did not record the convert step second."
Assert-Equal -Actual $summaryObject.steps[2].name -Expected "patch_template_render_plan" `
    -Message "Invoice data-render summary did not record the patch step third."
Assert-Equal -Actual $summaryObject.steps[3].name -Expected "render_template_document" `
    -Message "Invoice data-render summary did not record the render step fourth."
Assert-Equal -Actual $summaryObject.steps[0].status -Expected "completed" `
    -Message "Invoice export step did not complete."
Assert-Equal -Actual $summaryObject.steps[1].status -Expected "completed" `
    -Message "Invoice convert step did not complete."
Assert-Equal -Actual $summaryObject.steps[2].status -Expected "completed" `
    -Message "Invoice patch step did not complete."
Assert-Equal -Actual $summaryObject.steps[3].status -Expected "completed" `
    -Message "Invoice render step did not complete."
Assert-True -Condition ([bool]$summaryObject.kept_intermediate_files.patch_plan) `
    -Message "Invoice data-render summary should record the kept patch plan."
Assert-True -Condition ([bool]$summaryObject.kept_intermediate_files.draft_plan) `
    -Message "Invoice data-render summary should record the kept draft plan."
Assert-True -Condition ([bool]$summaryObject.kept_intermediate_files.patched_plan) `
    -Message "Invoice data-render summary should record the kept patched plan."
Assert-Equal -Actual $summaryObject.steps[0].summary.plan_counts.bookmark_text -Expected 3 `
    -Message "Invoice export step summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $summaryObject.steps[1].summary.patch_counts.bookmark_text -Expected 3 `
    -Message "Invoice convert step summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $summaryObject.steps[2].summary.patch_counts.bookmark_text.applied -Expected 3 `
    -Message "Invoice patch step summary reported an unexpected applied bookmark_text count."
Assert-Equal -Actual $summaryObject.steps[3].summary.operation_count -Expected 3 `
    -Message "Invoice render step summary reported an unexpected operation_count."
Assert-Equal -Actual $generatedPatchObject.bookmark_text[0].text -Expected ([string]$sampleDataObject.customer.name) `
    -Message "Invoice generated patch did not resolve customer_name."

Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.customer.name) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.invoice.number) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.invoice.issue_date) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.notes[0]) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.line_items[0].title) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.line_items[1].title) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$sampleDataObject.line_items[2].title) -Label "Invoice document.xml"
Assert-NotContainsText -Text $invoiceDocumentXml -UnexpectedText "TODO:" -Label "Invoice document.xml"

$incompleteMappingPath = Join-Path $resolvedWorkingDir "invoice.incomplete.render_data_mapping.json"
$incompleteSummaryPath = Join-Path $resolvedWorkingDir "invoice.incomplete.rendered.from-data.summary.json"
$incompleteOutputDocx = Join-Path $resolvedWorkingDir "invoice.incomplete.rendered.from-data.docx"

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
        -DataPath $sampleDataPath `
        -MappingPath $incompleteMappingPath `
        -OutputDocx $incompleteOutputDocx `
        -SummaryJson $incompleteSummaryPath `
        -BuildDir $resolvedBuildDir `
        -SkipBuild

    if ($LASTEXITCODE -ne 0) {
        $incompleteFailed = $true
    }
} catch {
    $incompleteFailed = $true
}

if (-not $incompleteFailed) {
    throw "render_template_document_from_data.ps1 should fail when required mappings are missing."
}

Assert-True -Condition (Test-Path -LiteralPath $incompleteSummaryPath) `
    -Message "Incomplete invoice data-render summary JSON was not created."

$incompleteSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $incompleteSummaryPath | ConvertFrom-Json
Assert-Equal -Actual $incompleteSummaryObject.status -Expected "failed" `
    -Message "Incomplete invoice data-render summary did not report status=failed."
Assert-Equal -Actual $incompleteSummaryObject.steps[0].status -Expected "completed" `
    -Message "Incomplete invoice export step did not complete."
Assert-Equal -Actual $incompleteSummaryObject.steps[1].status -Expected "completed" `
    -Message "Incomplete invoice convert step did not complete."
Assert-Equal -Actual $incompleteSummaryObject.steps[2].status -Expected "failed" `
    -Message "Incomplete invoice patch step should fail."
Assert-Equal -Actual $incompleteSummaryObject.steps[3].status -Expected "pending" `
    -Message "Incomplete invoice render step should not start."
Assert-ContainsText -Text ([string]$incompleteSummaryObject.error) `
    -ExpectedText "render plan still contains" `
    -Label "Incomplete invoice data-render error"

$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility_fixture.docx"
$visibilityDataPath = Join-Path $resolvedWorkingDir "block_visibility.render_data.json"
$visibilityMappingPath = Join-Path $resolvedWorkingDir "block_visibility.render_data_mapping.json"
$visibilityRenderedDocx = Join-Path $resolvedWorkingDir "block_visibility.rendered.from-data.docx"
$visibilitySummaryPath = Join-Path $resolvedWorkingDir "block_visibility.rendered.from-data.summary.json"

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
    -DataPath $visibilityDataPath `
    -MappingPath $visibilityMappingPath `
    -OutputDocx $visibilityRenderedDocx `
    -SummaryJson $visibilitySummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_data.ps1 failed for the visibility fixture."
}

$visibilitySummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilitySummaryPath | ConvertFrom-Json
$visibilityDocumentXml = Read-DocxEntryText -DocxPath $visibilityRenderedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $visibilitySummaryObject.status -Expected "completed" `
    -Message "Visibility data-render summary did not report status=completed."
Assert-Equal -Actual $visibilitySummaryObject.steps[1].summary.patch_counts.bookmark_block_visibility -Expected 2 `
    -Message "Visibility convert step summary reported an unexpected visibility count."
Assert-Equal -Actual $visibilitySummaryObject.steps[2].summary.patch_counts.bookmark_block_visibility.applied -Expected 2 `
    -Message "Visibility patch step summary reported an unexpected applied count."
Assert-Equal -Actual $visibilitySummaryObject.steps[3].summary.operation_count -Expected 1 `
    -Message "Visibility render step summary reported an unexpected operation_count."
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "Keep me" -Label "Visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Hide me" -Label "Visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Secret Cell" -Label "Visibility document.xml"

$partTemplateDir = Join-Path $resolvedWorkingDir "part_template"
$partTemplateDocx = Join-Path $partTemplateDir "part_template_validation.docx"
$partTemplateDataPath = Join-Path $resolvedWorkingDir "part_template.render_data.json"
$partTemplateMappingPath = Join-Path $resolvedWorkingDir "part_template.render_data_mapping.json"
$partTemplatePatchPath = Join-Path $resolvedWorkingDir "part_template.render_patch.generated.json"
$partTemplateDraftPlan = Join-Path $resolvedWorkingDir "part_template.render-plan.draft.json"
$partTemplatePatchedPlan = Join-Path $resolvedWorkingDir "part_template.render-plan.filled.json"
$partTemplateRenderedDocx = Join-Path $resolvedWorkingDir "part_template.rendered.from-data.docx"
$partTemplateSummaryPath = Join-Path $resolvedWorkingDir "part_template.rendered.from-data.summary.json"

& $partTemplateSampleExecutable $partTemplateDir
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_part_template_validation failed."
}

$partTemplateData = [ordered]@{
    header = [ordered]@{
        title = "Section Header Rendered From Data"
        note_lines = @("Header note line 1", "Header note line 2")
        rows = @(
            [ordered]@{ name = "Alpha"; quantity = "12" },
            [ordered]@{ name = "Beta"; quantity = "34" }
        )
    }
    footer = [ordered]@{
        company = "FeatherDoc Section Footer Ltd."
        summary = "Section footer summary rendered from business data"
    }
}
($partTemplateData | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $partTemplateDataPath -Encoding UTF8

$partTemplateMapping = [ordered]@{
    bookmark_text = @(
        [ordered]@{
            bookmark_name = "header_title"
            part = "section-header"
            section = 0
            kind = "default"
            source = "header.title"
        },
        [ordered]@{
            bookmark_name = "footer_company"
            part = "section-footer"
            section = 0
            kind = "default"
            source = "footer.company"
        },
        [ordered]@{
            bookmark_name = "footer_summary"
            part = "section-footer"
            section = 0
            kind = "default"
            source = "footer.summary"
        }
    )
    bookmark_paragraphs = @(
        [ordered]@{
            bookmark_name = "header_note"
            part = "section-header"
            section = 0
            kind = "default"
            source = "header.note_lines"
        }
    )
    bookmark_table_rows = @(
        [ordered]@{
            bookmark_name = "header_rows"
            part = "section-header"
            section = 0
            kind = "default"
            source = "header.rows"
            columns = @("name", "quantity")
        }
    )
}
($partTemplateMapping | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $partTemplateMappingPath -Encoding UTF8

& $scriptPath `
    -InputDocx $partTemplateDocx `
    -DataPath $partTemplateDataPath `
    -MappingPath $partTemplateMappingPath `
    -OutputDocx $partTemplateRenderedDocx `
    -SummaryJson $partTemplateSummaryPath `
    -PatchPlanOutput $partTemplatePatchPath `
    -DraftPlanOutput $partTemplateDraftPlan `
    -PatchedPlanOutput $partTemplatePatchedPlan `
    -BuildDir $resolvedBuildDir `
    -ExportTargetMode resolved-section-targets `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_data.ps1 failed for the section header/footer fixture."
}

$partTemplateSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplateSummaryPath | ConvertFrom-Json
$partTemplatePatchObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplatePatchPath | ConvertFrom-Json
$partTemplateDraftPlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $partTemplateDraftPlan | ConvertFrom-Json
$partTemplateHeaderXml = Read-DocxEntryText -DocxPath $partTemplateRenderedDocx -EntryName "word/header1.xml"
$partTemplateFooterXml = Read-DocxEntryText -DocxPath $partTemplateRenderedDocx -EntryName "word/footer1.xml"

Assert-Equal -Actual $partTemplateSummaryObject.status -Expected "completed" `
    -Message "Section part data-render summary did not report status=completed."
Assert-Equal -Actual $partTemplateSummaryObject.export_target_mode -Expected "resolved-section-targets" `
    -Message "Section part data-render summary did not record export_target_mode."
Assert-Equal -Actual $partTemplateSummaryObject.steps[0].summary.target_mode -Expected "resolved-section-targets" `
    -Message "Section part export summary did not record resolved-section target mode."
Assert-Equal -Actual $partTemplateSummaryObject.steps[0].summary.plan_counts.bookmark_text -Expected 3 `
    -Message "Section part export step reported an unexpected bookmark_text count."
Assert-Equal -Actual $partTemplateSummaryObject.steps[1].summary.patch_counts.bookmark_table_rows -Expected 1 `
    -Message "Section part convert step reported an unexpected table-row count."
Assert-Equal -Actual $partTemplateSummaryObject.steps[2].summary.patch_counts.bookmark_table_rows.applied -Expected 1 `
    -Message "Section part patch step reported an unexpected applied table-row count."
Assert-Equal -Actual $partTemplateSummaryObject.steps[3].summary.operation_count -Expected 4 `
    -Message "Section part render step reported an unexpected operation_count."
Assert-Equal -Actual $partTemplatePatchObject.bookmark_text[0].part -Expected "section-header" `
    -Message "Section header text patch did not preserve the section-header selector."
Assert-Equal -Actual $partTemplateDraftPlanObject.bookmark_text[0].part -Expected "section-header" `
    -Message "Section header draft plan did not export section-header selectors."
Assert-Equal -Actual $partTemplateDraftPlanObject.bookmark_text[0].section -Expected 0 `
    -Message "Section header draft plan did not preserve section=0."
Assert-Equal -Actual $partTemplateDraftPlanObject.bookmark_text[0].kind -Expected "default" `
    -Message "Section header draft plan did not preserve kind=default."

Assert-ContainsText -Text $partTemplateHeaderXml -ExpectedText "Section Header Rendered From Data" -Label "Section header XML"
Assert-ContainsText -Text $partTemplateHeaderXml -ExpectedText "Header note line 1" -Label "Section header XML"
Assert-ContainsText -Text $partTemplateHeaderXml -ExpectedText "Header note line 2" -Label "Section header XML"
Assert-ContainsText -Text $partTemplateHeaderXml -ExpectedText "Alpha" -Label "Section header XML"
Assert-ContainsText -Text $partTemplateHeaderXml -ExpectedText "34" -Label "Section header XML"
Assert-ContainsText -Text $partTemplateFooterXml -ExpectedText "FeatherDoc Section Footer Ltd." -Label "Section footer XML"
Assert-ContainsText -Text $partTemplateFooterXml -ExpectedText "Section footer summary rendered from business data" -Label "Section footer XML"
Assert-NotContainsText -Text $partTemplateHeaderXml -UnexpectedText "TODO:" -Label "Section header XML"
Assert-NotContainsText -Text $partTemplateFooterXml -UnexpectedText "TODO:" -Label "Section footer XML"

Write-Host "Render-from-data regression passed."
