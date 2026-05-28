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

Detailed references
-------------------

- PDF Import JSON Diagnostics (:doc:`pdf_import_json_diagnostics`) documents
  the ``--json`` success and failure shape,
  ``table_continuation_diagnostics`` fields, and enum values.
- PDF Import Supported Scope And Limits (:doc:`pdf_import_scope`) records the
  supported scope, conservative split-table behavior, and known unsupported PDF
  classes.
