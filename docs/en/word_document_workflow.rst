Word Document Workflows
=======================

This page is the task-oriented entry point for using FeatherDoc to create,
fill, inspect, and rewrite Microsoft Word ``.docx`` files. Use it when you know
the document workflow you want, then jump to the focused API pages for exact
method signatures.

Workflow Map
------------

.. list-table::
   :header-rows: 1
   :widths: 24 42 34

   * - Task
     - Recommended entry point
     - Details
   * - Open, create, and save a document
     - ``featherdoc::Document``
     - :doc:`api/document`
   * - Fill a Word template
     - ``Document::body_template()`` and ``TemplatePart``
     - :doc:`api/template_part`
   * - Edit body text
     - ``Paragraph`` and ``Run``
     - :doc:`api/paragraph_run`
   * - Edit tables
     - ``Table``, ``TableRow``, and ``TableCell``
     - :doc:`api/table`
   * - Add images
     - ``Document::append_image(...)`` or ``TemplatePart::append_image(...)``
     - :doc:`api/images`
   * - Manage sections, headers, and footers
     - ``section_header_template(...)``, ``section_footer_template(...)``, and page setup APIs
     - :doc:`api/sections`
   * - Manage fields, hyperlinks, comments, and revisions
     - Document or part-level fields and review APIs
     - :doc:`api/fields_links_reviews`
   * - Batch edits from automation
     - ``scripts/edit_document_from_plan.ps1``
     - :doc:`api/edit_plan_operations`

Open And Save
-------------

The smallest safe document workflow is open, mutate, and save to a new path.
Use ``last_error()`` when an operation returns a non-empty ``std::error_code``.

.. code-block:: cpp

   #include <featherdoc.hpp>

   int main() {
       featherdoc::Document doc{"template.docx"};
       if (auto error = doc.open()) {
           return 1;
       }

       const auto replaced =
           doc.replace_content_control_text_by_tag("customer", "Ada Lovelace");
       if (replaced == 0) {
           return 1;
       }

       return doc.save_as("filled.docx") ? 1 : 0;
   }

``open()`` performs strict OPC structure validation by default. The five public
``archive_limits`` fields default to 10,000 entries, 64 MiB per XML part,
256 MiB per binary part, 512 MiB total uncompressed data, and a compression
ratio of 200. Fixed internal metadata limits additionally cap the central
directory at 64 MiB, one physical entry name at 511 bytes, and all entry names
at 8 MiB; they are not caller-configurable ``archive_limits`` fields. Embedded
NUL bytes in entry names are rejected. Use ``document_open_options`` with
``tolerant`` only to repair known legacy damage. Reacquire paragraph, run,
table, and template-part handles after ``set_path()``, another ``open()``, or
``create_empty()``.
Lazy reads, Custom XML synchronization, image extraction, and saves revalidate
the complete current source archive, so replacing the file after ``open()``
cannot bypass package limits or UTF-8/canonical-name checks. They also compare
an ordered metadata fingerprint (names, logical identities, CRC32, sizes, and
directory flags) with the ``open()`` snapshot and return
``source_archive_changed`` instead of mixing package generations. A same-source
save performs one final comparison before replacement and validates the output
under the original open limits. This is cooperative consistency detection, not
a cryptographic hash or a cross-process lock.
Strict mode requires canonical ASCII ZIP entry names with uppercase ``%HH``
escapes. Tolerant mode may read legacy raw-Unicode physical names, but it still
rejects ambiguous logical identities; ``save()``/``save_as()`` rewrites copied
entries under their canonical percent-encoded names.
After any tolerant legacy spelling is canonicalized, both modes reject logical
names outside the OPC ``pchar`` repertoire, empty or dot-only segments,
segments ending in a dot, and part-name pairs where one name is formed by
appending path segments to the other. ``[Content_Types].xml``
is also checked for case-equivalent duplicate ``Override/@PartName`` and
``Default/@Extension`` declarations. Unicode extensions use UTF-8 ``%HH``
bytes because raw non-ASCII text is not valid ``ST_Extension`` markup.

Core WordprocessingML parts accept the transitional namespace through a default,
alternate, or valid UTF-8 prefix, including Chinese prefixes. FeatherDoc
canonicalizes their in-memory element and attribute names to ``w:``. Normal
saves always reserialize the main document and active header/footer parts;
clean lazy singleton parts can still be copied byte-for-byte, while dirty ones
save canonically. Wrong explicit ``xmlns:w`` bindings fail closed. Main-root
mismatches are rejected in strict mode or diagnosed in tolerant mode;
header/footer roots fail open in both modes, and lazy singleton roots fail on
first access. ``commentsExtended`` remains outside this WML canonicalizer.
Every ``Default`` and ``Override`` must provide a syntactically valid
``ContentType`` media type; malformed parameters, controls, invalid UTF-8, and
unsupported non-ASCII token text are rejected in both validation modes.
Internal OPC relationship targets are resolved as UTF-8 package paths relative
to their source part, independent of the host file-system code page. This also
applies to nested header/footer parts and their image relationships.
Moving a ``Document`` also invalidates handles from the source; move assignment
invalidates handles from the destination's previous state as well. The moved-from
object is closed and can be initialized again. These operations remain
``noexcept`` and preserve the 1.13 ``Document`` object layout; added internal
bookkeeping lives in an opaque sidecar.

