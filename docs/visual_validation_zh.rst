Visual Validation
=================

FeatherDoc keeps Word-rendered validation artifacts in the repository so readers
can inspect real layout output instead of relying on XML-only claims.

.. image:: assets/readme/visual-smoke-contact-sheet.png
   :alt: Full Word visual smoke contact sheet
   :align: center
   :width: 100%

The visual smoke flow covers multi-page tables, repeated headers, vertical and
rotated text, fixed-grid width edits, merge/unmerge checks, and mixed
RTL/LTR/CJK review content.

Focused Preview
---------------

.. list-table::
   :widths: 25 25 25 25

   * - .. image:: assets/readme/fixed-grid-merge-right-page-01.png
          :alt: Word-rendered standalone merge_right fixed-grid sample page
          :width: 100%
     - .. image:: assets/readme/fixed-grid-merge-down-page-01.png
          :alt: Word-rendered standalone merge_down fixed-grid sample page
          :width: 100%
     - .. image:: assets/readme/fixed-grid-aggregate-contact-sheet.png
          :alt: Fixed-grid merge and unmerge regression contact sheet
          :width: 100%
     - .. image:: assets/readme/sample-chinese-template-page-01.png
          :alt: Word-rendered Chinese invoice template sample page
          :width: 100%
   * - Standalone ``merge_right()`` after reopen.
     - Standalone ``merge_down()`` after reopen.
     - Fixed-grid merge and unmerge regression contact sheet.
     - Chinese invoice template sample rendered through Word.

Local Reproduction
------------------

.. code-block:: powershell

   powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1
   powershell -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1 -PrepareReviewTask -ReviewMode review-only
   powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1

For the full review workflow, see :doc:`automation/word_visual_workflow_zh`.

Curated Regression Entrypoints
------------------------------

The release gate and targeted review tasks keep several focused visual
regression entrypoints. They are listed here instead of on the documentation
home page so the home page stays a short navigation surface.

* ``scripts/run_omml_visual_regression.ps1`` validates OMML formula mutation
  output with Word-rendered evidence.
* ``scripts/run_semantic_diff_visual_regression.ps1`` validates semantic diff
  before/after evidence, including style, numbering, and template part changes.
* ``scripts/run_generic_fields_visual_regression.ps1`` validates TOC, REF, SEQ,
  and update-fields evidence through generated documents and semantic diff JSON.
* ``.\scripts\run_page_number_fields_visual_regression.ps1`` validates page
  number and total-pages fields in header and footer template parts.
