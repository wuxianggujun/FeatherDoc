Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $aggregateContactSheetPath `
    -Images $aggregateImages `
    -Labels $aggregateLabels

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    shared_baseline_docx = $baselineDocxPath
    shared_baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page = (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label "shared-baseline")
    }
    shared_baseline_artifacts = [ordered]@{
        review_para_paragraph = $baselineParagraphJson
        review_para_child_paragraph = $baselineInheritedParagraphJson
        accent_marker_run = $baselineRunJson
        review_table = $baselineReviewTableJson
        review_para_style = $baselineReviewParaStyleJson
        review_para_child_style = $baselineReviewParaChildStyleJson
        review_para_child_inheritance = $baselineReviewParaChildInheritanceJson
        accent_marker_style = $baselineAccentMarkerStyleJson
        review_table_style = $baselineReviewTableStyleJson
        review_para_styles_xml = $baselineReviewParaXmlState
        review_para_child_styles_xml = $baselineReviewParaChildXmlState
        accent_marker_styles_xml = $baselineAccentMarkerXmlState
        review_table_styles_xml = $baselineReviewTableXmlState
    }
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed ensure-style visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
