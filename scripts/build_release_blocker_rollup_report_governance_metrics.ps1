function Get-ReportIdFromSchema {
    param([string]$SourceSchema)

    switch ($SourceSchema) {
        "featherdoc.numbering_catalog_governance_report.v1" { return "numbering_catalog_governance" }
        "featherdoc.table_layout_delivery_governance_report.v1" { return "table_layout_delivery_governance" }
        default { return "" }
    }
}

function Get-GovernanceMetricByContract {
    param(
        $Metrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($Metrics | Where-Object {
        [string]$_.metric -eq $Metric -and
        [string]$_.id -eq $Id -and
        [string]$_.report_id -eq $ReportId -and
        [string]$_.source_schema -eq $SourceSchema
    }) | Select-Object -First 1
}

function Add-GovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        $Metric
    )

    $details = Get-JsonProperty -Object $Metric -Name "details"
    if ($null -eq $details) { return }

    $detailFields = @(
        "document_count",
        "catalog_exemplar_count",
        "baseline_entry_count",
        "matched_document_count",
        "unmatched_catalog_document_count",
        "unmatched_baseline_document_count",
        "alignment_gap_count",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "coverage_score",
        "ready_document_count",
        "ready_document_percent",
        "needs_review_document_count",
        "failed_document_count",
        "table_style_issue_count",
        "automatic_tblLook_fix_count",
        "manual_table_style_fix_count",
        "table_position_automatic_count",
        "table_position_review_count",
        "pdf_floating_table_supported_geometry_count",
        "pdf_floating_table_metadata_only_count",
        "pdf_floating_table_tracked_geometry_count",
        "pdf_floating_table_supported_geometry_percent",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "metadata_only_fields",
        "review_required_fields",
        "command_failure_count",
        "unresolved_item_count"
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-JsonProperty -Object $details -Name $fieldName
        if ($null -eq $value) { continue }
        if (($value -is [System.Collections.IEnumerable]) -and -not ($value -is [string])) {
            $values = @($value |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
            if ($values.Count -gt 0) {
                $detailParts.Add("$fieldName=$($values -join ',')") | Out-Null
            }
            continue
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
            $detailParts.Add("$fieldName=$value") | Out-Null
        }
    }
    foreach ($fieldName in @("catalog_document_keys", "baseline_document_keys", "matched_document_keys")) {
        $values = @(Get-JsonArray -Object $details -Name $fieldName)
        if ($values.Count -gt 0) {
            $detailParts.Add("$fieldName=$($values -join ',')") | Out-Null
        }
    }
    if ($detailParts.Count -gt 0) {
        $Lines.Add("  - details: ``$($detailParts -join ', ')``") | Out-Null
    }

    $pdfFloatingTableSupportedGeometryCount = Get-JsonInt -Object $details -Name "pdf_floating_table_supported_geometry_count"
    $pdfFloatingTableMetadataOnlyCount = Get-JsonInt -Object $details -Name "pdf_floating_table_metadata_only_count"
    $pdfFloatingTableTrackedGeometryCount = Get-JsonInt -Object $details -Name "pdf_floating_table_tracked_geometry_count"
    $pdfFloatingTableSupportedGeometryPercent = Get-JsonInt -Object $details -Name "pdf_floating_table_supported_geometry_percent"
    $pdfFloatingTableSupportCoverage = Get-JsonString -Object $details -Name "pdf_floating_table_support_coverage"
    $pdfFloatingTableReviewerFocus = Get-JsonString -Object $details -Name "pdf_floating_table_reviewer_focus"
    if ($pdfFloatingTableTrackedGeometryCount -gt 0 -or -not [string]::IsNullOrWhiteSpace($pdfFloatingTableSupportCoverage)) {
        if ([string]::IsNullOrWhiteSpace($pdfFloatingTableSupportCoverage)) {
            $pdfFloatingTableSupportCoverage = "$pdfFloatingTableSupportedGeometryCount/$pdfFloatingTableTrackedGeometryCount supported ($pdfFloatingTableSupportedGeometryPercent%); metadata_only=$pdfFloatingTableMetadataOnlyCount"
        }
        $Lines.Add("  - pdf_floating_table_support_coverage: ``$pdfFloatingTableSupportCoverage``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($pdfFloatingTableReviewerFocus)) {
            $Lines.Add("  - pdf_floating_table_reviewer_focus: ``$pdfFloatingTableReviewerFocus``") | Out-Null
        } elseif ($pdfFloatingTableSupportedGeometryPercent -lt 100) {
            $Lines.Add("  - pdf_floating_table_reviewer_focus: review metadata-only ``tblpPr`` fields before approving PDF-layout-sensitive release.") | Out-Null
        }
        $pdfFloatingTableMetadataOnlyFields = @(Get-JsonArray -Object $details -Name "pdf_floating_table_metadata_only_fields" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($pdfFloatingTableMetadataOnlyFields.Count -gt 0) {
            $Lines.Add("  - pdf_floating_table_metadata_only_fields: ``$($pdfFloatingTableMetadataOnlyFields -join ', ')``") | Out-Null
        }
        $pdfFloatingTableReviewRequiredFields = @(Get-JsonArray -Object $details -Name "pdf_floating_table_review_required_fields" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($pdfFloatingTableReviewRequiredFields.Count -gt 0) {
            $Lines.Add("  - pdf_floating_table_review_required_fields: ``$($pdfFloatingTableReviewRequiredFields -join ', ')``") | Out-Null
        }
        $metadataOnlyFields = @(Get-JsonArray -Object $details -Name "metadata_only_fields" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($metadataOnlyFields.Count -gt 0) {
            $Lines.Add("  - metadata_only_fields: ``$($metadataOnlyFields -join ', ')``") | Out-Null
        }
        $reviewRequiredFields = @(Get-JsonArray -Object $details -Name "review_required_fields" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($reviewRequiredFields.Count -gt 0) {
            $Lines.Add("  - review_required_fields: ``$($reviewRequiredFields -join ', ')``") | Out-Null
        }
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-JsonArray -Object $details -Name "penalty_summary")) {
        $factor = Get-JsonString -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) { continue }
        $count = Get-JsonProperty -Object $penalty -Name "count"
        $penaltyValue = Get-JsonProperty -Object $penalty -Name "penalty"
        $penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)") | Out-Null
    }
    if ($penaltyParts.Count -gt 0) {
        $Lines.Add("  - penalty_summary: ``$($penaltyParts -join '; ')``") | Out-Null
    }
}

function New-GovernanceMetrics {
    param(
        $Summary,
        [string]$SourceSchema,
        [string]$SourceReport,
        [string]$SourceReportDisplay
    )

    $metrics = New-Object 'System.Collections.Generic.List[object]'
    $reportId = Get-ReportIdFromSchema -SourceSchema $SourceSchema

    if ($SourceSchema -eq "featherdoc.numbering_catalog_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "real_corpus_confidence_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "real_corpus_confidence_level")))) {
        $metrics.Add([ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = $reportId
            source_schema = $SourceSchema
            source_report = $SourceReportDisplay
            source_report_display = $SourceReportDisplay
            source_json = $SourceReportDisplay
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "real_corpus_confidence_score"
            level = Get-JsonString -Object $Summary -Name "real_corpus_confidence_level"
            details = Get-JsonProperty -Object $Summary -Name "real_corpus_confidence"
        }) | Out-Null
    }

    if ($SourceSchema -eq "featherdoc.table_layout_delivery_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "delivery_quality_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "delivery_quality_level")))) {
        $metrics.Add([ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = $reportId
            source_schema = $SourceSchema
            source_report = $SourceReportDisplay
            source_report_display = $SourceReportDisplay
            source_json = $SourceReportDisplay
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "delivery_quality_score"
            level = Get-JsonString -Object $Summary -Name "delivery_quality_level"
            details = Get-JsonProperty -Object $Summary -Name "delivery_quality"
        }) | Out-Null
    }

    return @($metrics.ToArray())
}
