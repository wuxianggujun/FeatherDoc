
FeatherDoc
==========

Release v\ |version|

*FeatherDoc* is a C++ library for creating and editing Microsoft Word
(.docx) files.

Repository-level README entry points are available at ``README.md`` (English)
and ``README.zh-CN.md`` (Simplified Chinese).

.. toctree::
   :maxdepth: 2
   :hidden:

   project_identity_zh
   release_policy_zh
   release_history_backfill_playbook_zh
   v1_7_roadmap_zh
   project_audit_zh
   upstream_issue_triage_zh
   licensing_zh
   automation/word_visual_workflow_zh

Visual Validation Preview
-------------------------
FeatherDoc now keeps Word-rendered validation artifacts in the repository, so
readers can inspect real layout output instead of relying on XML-only claims.

.. image:: assets/readme/visual-smoke-contact-sheet.png
   :alt: Full Word visual smoke contact sheet
   :align: center
   :width: 100%

The current smoke flow covers multi-page tables, repeated headers, vertical and
rotated text, fixed-grid width edits, merge/unmerge checks, and mixed
RTL/LTR/CJK review content. The gallery below switches to more legible
front-page slices: two standalone fixed-grid closure checks, the aggregate
quartet signoff bundle, and one Chinese invoice template page rendered through
Word.

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
   * - Standalone ``merge_right()`` after reopen; the merged blue cell is
       visibly wider than the ``1000`` twip base cell below it while remaining
       narrower than the ``4100`` twip tail column.
     - Standalone ``merge_down()`` after reopen; the merged orange pillar keeps
       the fixed ``1000`` twip width across both rows while the neighbor
       columns remain aligned.
     - The dedicated fixed-grid regression bundle still keeps
       ``merge_right()``, ``merge_down()``, ``unmerge_right()``, and
       ``unmerge_down()`` in one screenshot-backed manual signoff surface.
     - A Chinese invoice template sample keeps CJK font metadata, business
       table output, and rendered spacing visible in Word instead of XML-only
       claims.

For the end-to-end local review flow, see
:doc:`automation/word_visual_workflow_zh`. For the release-preflight wrapper,
see :doc:`release_policy_zh`.

When browsing an installed ``share/FeatherDoc`` tree, start with
``VISUAL_VALIDATION_QUICKSTART.md`` or
``VISUAL_VALIDATION_QUICKSTART.zh-CN.md`` for the shortest
``preview PNG -> repro command -> review task`` entry.

To reproduce the same screenshots locally on Windows with a real
``Microsoft Word`` install, use:

