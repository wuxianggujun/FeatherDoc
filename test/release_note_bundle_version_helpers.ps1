function Get-Utf8FileText {
    param([string]$Path)

    $encoding = New-Object System.Text.UTF8Encoding($false)
    return [System.IO.File]::ReadAllText($Path, $encoding)
}

function Get-Utf8FileLines {
    param([string]$Path)

    $encoding = New-Object System.Text.UTF8Encoding($false)
    return [System.IO.File]::ReadAllLines($Path, $encoding)
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Utf8FileText -Path $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw ("{0} does not contain expected text '{1}': {2}" -f $Label, $ExpectedText, $Path)
    }
}

function Assert-LineContainsAll {
    param(
        [string]$Path,
        [string[]]$ExpectedFragments,
        [string]$Label
    )

    $lines = Get-Utf8FileLines -Path $Path
    foreach ($line in $lines) {
        $normalizedLine = $line -replace '/', '\'
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($normalizedLine -notmatch [regex]::Escape($normalizedFragment)) {
                $matchesAllFragments = $false
                break
            }
        }

        if ($matchesAllFragments) {
            return
        }
    }

    throw ("{0} does not contain one line with all expected fragments: {1}" -f $Label, ($ExpectedFragments -join ", "))
}

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Path,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Label
    )

    $lines = Get-Utf8FileLines -Path $Path
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $blockLines = @($lines[$lineIndex])
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if ([string]::IsNullOrWhiteSpace($nextLine) -or
                $nextLine -match '^#{1,6}\s' -or
                ($nextLine -match '^\s*-\s*' -and $nextLine -notmatch '^\s{2,}-\s*')) {
                break
            }

            $blockLines += $nextLine
        }

        $block = ($blockLines -join "`n") -replace '/', '\'
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($block -notmatch [regex]::Escape($normalizedFragment)) {
                $matchesAllFragments = $false
                break
            }
        }

        if ($matchesAllFragments) {
            return
        }
    }

    throw ("{0} does not contain one Markdown list block anchored by '{1}' with all expected fragments: {2}" -f $Label, $Anchor, ($ExpectedFragments -join ", "))
}

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Utf8FileText -Path $Path
    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $content -match [regex]::Escape($UnexpectedText)) {
        throw ("{0} unexpectedly contains '{1}': {2}" -f $Label, $UnexpectedText, $Path)
    }
}

function Assert-FileHasNoBom {
    param(
        [string]$Path,
        [string]$Label
    )

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw ("{0} starts with a UTF-8 BOM: {1}" -f $Label, $Path)
    }
}

