function Get-PdfImportDiagnosticsContractFields {
    return @(
        "table_continuation_diagnostics",
        "table_continuation_diagnostics=[]",
        "tables_imported=0",
        "import_table_candidates_as_tables=true",
        "failure_kind=no_text_paragraphs",
        "source_row_offset=0",
        "skipped_repeating_header=false",
        "source_rows_consistent=false",
        "disposition=created_new_table",
        "blocker=inconsistent_source_rows",
        "blocker=repeated_header_mismatch",
        "blocker=column_count_mismatch",
        "blocker=column_anchors_mismatch",
        "blocker=continuation_confidence_below_threshold",
        "continuation_confidence=70",
        "continuation_confidence=55",
        "continuation_confidence=85",
        "continuation_confidence=25",
        "continuation_confidence=30",
        "minimum_continuation_confidence=90",
        "column_count_matches=false",
        "column_anchors_match=false"
    )
}