.. code-block:: powershell

    # Full smoke contact sheet plus page slices
    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1

    # Build the standalone samples that feed the focused README gallery pages.
    # Adjust the executable path if your generator stores sample binaries
    # somewhere other than the build root.
    cmake --build build-msvc-nmake --target `
        featherdoc_sample_merge_right_fixed_grid `
        featherdoc_sample_merge_down_fixed_grid `
        featherdoc_sample_chinese_template

    .\build-msvc-nmake\featherdoc_sample_merge_right_fixed_grid.exe `
        .\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx

    .\build-msvc-nmake\featherdoc_sample_merge_down_fixed_grid.exe `
        .\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx

    .\build-msvc-nmake\featherdoc_sample_chinese_template.exe `
        .\samples\chinese_invoice_template.docx `
        .\output\sample-chinese-template\sample_chinese_invoice_output.docx

    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
        -InputDocx .\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx `
        -OutputDir .\output\word-visual-sample-merge-right-fixed-grid

    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
        -InputDocx .\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx `
        -OutputDir .\output\word-visual-sample-merge-down-fixed-grid

    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
        -InputDocx .\output\sample-chinese-template\sample_chinese_invoice_output.docx `
        -OutputDir .\output\word-visual-sample-chinese-template

    # Fixed-grid quartet, aggregate contact sheet, plus a ready-to-review task package
    powershell -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
        -PrepareReviewTask `
        -ReviewMode review-only

    # Section page setup sample plus CLI rewrite bundle
    powershell -ExecutionPolicy Bypass -File .\scripts\run_section_page_setup_regression.ps1

    # PAGE / NUMPAGES visual regression bundle
    powershell -ExecutionPolicy Bypass -File .\scripts\run_page_number_fields_regression.ps1

    # One-shot local gate for both flows
    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1

    # Same gate, but also refresh the repository gallery PNGs
    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
        -RefreshReadmeAssets

    # Refresh the repository gallery PNGs from already-generated tasks only
    powershell -ExecutionPolicy Bypass -File .\scripts\refresh_readme_visual_assets.ps1

    # After both screenshot-backed task verdicts are written, sync them back
    # into the gate summary
    powershell -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1

    # Same sync step, but with explicit paths when you need to override the
    # auto-detected gate or release summary
    powershell -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
        -GateSummaryJson output\word-visual-release-gate\report\gate_summary.json

The install tree mirrors these preview entry points under ``share/FeatherDoc``
through ``RELEASE_ARTIFACT_TEMPLATE*.md``,
``VISUAL_VALIDATION_QUICKSTART*.md``, ``VISUAL_VALIDATION*.md``, and
``visual-validation/*.png``.


How to install
--------------
The instructions to build and install FeatherDoc

.. code-block:: sh

    cmake -S . -B build
    cmake --build build
    cmake --install build --prefix install

The installed package also carries project-facing metadata, visual-validation
preview assets, repro guides, release-artifact templates, and legal files
under ``share/FeatherDoc``.
Top-level builds enable ``BUILD_CLI`` by default, so ``featherdoc_cli`` is
built unless ``-DBUILD_CLI=OFF`` is passed explicitly.

The shortest installed-package path for visual reproduction is:

.. code-block:: text

    share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md
        -> run_word_visual_release_gate.ps1
        -> open_latest_*_review_task.ps1
        -> sync_latest_visual_review_verdict.ps1
        -> sync_visual_review_verdict.ps1
        -> RELEASE_ARTIFACT_TEMPLATE.zh-CN.md

MSVC Build
----------
Build FeatherDoc with the MSVC toolchain from a Visual Studio Developer
Command Prompt. Use an ``x64`` prompt, or initialize the environment with
``VsDevCmd.bat -arch=x64 -host_arch=x64`` first.

.. code-block:: bat

    cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON
    cmake --build build-msvc-nmake
    ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60

For a one-shot local release-preflight entry on Windows, use:

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1

That wrapper chains MSVC build/test, install ``find_package`` smoke, and the
Word visual release gate. The visual stage is intentionally local because it
depends on a real ``Microsoft Word`` installation for final rendering.
It also writes ``output/release-candidate-checks/START_HERE.md`` plus
``report/ARTIFACT_GUIDE.md``, ``REVIEWER_CHECKLIST.md``,
``release_handoff.md``, ``release_body.zh-CN.md``, and
``release_summary.zh-CN.md``. Use ``START_HERE.md`` as the local entry point.
The CI metadata artifact adds a root ``RELEASE_METADATA_START_HERE.md`` that
points back into the same bundle.
After a later manual visual verdict update, prefer
``scripts/sync_latest_visual_review_verdict.ps1`` for the shortest
auto-detected path. When you need to override paths explicitly, keep using
``scripts/sync_visual_review_verdict.ps1`` against the saved
``gate_summary.json``. If a release-preflight ``summary.json`` also exists,
pass it as ``-ReleaseCandidateSummaryJson`` together with
``-RefreshReleaseBundle`` to refresh ``START_HERE.md`` plus the release-facing
notes without rerunning the full preflight.

.. _featherdoc-cli:

CLI
---
``featherdoc_cli`` exposes a compact command-line layer for the current
inspection and editing workflows around sections, styles, numbering, page
setup, bookmarks, images, and template parts.

.. code-block:: sh

    featherdoc_cli inspect-sections input.docx
    featherdoc_cli inspect-sections input.docx --json
    featherdoc_cli inspect-styles input.docx
    featherdoc_cli inspect-styles input.docx --style Strong --json
    featherdoc_cli inspect-styles input.docx --style Strong --usage
    featherdoc_cli inspect-numbering input.docx
    featherdoc_cli inspect-numbering input.docx --instance 1
    featherdoc_cli inspect-numbering input.docx --definition 1 --json
    featherdoc_cli inspect-page-setup input.docx
    featherdoc_cli inspect-page-setup input.docx --section 1 --json
    featherdoc_cli set-section-page-setup input.docx 1 --orientation landscape --width 15840 --height 12240 --margin-top 720 --output rotated.docx --json
    featherdoc_cli inspect-bookmarks input.docx
    featherdoc_cli inspect-bookmarks input.docx --part header --index 0 --bookmark header_rows --json
    featherdoc_cli inspect-images input.docx
    featherdoc_cli inspect-images input.docx --relationship-id rId5 --image-entry-name word/media/image1.png --json
    featherdoc_cli inspect-images input.docx --part header --index 0 --image 0 --json
    featherdoc_cli extract-image input.docx exported.png --relationship-id rId5 --json
    featherdoc_cli replace-image input.docx replacement.gif --relationship-id rId5 --output updated.docx --json
    featherdoc_cli remove-image input.docx --relationship-id rId5 --output pruned.docx --json
    featherdoc_cli append-image input.docx badge.png --width 96 --height 48 --output with-image.docx --json
    featherdoc_cli inspect-header-parts input.docx --json
    featherdoc_cli inspect-footer-parts input.docx
    featherdoc_cli insert-section input.docx 1 --no-inherit --output inserted.docx --json
    featherdoc_cli copy-section-layout input.docx 0 2 --output copied.docx
    featherdoc_cli move-section input.docx 2 0 --output reordered.docx
    featherdoc_cli remove-section input.docx 3 --output trimmed.docx
    featherdoc_cli assign-section-header input.docx 2 0 --kind even --output shared-header.docx --json
    featherdoc_cli assign-section-footer input.docx 2 1 --output shared-footer.docx --json
    featherdoc_cli remove-section-header input.docx 2 --kind even --output detached-header.docx
    featherdoc_cli remove-section-footer input.docx 1 --kind first --output detached-footer.docx
    featherdoc_cli remove-header-part input.docx 1 --output headers-pruned.docx
    featherdoc_cli remove-footer-part input.docx 0 --output footers-pruned.docx --json
    featherdoc_cli show-section-header input.docx 1 --kind even
    featherdoc_cli show-section-footer input.docx 2 --json
    featherdoc_cli set-section-footer input.docx 0 --text "Page 1" --output footer.docx --json
    featherdoc_cli set-section-header input.docx 2 --kind even --text-file header.txt --json
    featherdoc_cli append-page-number-field input.docx --part section-header --section 1 --output page-number.docx --json
    featherdoc_cli append-total-pages-field input.docx --part section-footer --section 1 --kind first --output total-pages.docx --json
    featherdoc_cli validate-template input.docx --part body --slot customer:text --slot line_items:table_rows --json
    featherdoc_cli validate-template input.docx --part header --index 0 --slot header_title:text --slot header_rows:table_rows --json
    featherdoc_cli validate-template input.docx --part section-footer --section 1 --kind first --slot footer_company:text --slot footer_note:block:optional

The command block above is representative rather than exhaustive. Keep reading
this page when you need the broader CLI surface, including table inspection and
mutation commands, template-table row/cell edits, image extraction and
replacement, or per-section page-setup rewrites.

``inspect-sections`` reports section counts together with per-section
``default`` / ``first`` / ``even`` header and footer attachment flags. The
mutating commands overwrite the input file unless ``--output <path>`` is
provided. Pass ``--json`` to ``inspect-sections`` when you need the same layout
data in a machine-readable object. The mutating commands also accept
``--json`` and emit ``command``, ``ok``, ``in_place``, ``sections``,
``headers``, and ``footers`` together with command-specific fields such as
``section``, ``source``, ``target``, ``part``, and ``kind``.
``inspect-styles`` exposes the current style catalog through
``Document::list_styles()`` and ``Document::find_style(...)``. With no extra
flags it prints every loaded style summary in document order. Pass
``--style <style-id>`` when you only want one style record, and ``--json``
when you need the same fields in a machine-readable object. The output
includes ``style_id``, ``name``, ``based_on``, ``kind``, ``type``, and the
default/custom/hidden/quick-format flags.
``inspect-page-setup`` exposes explicit per-section page geometry through
``Document::get_section_page_setup(section_index)``. With no extra flags it
prints every section together with a ``present=yes|no`` marker. Use
``--section <index>`` when you only want one record, and ``--json`` when you
need ``orientation``, ``width_twips``, ``height_twips``, margin twips, and
the optional ``page_number_start`` in a machine-readable object. Sections
without an explicit ``w:sectPr`` / ``w:pgSz`` / ``w:pgMar`` report
``present=false``.
``set-section-page-setup`` updates the explicit page geometry for one
section. Pass any subset of ``--orientation``, ``--width``, ``--height``,
margin flags, and ``--page-number-start`` / ``--clear-page-number-start`` to
change only the fields you need. When the target section already has an
explicit page setup, omitted fields are preserved. When the final section is
still implicit, provide the full geometry once and FeatherDoc materializes the
editable ``w:sectPr`` / ``w:pgSz`` / ``w:pgMar`` nodes automatically. Use
``--output <path>`` to keep the source file untouched, and ``--json`` to emit
the resolved page setup after the write.
``inspect-bookmarks`` exposes the current bookmark catalog through
``Document::list_bookmarks()``, ``Document::find_bookmark(...)``, and the
same methods on ``TemplatePart``. With no extra flags it prints every
bookmark summary for the resolved part. Use ``--bookmark <name>`` when you
only want one record, and combine ``--part``, ``--index``, ``--section``,
and ``--kind`` when you need to inspect headers, footers, or
section-scoped parts. The output includes ``bookmark_name``,
``occurrence_count``, inferred ``kind``, ``is_duplicate``, and the resolved
``entry_name``.
``inspect-header-parts`` / ``inspect-footer-parts`` list loaded part indexes in
the same order consumed by ``assign-section-*`` and ``remove-*-part``. Their
output includes each part's relationship id, package entry path, section
references, and paragraph text. Pass ``--json`` when you need the same data in
machine-readable form.
``assign-section-header`` / ``assign-section-footer`` make a section reuse an
already loaded header/footer part by index. ``remove-section-header`` /
``remove-section-footer`` detach one section-level reference kind without
removing the underlying part when it is still referenced elsewhere.
``remove-header-part`` / ``remove-footer-part`` drop one loaded part entirely
and detach every section reference that points at it.
``show-section-header`` / ``show-section-footer`` print the referenced
paragraphs one line per paragraph. ``set-section-header`` /
``set-section-footer`` rewrite the target part as plain paragraphs from
``--text`` or ``--text-file`` and create the requested section reference when
it is missing. ``show-section-header`` / ``show-section-footer`` also accept
``--json`` and emit ``part``, ``section``, ``kind``, ``present``, and
``paragraphs`` fields for scriptable inspection.
``append-page-number-field`` / ``append-total-pages-field`` append Word
``PAGE`` and ``NUMPAGES`` simple fields through
``TemplatePart::append_page_number_field()`` and
``TemplatePart::append_total_pages_field()``. They use the same target
selection shape as ``inspect-bookmarks`` and ``validate-template``. Use
``--part`` with ``body``, ``header``, ``footer``, ``section-header``, or
``section-footer``; use ``--index`` for loaded header/footer parts; and use
``--section`` plus optional ``--kind default|first|even`` for section-scoped
parts. When the requested
header/footer part does not exist yet, FeatherDoc materializes the writable
part automatically before appending the field. Use ``--json`` to capture the
resolved ``part``, ``part_index``, ``section``, ``kind``, ``entry_name``, and
appended ``field``.
For a runnable field-insertion example, build
``featherdoc_sample_page_number_fields`` from
``samples/sample_page_number_fields.cpp``. It creates a new document,
materializes writable section header/footer parts, appends ``PAGE`` /
``NUMPAGES`` fields through ``TemplatePart`` handles, and saves the result for
Word-side field refresh.
``validate-template`` exposes the same read-only slot-schema validation as
``Document::validate_template(...)`` and
``TemplatePart::validate_template(...)``. Use
``--part body|header|footer|section-header|section-footer`` to choose the
target, ``--index`` for header/footer part indexes, ``--section`` plus the
optional ``--kind default|first|even`` for section-scoped parts, and one or
more ``--slot <bookmark>:<kind>[:required|optional]`` rules to describe the
expected placeholders. The command always reports ``passed``,
``missing_required``, ``duplicate_bookmarks``, and
``malformed_placeholders``; ``--json`` also includes the resolved
``entry_name``.

Package Metadata
----------------
The generated CMake package config exposes:

- ``FeatherDoc_VERSION``
- ``FeatherDoc_DESCRIPTION``
- ``FeatherDoc_PACKAGE_DATA_DIR``

Quickstart
--------------
How to start with FeatherDoc quickly

.. code-block:: cpp

    #include <iostream>
    #include <featherdoc.hpp>

    int main() {

        featherdoc::Document doc("file.docx");
        if (const auto error = doc.open()) {
            const auto& error_info = doc.last_error();
            std::cerr << error.message() << std::endl;
            std::cerr << error_info.detail << std::endl;
            return 1;
        }

        for (auto p : doc.paragraphs())
        {
            std::string text;
            for (auto r : p.runs())
                text += r.get_text();
            std::cout << text << std::endl;
        }

        for (auto table : doc.tables())
            for (auto row : table.rows())
                for (auto cell : row.cells())
                    for (auto paragraph : cell.paragraphs())
                    {
                        std::string text;
                        for (auto run : paragraph.runs())
                            text += run.get_text();
                        std::cout << text << std::endl;
                    }

        return 0;
    }

``Run`` represents WordprocessingML runs, not whole lines. If one visible line
is split into multiple runs, concatenate the run texts inside a paragraph before
printing. Text stored inside tables is accessed through
``doc.tables() -> rows() -> cells() -> paragraphs()``.

.. _featherdoc-api-map:

.. rubric:: Task-Oriented API Map

When you are skimming the public API surface, start from the entry point that
matches the job at hand.

If you mainly want runnable entry points instead of API-first navigation, jump
to :ref:`Task-Oriented Sample And CLI Map <featherdoc-sample-cli-map>`.

- Open, create, save, or diagnose a document:
  ``Document(path)``, ``open()``, ``create_empty()``, ``save()``,
  ``save_as()``, and ``last_error()``. See
  :ref:`Creating New Documents And Language Defaults <featherdoc-doc-lifecycle>`.
- Edit body text structure:
  ``paragraphs()``, ``runs()``, ``set_text(...)``,
  ``insert_paragraph_before(...)``, ``insert_paragraph_after(...)``,
  ``insert_run_before(...)``, ``insert_run_after(...)``, and ``remove()``.
  See :ref:`Text Editing <featherdoc-text-editing>`.
- Build or mutate tables:
  ``append_table(...)``, ``append_row(...)``, ``append_cell()``,
  ``insert_row_before()``, ``insert_row_after()``, ``insert_cell_before()``,
  ``insert_cell_after()``, ``merge_right(...)``, ``merge_down(...)``,
  ``unmerge_right()``, ``unmerge_down()``, ``set_width_twips(...)``,
  ``set_column_width_twips(...)``, and ``set_layout_mode(...)``. See
  :ref:`Tables <featherdoc-tables>`.
- Fill templates or inspect bookmarks:
  ``list_bookmarks()``, ``find_bookmark(...)``, ``validate_template(...)``,
  ``fill_bookmarks(...)``, ``replace_bookmark_with_*()``, and
  ``TemplatePart`` handles such as ``body_template()`` or
  ``section_header_template(...)``. See
  :ref:`Bookmarks And Templates <featherdoc-bookmarks-templates>`.
- Append or replace images and page fields:
  ``append_image(...)``, ``append_floating_image(...)``,
  ``replace_inline_image(...)``, ``replace_drawing_image(...)``,
  ``replace_bookmark_with_image(...)``,
  ``replace_bookmark_with_floating_image(...)``,
  ``append_page_number_field()``, and ``append_total_pages_field()``. See
  :ref:`Images <featherdoc-images>` and
  :ref:`Headers, Footers, Sections, And Page Setup <featherdoc-sections-page-setup>`.
- Work with styles, numbering, and language metadata:
  ``list_styles()``, ``find_style(...)``, ``ensure_*style(...)``,
  ``set_paragraph_style(...)``, ``set_run_style(...)``,
  ``ensure_numbering_definition(...)``, ``set_paragraph_numbering(...)``,
  ``set_paragraph_style_numbering(...)``, and the default run font/language
  helpers. See :ref:`Lists And Styles <featherdoc-lists-styles>` and
  :ref:`Creating New Documents And Language Defaults <featherdoc-doc-lifecycle>`.
- Inspect or mutate sections, headers, footers, and page setup:
  ``inspect_sections()``, ``get_section_page_setup(...)``,
  ``set_section_page_setup(...)``, ``ensure_header_paragraphs()``,
  ``ensure_footer_paragraphs()``,
  ``ensure_section_header_paragraphs(...)``,
  ``ensure_section_footer_paragraphs(...)``,
  ``assign_section_header_paragraphs(...)``,
  ``assign_section_footer_paragraphs(...)``, ``append_section()``,
  ``insert_section()``, ``remove_section()``, and ``move_section()``. See
  :ref:`Headers, Footers, Sections, And Page Setup <featherdoc-sections-page-setup>`.
- Reach for the CLI when you need scriptable inspection or one-shot edits:
  ``inspect-styles``, ``inspect-numbering``, ``inspect-page-setup``,
  ``inspect-bookmarks``, ``inspect-images``, ``inspect-sections``,
  ``set-section-page-setup``, ``append-page-number-field``, and
  ``validate-template``. See :ref:`CLI <featherdoc-cli>`.

The detailed walkthrough below follows roughly the same order, so this map is
meant to reduce scrolling and document-hopping rather than replace the examples.

.. _featherdoc-sample-cli-map:

.. rubric:: Task-Oriented Sample And CLI Map

When you want the fastest runnable starting point instead of the raw API names,
use the same buckets to jump straight to samples or CLI commands.

If you want the API-first version of the same categories, jump back to
:ref:`Task-Oriented API Map <featherdoc-api-map>`.

- Paragraph/run editing and reopen flows:
  ``featherdoc_sample_edit_existing``,
  ``featherdoc_sample_insert_paragraph_before``,
  ``featherdoc_sample_insert_paragraph_like_existing``,
  ``featherdoc_sample_insert_run_around_existing``, and
  ``featherdoc_sample_insert_run_like_existing``. Prefer the library samples
  here; the CLI currently focuses more on inspection and structural rewrites
  than free-form paragraph text editing. See
  :ref:`Text Editing <featherdoc-text-editing>`.
- Tables, cell formatting, and row/column rewrites:
  ``featherdoc_sample_insert_table_row``,
  ``featherdoc_sample_insert_table_row_before``,
  ``featherdoc_sample_insert_table_column``,
  ``featherdoc_sample_unmerge_table_cells``,
  ``featherdoc_sample_edit_existing_table_spacing``,
  ``featherdoc_sample_edit_existing_table_column_widths``,
  ``featherdoc_sample_edit_existing_table_style_look``, and
  ``featherdoc_sample_template_table_cli_visual``. On the CLI side, start with
  ``inspect-tables``, ``inspect-table-rows``, ``inspect-table-cells``,
  ``set-table-cell-text``, ``append-table-row``, ``insert-table-row-before``,
  ``insert-table-row-after``, ``remove-table-row``, ``merge-table-cells``,
  ``unmerge-table-cells``, and the ``set-template-table-cell-text`` /
  ``append-template-table-row`` family for template parts. See
  :ref:`Tables <featherdoc-tables>`.
- Images and drawing replacement:
  ``featherdoc_sample_floating_images``,
  ``featherdoc_sample_remove_images``, and
  ``featherdoc_sample_edit_existing_part_append_images``. The matching CLI
  entry points are ``inspect-images``, ``extract-image``, ``replace-image``,
  ``remove-image``, and ``append-image``. See
  :ref:`Images <featherdoc-images>`.
- Styles, numbering, and language-aware defaults:
  ``featherdoc_sample_restart_paragraph_list``,
  ``featherdoc_sample_style_linked_numbering``, and
  ``featherdoc_sample_chinese``. The matching CLI entry points are
  ``inspect-numbering`` and ``inspect-styles``. See
  :ref:`Lists And Styles <featherdoc-lists-styles>` and
  :ref:`Creating New Documents And Language Defaults <featherdoc-doc-lifecycle>`.
- Templates, bookmarks, and field insertion:
  ``featherdoc_sample_template_validation``,
  ``featherdoc_sample_part_template_validation``,
  ``featherdoc_sample_remove_bookmark_block``,
  ``featherdoc_sample_chinese_template``, and
  ``featherdoc_sample_page_number_fields``. The matching CLI entry points are
  ``inspect-bookmarks``, ``validate-template``,
  ``append-page-number-field``, and ``append-total-pages-field``. See
  :ref:`Bookmarks And Templates <featherdoc-bookmarks-templates>` and
  :ref:`Headers, Footers, Sections, And Page Setup <featherdoc-sections-page-setup>`.
- Sections, page setup, and header/footer layout:
  ``featherdoc_sample_section_page_setup``,
  ``featherdoc_sample_edit_existing_part_tables``, and
  ``featherdoc_sample_edit_existing_part_images``. The matching CLI entry
  points are ``inspect-sections``, ``inspect-header-parts``,
  ``inspect-footer-parts``, ``set-section-page-setup``,
  ``assign-section-header``, ``assign-section-footer``,
  ``remove-section-header``, and ``remove-section-footer``. See
  :ref:`Headers, Footers, Sections, And Page Setup <featherdoc-sections-page-setup>`.

For exact command flags, selectors, and JSON payload shapes, keep
:ref:`CLI <featherdoc-cli>` open next to the relevant detailed section below.

.. _featherdoc-text-editing:

.. rubric:: Text Editing

``Paragraph::set_text(...)`` replaces one paragraph's body content in place
while preserving paragraph-level properties such as style or bidi settings.
``Paragraph::insert_paragraph_before(...)`` and
``insert_paragraph_after(...)`` add a sibling paragraph around the current
anchor paragraph, ``insert_paragraph_like_before(...)`` and
``insert_paragraph_like_after(...)`` add a sibling paragraph that inherits the
anchor paragraph's paragraph-level properties, ``Run::insert_run_before(...)`` and
``insert_run_after(...)`` add sibling runs around the current anchor run,
``Run::insert_run_like_before(...)`` and ``insert_run_like_after(...)`` clone
the anchor run's formatting into a new empty sibling run, ``Run::remove()``
drops one run from a paragraph, and ``Paragraph::remove()``
deletes a paragraph only when doing so would not leave an invalid body,
header, footer, or table-cell container behind. The paragraph removal helper
also refuses to remove section-break paragraphs.

.. code-block:: cpp

    auto paragraph = doc.paragraphs();
    paragraph.set_text("anchor");

    auto prepended = paragraph.insert_paragraph_before("Lead-in");
    prepended.add_run(" note");

    auto cloned = paragraph.insert_paragraph_like_after();
    cloned.set_text("Another paragraph with the same paragraph style");

    auto anchor = paragraph.runs();
    anchor.insert_run_before("left ", featherdoc::formatting_flag::bold);
    anchor.insert_run_after(" right");

    auto cloned_run = anchor.insert_run_like_before();
    cloned_run.set_text("styled clone ");

    auto removable_run = paragraph.add_run(" temporary");
    removable_run.remove();

    auto removable_paragraph = paragraph.insert_paragraph_after("Delete me");
    removable_paragraph.remove();

For a runnable "edit an existing document and save it back" example, build
``featherdoc_sample_edit_existing`` from ``samples/sample_edit_existing.cpp``.
FeatherDoc already supports opening an existing ``.docx``, mutating
paragraphs, runs, table cells, inline body images, headers, footers, and
bookmark-backed template regions, and saving the result back to disk.
For a focused "reopen and replace existing header/footer images" example,
build ``featherdoc_sample_edit_existing_part_images`` from
``samples/sample_edit_existing_part_images.cpp``.
For a focused "reopen and append body/header/footer paragraphs through
TemplatePart handles" example, build
``featherdoc_sample_edit_existing_part_paragraphs`` from
``samples/sample_edit_existing_part_paragraphs.cpp``.
For a focused "reopen and insert body/header/footer paragraphs before existing
anchor paragraphs" example, build ``featherdoc_sample_insert_paragraph_before``
from ``samples/sample_insert_paragraph_before.cpp``.
For a focused "reopen and clone paragraph formatting around existing
body/header/footer anchor paragraphs" example, build
``featherdoc_sample_insert_paragraph_like_existing`` from
``samples/sample_insert_paragraph_like_existing.cpp``.
For a focused "reopen and insert runs around existing body/header/footer anchor
runs" example, build ``featherdoc_sample_insert_run_around_existing`` from
``samples/sample_insert_run_around_existing.cpp``.
For a focused "reopen and clone run formatting around existing body/header/footer
anchor runs" example, build ``featherdoc_sample_insert_run_like_existing`` from
``samples/sample_insert_run_like_existing.cpp``.
For a focused "reopen and append new images to existing body/header/footer
parts" example, build ``featherdoc_sample_edit_existing_part_append_images``
from ``samples/sample_edit_existing_part_append_images.cpp``.

.. _featherdoc-tables:

.. rubric:: Tables

``append_table(row_count, column_count)`` creates a new body table
programmatically. The returned ``Table`` can then grow through
``append_row()``, be widened through ``append_cell()``, or act as an anchor for
``insert_table_before(...)`` / ``insert_table_after(...)`` when you need to
insert new sibling tables around an existing one. Use
``insert_table_like_before()`` / ``insert_table_like_after()`` when you want to
clone the current table's layout and formatting into a new empty sibling table.

.. code-block:: cpp

    auto table = doc.append_table(2, 2);

    auto first_row = table.rows();
    auto first_cell = first_row.cells();
    first_cell.set_text("r0c0");
    first_cell.next();
    first_cell.set_text("r0c1");

    auto extra_row = table.append_row();
    auto extra_cell = extra_row.cells();
    extra_cell.set_text("tail");
    extra_row.append_cell().set_text("tail-2");

    auto inserted = table.insert_table_after(1, 2);
    inserted.rows().cells().set_text("inserted");

``Table::set_width_twips(...)``, ``set_column_width_twips(...)``,
``clear_column_width()``, ``set_style_id(...)``, ``set_border(...)``,
``set_layout_mode(...)``, ``set_alignment(...)``, ``set_indent_twips(...)``,
``set_cell_spacing_twips(...)``, ``set_cell_margin_twips(...)``, and
``set_style_look(...)`` work alongside
``TableCell::set_text(...)`` and ``get_text()``, plus ``set_width_twips(...)``,
``Table::remove()``, ``insert_table_before()``, ``insert_table_after()``,
``insert_table_like_before()``, ``insert_table_like_after()``,
``TableCell::remove()``, ``insert_cell_before()``, ``insert_cell_after()``,
``TableRow::remove()``, ``insert_row_before()``,
``insert_row_after()``,
``merge_right(...)``, ``merge_down(...)``, ``unmerge_right()``,
``unmerge_down()``,
``set_vertical_alignment(...)``,
``set_text_direction(...)``,
``set_border(...)``, ``set_fill_color(...)``, and ``set_margin_twips(...)``
for higher-level table layout edits without dropping down to raw XML.
``width_twips()`` reports an explicit ``dxa`` width when present,
``column_width_twips(column_index)`` reports the current ``w:tblGrid`` width
for one grid column, ``style_id()`` reports the current table style reference,
``style_look()`` reports the current first/last row or column emphasis
together with row/column banding flags, ``layout_mode()`` reports the current
auto-fit mode, ``alignment()`` / ``indent_twips()`` report table placement,
``cell_spacing_twips()`` reports inter-cell spacing, and
``cell_margin_twips()`` reports per-edge default cell margins,
``height_twips()`` / ``height_rule()`` report the current row height override,
``cant_split()`` reports whether Word keeps the row on one page,
``repeats_header()`` reports whether a row repeats as a table header,
``Table::remove()`` deletes one table while refusing to leave the parent
container without the required block content and retargets the wrapper to the
next surviving table when possible (otherwise the previous one).
``TableCell::remove()`` deletes one unmerged column across the whole table
while refusing to remove the last remaining column, refusing any column that
intersects a horizontal merge span, and retargeting the wrapper to the next
surviving cell when possible (otherwise the previous one). When the table uses
explicit ``w:tblGrid`` widths, removal also drops the matching ``w:gridCol``
entry so the deleted column's saved grid width disappears with it.
``TableCell::insert_cell_before()`` and ``insert_cell_after()`` clone the
target column structure across every row, expand ``w:tblGrid``, clear copied
``w:gridSpan`` / ``w:vMerge`` markup on the inserted cells, copy the
source-side ``w:gridCol`` width into the inserted grid column, and refuse
insertion boundaries that would land inside a horizontal merge span.
On tables that are already fixed-layout, or that later switch to
fixed-layout, with explicit widths for every ``w:gridCol``,
``set_layout_mode(featherdoc::table_layout_mode::fixed)``,
``set_column_width_twips(...)``, ``TableCell::merge_right()``,
``unmerge_right()``, ``TableCell::insert_cell_before()``,
``insert_cell_after()``, and ``TableCell::remove()`` also normalize each
cell's ``w:tcW`` to match the grid, while ``clear_column_width()`` drops
``w:tcW`` from any cell that still covers the cleared grid column so stale
width hints do not linger.
Switching to ``autofit`` or calling ``clear_layout_mode()`` does not erase the
saved ``w:gridCol`` / ``w:tcW`` widths; it only changes the layout algorithm
Word uses when rendering the table.
Likewise, ``set_width_twips(...)`` and ``clear_width()`` only rewrite
``w:tblW``; they do not rescale or discard the saved ``w:gridCol`` /
``w:tcW`` widths.
When a table later re-runs fixed-grid normalization, any manual
``TableCell::set_width_twips(...)`` / ``clear_width()`` edits on cells covered
by that complete grid are overwritten back to the summed ``w:gridCol``
widths.
For reopened legacy fixed-layout tables whose saved ``w:tcW`` no longer
matches ``w:gridCol``, reapplying
``set_layout_mode(featherdoc::table_layout_mode::fixed)`` forces that same
normalization pass.
The same normalization also runs when those reopened fixed-layout tables go
through explicit span or column edits such as ``merge_right()``,
``unmerge_right()``, ``set_column_width_twips(...)``,
``insert_cell_before()`` / ``insert_cell_after()``, or
``TableCell::remove()``.
``clear_column_width()`` follows the same reopened fixed-layout path for the
cleared grid column, but only clears ``w:tcW`` on cells that still cover that
column instead of re-normalizing unrelated stale cell widths.
Plain non-grid edits such as ``TableCell::set_text(...)``,
``set_fill_color(...)``, and ``set_border(...)`` do not trigger that
normalization on their own.
The same is true for row-only structural edits:
``TableRow::insert_row_before()``, ``insert_row_after()``, and ``remove()``
keep reopened stale ``w:tcW`` values as-is instead of re-normalizing them from
``w:tblGrid``.
``unmerge_right()`` splits one pure horizontal merged cell back into standalone
cells in the current row while keeping the anchor text in the leftmost cell and
leaving restored siblings empty for later editing.
``insert_table_before()`` and ``insert_table_after()`` create a new empty
sibling table directly before or after the selected table and retarget the
wrapper to the inserted table. ``insert_table_like_before()`` and
``insert_table_like_after()`` clone the selected table's structure plus
table/row/cell formatting into a new sibling table, clear the cloned cell
content, and retarget the wrapper to the inserted table.
``TableRow::remove()`` deletes one row while refusing to remove the last
remaining row, ``insert_row_before()`` and ``insert_row_after()`` clone the
current row structure into a new empty row directly before or after it while
refusing rows that participate in vertical merge chains, and
``unmerge_down()`` removes one whole vertical merge chain even when the current
wrapper points at a continuation cell, leaving the restored lower cells empty
until the caller writes new content. ``column_span()`` reports the current
horizontal span. ``text_direction()``
reports the current table-cell writing direction when a cell uses vertical or
rotated text flow. ``TableCell::set_text(...)`` replaces one cell's body with
a single paragraph while preserving cell-level properties such as shading,
margins, borders, and explicit width.

.. code-block:: cpp

    auto table = doc.append_table(1, 3);
    table.set_width_twips(7200);
    table.set_column_width_twips(0, 1800);
    table.set_column_width_twips(1, 2400);
    table.set_column_width_twips(2, 3000);
    table.set_style_id("TableGrid");
    table.set_style_look({true, false, false, false, true, false});
    table.set_layout_mode(featherdoc::table_layout_mode::fixed);
    table.set_alignment(featherdoc::table_alignment::center);
    table.set_indent_twips(240);
    table.set_cell_spacing_twips(120);
    table.set_cell_margin_twips(featherdoc::cell_margin_edge::left, 96);
    table.set_cell_margin_twips(featherdoc::cell_margin_edge::right, 96);
    table.set_border(featherdoc::table_border_edge::inside_vertical,
                     {featherdoc::border_style::single, 8, "808080", 0});

    auto row = table.rows();
    row.set_height_twips(360, featherdoc::row_height_rule::exact);
    row.set_cant_split();
    row.set_repeats_header();
    auto cell = row.cells();

    cell.set_width_twips(2400);
    cell.set_vertical_alignment(featherdoc::cell_vertical_alignment::center);
    cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left);
    cell.set_fill_color("D9EAF7");
    cell.set_margin_twips(featherdoc::cell_margin_edge::left, 120);
    cell.set_margin_twips(featherdoc::cell_margin_edge::right, 120);
    cell.paragraphs().add_run("Merged title");
    cell.merge_right(1);
    cell.set_border(featherdoc::cell_border_edge::bottom,
                    {featherdoc::border_style::thick, 12, "000000", 0});

    auto next_row = table.append_row(3);
    auto merged_column = next_row.cells();
    merged_column.paragraphs().add_run("Below");
    cell = row.cells();
    cell.merge_down(1);

    auto inserted_row = row.insert_row_after();
    inserted_row.cells().set_text("Inserted");

    auto inserted_before = next_row.insert_row_before();
    inserted_before.cells().set_text("Inserted above");

    std::cout << cell.column_span() << std::endl; // 2

``style_look()`` only changes the style-routing flags stored in ``w:tblLook``,
so visible differences depend on the table style definition present in the
document. Use it when you want to keep the same style id but switch which rows
or columns the current style treats as emphasized or banded.

For runnable insertion examples, build ``featherdoc_sample_insert_table_row``
from ``samples/sample_insert_table_row.cpp`` for the
``insert_row_after()`` flow, or ``featherdoc_sample_insert_table_row_before``
from ``samples/sample_insert_table_row_before.cpp`` for the
``insert_row_before()`` flow. Both samples create a seed table, reopen the
saved ``.docx``, insert a cloned row in the middle, and write the updated
result back out.

For a runnable column-insertion example, build
``featherdoc_sample_insert_table_column`` from
``samples/sample_insert_table_column.cpp``. It seeds a two-column table,
reopens the saved ``.docx``, inserts one cloned column after the first column
and another before the result column, and writes the updated four-column
result back out.

For a runnable unmerge example, build
``featherdoc_sample_unmerge_table_cells`` from
``samples/sample_unmerge_table_cells.cpp``. It creates one horizontal merge and
one vertical merge, reopens the saved ``.docx``, splits them back into
standalone cells, and continues editing the restored cells.

For a runnable table-removal example, build
``featherdoc_sample_remove_table`` from
``samples/sample_remove_table.cpp``. It creates three body tables, reopens the
saved ``.docx``, removes the temporary middle table, and continues editing the
following table through the same wrapper.
For a runnable table-column removal example, build
``featherdoc_sample_remove_table_column`` from
``samples/sample_remove_table_column.cpp``. It creates a three-column body
table, reopens the saved ``.docx``, removes the temporary middle column, and
continues editing the surviving result column through the same cell wrapper.
For a runnable table-insertion example, build
``featherdoc_sample_insert_table_around_existing`` from
``samples/sample_insert_table_around_existing.cpp``. It reopens a saved
``.docx``, inserts new tables directly before and after an existing anchor
table, and continues editing the surrounding tables through the returned
wrappers.
For a runnable styled-table cloning example, build
``featherdoc_sample_insert_table_like_existing`` from
``samples/sample_insert_table_like_existing.cpp``. It reopens a saved
``.docx``, duplicates an existing table's layout and styling into new empty
sibling tables, fills the clones, and keeps the original anchor table
editable.
For a runnable table-spacing edit example, build
``featherdoc_sample_edit_existing_table_spacing`` from
``samples/sample_edit_existing_table_spacing.cpp``. It reopens a saved
``.docx``, adds ``tblCellSpacing`` to an existing table, and makes the new
gutters visible in Word's rendered output.
For a runnable table-column-width edit example, build
``featherdoc_sample_edit_existing_table_column_widths`` from
``samples/sample_edit_existing_table_column_widths.cpp``. It reopens a saved
``.docx``, rewrites ``w:tblGrid`` column widths on an existing table, and
makes the narrow/medium/wide column split visible in Word's rendered output.
For a runnable table-style-look edit example, build
``featherdoc_sample_edit_existing_table_style_look`` from
``samples/sample_edit_existing_table_style_look.cpp``. It reopens a saved
``.docx``, updates ``tblLook`` on an existing table, and keeps the original
table style reference in place.

.. _featherdoc-images:

.. rubric:: Images

``append_image(path)`` appends an inline image at the source image's intrinsic
pixel size. Use ``append_image(path, width_px, height_px)`` when you want
explicit scaling. These APIs are available on both ``Document`` and
``TemplatePart``, so you can append images to the main body or to an existing
body/header/footer part wrapper. The current image support is limited to
``.png``, ``.jpg``, ``.jpeg``, ``.gif``, and ``.bmp``.

.. code-block:: cpp

    doc.append_image("logo.png");
    doc.append_image("badge.png", 96, 48);

    auto header_template = doc.section_header_template(0);
    header_template.append_image("header-logo.png", 144, 48);

``append_floating_image(path, options)`` and
``append_floating_image(path, width_px, height_px, options)`` create anchored
``wp:anchor`` images with explicit page/margin-relative offsets.
``floating_image_options`` currently lets you pick horizontal/vertical
reference frames, pixel offsets, whether the image sits behind text, whether
overlap is allowed, the floating wrap mode plus wrap distances, and an
optional per-edge crop expressed in per-mille units. The same API works on
``Document`` and ``TemplatePart``. Use ``wrap_mode`` when you want
``wrapNone``, square wrapping, or top/bottom-only flow around the anchored
drawing. Use ``crop`` when you want FeatherDoc to emit ``a:srcRect`` trimming
on the anchored picture fill.

.. code-block:: cpp

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    options.horizontal_offset_px = 460;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 24;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    options.wrap_distance_left_px = 12;
    options.wrap_distance_right_px = 12;
    options.crop = featherdoc::floating_image_crop{80, 0, 120, 0};

    doc.append_floating_image("badge.png", 144, 48, options);

    auto body_template = doc.body_template();
    body_template.append_floating_image("stamp.png", 128, 48, options);

``inline_images()`` inspects inline images that already exist in the main
document body. Each returned ``inline_image_info`` includes the image index,
relationship id, media entry path, display name, content type, and rendered
pixel size derived from ``wp:extent``.

``drawing_images()`` returns the broader existing drawing image list for the
current part, including both ``wp:inline`` and ``wp:anchor`` objects. Each
``drawing_image_info`` carries the same metadata plus a
``drawing_image_placement`` value that tells you whether the object is inline
or anchored. Anchored objects also expose optional ``floating_options`` with
parsed reference targets, pixel offsets, wrap mode, wrap distances, overlap
flags, and optional crop percentages read back from the current XML.

``extract_drawing_image(index, path)`` copies any existing drawing-backed
image out of the ``.docx``. ``replace_drawing_image(index, path)`` swaps one
inline or anchored image with a new file while preserving the current rendered
size and placement XML. ``remove_drawing_image(index)`` deletes one inline or
anchored drawing and drops its media part on the next save once no
relationship still references it.

.. code-block:: cpp

    const auto drawings = doc.drawing_images();
    for (const auto &image : drawings) {
        if (image.placement ==
            featherdoc::drawing_image_placement::anchored_object) {
            doc.remove_drawing_image(image.index);
        }
    }

The CLI exposes the same metadata through ``featherdoc_cli inspect-images``
using the same body/header/footer/section-part selection shape as
``inspect-bookmarks``. You can also narrow the result set by relationship id
or resolved media entry path before inspecting the whole list or one image:

.. code-block:: sh

    featherdoc_cli inspect-images report.docx
    featherdoc_cli inspect-images report.docx --relationship-id rId5 --image-entry-name word/media/image1.png --json
    featherdoc_cli inspect-images report.docx --part header --index 0 --image 0 --json

To export one existing drawing-backed image without mutating the ``.docx``,
use ``featherdoc_cli extract-image`` with the same selectors plus the
destination file path:

.. code-block:: sh

    featherdoc_cli extract-image report.docx exported.png --image 1 --json
    featherdoc_cli extract-image report.docx exported.png --part header --index 0 --image-entry-name word/media/image1.png

To replace one existing drawing-backed image while preserving the current size
and inline/anchor placement XML, use ``featherdoc_cli replace-image`` with the
same part/image selectors plus the new image path:

.. code-block:: sh

    featherdoc_cli replace-image report.docx replacement.gif --image 1 --output updated.docx --json
    featherdoc_cli replace-image report.docx replacement.gif --part header --index 0 --image-entry-name word/media/image1.png

To delete one existing drawing-backed image from the selected part, use
``featherdoc_cli remove-image`` with the same selectors plus an optional
``--output`` path:

.. code-block:: sh

    featherdoc_cli remove-image report.docx --image 1 --output pruned.docx --json
    featherdoc_cli remove-image report.docx --part header --index 0 --relationship-id rId5

To append a new image paragraph into the body, a header/footer part, or a
section-scoped header/footer reference, use ``featherdoc_cli append-image``.
The default output is an inline drawing; add ``--floating`` (or any floating
layout option) when you want an anchored ``wp:anchor`` image instead. Use
``--width`` plus ``--height`` for explicit scaling, or omit them to keep the
source image's intrinsic size:

.. code-block:: sh

    featherdoc_cli append-image report.docx logo.png --width 96 --height 48 --output with-logo.docx --json
    featherdoc_cli append-image report.docx badge.png --part section-header --section 0 --floating --horizontal-reference page --horizontal-offset 40 --vertical-reference margin --vertical-offset 12 --wrap-mode square --output header-badge.docx

``extract_inline_image(index, path)`` copies one existing inline body image out
of the ``.docx``. ``replace_inline_image(index, path)`` swaps one body image
with a new file while preserving the current displayed size.
``remove_inline_image(index)`` deletes only inline images. Replacement only
retargets the selected relationship. If the old media part becomes
unreferenced afterwards, FeatherDoc drops it from the next saved archive.

.. code-block:: cpp

    const auto images = doc.inline_images();
    if (!images.empty()) {
        doc.extract_inline_image(images[0].index, "first-image.bin");
        doc.remove_inline_image(images[0].index);
    }

The same ``drawing_images()``, ``extract_drawing_image(...)``,
``remove_drawing_image(...)``, ``replace_drawing_image(...)``,
``inline_images()``, ``extract_inline_image(...)``,
``remove_inline_image(...)``, and ``replace_inline_image(...)`` APIs are also
available on ``TemplatePart`` handles when you need existing body/header/footer
drawings from already loaded parts.

For runnable visual samples, build ``featherdoc_sample_floating_images`` from
``samples/sample_floating_images.cpp`` when you want mixed inline/floating
body drawings, or ``featherdoc_sample_remove_images`` from
``samples/sample_remove_images.cpp`` when you want a minimal existing-image
removal workflow.

.. _featherdoc-lists-styles:

.. rubric:: Lists And Styles

``set_paragraph_list(paragraph, kind, level)`` attaches managed bullet or
decimal numbering to a paragraph. Use
``restart_paragraph_list(paragraph, kind, level)`` when you want a fresh
managed list sequence that starts over at the default marker for that kind,
and ``clear_paragraph_list(paragraph)`` when you want to remove the list
marker again.

.. code-block:: cpp

    auto item = doc.paragraphs();
    doc.set_paragraph_list(item, featherdoc::list_kind::bullet);
    item.add_run("first item");

    auto nested = item.insert_paragraph_after("");
    doc.set_paragraph_list(nested, featherdoc::list_kind::decimal, 1);
    nested.add_run("nested item");

    auto restarted = nested.insert_paragraph_after("");
    doc.restart_paragraph_list(restarted, featherdoc::list_kind::decimal);
    restarted.add_run("restarted item 1");

For a runnable list-restart example, build
``featherdoc_sample_restart_paragraph_list`` from
``samples/sample_restart_paragraph_list.cpp``. It reopens a saved ``.docx``,
starts a second decimal list from ``1.``, and keeps the restarted sequence
consistent in Word's rendered output.
For a runnable style-linked numbering example, build
``featherdoc_sample_style_linked_numbering`` from
``samples/sample_style_linked_numbering.cpp``. It creates custom heading/body
styles, attaches one shared numbering definition to the heading styles, and
shows how outline numbering can stay style-driven instead of writing
``w:numPr`` on every paragraph.

When you need an explicit custom numbering definition instead of the managed
bullet or decimal presets, use ``ensure_numbering_definition(...)`` to create
or refresh a named abstract numbering definition, then attach it with
``set_paragraph_numbering(...)``.

.. code-block:: cpp

    featherdoc::numbering_definition outline{};
    outline.name = "LegalOutline";
    outline.levels = {
        {featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        {featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(outline);
    if (numbering_id.has_value()) {
        auto paragraph = doc.paragraphs();
        doc.set_paragraph_numbering(paragraph, *numbering_id);
    }

If you want paragraphs to inherit numbering through a paragraph style instead
of writing ``w:numPr`` on each paragraph, attach the numbering definition to
the style once with ``set_paragraph_style_numbering(...)``.

.. code-block:: cpp

    doc.ensure_paragraph_style("LegalHeading", body_style);

    const auto numbering_id = doc.ensure_numbering_definition(outline);
    if (numbering_id.has_value()) {
        doc.set_paragraph_style_numbering("LegalHeading", *numbering_id, 1U);
    }

Use ``list_numbering_definitions()`` when you need the current numbering
catalog as ``numbering_definition_summary`` records, or
``find_numbering_definition(definition_id)`` when you want one definition's
levels and attached instance ids. Use ``find_numbering_instance(instance_id)``
to resolve a specific ``numId`` back to its definition and override summary.

.. code-block:: cpp

    const auto numbering = doc.list_numbering_definitions();
    const auto outline_summary = numbering_id.has_value()
                                     ? doc.find_numbering_definition(*numbering_id)
                                     : std::nullopt;
    const auto first_instance =
        outline_summary.has_value() && !outline_summary->instance_ids.empty()
            ? doc.find_numbering_instance(outline_summary->instance_ids.front())
            : std::nullopt;

The CLI exposes the same metadata through ``inspect-numbering``:

.. code-block:: sh

    featherdoc_cli inspect-numbering report.docx
    featherdoc_cli inspect-numbering report.docx --instance 1
    featherdoc_cli inspect-numbering report.docx --definition 1 --json

``set_paragraph_style(paragraph, style_id)`` and ``set_run_style(run,
style_id)`` attach paragraph/run style references. When the source document
does not already contain ``word/styles.xml``, FeatherDoc creates a minimal
styles part automatically. The generated catalog currently includes
``Normal``, ``Heading1``, ``Heading2``, ``Quote``,
``DefaultParagraphFont``, ``Emphasis``, ``Strong``, ``TableNormal``, and
``TableGrid``. Use ``list_styles()`` when you want the current catalog as
``style_summary`` records, or ``find_style(style_id)`` when you need one
style's kind, display name, ``basedOn``, and common visibility or quick-format
flags before attaching or editing style references. When a paragraph style
already carries numbering metadata, ``style_summary::numbering`` also includes
the resolved definition fields and the concrete numbering ``instance``
summary.

.. code-block:: cpp

    const auto styles = doc.list_styles();
    const auto heading = doc.find_style("Heading1");

    auto paragraph = doc.paragraphs();
    doc.set_paragraph_style(paragraph, "Heading1");

    auto styled_run = paragraph.add_run("Styled heading");
    doc.set_run_style(styled_run, "Strong");

    doc.clear_run_style(styled_run);
    doc.clear_paragraph_style(paragraph);

The CLI exposes the same metadata through ``inspect-styles``:

.. code-block:: sh

    featherdoc_cli inspect-styles report.docx
    featherdoc_cli inspect-styles report.docx --style BodyText --json
    featherdoc_cli inspect-styles report.docx --style BodyText --usage

When a paragraph style carries ``w:numPr``, ``inspect-styles`` also resolves the
referenced numbering instance so JSON output includes the concrete
``instance`` summary and text output reports the override count.
When you add ``--usage`` to a single-style inspection, the CLI also reports how
many paragraphs, runs, and tables across ``word/document.xml``, headers, and
footers reference that style. JSON and text output both include the aggregated
totals plus separate ``body``, ``header``, and ``footer`` breakdowns.
The usage payload also includes a ``hits`` list so you can see which part each
match came from and its document-order ordinal within that part. Body hits now
also expose ``section_index`` directly so each main-document match can be
mapped back to its owning section. Header/footer hits keep ``section_index``
empty and instead expose a ``references`` array so shared parts report every
referencing ``section_index`` plus its ``default`` / ``first`` / ``even``
reference kind.

Use ``ensure_paragraph_style(...)``, ``ensure_character_style(...)``, and
``ensure_table_style(...)`` when you want to create or refresh a custom style
definition declaratively without replacing unrelated XML already stored on the
style node.

.. code-block:: cpp

    featherdoc::paragraph_style_definition body_style{};
    body_style.name = "Body Text";
    body_style.based_on = std::string{"Normal"};
    body_style.run_font_family = std::string{"Segoe UI"};
    body_style.run_language = std::string{"en-US"};
    body_style.is_quick_format = true;

    doc.ensure_paragraph_style("BodyText", body_style);

Formatting
----------
Use the scoped formatting flags when creating new runs.

.. code-block:: cpp

    paragraph.add_run("bold text", featherdoc::formatting_flag::bold);
    paragraph.add_run(
        "mixed style",
        featherdoc::formatting_flag::bold |
        featherdoc::formatting_flag::italic |
        featherdoc::formatting_flag::underline
    );

Document API
------------
``Document`` now stores its file location as ``std::filesystem::path`` and
exposes explicit success/failure through ``std::error_code`` from ``open()``
and ``save()``. The ``last_error()`` accessor exposes structured failure
context for the most recent operation.

.. code-block:: cpp

    featherdoc::Document doc("file.docx");
    if (const auto error = doc.open()) {
        const auto& error_info = doc.last_error();
        std::cerr << error.message() << std::endl;
        std::cerr << error_info.detail << std::endl;
        return 1;
    }

    if (const auto error = doc.save()) {
        const auto& error_info = doc.last_error();
        std::cerr << error.message() << std::endl;
        std::cerr << error_info.detail << std::endl;
        return 1;
    }

``last_error()`` provides:

- ``code``: the last ``std::error_code``
- ``detail``: richer diagnostic text
- ``entry_name``: the ZIP entry involved in the failure
- ``xml_offset``: parse offset for malformed XML

``save_as(path)`` writes the modified document to a new output path without
changing ``Document::path()``.

.. code-block:: cpp

    if (const auto error = doc.save_as("copy.docx")) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

.. _featherdoc-bookmarks-templates:

.. rubric:: Bookmarks And Templates

``replace_bookmark_text(name, replacement)`` rewrites the content enclosed by a
named bookmark range and returns the number of bookmark ranges replaced.

.. code-block:: cpp

    if (doc.replace_bookmark_text("customer_name", "FeatherDoc User") == 0) {
        std::cerr << "bookmark not found" << std::endl;
        return 1;
    }

``fill_bookmarks(...)`` is the first higher-level template entrypoint for
batch bookmark text filling. It accepts bookmark bindings, rewrites every
matching bookmark range, and reports which requested fields were missing.

.. code-block:: cpp

    const auto result = doc.fill_bookmarks({
        {"customer_name", "FeatherDoc User"},
        {"invoice_no", "INV-2026-0001"},
        {"due_date", "2026-04-30"},
    });

    if (!result) {
        for (const auto& missing : result.missing_bookmarks) {
            std::cerr << "missing bookmark: " << missing << std::endl;
        }
    }

``list_bookmarks()`` and ``find_bookmark(...)`` provide a read-only summary of
the currently loaded bookmark placeholders before you start filling or
rewriting them. Each ``bookmark_summary`` reports the bookmark name,
occurrence count, and inferred shape (``text``, ``block``, ``table_rows``,
``block_range``, ``mixed``, or ``malformed``).

.. code-block:: cpp

    const auto bookmarks = doc.list_bookmarks();
    const auto line_items = doc.find_bookmark("line_items");

    if (line_items.has_value() &&
        line_items->kind == featherdoc::bookmark_kind::table_rows) {
        std::cout << "line_items is a table-row placeholder" << std::endl;
    }

The CLI exposes the same metadata through ``featherdoc_cli inspect-bookmarks``:

.. code-block:: sh

    featherdoc_cli inspect-bookmarks invoice.docx
    featherdoc_cli inspect-bookmarks report.docx --part header --index 0 --bookmark header_rows --json

``validate_template(...)`` provides a read-only schema pass for body templates
before you start filling bookmarks. It reports missing required slots,
duplicate bookmark names, and placeholder shapes that do not match the
requested slot kind.

.. code-block:: cpp

    const auto validation = doc.validate_template({
        {"customer_name", featherdoc::template_slot_kind::text, true},
        {"line_item_row", featherdoc::template_slot_kind::table_rows, true},
        {"signature_block", featherdoc::template_slot_kind::block, false},
    });

    if (!validation) {
        for (const auto& missing : validation.missing_required) {
            std::cerr << "missing required slot: " << missing << std::endl;
        }
        for (const auto& duplicate : validation.duplicate_bookmarks) {
            std::cerr << "duplicate bookmark: " << duplicate << std::endl;
        }
        for (const auto& malformed : validation.malformed_placeholders) {
            std::cerr << "malformed placeholder: " << malformed << std::endl;
        }
    }

For a runnable sample, build ``featherdoc_sample_template_validation`` from
``samples/sample_template_validation.cpp``. It writes both a valid and an
invalid template plus Markdown/JSON validation reports under
``output/template-validation-visual``.

The CLI exposes the same body-template check through
``featherdoc_cli validate-template``:

.. code-block:: sh

    featherdoc_cli validate-template invoice.docx \
      --part body \
      --slot customer_name:text \
      --slot line_item_row:table_rows \
      --slot signature_block:block:optional \
      --json

``replace_bookmark_with_paragraphs(...)`` replaces a bookmark that occupies
its own paragraph with zero or more plain-text paragraphs. Passing an empty
list removes the placeholder paragraph.

.. code-block:: cpp

    doc.replace_bookmark_with_paragraphs(
        "line_items",
        {
            "Apple",
            "Pear",
            "Orange",
        });

``remove_bookmark_block(...)`` deletes that same standalone bookmark paragraph
directly when you do not need to build an explicit empty replacement list.

.. code-block:: cpp

    doc.remove_bookmark_block("optional_note");

For a runnable example, build ``featherdoc_sample_remove_bookmark_block`` from
``samples/sample_remove_bookmark_block.cpp``. It opens the Chinese invoice
template, fills the core fields, expands the item table, removes the
standalone ``note_lines`` bookmark block, and saves the result for visual
review.

``replace_bookmark_with_table_rows(...)`` replaces a bookmark that occupies
its own paragraph inside a template table row with zero or more cloned rows.
The template row's row/cell properties are preserved, while each generated
cell body is rewritten to a single plain-text paragraph. Passing an empty list
removes the template row.

.. code-block:: cpp

    doc.replace_bookmark_with_table_rows(
        "line_item_row",
        {
            {"Apple", "2"},
            {"Pear", "5"},
            {"Orange", "1"},
        });

For a runnable Chinese business example, build
``featherdoc_sample_chinese_template`` from
``samples/sample_chinese_template.cpp``. It opens
``samples/chinese_invoice_template.docx``, fills Chinese customer fields,
expands a bookmarked table row into a three-column quote table, and writes a
finished output document.

``replace_bookmark_with_table(...)`` replaces a bookmark that occupies its own
paragraph with a generated table block.

.. code-block:: cpp

    doc.replace_bookmark_with_table(
        "line_items",
        {
            {"Name", "Qty"},
            {"Apple", "2"},
            {"Pear", "5"},
        });

``replace_bookmark_with_image(...)`` replaces a bookmark that occupies its own
paragraph with an inline image paragraph. The overload without dimensions uses
the source image size; the second overload applies explicit scaling.

.. code-block:: cpp

    doc.replace_bookmark_with_image("company_logo", "logo.png");
    doc.replace_bookmark_with_image("stamp", "stamp.png", 96, 48);

``replace_bookmark_with_floating_image(...)`` performs the same block-level
bookmark replacement, but emits an anchored image paragraph instead. The
floating placement uses the same ``floating_image_options`` struct as
``append_floating_image(...)``, including wrap mode and wrap-distance control.

.. code-block:: cpp

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;

    doc.replace_bookmark_with_floating_image("callout", "badge.png", 144, 48,
                                             options);

``body_template()``, ``header_template(index)``, ``footer_template(index)``,
``section_header_template(section_index, kind)``, and
``section_footer_template(section_index, kind)`` return a lightweight
``TemplatePart`` handle for running the same bookmark-based template APIs on
an already loaded body/header/footer part. A valid handle exposes
``entry_name()``, ``paragraphs()``, ``append_paragraph(...)``, ``tables()``,
``append_table(...)``,
``replace_bookmark_text(...)``, ``fill_bookmarks(...)``,
``validate_template(...)``,
``replace_bookmark_with_paragraphs(...)``, ``remove_bookmark_block(...)``,
``replace_bookmark_with_table_rows(...)``, and
``replace_bookmark_with_table(...)``, ``replace_bookmark_with_image(...)``,
``replace_bookmark_with_floating_image(...)``,
``drawing_images()``, ``extract_drawing_image(...)``,
``remove_drawing_image(...)``, ``replace_drawing_image(...)``,
``inline_images()``, ``extract_inline_image(...)``,
``remove_inline_image(...)``, ``replace_inline_image(...)``,
``set_bookmark_block_visibility(...)``, and
``apply_bookmark_block_visibility(...)``. Missing section-specific references
return an empty handle instead of creating a new part implicitly.

This also means header/footer and section-scoped template parts can be
preflighted with the same slot schema rules used by
``Document::validate_template(...)``. For a runnable example, build
``featherdoc_sample_part_template_validation`` from
``samples/sample_part_template_validation.cpp``. It writes one document with a
valid header schema and an intentionally invalid footer schema under
``output/part-template-validation-visual``.

The same flow is also available from the CLI:

.. code-block:: sh

    featherdoc_cli validate-template report.docx \
      --part section-footer \
      --section 0 \
      --slot footer_company:text \
      --slot footer_summary:block \
      --json

.. code-block:: cpp

    auto header_template = doc.section_header_template(0);
    if (header_template) {
        auto note = header_template.append_paragraph("Header note");
        note.add_run(" added after opening the template part.");
        auto summary_table = header_template.append_table(2, 2);
        summary_table.rows().cells().set_text("Header cell");
        header_template.replace_bookmark_with_image("header_logo", "logo.png");
        header_template.replace_bookmark_with_table_rows(
            "line_item_row",
            {
                {"Apple", "2"},
                {"Pear", "5"},
            });
    }

    auto footer_template = doc.section_footer_template(0);
    if (footer_template) {
        footer_template.fill_bookmarks({
            {"company_name", "Acme Corp"},
        });
        footer_template.remove_bookmark_block("optional_legal_notice");
        footer_template.replace_bookmark_with_paragraphs(
            "footer_lines",
            {
                "First line",
                "Second line",
            });
    }

``append_paragraph(...)`` appends a new paragraph to the existing
body/header/footer part and returns the appended ``Paragraph``, so you can
continue editing it immediately through ``add_run(...)``, ``set_text(...)``,
or the paragraph-level bidi helpers.

For a runnable existing-part table example, build
``featherdoc_sample_edit_existing_part_tables`` from
``samples/sample_edit_existing_part_tables.cpp``. It creates a default header,
adds three header tables, reopens the saved ``.docx``, removes the temporary
middle table, and continues editing the following header table through the same
wrapper.
For a runnable existing-part image-append example, build
``featherdoc_sample_edit_existing_part_append_images`` from
``samples/sample_edit_existing_part_append_images.cpp``. It seeds
body/header/footer content, reopens the saved ``.docx``, and appends new
inline or floating images through ``TemplatePart`` handles before saving the
edited result back out.

``set_bookmark_block_visibility(name, visible)`` and
``apply_bookmark_block_visibility(...)`` control optional template blocks backed
by a bookmark pair. The template must place ``w:bookmarkStart`` in its own
paragraph, ``w:bookmarkEnd`` in a later paragraph, and both marker paragraphs
must live under the same parent container. The sibling nodes between those
marker paragraphs may include multiple paragraphs or tables. Passing
``visible = true`` keeps the inner content and removes only the marker
paragraphs; passing ``false`` removes the whole block including the markers.

.. code-block:: cpp

    const auto visibility = doc.apply_bookmark_block_visibility({
        {"promo_block", false},
        {"legal_block", true},
    });

    if (!visibility) {
        for (const auto& missing : visibility.missing_bookmarks) {
            std::cerr << "missing block bookmark: " << missing << std::endl;
        }
    }

.. _featherdoc-sections-page-setup:

.. rubric:: Headers, Footers, Sections, And Page Setup

``header_count()``, ``footer_count()``, ``header_paragraphs(index)``, and
``footer_paragraphs(index)`` expose paragraph-level access to existing
header/footer parts.

.. code-block:: cpp

    for (std::size_t i = 0; i < doc.header_count(); ++i) {
        for (auto paragraph = doc.header_paragraphs(i); paragraph.has_next();
             paragraph.next()) {
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                std::cout << run.get_text() << std::endl;
            }
        }
    }

``ensure_header_paragraphs()`` and ``ensure_footer_paragraphs()`` create and
attach a default header/footer to the body-level section properties when the
document does not already have one.

.. code-block:: cpp

    auto header = doc.ensure_header_paragraphs();
    header.add_run("Generated header");

    auto footer = doc.ensure_footer_paragraphs();
    footer.add_run("Page 1");

``section_count()``, ``section_header_paragraphs(section_index, kind)``, and
``section_footer_paragraphs(section_index, kind)`` resolve the existing
header/footer reference attached to a specific section.

.. code-block:: cpp

    for (std::size_t i = 0; i < doc.section_count(); ++i) {
        auto header = doc.section_header_paragraphs(i);
        if (header.has_next()) {
            std::cout << header.runs().get_text() << std::endl;
        }
    }

``get_section_page_setup(section_index)`` reads the explicit page geometry
attached to a section. The returned ``section_page_setup`` contains the
resolved ``orientation``, ``width_twips``, ``height_twips``,
``margins``, and an optional ``page_number_start``. If the target section
does not expose an explicit ``w:sectPr`` / ``w:pgSz`` / ``w:pgMar``, the API
returns ``std::nullopt``.

.. code-block:: cpp

    for (std::size_t i = 0; i < doc.section_count(); ++i) {
        const auto page_setup = doc.get_section_page_setup(i);
        if (!page_setup.has_value()) {
            continue;
        }

        std::cout << i << ": "
                  << (page_setup->orientation ==
                              featherdoc::page_orientation::landscape
                          ? "landscape"
                          : "portrait")
                  << " " << page_setup->width_twips << "x"
                  << page_setup->height_twips << std::endl;
    }

``set_section_page_setup(section_index, setup)`` updates the explicit page
geometry for a section. FeatherDoc rewrites ``w:pgSz``, ``w:pgMar``, and
``w:pgNumType/@w:start`` while preserving unrelated attributes such as paper
codes, gutter values, or numbering formats. If the final section is still using
an implicit body-level layout, the API materializes an editable ``w:sectPr``
node automatically.

.. code-block:: cpp

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 1080U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 540U;
    setup.page_number_start = 1U;

    doc.set_section_page_setup(1, setup);

See ``samples/sample_section_page_setup.cpp`` for a complete two-section sample
that keeps the opening page portrait and rotates the appendix to landscape.

``ensure_section_header_paragraphs(section_index, kind)`` and
``ensure_section_footer_paragraphs(section_index, kind)`` create and attach a
missing header/footer reference for the requested section before editing it.
When ``kind`` is ``first_page`` or ``even_page``, FeatherDoc also enables the
required WordprocessingML switches automatically (``w:titlePg`` or
``word/settings.xml`` -> ``w:evenAndOddHeaders``).

.. code-block:: cpp

    auto even_header = doc.ensure_section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page);
    even_header.add_run("Even page header");

    auto first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    first_footer.add_run("First page footer");

``body_template()``, ``header_template(index)``, ``footer_template(index)``,
``section_header_template(section_index, kind)``, and
``section_footer_template(section_index, kind)`` return writable
``TemplatePart`` handles for already loaded parts. Use
``append_page_number_field()`` and ``append_total_pages_field()`` on those
handles when you need Word ``PAGE`` / ``NUMPAGES`` fields in body, header, or
footer content. For section-scoped fields, materialize the target part first
through ``ensure_section_header_paragraphs(...)`` or
``ensure_section_footer_paragraphs(...)`` when it does not exist yet.

.. code-block:: cpp

    auto header_part = doc.ensure_section_header_paragraphs(0);
    header_part.set_text("Page ");

    auto header_template = doc.section_header_template(0);
    if (header_template) {
        header_template.append_page_number_field();
    }

For a runnable section/header/footer field example, build
``featherdoc_sample_page_number_fields`` from
``samples/sample_page_number_fields.cpp``.

``assign_section_header_paragraphs(section_index, header_index, kind)`` and
``assign_section_footer_paragraphs(section_index, footer_index, kind)`` rebind
the requested section to an already loaded header/footer part instead of
creating a new one. Each call only updates the requested reference ``kind``, so
sharing one part across multiple kinds on the same section needs one call per
kind.

.. code-block:: cpp

    auto shared_header = doc.assign_section_header_paragraphs(1, 0);
    shared_header.runs().set_text("Shared header");

    auto shared_footer = doc.assign_section_footer_paragraphs(1, 0);
    shared_footer.runs().set_text("Shared footer");

    auto shared_first_footer = doc.assign_section_footer_paragraphs(
        1, 0, featherdoc::section_reference_kind::first_page);
    shared_first_footer.runs().set_text("Shared footer");

``remove_section_header_reference(section_index, kind)`` and
``remove_section_footer_reference(section_index, kind)`` detach one
section-specific header/footer reference without modifying other kinds already
attached to the same section.

.. code-block:: cpp

    doc.remove_section_header_reference(1);
    doc.remove_section_footer_reference(
        1, featherdoc::section_reference_kind::first_page);

When a header/footer part is no longer referenced by ``document.xml``,
``save()`` / ``save_as()`` automatically omit that orphaned part together with
the matching ``document.xml.rels`` relationship and
``[Content_Types].xml`` override.
Removing the last first-page or even-page reference also removes
``w:titlePg`` or ``w:evenAndOddHeaders`` when that marker is no longer needed.

``remove_header_part(index)`` and ``remove_footer_part(index)`` remove one
loaded header/footer part entirely. Matching section references are detached in
memory, ``header_count()`` / ``footer_count()`` shrink immediately, and the
orphaned ZIP entries disappear on the next save.

.. code-block:: cpp

    doc.remove_header_part(1);
    doc.remove_footer_part(1);

``copy_section_header_references(source_section, target_section)`` and
``copy_section_footer_references(source_section, target_section)`` make the
target section adopt the source section's current header/footer reference
layout. Existing target-side references of that family are replaced, so stale
``first`` / ``even`` references disappear automatically.

.. code-block:: cpp

    doc.copy_section_header_references(0, 1);
    doc.copy_section_footer_references(0, 1);

``replace_section_header_text(section_index, replacement, kind)`` and
``replace_section_footer_text(section_index, replacement, kind)`` rewrite one
section-specific header/footer as plain paragraphs in a single call. The
replacement text is split on newlines, and the requested reference is created
automatically when needed.

.. code-block:: cpp

    doc.replace_section_header_text(0, "Title line\nSubtitle line");
    doc.replace_section_footer_text(
        0, "First page footer",
        featherdoc::section_reference_kind::first_page);

``append_section(inherit_header_footer)`` appends a new final section at the
end of the document. By default it inherits the previous final section's
header/footer reference layout; passing ``false`` appends the new section
without copying those references.

.. code-block:: cpp

    doc.append_section();
    doc.append_section(false);

``insert_section(section_index, inherit_header_footer)`` inserts a new section
after an existing section. By default the inserted section inherits the
referenced section's current header/footer reference layout; passing ``false``
creates the section break without copying those references.

.. code-block:: cpp

    doc.insert_section(0);
    doc.insert_section(1, false);

``remove_section(section_index)`` removes one section while preserving the
document content around it. Removing a non-final section collapses its break so
that content flows into the following section; removing the final section makes
the previous section become the new final section.

.. code-block:: cpp

    doc.remove_section(1);

``move_section(source_section_index, target_section_index)`` reorders whole
sections. The section content and its header/footer reference layout move
together, and ``target_section_index`` is the final index of the moved section
after reordering.

.. code-block:: cpp

    doc.move_section(2, 0);

.. _featherdoc-doc-lifecycle:

.. rubric:: Creating New Documents And Language Defaults

``create_empty()`` initializes a new in-memory document so callers can produce
fresh ``.docx`` files without opening an existing template archive first.

.. code-block:: cpp

    featherdoc::Document doc("new-file.docx");
    if (const auto error = doc.create_empty()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

    doc.paragraphs().add_run("Hello FeatherDoc");
    if (const auto error = doc.save()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

Use the default run font/language APIs when Chinese/CJK text should carry
explicit ``w:rFonts`` and ``w:lang`` metadata instead of relying on Word
fallback behavior.

.. code-block:: cpp

    featherdoc::Document doc("zh-demo.docx");
    if (const auto error = doc.create_empty()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

    if (!doc.set_default_run_font_family("Segoe UI") ||
        !doc.set_default_run_east_asia_font_family("Microsoft YaHei") ||
        !doc.set_default_run_language("en-US") ||
        !doc.set_default_run_east_asia_language("zh-CN")) {
        std::cerr << "failed to configure default run fonts/languages" << std::endl;
        return 1;
    }

    auto run = doc.paragraphs().add_run("你好，FeatherDoc。这里是一段中文/CJK 文本。");
    if (!run.has_next()) {
        std::cerr << "failed to append Chinese/CJK paragraph" << std::endl;
        return 1;
    }

    if (const auto error = doc.save()) {
        std::cerr << error.message() << std::endl;
        return 1;
    }

Call ``run.set_font_family(...)``, ``run.set_east_asia_font_family(...)``,
``run.set_language(...)``, and ``run.set_east_asia_language(...)`` on the
returned ``Run`` when one paragraph needs a per-run override.
Use ``run.clear_primary_language()``, ``run.clear_east_asia_font_family()``,
``run.clear_east_asia_language()``, and ``run.clear_bidi_language()`` when
only one CJK/RTL override should be removed while preserving the rest of the
run formatting. ``run.clear_language()`` still removes all ``w:lang``
attributes (``w:val``, ``w:eastAsia``, and ``w:bidi``) in one call.
Use ``doc.inspect_paragraph_runs(paragraph_index)`` or
``doc.inspect_paragraph_run(paragraph_index, run_index)`` when you need a
library-level summary of run style/font/language/RTL metadata without going
through the CLI helpers.
Use ``doc.inspect_paragraphs()`` or ``doc.inspect_paragraph(paragraph_index)``
when you need paragraph-level style/bidi/numbering/run-count metadata from the
core library.
Use ``doc.inspect_tables()`` or ``doc.inspect_table(table_index)`` when you
need table-level style/width/grid/text metadata from the core library.
Use ``doc.inspect_table_cells(table_index)`` or
``doc.inspect_table_cell(table_index, row_index, cell_index)`` when you need
cell-level width/span/layout/text metadata from the core library.
Use ``doc.inspect_sections()`` or ``doc.inspect_section(section_index)`` when
you need a section/header/footer summary of both the explicit default/first/even
references stored on each section and the resolved linked-to-previous fallback
chain, including the underlying ``word/headerN.xml`` / ``word/footerN.xml``
entry names, the source section index that currently supplies each slot, and
the document-wide ``w:evenAndOddHeaders`` setting from ``word/settings.xml``
when it can be read.
The same inspection summaries are also available on ``TemplatePart`` handles
returned by ``body_template()``, ``header_template()``, ``footer_template()``,
``section_header_template()``, and ``section_footer_template()``.
For a runnable end-to-end version, build ``featherdoc_sample_chinese`` from
``samples/sample_chinese.cpp`` with ``-DBUILD_SAMPLES=ON``.

Performance Notes
-----------------
``open()`` now keeps XML buffer ownership on the FeatherDoc side before parsing,
which avoids cross-library allocator mismatches in shared-library builds.
``save()`` now streams both the updated ``document.xml`` and the unchanged ZIP
entries instead of buffering whole archive entries in memory first.

Current Limitations
-------------------
- Password-protected or encrypted ``.docx`` files are not supported yet.
- Section-specific header/footer references can now be created and rebound
  through ``ensure_section_header_paragraphs()`` /
  ``ensure_section_footer_paragraphs()`` and
  ``assign_section_header_paragraphs()`` / ``assign_section_footer_paragraphs()``,
  and removed through ``remove_section_header_reference()`` /
  ``remove_section_footer_reference()``. Whole parts can also be dropped
  through ``remove_header_part()`` / ``remove_footer_part()``, and section
  layouts can be copied through ``copy_section_header_references()`` /
  ``copy_section_footer_references()``. New sections can now be appended or
  inserted after an existing section through ``append_section()`` /
  ``insert_section()``, removed through ``remove_section()``, and reordered
  through ``move_section()``, but there is still no high-level API for part
  reordering.
- Word equations (``OMML``) are not surfaced through a typed equation API.
- Tables can now be appended, extended structurally, given explicit cell,
  column, and table widths, merged horizontally and vertically, assigned
  table/cell
  borders, switched between fixed and autofit layout, aligned/indented within
  the page, pointed at existing table style ids, given basic table-level
  default cell margins and cell shading/margins, assigned row heights,
  controlled for page splitting, assigned cell vertical alignment and text
  direction, marked to repeat header rows, retuned through ``tblLook``
  style-routing flags, and given minimal custom table style definitions, but
  there is still no high-level API for advanced table-style property editing or
  floating table positioning.
- Paragraphs can now be attached to managed bullet and decimal lists and can
  restart managed list sequences, and custom numbering definitions can now be
  created through ``ensure_numbering_definition(...)`` /
  ``set_paragraph_numbering(...)``, and paragraph styles can now be linked to a
  numbering definition through ``set_paragraph_style_numbering(...)``, but
  there is still no higher-level numbering-style abstraction.
- Paragraph and run style references can now be attached and cleared, style
  catalogs can be inspected through ``list_styles()`` / ``find_style()``, and
  minimal paragraph/character/table style definitions can be created through
  ``ensure_*_style(...)``, but there is still no inheritance-aware style
  refactoring or style-linked numbering API.
- Bookmark-based template filling now works across body, header, and footer
  parts through ``fill_bookmarks(...)``, the standalone replacement helpers,
  and ``TemplatePart`` handles returned by ``body_template()``,
  ``header_template()``, ``footer_template()``,
  ``section_header_template()``, and ``section_footer_template()``.
  Conditional block visibility is now supported through
  ``set_bookmark_block_visibility(...)`` and
  ``apply_bookmark_block_visibility(...)``, but there is still no high-level
  API for structured template schema validation.
- Images can now be appended as inline body drawings, enumerated through
  ``inline_images()`` or the broader ``drawing_images()``, extracted through
  ``extract_inline_image(...)`` / ``extract_drawing_image(...)``, removed
  through ``remove_inline_image(...)`` / ``remove_drawing_image(...)``, and
  replaced through ``replace_inline_image(...)`` / ``replace_drawing_image(...)``.
  Floating body image creation is now supported through
  ``append_floating_image(...)``, and bookmark-based floating image
  replacement is available through
  ``replace_bookmark_with_floating_image(...)`` across body, header, and
  footer ``TemplatePart`` handles. Basic wrapping control is now exposed
  through ``floating_image_options::wrap_mode`` plus per-edge wrap distances,
  and anchored image cropping is exposed through
  ``floating_image_options::crop``.

Source Layout
-------------
The core implementation is now split into smaller source files instead of
staying in one large translation unit:

- ``src/document.cpp``: ``Document`` open/save flow, ZIP archive handling, and diagnostics
- ``src/document_image.cpp``: body/part image insertion, drawing enumeration, extraction, replacement, media part allocation, and drawing relationship updates
- ``src/document_numbering.cpp``: managed paragraph list numbering, numbering part attachment, and numbering definition generation
- ``src/document_styles.cpp``: paragraph/run style references and ``word/styles.xml`` attachment/persistence
- ``src/document_template.cpp``: bookmark-based template filling and batch replacement APIs
- ``src/paragraph.cpp``: paragraph traversal, run creation, paragraph insertion, and paragraph-property cloning
- ``src/image_helpers.cpp`` / ``src/image_helpers.hpp``: image binary loading plus file format and size detection helpers
- ``src/run.cpp``: run traversal, text/property edits, and run insertion/removal
- ``src/table.cpp``: table creation plus row/cell traversal and editing helpers
- ``src/xml_helpers.cpp`` / ``src/xml_helpers.hpp``: shared internal XML helper utilities
- ``src/constants.cpp``: exported constants and error-category plumbing
- ``cli/featherdoc_cli.cpp``: scriptable inspection and editing utility for
  sections, styles, numbering, page setup, bookmarks, images, and template
  parts

This keeps archive I/O, XML navigation, and public API behavior easier to
extend independently.

Bundled Dependencies
--------------------
- ``pugixml`` ``1.15``
- ``kuba--/zip`` ``0.3.8``
- ``doctest`` ``2.5.1``


Contact
--------------
Do you have an issue using FeatherDoc?
Feel free to open an issue in your project repository.

Changelog
---------
Release-facing change history for the current fork:

``CHANGELOG.md``

Project Identity
----------------
Chinese positioning notes for the current FeatherDoc fork:

:doc:`project_identity_zh`

Release Policy
--------------
Chinese release and versioning notes for the current FeatherDoc fork:

:doc:`release_policy_zh`

Release History Backfill
------------------------
Chinese internal playbook for historical GitHub Release body cleanup,
artifact rebuilding, and sanitized re-upload workflow:

:doc:`release_history_backfill_playbook_zh`

Project Audit
--------------
Chinese audit and modernization notes for the current fork:

:doc:`project_audit_zh`

Upstream Issue Triage
---------------------
Chinese notes mapping relevant DuckX issues onto the current FeatherDoc status:

:doc:`upstream_issue_triage_zh`

License Guide
-------------
Chinese-readable guidance for the current split licensing model:

:doc:`licensing_zh`

Word Visual Workflow
--------------------
Chinese workflow notes for Word visual review automation and repair loops:

:doc:`automation/word_visual_workflow_zh`

Repository-level short references are also available in the project root:
``README.md``, ``README.zh-CN.md``, ``LEGAL.md``, and ``NOTICE``.

Licensing
--------------
This fork should be described as source-available rather than open source for
its fork-specific modifications.

Fork-specific FeatherDoc modifications are distributed under the
non-commercial source-available terms described in ``LICENSE``.

Portions derived from the upstream DuckX codebase still retain the original
MIT text preserved in ``LICENSE.upstream-mit``. Bundled third-party
dependencies keep their own original licenses.