A successful tolerant open does not mean the package is valid. Inspect the
diagnostics, repair explicitly, and save to a new path. Reacquire XML-backed
handles after a repair that changes the package.

Do not run the normal editing pipeline merely because tolerant open succeeded;
use that mode for inspection and explicit repair only. A missing relationship
part can be created by the API that owns it, but an existing document, header,
or footer ``.rels`` part with an invalid root, relationship child namespace,
required attribute, identifier uniqueness, or ``TargetMode`` is protected from
implicit replacement. Hyperlink, image, singleton-part, and section
relationship mutations fail with ``invalid_package_structure``. Content-control
image replacement fails before changing the control, adding drawing XML or a
media part, or updating Content Types. Saving afterward preserves the original
invalid relationship structure instead of silently replacing it. Repair the
reported package issue explicitly before continuing edits.

Valid relationship elements are matched by expanded QName, so both default
namespace and prefixed ``Relationships``/``Relationship`` forms are supported.
Existing prefixes are preserved and new relationship children use the root
prefix. The OPC relationship attributes themselves remain unqualified.

.. code-block:: cpp

   featherdoc::Document damaged{"legacy.docx"};
   featherdoc::document_open_options options;
   options.validation = featherdoc::package_validation_mode::tolerant;
   if (damaged.open(options)) {
       return 1;
   }

   for (const auto &diagnostic : damaged.package_diagnostics()) {
       if (!diagnostic.repairable) {
           return 1;
       }
   }
   if (!damaged.repair_package()) {
       return 1;
   }
   return damaged.save_as("legacy-repaired.docx") ? 1 : 0;

The equivalent CLI flow is ``featherdoc_cli inspect-package legacy.docx
--json`` followed by ``featherdoc_cli repair-package legacy.docx --output
legacy-repaired.docx --json``. The repair command requires an output path and
never overwrites its input.

Template Filling
----------------

For stable business templates, prefer Word bookmarks or content controls over
search-and-replace. Tags and aliases survive most visible edits better than
paragraph indexes.

.. code-block:: cpp

   auto body = doc.body_template();
   if (!body) {
       return 1;
   }

   body.replace_bookmark_text("invoice_no", "INV-2026-001");
   body.replace_content_control_text_by_tag("customer_name", "Ada Lovelace");
   body.replace_content_control_with_table_by_tag("line_items", {
       {"Item", "Qty", "Amount"},
       {"Support", "1", "100.00"}
   });

Body Editing
------------

Use ``Paragraph`` and ``Run`` when the target is text content or run-level
formatting. Use ``Table`` APIs when the target is table structure, cell text,
widths, borders, merged cells, or repeated headers.

.. code-block:: cpp

   auto summary = doc.body_template().append_paragraph("Status:");
   auto value = summary.add_run(" approved", featherdoc::formatting_flag::bold);
   value.set_font_family("Aptos");
   summary.set_alignment(featherdoc::paragraph_alignment::center);

   auto table = doc.append_table(2, 2);
   table.set_row_texts(0, {"Name", "Status"});
   table.set_row_texts(1, {"Ada", "Approved"});

Headers, Footers, And Sections
------------------------------

When a template has section-specific headers or footers, resolve the target
section first and then use the same ``TemplatePart`` operations as the body.

.. code-block:: cpp

   auto footer = doc.section_footer_template(
       0, featherdoc::section_reference_kind::default_reference);
   if (footer) {
       footer.append_page_number_field();
   }

Batch Edits
-----------

``scripts/edit_document_from_plan.ps1`` is the safer automation entry point for
repeatable document rewrites. Keep the edit plan in UTF-8 JSON and write a
summary file so CI can inspect the operation results.

The CLI requires valid UTF-8 command-line arguments on POSIX. On Windows it
reads the native UTF-16 command line directly, so Chinese, Japanese, emoji,
spaces, and long paths do not pass through the active console code page. JSON,
text input, stdout, and stderr remain UTF-8 on every platform.

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_content_control_text_by_tag",
         "tag": "customer_name",
         "text": "Ada Lovelace"
       },
       {
         "op": "append_page_number_field",
         "part": "section-footer",
         "section": 0
       }
     ]
   }

.. code-block:: powershell

   powershell -ExecutionPolicy Bypass -File .\scripts\edit_document_from_plan.ps1 `
     -InputDocx .\template.docx `
     -EditPlan .\plan.json `
     -OutputDocx .\output\filled.docx `
     -SummaryJson .\output\filled.summary.json `
     -SkipBuild

Validation
----------

Use a layered validation path:

* Run focused C++ or PowerShell tests for the edited API surface.
* Inspect generated summary JSON for automation workflows.
* Run ``scripts/run_word_visual_smoke.ps1`` when the visible Word layout matters.
* Keep source, JSON plans, and generated text as UTF-8. On Windows, read Chinese
  files with ``Get-Content -Encoding UTF8`` and set console output encoding when
  diagnosing text output.

Related Pages
-------------

* :doc:`getting_started`
* :doc:`api/index`
* :doc:`api/document`
* :doc:`api/template_part`
* :doc:`api/edit_plan_operations`
