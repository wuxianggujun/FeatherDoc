PDF Import Supported Scope And Limits
=====================================

This page records the PDF import supported scope and limits. The reliable scope
is intentionally narrow:

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
