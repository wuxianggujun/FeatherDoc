PDF Export
==========

PDF export is an experimental, opt-in path for writing a ``.docx`` document to
PDF through the in-process PDF writer. It is available only in builds
configured with PDF writer support:

.. code-block:: sh

    cmake -S . -B build-pdf -DFEATHERDOC_BUILD_PDF=ON -DBUILD_CLI=ON

``export-pdf`` is the scriptable CLI entry point:

.. code-block:: sh

    featherdoc_cli export-pdf input.docx --output output.pdf --json
    featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --summary-json output.summary.json --json
    featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --expand-header-footer-page-placeholders --summary-json output.summary.json --json

Common options
--------------

- ``--output <pdf>`` selects the output PDF path.
- ``--render-headers-and-footers`` includes section headers and footers in the
  generated pages.
- ``--expand-header-footer-page-placeholders`` expands header and footer
  placeholders such as ``{{page}}`` and ``{{total_pages}}`` during export.
- ``--render-inline-images`` enables inline image rendering for supported image
  content.
- ``--font-file <path>`` and ``--cjk-font-file <path>`` provide explicit font
  files.
- ``--font-map <family>=<path>`` maps a document font family to a font file.
- ``--no-font-subset`` disables Unicode font subsetting.
- ``--no-system-font-fallbacks`` disables system font fallback lookup.
- ``--summary-json <path>`` writes a stable machine-readable export summary.
- ``--json`` prints a stable machine-readable command result to stdout.

Detailed references
-------------------

- ``BUILDING_PDF.md`` is the build and local validation runbook for PDF writer
  work.
- ``docs/pdf_release_readiness_checklist_zh.rst`` records the release-readiness
  gate, visual evidence expectations, and current PDF evidence boundaries.

JSON result shape
-----------------

Successful ``--json`` output and ``--summary-json`` files include the command
name, output path, byte count, and the effective export switches:

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
------------------

When ``--json`` is supplied, failures use the shared CLI command-error shape on
stderr. The stable fields are ``command``, ``ok``, ``stage``, and ``message``;
document or writer failures can also include ``detail``, ``entry``, and
``xml_offset`` when that context is available:

.. code-block:: json

    {
      "command": "export-pdf",
      "ok": false,
      "stage": "export",
      "message": "Operation not supported",
      "detail": "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON"
    }

Current failure stages are:

The stage field values are ``"stage": "parse"``, ``"stage": "open"``,
``"stage": "export"``, and ``"stage": "summary"``.

- ``parse`` for invalid command-line options such as malformed
  ``--font-map`` values or missing ``--output``.
- ``open`` when the input ``.docx`` cannot be opened.
- ``export`` when PDF writing is disabled or the writer cannot produce the
  output PDF.
- ``summary`` when ``--summary-json`` was requested but the summary file cannot
  be written.

Supported scope and limits
--------------------------

The export path currently covers paragraph text, basic tables, baseline run
styling, selected CJK font fallback flows, section page setup, optional
headers and footers, optional inline images, and regression samples used by the
PDF visual validation workflow.

Floating table placement
------------------------

The PDF adapter understands the first stable subset of body-table
``w:tblpPr`` placement metadata. Absolute signed twips offsets are applied for
``page``, ``margin``, ``column``, and ``paragraph`` anchors in the same
coordinate conventions used by the in-process layout adapter.

Word-native ``tblpXSpec`` values also affect PDF geometry. ``left`` and
``inside`` align the table to the left side of the selected horizontal
reference frame, ``center`` centers it, and ``right`` / ``outside`` align it
to the right side. The explicit horizontal offset remains a signed fine-tuning
amount after the spec is resolved.

Word-native ``tblpYSpec`` values affect PDF geometry for the reliable
``page`` and ``margin`` vertical reference frames. ``top`` anchors the table
top to the reference top, ``center`` centers the full table block in that
reference frame, and ``bottom`` anchors the table bottom to the reference
bottom. The explicit vertical offset remains a signed fine-tuning amount after
the spec is resolved.

Vertical ``paragraph`` specs and vertical ``inside`` / ``outside`` specs are
preserved in DOCX metadata, but the PDF adapter keeps the older offset-based
top anchoring for those cases. They require Word page/paragraph and page-side
context that the experimental PDF layout path does not yet model as a stable
contract, though paragraph-anchored tables still consume ``topFromText`` as a
local top spacing adjustment.

``topFromText`` is honored as extra spacing above paragraph-anchored positioned
tables, and ``bottomFromText`` is honored as extra vertical spacing between a
positioned table and following body text in the in-process PDF layout. The
remaining per-edge text distances and ``tblOverlap`` are still preserved in
DOCX metadata and intentionally remain layout-neutral for PDF export until the
adapter has a full Word-compatible table text-wrapping contract.

It is not a production Word-compatible layout engine. Complex pagination,
full Word field evaluation, arbitrary drawing reconstruction, and
visual-perfect reproduction remain outside the stable contract. Treat the
exporter as an experimental automation surface and keep visual or PDFium
readback checks in release workflows.
