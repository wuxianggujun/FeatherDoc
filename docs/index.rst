
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

``Table::set_width_twips(...)``, ``set_style_id(...)``, ``set_border(...)``,
``set_layout_mode(...)``, ``set_alignment(...)``, ``set_indent_twips(...)``,
``set_cell_spacing_twips(...)``, ``set_cell_margin_twips(...)``, and
``set_style_look(...)`` work alongside
``TableCell::set_text(...)`` and ``get_text()``, plus ``set_width_twips(...)``,
``Table::remove()``, ``insert_table_before()``, ``insert_table_after()``,
``insert_table_like_before()``, ``insert_table_like_after()``,
``TableRow::remove()``, ``insert_row_before()``, ``insert_row_after()``,
``merge_right(...)``, ``merge_down(...)``,
``set_vertical_alignment(...)``,
``set_text_direction(...)``,
``set_border(...)``, ``set_fill_color(...)``, and ``set_margin_twips(...)``
for higher-level table layout edits without dropping down to raw XML.
``width_twips()`` reports an
explicit ``dxa`` width when present, ``style_id()`` reports the current table
style reference, ``style_look()`` reports the current first/last row or column
emphasis together with row/column banding flags, ``layout_mode()`` reports the
current auto-fit mode, ``alignment()`` / ``indent_twips()`` report table placement,
``cell_spacing_twips()`` reports inter-cell spacing, and
``cell_margin_twips()`` reports per-edge default cell margins,
``height_twips()`` / ``height_rule()`` report the current row height override,
``cant_split()`` reports whether Word keeps the row on one page,
``repeats_header()`` reports whether a row repeats as a table header,
``Table::remove()`` deletes one table while refusing to leave the parent
container without the required block content and retargets the wrapper to the
next surviving table when possible (otherwise the previous one).
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
``column_span()`` reports the current horizontal span. ``text_direction()``
reports the current table-cell writing direction when a cell uses vertical or
rotated text flow. ``TableCell::set_text(...)`` replaces one cell's body with
a single paragraph while preserving cell-level properties such as shading,
margins, borders, and explicit width.

.. code-block:: cpp

    auto table = doc.append_table(1, 3);
    table.set_width_twips(7200);
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

For a runnable table-removal example, build
``featherdoc_sample_remove_table`` from
``samples/sample_remove_table.cpp``. It creates three body tables, reopens the
saved ``.docx``, removes the temporary middle table, and continues editing the
following table through the same wrapper.
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
For a runnable table-style-look edit example, build
``featherdoc_sample_edit_existing_table_style_look`` from
``samples/sample_edit_existing_table_style_look.cpp``. It reopens a saved
``.docx``, updates ``tblLook`` on an existing table, and keeps the original
table style reference in place.

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
reference frames, pixel offsets, whether the image sits behind text, and
whether overlap is allowed. The same API works on ``Document`` and
``TemplatePart``. The generated floating drawing currently uses ``wrapNone``,
so Word does not reflow surrounding text for you.

.. code-block:: cpp

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    options.horizontal_offset_px = 460;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 24;

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
or anchored.

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
``append_floating_image(...)``.

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
- Tables can now be appended, extended structurally, given explicit cell and
  table widths, merged horizontally and vertically, assigned table/cell
  borders, switched between fixed and autofit layout, aligned/indented within
  the page, pointed at existing table style ids, given basic table-level
  default cell margins and cell shading/margins, assigned row heights,
  controlled for page splitting, assigned cell vertical alignment and text
  direction, marked to repeat header rows, and retuned through ``tblLook``
  style-routing flags, but there is still no high-level API for custom table
  style definitions or floating table positioning.
- Paragraphs can now be attached to managed bullet and decimal lists and can
  restart managed list sequences, but there is still no high-level API for
  custom numbering definitions or paragraph style-based numbering.
- Paragraph and run style references can now be attached and cleared, and a
  minimal ``word/styles.xml`` is created automatically when needed, but there
  is still no high-level API for custom style definition editing, style
  catalog inspection, or inheritance-aware style management.
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
  footer ``TemplatePart`` handles. Advanced wrapping and cropping control are
  still not exposed as high-level APIs.

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

Word Visual Workflow
--------------------
Chinese workflow notes for Word visual review automation and repair loops:

:doc:`automation/word_visual_workflow_zh`

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