function Assert-ContentControlGovernanceTrace {
    param(
        [string]$Path,
        [string]$Label
    )

    $expectedFragments = @(
        'content_control_data_binding.bound_placeholder',
        'custom_xml_sync_evidence_missing',
        'action=sync_or_fill_bound_content_control',
        'repair_action_classes: release_blocking, auto_repair_candidate, manual_confirmation_required',
        'source_schema=featherdoc.content_control_data_binding_governance_report.v1',
        'source_report_display: .\output\content-control-data-binding-governance\summary.json',
        'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json',
        'input_docx_display: .\samples\invoice.docx',
        'input_docx: samples/invoice.docx',
        'template_name=invoice-template',
        'schema_target: invoice',
        'target_mode: resolved-section-targets',
        'repair_strategy: sync_bound_content_control',
        'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.',
        $contentControlCommandTemplateMarker,
        'review_duplicate_content_control_binding',
        'action=review_duplicate_content_control_binding',
        'repair_action_classes: manual_confirmation_required',
        'repair_strategy: deduplicate_or_confirm_shared_binding',
        'repair_hint: Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths.',
        $contentControlDuplicateActionCommandTemplateMarker,
        'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1'
    )

    foreach ($expectedFragment in $expectedFragments) {
        Assert-Contains -Path $Path -ExpectedText $expectedFragment -Label $Label
    }

    Assert-MarkdownListBlockContainsAll -Path $Path `
        -Anchor 'content_control_data_binding.bound_placeholder: action=sync_or_fill_bound_content_control' `
        -ExpectedFragments @(
            'source_schema=featherdoc.content_control_data_binding_governance_report.v1',
            'source_report_display: .\output\content-control-data-binding-governance\summary.json',
            'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json',
            'input_docx_display: .\samples\invoice.docx',
            'input_docx: samples/invoice.docx',
            'template_name=invoice-template',
            'schema_target: invoice',
            'target_mode: resolved-section-targets',
            'repair_action_classes: release_blocking, auto_repair_candidate, manual_confirmation_required',
            'repair_strategy: sync_bound_content_control',
            'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.',
            $contentControlCommandTemplateMarker
        ) -Label $Label

    Assert-MarkdownListBlockContainsAll -Path $Path `
        -Anchor 'review_duplicate_content_control_binding: action=review_duplicate_content_control_binding' `
        -ExpectedFragments @(
            'source_schema=featherdoc.content_control_data_binding_governance_report.v1',
            'source_report_display: .\output\content-control-data-binding-governance\summary.json',
            'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json',
            'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1',
            'input_docx_display: .\samples\invoice.docx',
            'input_docx: samples/invoice.docx',
            'template_name=invoice-template',
            'schema_target: invoice',
            'target_mode: resolved-section-targets',
            'repair_action_classes: manual_confirmation_required',
            'repair_strategy: deduplicate_or_confirm_shared_binding',
            'repair_hint: Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths.',
            $contentControlDuplicateActionCommandTemplateMarker
        ) -Label $Label
}

function New-NumberingGovernanceMetricFixture {
    return [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        source_report_display = ".\output\numbering-catalog-governance\summary.json"
        source_json_display = ".\output\numbering-catalog-governance\summary.json"
        score = 56
        level = "low"
        details = [ordered]@{
            score = 56
            level = "low"
            matched_document_count = 4
            unmatched_catalog_document_count = 0
            unmatched_baseline_document_count = 0
            alignment_gap_count = 0
            catalog_coverage_percent = 100
            baseline_coverage_percent = 100
            coverage_score = 100
            catalog_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
            baseline_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
            matched_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
            penalty_summary = @(
                [ordered]@{ factor = "style_numbering_issues"; count = 4; penalty = 20 }
            )
        }
    }
}

function New-TableLayoutDeliveryMetricFixture {
    return [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        source_report_display = ".\output\table-layout-delivery-governance\summary.json"
        source_json_display = ".\output\table-layout-delivery-governance\summary.json"
        score = 100
        level = "release_ready"
        details = [ordered]@{
            score = 100
            level = "release_ready"
            document_count = 3
            ready_document_count = 3
            ready_document_percent = 100
            needs_review_document_count = 0
            failed_document_count = 0
            table_style_issue_count = 0
            automatic_tblLook_fix_count = 0
            manual_table_style_fix_count = 0
            table_position_automatic_count = 0
            table_position_review_count = 0
            fixed_layout_table_count = 1
            autofit_layout_table_count = 1
            unspecified_layout_table_count = 1
            pdf_floating_table_supported_geometry_count = 4
            pdf_floating_table_metadata_only_count = 5
            pdf_floating_table_tracked_geometry_count = 9
            pdf_floating_table_supported_geometry_percent = 44
            pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
            pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
            pdf_floating_table_metadata_only_fields = @(
                "leftFromText",
                "rightFromText",
                "topFromText outside paragraph anchoring",
                "tblOverlap"
            )
            pdf_floating_table_review_required_fields = @(
                "full Word-compatible floating table text wrapping",
                "table overlap avoidance and collision resolution"
            )
            command_failure_count = 0
            unresolved_item_count = 0
            penalty_summary = @(
                [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
            )
        }
    }
}
