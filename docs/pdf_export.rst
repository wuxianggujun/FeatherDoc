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

Supported scope and limits
--------------------------

The export path currently covers paragraph text, basic tables, baseline run
styling, selected CJK font fallback flows, section page setup, optional
headers and footers, optional inline images, and regression samples used by the
PDF visual validation workflow.

It is not a production Word-compatible layout engine. Complex pagination,
floating layout, full Word field evaluation, arbitrary drawing reconstruction,
and visual-perfect reproduction remain outside the stable contract. Treat the
exporter as an experimental automation surface and keep visual or PDFium
readback checks in release workflows.
