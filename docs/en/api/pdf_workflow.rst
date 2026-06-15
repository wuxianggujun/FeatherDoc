PDF Workflows
=============

PDF support is experimental and opt-in. This page is the language-local public
contract for ``export-pdf`` and ``import-pdf`` CLI workflows, JSON diagnostics,
supported scope and limits, and release validation references.

Build Flags
-----------

.. list-table::
   :header-rows: 1
   :widths: 24 32 46

   * - Workflow
     - CMake flag
     - Purpose
   * - PDF export
     - ``-DFEATHERDOC_BUILD_PDF=ON``
     - Enables the in-process PDF writer used by ``featherdoc_cli export-pdf``.
   * - PDF import
     - ``-DFEATHERDOC_BUILD_PDF_IMPORT=ON``
     - Enables the PDFium-backed text importer used by ``featherdoc_cli import-pdf``.

Build PDF support before using the CLI entry points:

.. code-block:: sh

   cmake -S . -B build-pdf -DFEATHERDOC_BUILD_PDF=ON -DFEATHERDOC_BUILD_PDF_IMPORT=ON -DBUILD_CLI=ON

PDF Export
----------

``export-pdf`` writes a ``.docx`` document to PDF through the in-process PDF
writer:

.. code-block:: sh

   featherdoc_cli export-pdf input.docx --output output.pdf --json
   featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --summary-json output.summary.json --json
   featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --expand-header-footer-page-placeholders --summary-json output.summary.json --json

Common options:

* ``--output <pdf>`` selects the output PDF path.
* ``--render-headers-and-footers`` includes section headers and footers.
* ``--expand-header-footer-page-placeholders`` expands ``{{page}}`` and
  ``{{total_pages}}`` placeholders during export.
* ``--render-inline-images`` enables supported inline image rendering.
* ``--font-file <path>`` and ``--cjk-font-file <path>`` provide explicit fonts.
* ``--font-map <family>=<path>`` maps a document font family to a font file.
* ``--no-font-subset`` disables Unicode font subsetting.
* ``--no-system-font-fallbacks`` disables system font fallback lookup.
* ``--summary-json <path>`` writes a machine-readable export summary.
* ``--json`` prints a machine-readable command result.

Successful JSON includes ``command``, ``ok``, ``output``, ``bytes_written`` and
the effective option set:

.. code-block:: json

   {
     "command": "export-pdf",
     "ok": true,
     "output": "output.pdf",
     "bytes_written": 12345,
     "options": {
       "render_headers_and_footers": true,
       "render_inline_images": false,
       "expand_header_footer_page_placeholders": true,
       "subset_unicode_fonts": true,
       "use_system_font_fallbacks": true
     }
   }

Failure JSON shape
~~~~~~~~~~~~~~~~~~

When ``--json`` is supplied, failures keep ``ok`` set to ``false`` and report
``command``, ``stage`` and ``message``. Document or writer failures can also
include ``detail``, ``entry`` and ``xml_offset`` when context is available:

.. code-block:: json

   {
     "command": "export-pdf",
     "ok": false,
     "stage": "export",
     "message": "Operation not supported",
     "detail": "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON"
   }

Current export failure stages are ``"stage": "parse"``,
``"stage": "open"``, ``"stage": "export"`` and ``"stage": "summary"``.
``parse`` covers invalid command-line options, ``open`` covers input package
open failures, ``export`` covers disabled or failed PDF writing, and
``summary`` covers ``--summary-json`` write failures.

Supported scope and limits for export include paragraph text, basic tables,
baseline run styling, selected CJK fallback flows, section page setup, optional
headers and footers, optional inline images, and regression samples used by the
PDF visual validation workflow. It is not a production Word-compatible layout
engine; complex pagination, full Word field evaluation, arbitrary drawing
reconstruction, and visual-perfect reproduction remain outside the stable
contract.

Detailed developer setup and release-gate evidence live in ``BUILDING_PDF.md``
and ``docs/pdf_release_readiness_checklist_zh.rst``.

PDF Import
----------

``import-pdf`` is intended for text-first PDFs with extractable text and
character geometry. It is not a general PDF-to-Word converter and it does not
promise arbitrary visual fidelity.

.. code-block:: sh

   featherdoc_cli import-pdf input.pdf --output imported.docx --json
   featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --json
   featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --min-table-continuation-confidence 90 --json

``--import-table-candidates-as-tables`` opts in to table candidate promotion.
``--min-table-continuation-confidence`` raises or lowers the cross-page table
continuation threshold. Do not document placeholder forms for this option; use
a concrete numeric threshold in examples.

PDF Import JSON Diagnostics
---------------------------

This section documents the PDF import JSON diagnostics emitted by
``featherdoc_cli import-pdf --json``.

Successful JSON output includes the common fields ``command``, ``ok``,
``input`` and ``output`` plus import counters:

