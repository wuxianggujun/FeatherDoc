PDF Import JSON Diagnostics
===========================

This page documents the PDF import JSON diagnostics emitted by
``featherdoc_cli import-pdf --json``.

Successful JSON output includes the common fields ``command``, ``ok``,
``input``, and ``output`` plus import counters:

.. code-block:: json

    {
      "command": "import-pdf",
      "ok": true,
      "input": "input.pdf",
      "output": "imported.docx",
      "paragraphs_imported": 2,
      "tables_imported": 1,
      "table_continuation_diagnostics_count": 2,
      "table_continuation_diagnostics": [],
      "import_table_candidates_as_tables": true,
      "min_table_continuation_confidence": 90
    }

``min_table_continuation_confidence`` is present only when the command line
sets ``--min-table-continuation-confidence``. ``table_continuation_diagnostics``
is an array ordered by table candidate discovery. It explains why a detected
table started a new table, merged with the previous page's table, or was
blocked from merging across a page boundary.

Each diagnostic object uses these fields:

- ``page_index`` and ``block_index``: zero-based location of the table
  candidate in the parsed PDF document.
- ``source_row_offset``: number of source rows skipped before appending rows to
  a previous table. A value of ``1`` usually means a repeated header row was
  recognized and skipped.
- ``continuation_confidence`` and ``minimum_continuation_confidence``:
  rule-based scores used only for diagnostics and thresholding; they are not a
  probability model.
- ``has_previous_table``, ``is_first_block_on_page``, ``is_near_page_top``,
  ``source_rows_consistent``, ``column_count_matches``, and
  ``column_anchors_match``: boolean checks used by the continuation decision.
- ``previous_has_repeating_header``, ``source_has_repeating_header``,
  ``header_matches_previous``, ``header_match_kind``, and
  ``skipped_repeating_header``: repeated-header comparison details.
- ``disposition``: one of ``none``, ``created_new_table``, or
  ``merged_with_previous_table``.
- ``blocker``: the first reason a cross-page merge was rejected, or ``none``
  when the table was merged.

``header_match_kind`` can be ``none``, ``not_required``, ``exact``,
``normalized_text``, ``plural_variant``, ``canonical_text``, or ``token_set``.
``blocker`` can be ``none``, ``no_previous_table``, ``not_first_block_on_page``,
``not_near_page_top``, ``inconsistent_source_rows``,
``column_count_mismatch``, ``column_anchors_mismatch``,
``repeated_header_mismatch``, or
``continuation_confidence_below_threshold``.

For example, a compatible cross-page table with a repeated header may report:

.. code-block:: json

    {
      "source_row_offset": 1,
      "column_count_matches": true,
      "column_anchors_match": true,
      "header_match_kind": "exact",
      "skipped_repeating_header": true,
      "disposition": "merged_with_previous_table",
      "blocker": "none"
    }

A rejected continuation with incompatible columns may report:

.. code-block:: json

    {
      "column_count_matches": false,
      "column_anchors_match": false,
      "header_match_kind": "none",
      "disposition": "created_new_table",
      "blocker": "column_count_mismatch"
    }

Common continuation blockers
----------------------------

Use ``blocker`` as the first triage field when a table was kept separate:

- ``repeated_header_mismatch`` means both table candidates looked like
  repeated-header tables, but their header text did not match after the
  conservative normalization rules.
- ``column_anchors_mismatch`` means the column count was compatible, but one or
  more column x positions drifted beyond the accepted anchor tolerance.
- ``continuation_confidence_below_threshold`` means the candidate may otherwise
  look compatible, but its rule-based score did not meet
  ``--min-table-continuation-confidence``.
- ``not_first_block_on_page`` means the candidate was not the first content
  block on its page, so it is treated as a separate table rather than a
  cross-page continuation.
- ``not_near_page_top`` means the candidate started too far below the top of
  the page to be treated as a carried-over table.
- ``inconsistent_source_rows`` is an internal consistency guard for malformed
  table candidates whose rows do not share the same cell count. The current
  PDF parser normally builds consistent rows from column anchors, so this
  blocker is documented as a defensive fallback rather than a user-facing
  fixture example.

For example, a semantic repeated-header mismatch may report:

.. code-block:: json

    {
      "header_matches_previous": false,
      "header_match_kind": "none",
      "disposition": "created_new_table",
      "blocker": "repeated_header_mismatch"
    }

A local anchor drift may report:

.. code-block:: json

    {
      "column_count_matches": true,
      "column_anchors_match": false,
      "disposition": "created_new_table",
      "blocker": "column_anchors_mismatch"
    }

A caller-specified confidence threshold may report:

.. code-block:: json

    {
      "continuation_confidence": 80,
      "minimum_continuation_confidence": 90,
      "disposition": "created_new_table",
      "blocker": "continuation_confidence_below_threshold"
    }

A same-page follow-up table may report:

.. code-block:: json

    {
      "is_first_block_on_page": false,
      "disposition": "created_new_table",
      "blocker": "not_first_block_on_page"
    }

A next-page table that starts too low may report:

.. code-block:: json

    {
      "is_first_block_on_page": true,
      "is_near_page_top": false,
      "disposition": "created_new_table",
      "blocker": "not_near_page_top"
    }

Failure JSON keeps ``ok`` set to ``false`` and reports the ``import`` stage:

.. code-block:: json

    {
      "command": "import-pdf",
      "ok": false,
      "stage": "import",
      "failure_kind": "table_candidates_detected",
      "message": "PDF text import detected table-like structure candidates; enable table-candidate import to import them as DOCX tables",
      "input": "input.pdf",
      "output": "imported.docx"
    }

``failure_kind`` can be ``parse_failed``, ``document_create_failed``,
``document_population_failed``, ``extract_text_disabled``,
``extract_geometry_disabled``, ``table_candidates_detected``, or
``no_text_paragraphs``. Running without
``--import-table-candidates-as-tables`` against a PDF where table candidates are
detected reports ``table_candidates_detected`` and does not write the target
DOCX. Command-line argument errors still use the common command-error JSON shape
with ``stage`` set to ``parse``.
