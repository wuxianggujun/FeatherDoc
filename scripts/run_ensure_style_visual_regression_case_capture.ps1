    $paragraphJsonPath = Join-Path $caseDir "inspect-review-para-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "1", "--json") `
        -OutputPath $paragraphJsonPath `
        -FailureMessage "Failed to inspect ReviewPara paragraph for case '$($case.id)'."
    Assert-ParagraphState `
        -JsonPath $paragraphJsonPath `
        -ExpectedIndex 1 `
        -ExpectedText $paragraphTargetText `
        -ExpectedStyleId "ReviewPara" `
        -Label "$($case.id)-review-para"

    $inheritedParagraphJsonPath = Join-Path $caseDir "inspect-review-para-child-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-paragraphs", $mutatedDocxPath, "--paragraph", "2", "--json") `
        -OutputPath $inheritedParagraphJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild paragraph for case '$($case.id)'."
    Assert-ParagraphState `
        -JsonPath $inheritedParagraphJsonPath `
        -ExpectedIndex 2 `
        -ExpectedText $inheritedParagraphTargetText `
        -ExpectedStyleId "ReviewParaChild" `
        -Label "$($case.id)-review-para-child"

    $runJsonPath = Join-Path $caseDir "inspect-accent-marker-run.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-runs", $mutatedDocxPath, "3", "--run", "1", "--json") `
        -OutputPath $runJsonPath `
        -FailureMessage "Failed to inspect AccentMarker run for case '$($case.id)'."
    Assert-RunState `
        -JsonPath $runJsonPath `
        -ExpectedParagraphIndex 3 `
        -ExpectedRunIndex 1 `
        -ExpectedText $runTargetText `
        -ExpectedStyleId "AccentMarker" `
        -Label "$($case.id)-accent-marker-run"

    $inheritedRunJsonPath = Join-Path $caseDir "inspect-accent-marker-child-run.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-runs", $mutatedDocxPath, "4", "--run", "1", "--json") `
        -OutputPath $inheritedRunJsonPath `
        -FailureMessage "Failed to inspect AccentMarkerChild run for case '$($case.id)'."
    Assert-RunState `
        -JsonPath $inheritedRunJsonPath `
        -ExpectedParagraphIndex 4 `
        -ExpectedRunIndex 1 `
        -ExpectedText $inheritedRunTargetText `
        -ExpectedStyleId "AccentMarkerChild" `
        -Label "$($case.id)-accent-marker-child-run"

    $reviewTableJsonPath = Join-Path $caseDir "inspect-review-table.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-tables", $mutatedDocxPath, "--table", "0", "--json") `
        -OutputPath $reviewTableJsonPath `
        -FailureMessage "Failed to inspect ReviewTable target for case '$($case.id)'."
    Assert-TableState `
        -JsonPath $reviewTableJsonPath `
        -ExpectedIndex 0 `
        -ExpectedStyleId $reviewTableStyleId `
        -ExpectedRowCount 3 `
        -ExpectedColumnCount 2 `
        -Label "$($case.id)-review-table"

    $reviewParaStyleJsonPath = Join-Path $caseDir "inspect-review-para-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "ReviewPara", "--json") `
        -OutputPath $reviewParaStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewPara style for case '$($case.id)'."

    $reviewParaChildStyleJsonPath = Join-Path $caseDir "inspect-review-para-child-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "ReviewParaChild", "--json") `
        -OutputPath $reviewParaChildStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild style for case '$($case.id)'."

    $accentMarkerStyleJsonPath = Join-Path $caseDir "inspect-accent-marker-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "AccentMarker", "--json") `
        -OutputPath $accentMarkerStyleJsonPath `
        -FailureMessage "Failed to inspect AccentMarker style for case '$($case.id)'."

    $accentMarkerChildStyleJsonPath = Join-Path $caseDir "inspect-accent-marker-child-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", "AccentMarkerChild", "--json") `
        -OutputPath $accentMarkerChildStyleJsonPath `
        -FailureMessage "Failed to inspect AccentMarkerChild style for case '$($case.id)'."

    $reviewTableStyleJsonPath = Join-Path $caseDir "inspect-review-table-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-styles", $mutatedDocxPath, "--style", $reviewTableStyleId, "--json") `
        -OutputPath $reviewTableStyleJsonPath `
        -FailureMessage "Failed to inspect ReviewTable style for case '$($case.id)'."

    $reviewParaChildInheritanceJsonPath = Join-Path $caseDir "inspect-review-para-child-inheritance.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-style-inheritance", $mutatedDocxPath, "ReviewParaChild", "--json") `
        -OutputPath $reviewParaChildInheritanceJsonPath `
        -FailureMessage "Failed to inspect ReviewParaChild inheritance for case '$($case.id)'."

    $accentMarkerChildInheritanceJsonPath = Join-Path $caseDir "inspect-accent-marker-child-inheritance.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @("inspect-style-inheritance", $mutatedDocxPath, "AccentMarkerChild", "--json") `
        -OutputPath $accentMarkerChildInheritanceJsonPath `
        -FailureMessage "Failed to inspect AccentMarkerChild inheritance for case '$($case.id)'."

    $reviewParaXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "ReviewPara"
    $reviewParaChildXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "ReviewParaChild"
    $accentMarkerXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "AccentMarker"
    $accentMarkerChildXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "AccentMarkerChild"
    $reviewTableXmlState = Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId $reviewTableStyleId
