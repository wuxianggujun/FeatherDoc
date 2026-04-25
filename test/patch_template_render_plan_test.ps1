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

$exportScriptPath = Join-Path $resolvedRepoRoot "scripts\export_template_render_plan.ps1"
$patchScriptPath = Join-Path $resolvedRepoRoot "scripts\patch_template_render_plan.ps1"
$renderScriptPath = Join-Path $resolvedRepoRoot "scripts\render_template_document.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$samplePatchPath = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.render_patch.json"
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$draftPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.draft.json"
$draftSummary = Join-Path $resolvedWorkingDir "invoice.render-plan.draft.summary.json"
$filledPlan = Join-Path $resolvedWorkingDir "invoice.render-plan.filled.json"
$patchSummary = Join-Path $resolvedWorkingDir "invoice.render-plan.patch.summary.json"
$renderedDocx = Join-Path $resolvedWorkingDir "invoice.rendered.from-patch.docx"
$renderSummary = Join-Path $resolvedWorkingDir "invoice.rendered.from-patch.summary.json"

& $exportScriptPath `
    -InputDocx $sampleDocx `
    -OutputPlan $draftPlan `
    -SummaryJson $draftSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "export_template_render_plan.ps1 failed for the invoice sample."
}

& $patchScriptPath `
    -BasePlanPath $draftPlan `
    -PatchPlanPath $samplePatchPath `
    -OutputPlan $filledPlan `
    -SummaryJson $patchSummary `
    -RequireComplete

if ($LASTEXITCODE -ne 0) {
    throw "patch_template_render_plan.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $filledPlan) `
    -Message "Filled render plan was not created."
Assert-True -Condition (Test-Path -LiteralPath $patchSummary) `
    -Message "Patch summary JSON was not created."

$filledPlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $filledPlan | ConvertFrom-Json
$patchSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $patchSummary | ConvertFrom-Json

Assert-Equal -Actual $patchSummaryObject.status -Expected "completed" `
    -Message "Patch summary did not report status=completed."
Assert-True -Condition ([bool]$patchSummaryObject.require_complete) `
    -Message "Patch summary did not record require_complete=true."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_text.requested -Expected 3 `
    -Message "Unexpected requested bookmark_text patch count."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_text.applied -Expected 3 `
    -Message "Unexpected applied bookmark_text patch count."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_paragraphs.requested -Expected 1 `
    -Message "Unexpected requested bookmark_paragraphs patch count."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_paragraphs.applied -Expected 1 `
    -Message "Unexpected applied bookmark_paragraphs patch count."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_table_rows.requested -Expected 1 `
    -Message "Unexpected requested bookmark_table_rows patch count."
Assert-Equal -Actual $patchSummaryObject.patch_counts.bookmark_table_rows.applied -Expected 1 `
    -Message "Unexpected applied bookmark_table_rows patch count."
Assert-Equal -Actual $patchSummaryObject.remaining_placeholder_count -Expected 0 `
    -Message "Filled render plan should not retain placeholders."

Assert-Equal -Actual $filledPlanObject.bookmark_text[0].text -Expected "上海羽文档科技有限公司" `
    -Message "Filled render plan did not patch customer_name."
Assert-Equal -Actual $filledPlanObject.bookmark_paragraphs[0].paragraphs[0] `
    -Expected "1. 当成品文档同时包含中文与英文标签时，建议显式保留 East Asia 字体设置。" `
    -Message "Filled render plan did not patch note_lines paragraphs."
Assert-Equal -Actual $filledPlanObject.bookmark_table_rows[0].rows[0][0] -Expected "需求梳理" `
    -Message "Filled render plan did not patch line_item_row rows."

& $renderScriptPath `
    -InputDocx $sampleDocx `
    -PlanPath $filledPlan `
    -OutputDocx $renderedDocx `
    -SummaryJson $renderSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document.ps1 failed for the patched invoice render plan."
}

$renderedDocumentXml = Read-DocxEntryText -DocxPath $renderedDocx -EntryName "word/document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "上海羽文档科技有限公司" -Label "Patched invoice document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "报价单-2026-0410" -Label "Patched invoice document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "2026年4月10日" -Label "Patched invoice document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "需求梳理" -Label "Patched invoice document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "文档生成" -Label "Patched invoice document.xml"
Assert-ContainsText -Text $renderedDocumentXml -ExpectedText "验收交付" -Label "Patched invoice document.xml"
Assert-NotContainsText -Text $renderedDocumentXml -UnexpectedText "TODO:" -Label "Patched invoice document.xml"

$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility_fixture.docx"
$visibilityDraftPlan = Join-Path $resolvedWorkingDir "block_visibility.render-plan.draft.json"
$visibilityFilledPlan = Join-Path $resolvedWorkingDir "block_visibility.render-plan.filled.json"
$visibilityPatchPath = Join-Path $resolvedWorkingDir "block_visibility.render_patch.json"
$visibilityPatchSummary = Join-Path $resolvedWorkingDir "block_visibility.render-plan.patch.summary.json"
$visibilityRenderedDocx = Join-Path $resolvedWorkingDir "block_visibility.rendered.docx"
$visibilityRenderSummary = Join-Path $resolvedWorkingDir "block_visibility.rendered.summary.json"

& $blockVisibilitySampleExecutable $visibilityFixtureDocx
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_bookmark_block_visibility_visual failed."
}

& $exportScriptPath `
    -InputDocx $visibilityFixtureDocx `
    -OutputPlan $visibilityDraftPlan `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "export_template_render_plan.ps1 failed for the visibility fixture."
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

& $patchScriptPath `
    -BasePlanPath $visibilityDraftPlan `
    -PatchPlanPath $visibilityPatchPath `
    -OutputPlan $visibilityFilledPlan `
    -SummaryJson $visibilityPatchSummary

if ($LASTEXITCODE -ne 0) {
    throw "patch_template_render_plan.ps1 failed for the visibility fixture."
}

$visibilityPlanObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilityFilledPlan | ConvertFrom-Json
$hideBlockEntry = @($visibilityPlanObject.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "hide_block" })[0]
$keepBlockEntry = @($visibilityPlanObject.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "keep_block" })[0]

Assert-True -Condition ($null -ne $hideBlockEntry) -Message "Missing hide_block entry in filled visibility plan."
Assert-True -Condition ($null -ne $keepBlockEntry) -Message "Missing keep_block entry in filled visibility plan."
Assert-True -Condition (-not [bool]$hideBlockEntry.visible) -Message "hide_block should be patched to visible=false."
Assert-True -Condition ([bool]$keepBlockEntry.visible) -Message "keep_block should stay visible=true."

& $renderScriptPath `
    -InputDocx $visibilityFixtureDocx `
    -PlanPath $visibilityFilledPlan `
    -OutputDocx $visibilityRenderedDocx `
    -SummaryJson $visibilityRenderSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document.ps1 failed for the patched visibility render plan."
}

$visibilityDocumentXml = Read-DocxEntryText -DocxPath $visibilityRenderedDocx -EntryName "word/document.xml"
Assert-ContainsText -Text $visibilityDocumentXml -ExpectedText "Keep me" -Label "Patched visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Hide me" -Label "Patched visibility document.xml"
Assert-NotContainsText -Text $visibilityDocumentXml -UnexpectedText "Secret Cell" -Label "Patched visibility document.xml"

Write-Host "Template render-plan patch regression passed."
