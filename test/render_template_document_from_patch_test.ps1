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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\render_template_document_from_patch.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$samplePatchPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_patch.json"
$samplePatchObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePatchPath | ConvertFrom-Json
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$draftPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.draft.json"
$patchedPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.filled.json"
$renderedDocx = Join-Path $resolvedWorkingDir "invoice.rendered.from-patch.docx"
$summaryPath = Join-Path $resolvedWorkingDir "invoice.rendered.from-patch.summary.json"

& $scriptPath `
    -InputDocx $sampleDocx `
    -PatchPlanPath $samplePatchPath `
    -OutputDocx $renderedDocx `
    -SummaryJson $summaryPath `
    -DraftPlanOutput $draftPlan `
    -PatchedPlanOutput $patchedPlan `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_patch.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $draftPlan) `
    -Message "Invoice draft render plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $patchedPlan) `
    -Message "Invoice patched render plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $renderedDocx) `
    -Message "Invoice rendered DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Invoice orchestration summary JSON was not created."

$summaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual $summaryObject.status -Expected "completed" `
    -Message "Invoice orchestration summary did not report status=completed."
Assert-True -Condition ([bool]$summaryObject.require_complete) `
    -Message "Invoice orchestration summary did not record require_complete=true."
Assert-Equal -Actual $summaryObject.operation_count -Expected 3 `
    -Message "Invoice orchestration summary reported an unexpected operation_count."
Assert-Equal -Actual $summaryObject.steps.Count -Expected 3 `
    -Message "Invoice orchestration summary reported an unexpected step count."
Assert-Equal -Actual $summaryObject.steps[0].name -Expected "export_template_render_plan" `
    -Message "Invoice orchestration summary did not record the export step first."
Assert-Equal -Actual $summaryObject.steps[1].name -Expected "patch_template_render_plan" `
    -Message "Invoice orchestration summary did not record the patch step second."
Assert-Equal -Actual $summaryObject.steps[2].name -Expected "render_template_document" `
    -Message "Invoice orchestration summary did not record the render step third."
Assert-Equal -Actual $summaryObject.steps[0].status -Expected "completed" `
    -Message "Invoice export step did not complete."
Assert-Equal -Actual $summaryObject.steps[1].status -Expected "completed" `
    -Message "Invoice patch step did not complete."
Assert-Equal -Actual $summaryObject.steps[2].status -Expected "completed" `
    -Message "Invoice render step did not complete."
Assert-True -Condition ([bool]$summaryObject.kept_intermediate_files.draft_plan) `
    -Message "Invoice orchestration summary should record the kept draft plan."
Assert-True -Condition ([bool]$summaryObject.kept_intermediate_files.patched_plan) `
    -Message "Invoice orchestration summary should record the kept patched plan."
Assert-Equal -Actual $summaryObject.steps[0].summary.plan_counts.bookmark_text -Expected 3 `
    -Message "Invoice export step summary reported an unexpected bookmark_text count."
Assert-Equal -Actual $summaryObject.steps[1].summary.patch_counts.bookmark_table_rows.applied -Expected 1 `
    -Message "Invoice patch step summary reported an unexpected applied table-row count."
Assert-True -Condition ([bool]$summaryObject.steps[1].summary.require_complete) `
    -Message "Invoice patch step summary should record require_complete=true."
Assert-Equal -Actual $summaryObject.steps[2].summary.operation_count -Expected 3 `
    -Message "Invoice render step summary reported an unexpected operation_count."

$draftPlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $draftPlan | ConvertFrom-Json
$patchedPlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $patchedPlan | ConvertFrom-Json
$invoiceDocumentXml = Read-DocxEntryText -DocxPath $renderedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $draftPlanObject.bookmark_text[0].text -Expected "TODO: customer_name" `
    -Message "Invoice draft render plan did not keep the expected placeholder."
Assert-Equal -Actual $patchedPlanObject.bookmark_text[0].text -Expected ([string]$samplePatchObject.bookmark_text[0].text) `
    -Message "Invoice patched render plan did not apply customer_name."
Assert-Equal -Actual $patchedPlanObject.bookmark_table_rows[0].rows[2][0] `
    -Expected ([string]$samplePatchObject.bookmark_table_rows[0].rows[2][0]) `
    -Message "Invoice patched render plan did not apply the third table row."
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_text[0].text) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_text[1].text) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_text[2].text) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_paragraphs[0].paragraphs[0]) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_table_rows[0].rows[0][0]) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_table_rows[0].rows[1][0]) -Label "Invoice document.xml"
Assert-ContainsText -Text $invoiceDocumentXml -ExpectedText ([string]$samplePatchObject.bookmark_table_rows[0].rows[2][0]) -Label "Invoice document.xml"
Assert-NotContainsText -Text $invoiceDocumentXml -UnexpectedText "TODO:" -Label "Invoice document.xml"

$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility_fixture.docx"
$visibilityPatchPath = Join-Path $resolvedWorkingDir "block_visibility.render_patch.json"
$visibilityRenderedDocx = Join-Path $resolvedWorkingDir "block_visibility.rendered.from-patch.docx"
$visibilitySummaryPath = Join-Path $resolvedWorkingDir "block_visibility.rendered.from-patch.summary.json"

& $blockVisibilitySampleExecutable $visibilityFixtureDocx
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_bookmark_block_visibility_visual failed."
}

Set-Content -LiteralPath $visibilityPatchPath -Encoding UTF8 -Value @'
{
  "bookmark_block_visibility": [
    {
      "bookmark_name": "hide_block",
      "visible": false
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $visibilityFixtureDocx `
    -PatchPlanPath $visibilityPatchPath `
    -OutputDocx $visibilityRenderedDocx `
    -SummaryJson $visibilitySummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_patch.ps1 failed for the visibility fixture."
}

Assert-True -Condition (Test-Path -LiteralPath $visibilityRenderedDocx) `
    -Message "Visibility rendered DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $visibilitySummaryPath) `
    -Message "Visibility orchestration summary JSON was not created."

$visibilitySummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilitySummaryPath | ConvertFrom-Json
Assert-Equal -Actual $visibilitySummaryObject.status -Expected "completed" `
    -Message "Visibility orchestration summary did not report status=completed."
Assert-Equal -Actual $visibilitySummaryObject.steps[0].summary.plan_counts.bookmark_block_visibility -Expected 2 `
    -Message "Visibility export step summary reported an unexpected visibility count."
Assert-Equal -Actual $visibilitySummaryObject.steps[1].summary.patch_counts.bookmark_block_visibility.applied -Expected 1 `
    -Message "Visibility patch step summary reported an unexpected applied visibility count."
Assert-Equal -Actual $visibilitySummaryObject.steps[2].summary.operation_count -Expected 1 `
    -Message "Visibility render step summary reported an unexpected operation_count."

$visibilityDocumentXml = Read-DocxEntryText -DocxPath $visibilityRenderedDocx -EntryName "word/document.xml"
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "Keep me" -Label "Visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Hide me" -Label "Visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Secret Cell" -Label "Visibility document.xml"

Write-Host "Template render-from-patch regression passed."
