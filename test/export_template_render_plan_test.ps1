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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\export_template_render_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$sampleOutput = Join-Path $resolvedWorkingDir "invoice.render-plan.json"
$sampleSummary = Join-Path $resolvedWorkingDir "invoice.render-plan.summary.json"
$blockVisibilitySampleExecutable = Find-ExecutableByName `
    -SearchRoot $resolvedBuildDir `
    -TargetName "featherdoc_sample_bookmark_block_visibility_visual"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $scriptPath `
    -InputDocx $sampleDocx `
    -OutputPlan $sampleOutput `
    -SummaryJson $sampleSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "export_template_render_plan.ps1 failed for the sample invoice template."
}

Assert-True -Condition (Test-Path -LiteralPath $sampleOutput) `
    -Message "Sample render-plan output was not created."
Assert-True -Condition (Test-Path -LiteralPath $sampleSummary) `
    -Message "Sample render-plan summary was not created."

$samplePlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleOutput | ConvertFrom-Json
$samplePlanSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sampleSummary | ConvertFrom-Json

Assert-Equal -Actual $samplePlanSummary.status -Expected "completed" `
    -Message "Sample render-plan export did not report status=completed."
Assert-Equal -Actual $samplePlanSummary.plan_counts.bookmark_text -Expected 3 `
    -Message "Unexpected bookmark_text count in sample render-plan summary."
Assert-Equal -Actual $samplePlanSummary.plan_counts.bookmark_paragraphs -Expected 1 `
    -Message "Unexpected bookmark_paragraphs count in sample render-plan summary."
Assert-Equal -Actual $samplePlanSummary.plan_counts.bookmark_table_rows -Expected 1 `
    -Message "Unexpected bookmark_table_rows count in sample render-plan summary."
Assert-Equal -Actual $samplePlanSummary.plan_counts.bookmark_block_visibility -Expected 0 `
    -Message "Unexpected bookmark_block_visibility count in sample render-plan summary."
Assert-Equal -Actual $samplePlanSummary.unsupported_bookmark_count -Expected 0 `
    -Message "Sample render-plan export should not skip any bookmark."
Assert-True -Condition ([string]$samplePlan.'$schema' -match 'template_render_plan\.schema\.json$') `
    -Message "Sample render-plan output did not reference the render-plan schema."

$customerEntry = @($samplePlan.bookmark_text | Where-Object { $_.bookmark_name -eq "customer_name" })[0]
$invoiceEntry = @($samplePlan.bookmark_text | Where-Object { $_.bookmark_name -eq "invoice_number" })[0]
$dateEntry = @($samplePlan.bookmark_text | Where-Object { $_.bookmark_name -eq "issue_date" })[0]
$notesEntry = @($samplePlan.bookmark_paragraphs | Where-Object { $_.bookmark_name -eq "note_lines" })[0]
$rowsEntry = @($samplePlan.bookmark_table_rows | Where-Object { $_.bookmark_name -eq "line_item_row" })[0]

Assert-True -Condition ($null -ne $customerEntry) -Message "Missing customer_name entry in sample render-plan."
Assert-True -Condition ($null -ne $invoiceEntry) -Message "Missing invoice_number entry in sample render-plan."
Assert-True -Condition ($null -ne $dateEntry) -Message "Missing issue_date entry in sample render-plan."
Assert-True -Condition ($null -ne $notesEntry) -Message "Missing note_lines entry in sample render-plan."
Assert-True -Condition ($null -ne $rowsEntry) -Message "Missing line_item_row entry in sample render-plan."

Assert-Equal -Actual $customerEntry.part -Expected "body" `
    -Message "customer_name should export as a body bookmark."
Assert-Equal -Actual $customerEntry.text -Expected "TODO: customer_name" `
    -Message "customer_name draft text placeholder is incorrect."
Assert-Equal -Actual $invoiceEntry.text -Expected "TODO: invoice_number" `
    -Message "invoice_number draft text placeholder is incorrect."
Assert-Equal -Actual $dateEntry.text -Expected "TODO: issue_date" `
    -Message "issue_date draft text placeholder is incorrect."
Assert-Equal -Actual @($notesEntry.paragraphs).Count -Expected 1 `
    -Message "note_lines should export one draft paragraph."
Assert-Equal -Actual $notesEntry.paragraphs[0] -Expected "TODO: note_lines" `
    -Message "note_lines draft paragraph placeholder is incorrect."
Assert-Equal -Actual @($rowsEntry.rows).Count -Expected 0 `
    -Message "line_item_row should export with an empty rows draft."

$visibilityFixtureDocx = Join-Path $resolvedWorkingDir "block_visibility_fixture.docx"
$visibilityOutput = Join-Path $resolvedWorkingDir "block_visibility.render-plan.json"
$visibilitySummary = Join-Path $resolvedWorkingDir "block_visibility.render-plan.summary.json"

& $blockVisibilitySampleExecutable $visibilityFixtureDocx
if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_sample_bookmark_block_visibility_visual failed."
}

& $scriptPath `
    -InputDocx $visibilityFixtureDocx `
    -OutputPlan $visibilityOutput `
    -SummaryJson $visibilitySummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "export_template_render_plan.ps1 failed for the visibility fixture."
}

Assert-True -Condition (Test-Path -LiteralPath $visibilityOutput) `
    -Message "Visibility render-plan output was not created."
Assert-True -Condition (Test-Path -LiteralPath $visibilitySummary) `
    -Message "Visibility render-plan summary was not created."

$visibilityPlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilityOutput | ConvertFrom-Json
$visibilityPlanSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $visibilitySummary | ConvertFrom-Json

Assert-Equal -Actual $visibilityPlanSummary.plan_counts.bookmark_text -Expected 0 `
    -Message "Visibility fixture should not export text bookmarks."
Assert-Equal -Actual $visibilityPlanSummary.plan_counts.bookmark_paragraphs -Expected 0 `
    -Message "Visibility fixture should not export paragraph bookmarks."
Assert-Equal -Actual $visibilityPlanSummary.plan_counts.bookmark_table_rows -Expected 0 `
    -Message "Visibility fixture should not export table-row bookmarks."
Assert-Equal -Actual $visibilityPlanSummary.plan_counts.bookmark_block_visibility -Expected 2 `
    -Message "Visibility fixture should export two block-visibility entries."

$keepEntry = @($visibilityPlan.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "keep_block" })[0]
$hideEntry = @($visibilityPlan.bookmark_block_visibility | Where-Object { $_.bookmark_name -eq "hide_block" })[0]
Assert-True -Condition ($null -ne $keepEntry) -Message "Missing keep_block visibility entry."
Assert-True -Condition ($null -ne $hideEntry) -Message "Missing hide_block visibility entry."
Assert-True -Condition ([bool]$keepEntry.visible) -Message "keep_block should default to visible=true."
Assert-True -Condition ([bool]$hideEntry.visible) -Message "hide_block should default to visible=true in the draft."

Write-Host "Template render-plan export regression passed."
