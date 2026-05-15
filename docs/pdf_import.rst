PDF Import
==========

PDF import is an experimental, opt-in path intended for text-first PDFs with
extractable text and geometry. It is not a general "PDF to Word clone" feature.
The importer currently requires both text extraction and character geometry;
without geometry, it fails explicitly instead of falling back to a less
predictable plain-text import.

``import-pdf`` is available only in builds configured with PDF import support.
Use ``--json`` when a caller needs a stable machine-readable summary of a PDF
import run:

.. code-block:: sh

    featherdoc_cli import-pdf input.pdf --output imported.docx --json
    featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --json
    featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --min-table-continuation-confidence 90 --json

PDF import JSON diagnostics
---------------------------

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

Failure JSON keeps ``ok`` set to ``false`` and includes ``stage`` plus
``failure_kind``. For example, running without
``--import-table-candidates-as-tables`` against a PDF where table candidates are
detected reports ``failure_kind`` as ``table_candidates_detected``.

PDF import supported scope and limits
-------------------------------------

The reliable scope is intentionally narrow:

- Paragraph import from extractable PDF text.
- Conservative table-candidate detection for simple aligned grids, simple
  key-value tables, and selected borderless aligned tables.
- Opt-in table promotion through ``--import-table-candidates-as-tables``.
- Cross-page table continuation when the next page starts with a compatible
  table candidate near the top of the page.
- Repeated-header detection for exact text, whitespace/case/punctuation
  normalization, conservative plural variants, a small abbreviation whitelist
  such as ``Qty`` / ``Quantity`` and ``Amt`` / ``Amount``, and token-set word
  order normalization.
- Conservative subtotal / total summary-row handling in controlled invoice-like
  grids.
- Diagnostics through ``table_continuation_diagnostics`` when a table is
  merged or kept separate.

The importer deliberately stays conservative in ambiguous cases:

- Table candidates are rejected by default. Enable
  ``--import-table-candidates-as-tables`` only when table recovery is desired.
- Column-count mismatches, incompatible column anchors, semantic repeated-header
  mismatches, intervening paragraphs, or low continuation confidence keep tables
  separate.
- Ordinary two-column prose, numbered lists, short-label prose, and free-form
  forms should remain paragraphs rather than becoming tables.

The current implementation does not support scanned PDFs, OCR, image-only
tables, arbitrary nested table semantics, complex vector reconstruction,
rotated or floating content recovery, or exact visual reconstruction of an
arbitrary PDF. Sparse rows are supported only in controlled cases where the
surrounding table geometry is still compatible; arbitrary local column drift
remains a split-table condition.
