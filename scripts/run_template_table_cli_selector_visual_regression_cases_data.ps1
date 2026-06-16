# remove-row-selector
$caseId = "remove-row-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("remove-template-table-row", $baselineDocx) + $selectorArgs + @("1", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Remove row selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Remove row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Remove row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-tables-from-json-batch-selector
$caseId = "set-tables-from-json-batch-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "operations": [
    {
      "header_cells": ["Retained", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["carry-right", "8", "80"]
      ]
    },
    {
      "after_text": "Bookmark paragraph immediately before the target table.",
      "header_cells": ["Region", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["Batch south", "23", "230"],
        ["Batch east", "45", "450"]
      ]
    }
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-tables-from-json", $baselineDocx, "--patch-file", $patchPath, "--output", $mutatedDocx, "--json") `
    -OutputPath $mutationJson `
    -FailureMessage "Set-tables-from-json batch selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-tables-from-json batch retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows @(
    @("Retained", "Qty", "Amount"),
    @("carry-right", "8", "80")
) -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "1", "--json") -OutputPath $targetRowsJson -FailureMessage "Set-tables-from-json batch target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("Batch south", "23", "230"),
    @("Batch east", "45", "450")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-tables-from-json-batch-longtext-selector-visual
$caseId = "set-tables-from-json-batch-longtext-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "operations": [
    {
      "header_cells": ["Retained", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        [
          "Carry-over items require archive review",
          "8 units pending approval",
          "80 total awaiting confirmation"
        ]
      ]
    },
    {
      "after_text": "Bookmark paragraph immediately before the target table.",
      "header_cells": ["Region", "Qty", "Amount"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        [
          "North region requires manual follow-up",
          "18 units pending approval",
          "240 total awaiting finance sign-off"
        ],
        [
          "South backlog schedules phased shipment",
          "27 units queued for packaging",
          "410 total under manager review"
        ]
      ]
    }
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-tables-from-json", $baselineDocx, "--patch-file", $patchPath, "--output", $mutatedDocx, "--json") `
    -OutputPath $mutationJson `
    -FailureMessage "Set-tables-from-json batch longtext selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-tables-from-json batch longtext retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows @(
    @("Retained", "Qty", "Amount"),
    @("Carry-over items require archive review", "8 units pending approval", "80 total awaiting confirmation")
) -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "1", "--json") -OutputPath $targetRowsJson -FailureMessage "Set-tables-from-json batch longtext target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# remove-column-selector
$caseId = "remove-column-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("remove-template-table-column", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Remove column selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Remove column selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Remove column selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Amount"),
    @("North seed", "100"),
    @("South seed", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-from-json-selector
$caseId = "set-from-json-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["West refreshed", "31", "310"],
    ["East refreshed", "44", "440"]
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-from-json", $baselineDocx) + $selectorArgs + @("--patch-file", $patchPath, "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set-from-json selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-from-json selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set-from-json selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("West refreshed", "31", "310"),
    @("East refreshed", "44", "440")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-from-json-longtext-selector-visual
$caseId = "set-from-json-longtext-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$patchPath = Join-Path $caseDir "patch.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

@'
{
  "mode": "rows",
  "start_row": 1,
  "rows": [
    [
      "North region requires manual follow-up",
      "18 units pending approval",
      "240 total awaiting finance sign-off"
    ],
    [
      "South backlog schedules phased shipment",
      "27 units queued for packaging",
      "410 total under manager review"
    ]
  ]
}
'@ | Set-Content -Path $patchPath -Encoding UTF8

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-from-json", $baselineDocx) + $selectorArgs + @("--patch-file", $patchPath, "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set-from-json longtext selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set-from-json longtext selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set-from-json longtext selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($patchPath, $mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-row-texts-selector-visual
$caseId = "set-row-texts-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-row-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "--row", "North region requires manual follow-up",
            "--cell", "18 units pending approval",
            "--cell", "240 total awaiting finance sign-off",
            "--row", "South backlog schedules phased shipment",
            "--cell", "27 units queued for packaging",
            "--cell", "410 total under manager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set row-texts selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set row-texts selector visual retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set row-texts selector visual target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North region requires manual follow-up", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South backlog schedules phased shipment", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-text-selector-visual
$caseId = "set-cell-text-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-text", $baselineDocx) + $selectorArgs + @(
            "1",
            "2",
            "--text", "100 total pending finance approval after manual audit",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-text selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text selector visual retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-text selector visual target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100 total pending finance approval after manual audit"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-text-explicit-break-selector-visual
$caseId = "set-cell-text-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-text", $baselineDocx) + $selectorArgs + @(
            "1",
            "2",
            "--text", "240 total`nfinance review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-text explicit-break selector visual mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text explicit-break selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-text explicit-break selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "240 total`nfinance review"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-multiline-selector-visual
$caseId = "set-cell-block-texts-multiline-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "1",
            "--row", "18 units pending approval", "--cell", "240 total awaiting finance sign-off",
            "--row", "27 units queued for packaging", "--cell", "410 total under manager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts multiline selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts multiline selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts multiline selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units pending approval", "240 total awaiting finance sign-off"),
    @("South seed", "27 units queued for packaging", "410 total under manager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-explicit-break-selector-visual
$caseId = "set-cell-block-texts-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "1",
            "--row", "18 units`npending approval", "--cell", "240 total`nfinance review",
            "--row", "27 units`npackaging queue", "--cell", "410 total`nmanager review",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts explicit-break selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts explicit-break selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts explicit-break selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units`npending approval", "240 total`nfinance review"),
    @("South seed", "27 units`npackaging queue", "410 total`nmanager review")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-left-block-selector-visual
$caseId = "set-cell-block-texts-left-block-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @(
            "1",
            "0",
            "--row", "North allocation awaiting warehouse audit", "--cell", "18 units held for approval",
            "--row", "South backlog routed to overflow storage", "--cell", "27 units queued for packaging",
            "--output", $mutatedDocx,
            "--json"
        )) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts left-block selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts left-block selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts left-block selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North allocation awaiting warehouse audit", "18 units held for approval", "100"),
    @("South backlog routed to overflow storage", "27 units queued for packaging", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# set-cell-block-texts-selector
$caseId = "set-cell-block-texts-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("set-template-table-cell-block-texts", $baselineDocx) + $selectorArgs + @("1", "1", "--row", "18 units", "--cell", "240 total", "--output", $mutatedDocx, "--json")) `
    -OutputPath $mutationJson `
    -FailureMessage "Set cell-block-texts selector mutation failed."
$mutationResult = Get-Content -Raw -LiteralPath $mutationJson | ConvertFrom-Json
$tableIndex = [int]$mutationResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-block-texts selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Set cell-block-texts selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "18 units", "240 total"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mutationJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($sharedBaselineFirstPage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $sharedBaselineFirstPage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $sharedBaselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary
