# append-row-selector
$caseId = "append-row-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$appendDocx = Join-Path $caseDir "append.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$appendJson = Join-Path $caseDir "append.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("append-template-table-row", $baselineDocx) + $selectorArgs + @("--output", $appendDocx, "--json")) `
    -OutputPath $appendJson `
    -FailureMessage "Append row selector mutation failed."
$appendResult = Get-Content -Raw -LiteralPath $appendJson | ConvertFrom-Json
$tableIndex = [int]$appendResult.table_index
$rowIndex = if ($appendResult.PSObject.Properties.Name -contains "row_index") { [int]$appendResult.row_index } else { 3 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $appendDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "West direct", "--cell", "30", "--cell", "300", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Append row selector fill failed."

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") `
    -OutputPath $retainedRowsJson `
    -FailureMessage "Append row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") `
    -OutputPath $targetRowsJson `
    -FailureMessage "Append row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100"),
    @("South seed", "20", "200"),
    @("West direct", "30", "300")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($appendJson, $fillJson, $retainedRowsJson, $targetRowsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocx `
        -OutputDir $mutatedVisualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
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

# merge-right-selector
$caseId = "merge-right-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$mergeJson = Join-Path $caseDir "merge.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $baselineDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mutatedDocx, "--json")) -OutputPath $mergeJson -FailureMessage "Merge selector mutation failed."
$mergeResult = Get-Content -Raw -LiteralPath $mergeJson | ConvertFrom-Json
$tableIndex = [int]$mergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Merge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Merge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 2; text = "North seed"; width_twips = 4200 },
    [ordered]@{ cell_index = 1; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($mergeJson, $retainedRowsJson, $targetCellsJson)
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

# merge-right-explicit-break-selector-visual
$caseId = "merge-right-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged baseline for merge explicit-break selector case."
$prepareResult = Get-Content -Raw -LiteralPath $prepareJson | ConvertFrom-Json
$tableIndex = [int]$prepareResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $baselineDocx, $tableIndex.ToString(), "1", "0", "--text", "North seed`npending approval", "--output", $mutatedDocx, "--json") -OutputPath $mutationJson -FailureMessage "Set cell-text after merge selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text after merge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Set cell-text after merge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 2; text = "North seed`npending approval"; width_twips = 4200 },
    [ordered]@{ cell_index = 1; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $mutationJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# unmerge-right-selector
$caseId = "unmerge-right-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$unmergeJson = Join-Path $caseDir "unmerge.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged baseline for unmerge selector case."
Invoke-Capture -Executable $cliExecutable -Arguments (@("unmerge-template-table-cells", $baselineDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mutatedDocx, "--json")) -OutputPath $unmergeJson -FailureMessage "Unmerge selector mutation failed."
$unmergeResult = Get-Content -Raw -LiteralPath $unmergeJson | ConvertFrom-Json
$tableIndex = [int]$unmergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Unmerge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Unmerge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 1; text = "North seed"; width_twips = 2600 },
    [ordered]@{ cell_index = 1; column_index = 1; column_span = 1; text = ""; width_twips = 1600 },
    [ordered]@{ cell_index = 2; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $unmergeJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# unmerge-right-explicit-break-selector-visual
$caseId = "unmerge-right-explicit-break-selector-visual"
$caseDir = Join-Path $resolvedOutputDir $caseId
$sourceDocx = Join-Path $caseDir "source.docx"
$mergedDocx = Join-Path $caseDir "merged.docx"
$baselineDocx = Join-Path $caseDir "baseline.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$prepareJson = Join-Path $caseDir "prepare-merge.json"
$unmergeJson = Join-Path $caseDir "prepare-unmerge.json"
$mutationJson = Join-Path $caseDir "mutation.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetCellsJson = Join-Path $caseDir "target-cells.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$baselineVisualDir = Join-Path $caseDir "baseline-visual"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $sourceDocx -Force

Invoke-Capture -Executable $cliExecutable -Arguments (@("merge-template-table-cells", $sourceDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $mergedDocx, "--json")) -OutputPath $prepareJson -FailureMessage "Failed to prepare merged document for unmerge explicit-break selector case."
Invoke-Capture -Executable $cliExecutable -Arguments (@("unmerge-template-table-cells", $mergedDocx) + $selectorArgs + @("1", "0", "--direction", "right", "--output", $baselineDocx, "--json")) -OutputPath $unmergeJson -FailureMessage "Failed to prepare unmerged baseline for explicit-break selector case."
$unmergeResult = Get-Content -Raw -LiteralPath $unmergeJson | ConvertFrom-Json
$tableIndex = [int]$unmergeResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $baselineDocx, $tableIndex.ToString(), "1", "1", "--text", "10 units`nhold", "--output", $mutatedDocx, "--json") -OutputPath $mutationJson -FailureMessage "Set cell-text after unmerge selector mutation failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Set cell-text after unmerge selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-cells", $mutatedDocx, $tableIndex.ToString(), "--row", "1", "--json") -OutputPath $targetCellsJson -FailureMessage "Set cell-text after unmerge selector target-table inspect failed."
Assert-Cells -JsonPath $targetCellsJson -ExpectedCells @(
    [ordered]@{ cell_index = 0; column_index = 0; column_span = 1; text = "North seed"; width_twips = 2600 },
    [ordered]@{ cell_index = 1; column_index = 1; column_span = 1; text = "10 units`nhold"; width_twips = 1600 },
    [ordered]@{ cell_index = 2; column_index = 2; column_span = 1; text = "100"; width_twips = 2200 }
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($prepareJson, $unmergeJson, $mutationJson, $retainedRowsJson, $targetCellsJson)
}

if (-not $SkipVisual) {
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $baselineDocx -OutputDir $baselineVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $mutatedDocx -OutputDir $mutatedVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent
    $baselinePage = Join-Path $baselineVisualDir "evidence\pages\page-01.png"
    $mutatedPage = Join-Path $mutatedVisualDir "evidence\pages\page-01.png"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $contactSheet -Images @($baselinePage, $mutatedPage) -Labels @("$caseId-baseline", "$caseId-mutated")
    Register-VisualPair -CaseId $caseId -BaselinePage $baselinePage -MutatedPage $mutatedPage
    $caseSummary.visual = [ordered]@{
        baseline_visual_output_dir = $baselineVisualDir
        mutated_visual_output_dir = $mutatedVisualDir
        before_after_contact_sheet = $contactSheet
    }
}

$summary.cases += $caseSummary

# insert-column-before-selector
$caseId = "insert-column-before-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$stepHeaderDocx = Join-Path $caseDir "step-header.docx"
$stepRow1Docx = Join-Path $caseDir "step-row-1.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$setHeaderJson = Join-Path $caseDir "set-header.json"
$setRow1Json = Join-Path $caseDir "set-row-1.json"
$setRow2Json = Join-Path $caseDir "set-row-2.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-column-before", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert column selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $insertDocx, $tableIndex.ToString(), "0", "1", "--text", "Status", "--output", $stepHeaderDocx, "--json") -OutputPath $setHeaderJson -FailureMessage "Failed to set selector column header."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepHeaderDocx, $tableIndex.ToString(), "1", "1", "--text", "Open", "--output", $stepRow1Docx, "--json") -OutputPath $setRow1Json -FailureMessage "Failed to set selector row 1 column."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepRow1Docx, $tableIndex.ToString(), "2", "1", "--text", "Closed", "--output", $mutatedDocx, "--json") -OutputPath $setRow2Json -FailureMessage "Failed to set selector row 2 column."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert column selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert column selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Status", "Qty", "Amount"),
    @("North seed", "Open", "10", "100"),
    @("South seed", "Closed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $setHeaderJson, $setRow1Json, $setRow2Json, $retainedRowsJson, $targetRowsJson)
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

# insert-column-after-selector
$caseId = "insert-column-after-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$stepHeaderDocx = Join-Path $caseDir "step-header.docx"
$stepRow1Docx = Join-Path $caseDir "step-row-1.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$setHeaderJson = Join-Path $caseDir "set-header.json"
$setRow1Json = Join-Path $caseDir "set-row-1.json"
$setRow2Json = Join-Path $caseDir "set-row-2.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-column-after", $baselineDocx) + $selectorArgs + @("0", "1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert column-after selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index

Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $insertDocx, $tableIndex.ToString(), "0", "2", "--text", "Forecast", "--output", $stepHeaderDocx, "--json") -OutputPath $setHeaderJson -FailureMessage "Failed to set selector column-after header."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepHeaderDocx, $tableIndex.ToString(), "1", "2", "--text", "110", "--output", $stepRow1Docx, "--json") -OutputPath $setRow1Json -FailureMessage "Failed to set selector column-after row 1."
Invoke-Capture -Executable $cliExecutable -Arguments @("set-template-table-cell-text", $stepRow1Docx, $tableIndex.ToString(), "2", "2", "--text", "220", "--output", $mutatedDocx, "--json") -OutputPath $setRow2Json -FailureMessage "Failed to set selector column-after row 2."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert column-after selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert column-after selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Forecast", "Amount"),
    @("North seed", "10", "110", "100"),
    @("South seed", "20", "220", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $setHeaderJson, $setRow1Json, $setRow2Json, $retainedRowsJson, $targetRowsJson)
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

# insert-row-before-selector
$caseId = "insert-row-before-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-row-before", $baselineDocx) + $selectorArgs + @("1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert row selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index
$rowIndex = if ($insertResult.PSObject.Properties.Name -contains "inserted_row_index") { [int]$insertResult.inserted_row_index } else { 1 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $insertDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "Central direct", "--cell", "15", "--cell", "150", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Insert row selector fill failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert row selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert row selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("Central direct", "15", "150"),
    @("North seed", "10", "100"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $fillJson, $retainedRowsJson, $targetRowsJson)
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

# insert-row-after-selector
$caseId = "insert-row-after-selector"
$caseDir = Join-Path $resolvedOutputDir $caseId
$baselineDocx = Join-Path $caseDir "baseline.docx"
$insertDocx = Join-Path $caseDir "insert.docx"
$mutatedDocx = Join-Path $caseDir "mutated.docx"
$insertJson = Join-Path $caseDir "insert.json"
$fillJson = Join-Path $caseDir "fill.json"
$retainedRowsJson = Join-Path $caseDir "retained-rows.json"
$targetRowsJson = Join-Path $caseDir "target-rows.json"
$contactSheet = Join-Path $caseDir "before_after_contact_sheet.png"
$mutatedVisualDir = Join-Path $caseDir "mutated-visual"
New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
Copy-Item -Path $sharedBaselineDocxPath -Destination $baselineDocx -Force

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments (@("insert-template-table-row-after", $baselineDocx) + $selectorArgs + @("1", "--output", $insertDocx, "--json")) `
    -OutputPath $insertJson `
    -FailureMessage "Insert row-after selector mutation failed."
$insertResult = Get-Content -Raw -LiteralPath $insertJson | ConvertFrom-Json
$tableIndex = [int]$insertResult.table_index
$rowIndex = if ($insertResult.PSObject.Properties.Name -contains "inserted_row_index") { [int]$insertResult.inserted_row_index } else { 2 }

Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("set-template-table-row-texts", $insertDocx, $tableIndex.ToString(), $rowIndex.ToString(), "--row", "Follow-up direct", "--cell", "16", "--cell", "160", "--output", $mutatedDocx, "--json") `
    -OutputPath $fillJson `
    -FailureMessage "Insert row-after selector fill failed."

Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, "0", "--json") -OutputPath $retainedRowsJson -FailureMessage "Insert row-after selector retained-table inspect failed."
Assert-Rows -JsonPath $retainedRowsJson -ExpectedRows $retainedRows -Label "$caseId retained"
Invoke-Capture -Executable $cliExecutable -Arguments @("inspect-template-table-rows", $mutatedDocx, $tableIndex.ToString(), "--json") -OutputPath $targetRowsJson -FailureMessage "Insert row-after selector target-table inspect failed."
Assert-Rows -JsonPath $targetRowsJson -ExpectedRows @(
    @("Region", "Qty", "Amount"),
    @("North seed", "10", "100"),
    @("Follow-up direct", "16", "160"),
    @("South seed", "20", "200")
) -Label "$caseId target"

$caseSummary = [ordered]@{
    id = $caseId
    baseline_docx = $baselineDocx
    mutated_docx = $mutatedDocx
    artifacts = @($insertJson, $fillJson, $retainedRowsJson, $targetRowsJson)
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