.. code-block:: json

   {
     "command": "import-pdf",
     "ok": true,
     "input": "input.pdf",
     "output": "imported.docx",
     "paragraphs_imported": 2,
     "tables_imported": 1,
     "table_continuation_diagnostics_count": 2,
     "table_continuation_diagnostics": [
       {
         "page_index": 0,
         "block_index": 1,
         "source_row_offset": 0,
         "continuation_confidence": 0,
         "minimum_continuation_confidence": 0,
         "has_previous_table": false,
         "is_first_block_on_page": false,
         "is_near_page_top": true,
         "source_rows_consistent": true,
         "column_count_matches": false,
         "column_anchors_match": false,
         "previous_has_repeating_header": false,
         "source_has_repeating_header": false,
         "header_matches_previous": true,
         "header_match_kind": "not_required",
         "skipped_repeating_header": false,
         "disposition": "created_new_table",
         "blocker": "no_previous_table"
       },
       {
         "page_index": 1,
         "block_index": 0,
         "source_row_offset": 0,
         "continuation_confidence": 85,
         "minimum_continuation_confidence": 0,
         "has_previous_table": true,
         "is_first_block_on_page": true,
         "is_near_page_top": true,
         "source_rows_consistent": true,
         "column_count_matches": true,
         "column_anchors_match": true,
         "previous_has_repeating_header": false,
         "source_has_repeating_header": false,
         "header_matches_previous": true,
         "header_match_kind": "not_required",
         "skipped_repeating_header": false,
         "disposition": "merged_with_previous_table",
         "blocker": "none"
       }
     ],
     "import_table_candidates_as_tables": true
   }

``min_table_continuation_confidence`` is present only when the command line
sets ``--min-table-continuation-confidence``. ``table_continuation_diagnostics``
is an array ordered by table candidate discovery. It explains why a detected
table started a new table, merged with the previous page table, or was blocked
from merging across a page boundary.

Each diagnostic object uses these stable fields:

* ``page_index`` and ``block_index``: zero-based location of the table
  candidate in the parsed PDF document.
* ``source_row_offset``: number of source rows skipped before appending rows to
  a previous table.
* ``continuation_confidence`` and ``minimum_continuation_confidence``:
  rule-based scores used for diagnostics and thresholding.
* ``has_previous_table``, ``is_first_block_on_page``, ``is_near_page_top``,
  ``source_rows_consistent``, ``column_count_matches`` and
  ``column_anchors_match``: boolean continuation checks.
* ``previous_has_repeating_header``, ``source_has_repeating_header``,
  ``header_matches_previous``, ``header_match_kind`` and
  ``skipped_repeating_header``: repeated-header comparison details.
* ``disposition``: one of ``none``, ``created_new_table`` or
  ``merged_with_previous_table``.
* ``blocker``: the first reason a cross-page merge was rejected, or ``none``
  when the table was merged.

Diagnostic object fields are intentionally shown in CLI JSON emission order.
Keep the user-facing example aligned with the CLI implementation:
``page_index``, ``block_index``, ``source_row_offset``,
``continuation_confidence``, ``minimum_continuation_confidence``,
``has_previous_table``, ``is_first_block_on_page``, ``is_near_page_top``,
``source_rows_consistent``, ``column_count_matches``,
``column_anchors_match``, ``previous_has_repeating_header``,
``source_has_repeating_header``, ``header_matches_previous``,
``header_match_kind``, ``skipped_repeating_header``, ``disposition`` and
``blocker``.

``header_match_kind`` can be ``none``, ``not_required``, ``exact``,
``normalized_text``, ``plural_variant``, ``canonical_text`` or ``token_set``.
``blocker`` can be ``none``, ``no_previous_table``,
``not_first_block_on_page``, ``not_near_page_top``,
``inconsistent_source_rows``, ``column_count_mismatch``,
``column_anchors_mismatch``, ``repeated_header_mismatch`` or
``continuation_confidence_below_threshold``.

When a repeated-header continuation is merged, the diagnostic records the
rule-based decision explicitly: ``source_row_offset = 1`` means the first
source row was skipped as the repeated header,
``skipped_repeating_header = true``, ``continuation_confidence = 95``,
``header_match_kind = exact`` and ``blocker = none``. This confidence is a
deterministic heuristic score, not a probability.

.. code-block:: json

   {
     "source_row_offset": 1,
     "continuation_confidence": 95,
     "header_match_kind": "exact",
     "skipped_repeating_header": true,
     "disposition": "merged_with_previous_table",
     "blocker": "none"
   }

``inconsistent_source_rows`` is an internal consistency guard for malformed
table candidates. The current parser
normalizes each table-candidate row to the detected column anchors, so normal
imports should not rely on it as a stable user-triggered blocker.

Common continuation blockers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``blocker`` as the first triage field when a table was kept separate:

* ``column_count_mismatch`` means the candidate has a different detected
  column count than the previous table.
