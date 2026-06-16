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
