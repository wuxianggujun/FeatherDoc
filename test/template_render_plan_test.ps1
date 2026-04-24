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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\render_template_document.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$samplePlan = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_plan.json"
$samplePlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePlan | ConvertFrom-Json
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$renderedDocx = Join-Path $resolvedWorkingDir "invoice_rendered.docx"
$renderSummary = Join-Path $resolvedWorkingDir "invoice_rendered.summary.json"

& $scriptPath `
    -InputDocx $sampleDocx `
    -PlanPath $samplePlan `
    -OutputDocx $renderedDocx `
    -SummaryJson $renderSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document.ps1 failed for the sample render plan."
}

Assert-True -Condition (Test-Path -LiteralPath $renderedDocx) `
    -Message "Rendered sample DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $renderSummary) `
    -Message "Rendered sample summary JSON was not created."

$summary = Get-Content -Raw -LiteralPath $renderSummary | ConvertFrom-Json
Assert-True -Condition ($summary.status -eq "completed") `
    -Message "Sample render summary did not report status=completed."
Assert-True -Condition ($summary.plan_counts.bookmark_text -eq 3) `
    -Message "Sample render summary reported an unexpected bookmark_text count."
Assert-True -Condition ($summary.plan_counts.bookmark_paragraphs -eq 1) `
    -Message "Sample render summary reported an unexpected bookmark_paragraphs count."
Assert-True -Condition ($summary.plan_counts.bookmark_table_rows -eq 1) `
    -Message "Sample render summary reported an unexpected bookmark_table_rows count."
Assert-True -Condition ($summary.plan_counts.bookmark_block_visibility -eq 0) `
    -Message "Sample render summary reported an unexpected bookmark_block_visibility count."
Assert-True -Condition ($summary.operation_count -eq 3) `
    -Message "Sample render summary reported an unexpected operation_count."
Assert-True -Condition ($summary.steps.Count -eq 3) `
    -Message "Sample render summary reported an unexpected step count."
Assert-True -Condition ($summary.steps[0].command -eq "fill-bookmarks") `
    -Message "Sample render summary did not run fill-bookmarks first."
Assert-True -Condition ($summary.steps[1].command -eq "replace-bookmark-paragraphs") `
    -Message "Sample render summary did not run replace-bookmark-paragraphs second."
Assert-True -Condition ($summary.steps[2].command -eq "replace-bookmark-table-rows") `
    -Message "Sample render summary did not run replace-bookmark-table-rows third."

$renderedDocumentXml = Read-DocxEntryText -DocxPath $renderedDocx -EntryName "word/document.xml"
$expectedCustomerName = [string]$samplePlanObject.bookmark_text[0].text
$expectedInvoiceNumber = [string]$samplePlanObject.bookmark_text[1].text
$expectedIssueDate = [string]$samplePlanObject.bookmark_text[2].text
$expectedFirstRowName = [string]$samplePlanObject.bookmark_table_rows[0].rows[0][0]
$expectedSecondRowName = [string]$samplePlanObject.bookmark_table_rows[0].rows[1][0]
$expectedThirdRowName = [string]$samplePlanObject.bookmark_table_rows[0].rows[2][0]
$expectedFirstNoteLine = [string]$samplePlanObject.bookmark_paragraphs[0].paragraphs[0]
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedCustomerName -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedInvoiceNumber -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedIssueDate -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedFirstRowName -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedSecondRowName -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedThirdRowName -Label "Rendered document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText $expectedFirstNoteLine -Label "Rendered document.xml"
Assert-NotContainsText -Text $renderedDocumentXml -UnexpectedText "INV-TEMPLATE" -Label "Rendered document.xml"
Assert-NotContainsText -Text $renderedDocumentXml -UnexpectedText 'w:name="line_item_row"' -Label "Rendered document.xml"
Assert-NotContainsText -Text $renderedDocumentXml -UnexpectedText 'w:name="note_lines"' -Label "Rendered document.xml"

$visibilityPlanPath = Join-Path $resolvedWorkingDir "visibility_only.render_plan.json"
$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility_fixture.docx"
$visibilityRenderedDocx = Join-Path $resolvedWorkingDir "invoice_hidden_notes.docx"
$visibilitySummaryPath = Join-Path $resolvedWorkingDir "invoice_hidden_notes.summary.json"

& $blockVisibilitySampleExecutable $visibilityFixtureDocx
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_bookmark_block_visibility_visual failed."
}

Set-Content -LiteralPath $visibilityPlanPath -Encoding UTF8 -Value @'
{
  "bookmark_block_visibility": [
    {
      "bookmark_name": "keep_block",
      "visible": true
    },
    {
      "bookmark_name": "hide_block",
      "visible": false
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $visibilityFixtureDocx `
    -PlanPath $visibilityPlanPath `
    -OutputDocx $visibilityRenderedDocx `
    -SummaryJson $visibilitySummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document.ps1 failed for the visibility-only render plan."
}

Assert-True -Condition (Test-Path -LiteralPath $visibilityRenderedDocx) `
    -Message "Visibility-only rendered DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $visibilitySummaryPath) `
    -Message "Visibility-only rendered summary JSON was not created."

$visibilitySummary = Get-Content -Raw -LiteralPath $visibilitySummaryPath | ConvertFrom-Json
Assert-True -Condition ($visibilitySummary.status -eq "completed") `
    -Message "Visibility-only render summary did not report status=completed."
Assert-True -Condition ($visibilitySummary.plan_counts.bookmark_block_visibility -eq 2) `
    -Message "Visibility-only render summary reported an unexpected bookmark_block_visibility count."
Assert-True -Condition ($visibilitySummary.operation_count -eq 1) `
    -Message "Visibility-only render summary reported an unexpected operation_count."
Assert-True -Condition ($visibilitySummary.steps[0].command -eq "apply-bookmark-block-visibility") `
    -Message "Visibility-only render summary did not record apply-bookmark-block-visibility."

$visibilityDocumentXml = Read-DocxEntryText -DocxPath $visibilityRenderedDocx -EntryName "word/document.xml"
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "Keep me" -Label "Visibility-only document.xml"
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "middle" -Label "Visibility-only document.xml"
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "after" -Label "Visibility-only document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Hide me" -Label "Visibility-only document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Secret Cell" -Label "Visibility-only document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "keep_block" -Label "Visibility-only document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "hide_block" -Label "Visibility-only document.xml"

Write-Host "Template render plan regression passed."