* ``repeated_header_mismatch`` means both table candidates looked like
  repeated-header tables, but their header text did not match after the
  conservative normalization rules.
* ``column_anchors_mismatch`` means the column count was compatible, but one or
  more column x positions drifted beyond the accepted anchor tolerance.
* ``continuation_confidence_below_threshold`` means the candidate did not meet
  ``--min-table-continuation-confidence``.
* ``not_first_block_on_page`` means the candidate was not the first content
  block on its page.
* ``not_near_page_top`` means the candidate started too far below the top of
  the page.

Representative diagnostic snippets:

The following complete blocker diagnostic objects mirror CLI JSON field order
for common split cases:

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 70,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": true,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": false,
     "header_match_kind": "none",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "repeated_header_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 30,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": false,
     "column_anchors_match": false,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": false,
     "header_match_kind": "none",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "column_count_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 55,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": false,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": true,
     "header_match_kind": "exact",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "column_anchors_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 85,
     "minimum_continuation_confidence": 90,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": true,
     "previous_has_repeating_header": false,
     "source_has_repeating_header": false,
     "header_matches_previous": true,
     "header_match_kind": "not_required",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "continuation_confidence_below_threshold"
   }

.. code-block:: json

   {
     "is_first_block_on_page": false,
     "disposition": "created_new_table",
     "blocker": "not_first_block_on_page"
   }

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
``extract_geometry_disabled``, ``table_candidates_detected`` or
``no_text_paragraphs``. Running without
``--import-table-candidates-as-tables`` against a PDF where table candidates are
detected reports ``table_candidates_detected`` and does not write the target
DOCX. Scanned or image-only PDFs without extractable text paragraphs report
``no_text_paragraphs`` and also leave the target DOCX unwritten.

Command-line parse errors
~~~~~~~~~~~~~~~~~~~~~~~~~

When ``--json`` is present, command-line validation errors still report
``command``, ``ok``, ``stage`` and ``message``:

.. code-block:: json

   {
     "command": "import-pdf",
     "ok": false,
     "stage": "parse",
     "message": "missing value after --min-table-continuation-confidence"
   }

The same parse-error shape is used for invalid threshold values and duplicate
threshold options. Parse errors do not write the target DOCX. Current messages
include ``missing value after --min-table-continuation-confidence``,
``invalid value after --min-table-continuation-confidence`` and
``duplicate --min-table-continuation-confidence option``.

PDF Import Supported Scope And Limits
-------------------------------------

This section records the PDF import supported scope and limits. The importer is
text-first. It reconstructs a conservative ``Document`` from extractable PDF
text and character geometry, then optionally promotes selected table
candidates. The importer is text-first. It reconstructs from extractable PDF
text and character geometry. It is not a general PDF-to-Word converter, not a
general PDF-to-Word clone, and does not promise arbitrary visual fidelity.
The importer uses extractable PDF text and character geometry.
It is not a general PDF-to-Word clone.
arbitrary local column drift remains unsupported.

Reliable scope:

* Paragraph import from extractable PDF text.
* Conservative table-candidate detection for simple aligned grids, simple
  key-value tables and selected borderless aligned tables.
* Table candidates are rejected by default.
* Opt-in table promotion through ``--import-table-candidates-as-tables``.
* Cross-page table continuation when the next page starts with a compatible
  table candidate near the top of the page.
* Repeated-header detection for exact text, whitespace/case/punctuation
  normalization, conservative plural variants, a small abbreviation whitelist
  such as ``Qty`` / ``Quantity`` and ``Amt`` / ``Amount``, and token-set word
  order normalization.
* Conservative subtotal / total summary-row handling in controlled invoice-like
  grids.
* Diagnostics through ``table_continuation_diagnostics`` when a table is
  merged or kept separate.

The importer deliberately stays conservative:

* Column-count mismatches, incompatible column anchors, semantic
  repeated-header mismatches, intervening paragraphs, or low continuation
  confidence keep tables separate.
  This covers semantic repeated-header differences, low continuation confidence,
  intervening paragraphs, and incompatible column anchors.
* Ordinary two-column prose, numbered lists, short-label prose, and free-form
  forms should remain paragraphs rather than becoming tables.
* The current implementation does not support scanned PDFs, OCR, image-only
  tables, arbitrary nested table semantics, complex vector reconstruction,
  rotated or floating content recovery, or exact visual reconstruction of an
  arbitrary PDF. Arbitrary local column drift remains unsupported.
* Unsupported cases must fail or remain paragraphs rather than silently
  producing misleading Word structure.

In practical terms:

* Text extraction is the primary contract; layout reconstruction is best-effort
  and intentionally conservative.
* Table import is opt-in through ``--import-table-candidates-as-tables``.
* OCR, scanned pages, and visual-perfect recreation remain out of scope.

Release Validation
------------------

Use ``BUILDING_PDF.md`` for developer setup and local validation. PDF release
readiness evidence is still maintained by release-governance scripts and
bounded CTest checks.
