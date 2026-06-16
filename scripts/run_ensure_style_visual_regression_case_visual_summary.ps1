    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocxPath `
        -OutputDir $visualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    Assert-VisualPageCount -VisualOutputDir $visualDir -ExpectedCount 1 -Label $case.id

    $artifacts = Register-ComparisonArtifacts `
        -Python $renderPython `
        -ContactSheetScript $contactSheetScript `
        -AggregatePagesDir $aggregatePagesDir `
        -CaseOutputDir $caseDir `
        -CaseId $case.id `
        -Images @(
            (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $case.id),
            (Get-RenderedPagePath -VisualOutputDir $visualDir -PageNumber 1 -Label $case.id)
        ) `
        -Labels @("$($case.id) baseline-page-01", "$($case.id) mutated-page-01")

    $aggregateImages += $artifacts.selected_pages
    $aggregateLabels += $artifacts.labels
    $summaryCases += [ordered]@{
        id = $case.id
        command = $case.command
        mutation_steps = $mutationSteps
        source_docx = $baselineDocxPath
        mutated_docx = $mutatedDocxPath
        mutation_json = $mutationJsonPath
        inspect_review_paragraph_json = $paragraphJsonPath
        inspect_review_child_paragraph_json = $inheritedParagraphJsonPath
        inspect_accent_run_json = $runJsonPath
        inspect_accent_child_run_json = $inheritedRunJsonPath
        inspect_review_table_json = $reviewTableJsonPath
        inspect_review_style_json = $reviewParaStyleJsonPath
        inspect_review_child_style_json = $reviewParaChildStyleJsonPath
        inspect_review_child_inheritance_json = $reviewParaChildInheritanceJsonPath
        inspect_accent_style_json = $accentMarkerStyleJsonPath
        inspect_accent_child_style_json = $accentMarkerChildStyleJsonPath
        inspect_accent_child_inheritance_json = $accentMarkerChildInheritanceJsonPath
        inspect_review_table_style_json = $reviewTableStyleJsonPath
        review_para_styles_xml = $reviewParaXmlState
        review_para_child_styles_xml = $reviewParaChildXmlState
        accent_marker_styles_xml = $accentMarkerXmlState
        accent_marker_child_styles_xml = $accentMarkerChildXmlState
        review_table_styles_xml = $reviewTableXmlState
        expected_visual_cues = $case.expected_visual_cues
        visual_artifacts = $artifacts
    }
