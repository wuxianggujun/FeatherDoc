
FeatherDoc
==========

Release v\ |version|

*FeatherDoc* is a C++ library for creating and editing Microsoft Word
(.docx) files.


How to install
--------------
The instructions to build and install FeatherDoc

.. code-block:: sh

    cmake -S . -B build
    cmake --build build
    cmake --install build --prefix install

The installed package also carries project-facing metadata and legal files
under ``share/FeatherDoc``.
Top-level builds enable ``BUILD_CLI`` by default, so ``featherdoc_cli`` is
built unless ``-DBUILD_CLI=OFF`` is passed explicitly.

MSVC Build
----------
Build FeatherDoc with the MSVC toolchain from a Visual Studio Developer
Command Prompt. Use an ``x64`` prompt, or initialize the environment with
``VsDevCmd.bat -arch=x64 -host_arch=x64`` first.

.. code-block:: bat

    cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON
    cmake --build build-msvc-nmake
    ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60

CLI
---
``featherdoc_cli`` exposes a minimal command-line layer for the current
section-aware header/footer operations.

.. code-block:: sh

    featherdoc_cli inspect-sections input.docx
    featherdoc_cli inspect-sections input.docx --json
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

``inspect-sections`` reports section counts together with per-section
``default`` / ``first`` / ``even`` header and footer attachment flags. The
mutating commands overwrite the input file unless ``--output <path>`` is
provided. Pass ``--json`` to ``inspect-sections`` when you need the same layout
data in a machine-readable object. The mutating commands also accept
``--json`` and emit ``command``, ``ok``, ``in_place``, ``sections``,
``headers``, and ``footers`` together with command-specific fields such as
``section``, ``source``, ``target``, ``part``, and ``kind``.
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

``append_table(row_count, column_count)`` creates a new body table
programmatically. The returned ``Table`` can then grow through
``append_row()``, and each ``TableRow`` can be widened through
``append_cell()``.

.. code-block:: cpp

    auto table = doc.append_table(2, 2);

    auto first_row = table.rows();
    auto first_cell = first_row.cells();
    first_cell.paragraphs().add_run("r0c0");
    first_cell.next();
    first_cell.paragraphs().add_run("r0c1");

    auto extra_row = table.append_row();
    auto extra_cell = extra_row.cells();
    extra_cell.paragraphs().add_run("tail");
    extra_row.append_cell().paragraphs().add_run("tail-2");

``append_image(path)`` appends an inline body image at the source image's
intrinsic pixel size. Use ``append_image(path, width_px, height_px)`` when you
want explicit scaling. The current image support is limited to ``.png``,
``.jpg``, ``.jpeg``, ``.gif``, and ``.bmp``.

.. code-block:: cpp

    doc.append_image("logo.png");
    doc.append_image("badge.png", 96, 48);

``set_paragraph_list(paragraph, kind, level)`` attaches managed bullet or
decimal numbering to a paragraph. Use ``clear_paragraph_list(paragraph)`` when
you want to remove the list marker again.

.. code-block:: cpp

    auto item = doc.paragraphs();
    doc.set_paragraph_list(item, featherdoc::list_kind::bullet);
    item.add_run("first item");

    auto nested = item.insert_paragraph_after("");
    doc.set_paragraph_list(nested, featherdoc::list_kind::decimal, 1);
    nested.add_run("nested item");

``set_paragraph_style(paragraph, style_id)`` and ``set_run_style(run,
style_id)`` attach paragraph/run style references. When the source document
does not already contain ``word/styles.xml``, FeatherDoc creates a minimal
styles part automatically. The generated catalog currently includes
``Normal``, ``Heading1``, ``Heading2``, ``Quote``, ``Emphasis``, and
``Strong``.

.. code-block:: cpp

    auto paragraph = doc.paragraphs();
    doc.set_paragraph_style(paragraph, "Heading1");

    auto styled_run = paragraph.add_run("Styled heading");
    doc.set_run_style(styled_run, "Strong");

    doc.clear_run_style(styled_run);
    doc.clear_paragraph_style(paragraph);

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
- Tables can now be appended and extended structurally, but there is still no
  high-level API for cell merges, widths, borders, or table styling.
- Paragraphs can now be attached to managed bullet and decimal lists, but
  there is still no high-level API for custom numbering definitions, list
  restarts, or paragraph style-based numbering.
- Paragraph and run style references can now be attached and cleared, and a
  minimal ``word/styles.xml`` is created automatically when needed, but there
  is still no high-level API for custom style definition editing, style
  catalog inspection, or inheritance-aware style management.
- A first bookmark-based batch template API now exists through
  ``fill_bookmarks(...)``, and standalone bookmark paragraphs can now be
  replaced by generated paragraphs, tables, or inline images through
  ``replace_bookmark_with_paragraphs(...)``,
  ``replace_bookmark_with_table(...)``, and
  ``replace_bookmark_with_image(...)``, but there is still no high-level API
  for repeated table row expansion, conditional sections, header/footer
  template filling, or structured template schema validation.
- Images can now be appended as inline body drawings, but there is still no
  high-level API for reading existing images, floating placement, wrapping,
  cropping, or header/footer image insertion.

Source Layout
-------------
The core implementation is now split into smaller source files instead of
staying in one large translation unit:

- ``src/document.cpp``: ``Document`` open/save flow, ZIP archive handling, and diagnostics
- ``src/document_image.cpp``: inline body image insertion, media part allocation, and drawing relationship updates
- ``src/document_numbering.cpp``: managed paragraph list numbering, numbering part attachment, and numbering definition generation
- ``src/document_styles.cpp``: paragraph/run style references and ``word/styles.xml`` attachment/persistence
- ``src/document_template.cpp``: bookmark-based template filling and batch replacement APIs
- ``src/paragraph.cpp``: paragraph traversal, run creation, and paragraph insertion
- ``src/image_helpers.cpp`` / ``src/image_helpers.hpp``: image binary loading plus file format and size detection helpers
- ``src/run.cpp``: run traversal and text read/write behavior
- ``src/table.cpp``: table creation plus row/cell traversal and editing helpers
- ``src/xml_helpers.cpp`` / ``src/xml_helpers.hpp``: shared internal XML helper utilities
- ``src/constants.cpp``: exported constants and error-category plumbing
- ``cli/featherdoc_cli.cpp``: minimal section-layout inspection and editing utility

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

Repository-level short references are also available in the project root:
``LEGAL.md`` and ``NOTICE``.

Licensing
--------------
This fork should be described as source-available rather than open source for
its fork-specific modifications.

Fork-specific FeatherDoc modifications are distributed under the
non-commercial source-available terms described in ``LICENSE``.

Portions derived from the upstream DuckX codebase still retain the original
MIT text preserved in ``LICENSE.upstream-mit``. Bundled third-party
dependencies keep their own original licenses.
