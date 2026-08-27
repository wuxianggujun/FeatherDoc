Document
========

``featherdoc::Document`` is the root handle for a ``.docx`` package. Use it to
open and save files, access body/header/footer template parts, inspect sections,
and call document-wide editing APIs.

Common Tasks
------------

.. FDOC_EN_DOCUMENT_COMMON_TASKS

Start here when you already know the workflow you need:

* Open and save a document: use ``Document(path)``, ``open()``, ``save()``, and
  ``save_as(path)``.
* Fill a body template: use ``body_template()`` or the document-level
  ``replace_content_control_text_by_tag(...)`` shortcut.
* Work on headers or footers: use ``header_template(...)``,
  ``footer_template(...)``, ``section_header_template(...)``, or
  ``section_footer_template(...)``.
* Edit body content directly: use ``paragraphs()``, ``tables()``,
  ``append_table(...)``, and the image append methods.
* Inspect structure before changing it: use ``inspect_sections()``,
  ``inspect_body_blocks()``, ``list_bookmarks()``, ``list_content_controls()``,
  and ``last_error()``.

Success And Failure Semantics
-----------------------------

.. FDOC_EN_DOCUMENT_SUCCESS_FAILURE_SEMANTICS

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - Return shape
     - Success
     - Failure or no-op
   * - ``std::error_code``
     - Empty error code means the package operation completed.
     - Non-empty error code means the operation failed; inspect
       ``last_error()`` for detail and package entry context.
   * - ``bool``
     - ``true`` means the requested document XML or package metadata changed.
     - ``false`` means the target was unavailable, invalid, or already had no
       applicable change.
   * - ``std::size_t``
     - Non-zero value is the number of matched items changed or appended.
     - ``0`` means no matching bookmark, content control, comment target,
       hyperlink, or note target was found.
   * - ``std::optional<T>``
     - Contains the requested inspection or settings value.
     - Empty means the section, setting, or semantic comparison could not be
       resolved.
   * - Handle objects
     - Returned handles such as ``TemplatePart``, ``Paragraph``, and ``Table``
       are the next editing entry point.
     - Check handle-specific validity rules before assuming the target exists.

For mutating ``bool`` APIs, ``false`` is a combined “no mutation completed”
signal rather than a structured error code. It can mean an invalid target, an
inapplicable argument, or that the requested state was already present. Use
``last_error()`` to distinguish a failure only when that method explicitly
documents that it sets the error; do not attribute a stale error from an older
operation to the current ``false`` result. Callers that need an unambiguous
outcome should validate ``is_open()``, handle ``valid()``, and the current
target state, then inspect the state after the mutation. The ``1.13.x`` line
keeps these signatures source-compatible; a future breaking API may introduce
a uniform structured mutation result.

Short C++ Example
-----------------

.. FDOC_EN_DOCUMENT_SHORT_EXAMPLE

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) return 1;

   doc.replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

Typed Signature Guide
---------------------

.. FDOC_EN_DOCUMENT_TYPED_SIGNATURE_GUIDE

``Document`` is a package-level handle. Call ``open()`` for an existing file or
``create_empty()`` for a new package before using editing methods. Most indexes
are zero-based. Path parameters are filesystem paths; text parameters are
written into the resolved WordprocessingML target.

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``explicit Document(std::filesystem::path path)``
     - ``path``: source or target ``.docx`` path.
     - Creates a handle; no package is loaded until ``open()``.
   * - ``Document(Document &&other) noexcept`` / ``operator=(Document &&other) noexcept``
     - ``other``: source document object.
     - Transfer package state, invalidate handles from both objects, and leave
       ``other`` closed but reusable.
   * - ``std::error_code create_empty()``
     - None.
     - Empty error code means a new package was initialized.
   * - ``std::error_code open()``
     - None.
     - Empty error code means the current path was loaded.
   * - ``std::error_code save_as(std::filesystem::path path) const``
     - ``path``: non-empty output ``.docx`` path.
     - Empty error code means the package was written to the new path.
   * - ``TemplatePart body_template()``
     - None.
     - Body template-part handle; check handle validity before editing.
   * - ``TemplatePart header_template(std::size_t index = 0U)``
     - ``index``: physical header part index.
     - Header template-part handle for that physical part.
   * - ``TemplatePart section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: default, first,
       or even header reference.
     - Template-part handle for the resolved section header.
   * - ``bookmark_fill_result fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``: bookmark-name/text pairs.
     - Matched, replaced, requested, and missing-bookmark counts.
   * - ``std::size_t replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``: content-control tag. ``replacement``: inserted text.
     - Number of matching controls replaced; ``0`` means no match.
   * - ``Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` and ``column_count``: initial body table shape.
     - New body table handle.
   * - ``bool set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``: target section. ``setup``: page size, margins, and
       orientation data.
     - ``true`` when section page setup was written.
   * - ``std::optional<section_page_setup> get_section_page_setup(std::size_t section_index) const``
     - ``section_index``: target section.
     - Empty when the section or setup cannot be resolved.

Lifecycle
---------

Open, create, save, and inspect the current package state.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``Document()``
     - None.
     - ``Document``
     - Create an empty handle; call ``set_path()`` before ``open()``.
   * - ``explicit Document(std::filesystem::path path)``
     - ``path``: source or target ``.docx`` path.
     - ``Document``
     - Create a handle bound to a file path.
   * - ``Document(Document &&other) noexcept`` / ``operator=(Document &&other) noexcept``
     - ``other``: source document object.
     - ``Document`` / ``Document &``
     - Transfer package state; the source becomes closed and may be reused.
   * - ``create_empty()``
     - None.
     - ``std::error_code``
     - Initialize a new empty document package.
   * - ``set_path(std::filesystem::path path)``
     - ``path``: replacement path used by ``open()`` and ``save()``.
     - ``void``
     - Replace the current path and reset loaded package state.
   * - ``path() const``
     - None.
     - ``const std::filesystem::path &``
     - Return the path currently bound to the handle.
   * - ``open()``
     - None.
     - ``std::error_code``
     - Load the current ``.docx`` package with strict OPC validation.
   * - ``open(const document_open_options &options)``
     - ``options``: validation mode and ZIP resource limits.
     - ``std::error_code``
     - Load with the requested policy; use ``tolerant`` only for repair input.
   * - ``package_diagnostics() const noexcept``
     - None.
     - ``const std::vector<package_diagnostic> &``
     - Return package issues, severity, entry names, and repairability recorded
       during tolerant open.
   * - ``repair_package(const document_repair_options &options = {})``
     - ``options``: deterministic issue categories that may be repaired.
     - ``std::optional<package_repair_report>``
     - Repair package state transactionally; no partial mutation occurs when an
       unsafe or disabled issue is present.
   * - ``save() const``
     - None.
     - ``std::error_code``
     - Save changes back to the current path.
   * - ``save_as(std::filesystem::path path) const``
     - ``path``: output ``.docx`` path; it must not be empty.
     - ``std::error_code``
     - Save changes to a new path.
   * - ``is_open() const``
     - None.
     - ``bool``
     - Return whether ``open()`` or ``create_empty()`` has loaded a package.
   * - ``last_error() const noexcept``
     - None.
     - ``const document_error_info &``
     - Inspect the latest failure ``code``, ``detail``, ``entry_name``, and
       optional XML offset.
   * - ``enable_update_fields_on_open()``
     - None.
     - ``bool``
     - Add ``w:updateFields`` so Word updates fields when it opens the file.
   * - ``clear_update_fields_on_open()``
     - None.
     - ``bool``
     - Remove the update-fields-on-open setting.
   * - ``update_fields_on_open_enabled()``
     - None.
     - ``std::optional<bool>``
     - Inspect the update-fields-on-open setting; empty means the setting could
       not be read.

Path Binding And UTF-8
----------------------

Filesystem paths enter through ``std::filesystem::path``. Windows uses wide
system calls internally, while paths passed to the ZIP layer, error details,
and CLI JSON are converted to UTF-8. Chinese, Japanese, emoji, spaces, and
long-path-enabled names therefore never pass through the system ANSI code
page. Source files, documentation, CLI stdout/stderr, and JSON are UTF-8, and
MSVC builds keep ``/utf-8`` enabled.

On a successful ``open()``, FeatherDoc resolves the source against the current
working directory and freezes that absolute I/O path. Later lazy loads of
images, styles, numbering, headers, footers, and other parts, as well as source
archive reopens during ``save()``, use the frozen path even if the process
changes its current working directory. For source compatibility, ``path()``
still returns the spelling supplied by the caller. Each
``save_as(relative_path)`` instead resolves its output against the current
working directory at call time; it does not change ``path()`` or the frozen
source-archive path.

Save Transaction And Durability
-------------------------------

``save()`` and ``save_as()`` exclusively create a unique sibling temporary
file; fixed ``.tmp`` or ``.bak`` names are never used. After ZIP finalization,
the temporary file is flushed and synchronized before an atomic same-filesystem
replacement. POSIX then synchronizes the parent directory; Windows uses
write-through replacement APIs. On POSIX, the temporary remains mode ``0600``
while data is written. Replacing an existing target preserves its mode bits,
while a new target receives permissions derived from the process ``umask``.

The failure boundary is explicit:

* ``output_archive_open_failed``, entry write errors,
  ``output_archive_finalize_failed``, ``output_file_sync_failed``,
  ``package_repair_validation_failed``, and ``output_replace_failed`` occur
  before replacement completes, so the original target remains unchanged.
* ``output_directory_sync_failed_after_replace`` is returned only after atomic
  replacement succeeded but POSIX parent-directory durability could not be
  confirmed. The new target is already visible and ``last_error().detail``
  states that fact. Reopen and verify the target instead of assuming that the
  old file remains.
* A successful return means ZIP finalization, file synchronization, and the
  platform replacement completed; on platforms that support directory
  synchronization, it also confirms the directory entry was synchronized.

Save-time pruning and package repair never use pugixml's unchecked
``reset(source)`` copy operation. FeatherDoc builds a checked clone in an
isolated DOM, verifies every node, name, value, and attribute allocation, and
publishes it only after the clone is complete. Clone or repair-mutation
allocation failure returns ``std::errc::not_enough_memory`` before a partial DOM
can replace either the in-memory package state or the target file.

All archive readers, entry buffers, the temporary output stream, and the ZIP
writer are owned by RAII guards. A standard allocation failure before atomic
replacement therefore closes every archive resource, removes the unique
temporary file, leaves an existing target byte-for-byte unchanged, and permits
the same ``Document`` instance to retry. The post-replacement durability path
does not construct new path or diagnostic strings, so a completed replacement
cannot later be reported as an allocation failure.

Open Validation And Resource Limits
-----------------------------------

The parameterless ``open()`` uses the default ``document_open_options``. The
default is ``package_validation_mode::strict`` and requires valid
``[Content_Types].xml``, root relationships, ``word/document.xml``, and a
``w:document/w:body`` structure. Callers repairing known legacy damage must
explicitly select ``package_validation_mode::tolerant``. Tolerant validation
does not disable archive resource limits and never silently repairs the input.
XML parser allocation failure is an operational failure, not legacy-package
damage: every eager and lazy package XML parse fails closed with
``std::errc::not_enough_memory`` in both modes. It is never converted into a
tolerant diagnostic or skipped Custom XML item.
Both modes reject missing or malformed ``Default/@ContentType`` and
``Override/@ContentType`` media types because their interpretation is not safe
to guess.
Inspect ``package_diagnostics()``, call ``repair_package()`` explicitly, and
write the result to a new file with ``save_as()``.

Tolerant mode is an inspection and explicit-repair boundary, not a general
editing compatibility mode. A genuinely missing relationship part may be
created by an API that owns that relationship, but an existing
``word/_rels/document.xml.rels`` or header/footer ``.rels`` part whose root name
or relationship child structure is invalid is never reinitialized implicitly.
The validator rejects namespace resets, unknown or nested elements, missing
``Id``/``Type``/``Target``, duplicate identifiers, and invalid ``TargetMode``.
Element names are matched by expanded QName: both the default-namespace form
and a valid namespace prefix are accepted for ``Relationships`` and
``Relationship``. Existing child prefixes are preserved, and newly appended
relationships use the root element's prefix. Relationship attributes remain
unqualified; a prefixed ``Id``, ``Type``, ``Target``, or ``TargetMode`` does not
substitute for the required OPC attribute.

Every loaded ``.rels`` part is Markup Compatibility and Extensibility (MCE)
preprocessed before its OPC schema is inspected. This preprocessing configuration
understands only the OPC Relationships namespace and has no additional markup
configuration namespaces. ``mc:AlternateContent`` tests ``Choice/@Requires`` in
document order until the first supported Choice is found. Namespace
well-formedness and MCE syntax conformance are checked across the complete XML
tree, including every Choice/Fallback wrapper, every directive and ``Requires``
list, and content that will later be ignored or left unselected. Only the
selected Choice or Fallback enters content-semantic preprocessing; ignored and
unselected content does not execute its ``ProcessContent`` or enforce its
``MustUnderstand`` semantics. A foreign element's own ``Ignorable`` and
``ProcessContent`` declarations participate in deciding whether that element
is discarded or unwrapped during the semantic pass. A foreign wrapper selected
by ``ProcessContent`` is not completely ignored: its own ``MustUnderstand`` is
enforced before the wrapper is removed. Unknown
``mc:MustUnderstand`` requirements, invalid MCE
syntax, obsolete ``Preserve*`` directives, and non-ignorable extension markup
fail closed in both strict and
tolerant modes with ``mce_mismatch`` or ``invalid_mce_markup``.

An unchanged source relationship entry can still be copied byte-for-byte. Once
that relationship part becomes dirty, FeatherDoc writes the sanitized selected
view: ignored content, unselected branches, and MCE control attributes are
removed. XML declarations, comments, processing instructions, and the OPC
``Relationship`` simple text/CDATA content are retained, including valid UTF-8
Chinese and other Unicode text.

Any relationship-dependent mutation fails closed with
``document_errc::invalid_package_structure``. This includes hyperlink and image
relationships, singleton-part attachment, section header/footer relationship
edits, and content-control image replacement. The failure occurs before the
library commits related content XML, media entries, Content Types changes, or
dirty state. A subsequent save therefore does not replace the original invalid
``.rels`` relationship tree; dirty malformed relationship DOM is also rejected
by the save boundary, and content-control image operations cannot leave a
half-completed replacement behind. Inspect the diagnostic and use an explicit,
supported repair before resuming normal edits.

``[Content_Types].xml`` follows the same expanded-QName rule. A default
namespace or any prefix bound to the OPC Content Types namespace is accepted
for ``Types``, ``Default``, and ``Override``. Mutations preserve the existing
root prefix. Namespace shadowing or resets, unexpected or nested elements,
any text or CDATA (including whitespace), and undeclared attributes are
structural errors. Tolerant open records ``invalid_content_types_root`` and
permits inspection or an
unchanged round trip, but every Content Types-dependent mutation fails closed
with ``invalid_package_structure`` instead of extending the ambiguous tree.

Attaching the settings, numbering, or styles singleton part prepares checked
copies of this DOM and ``word/_rels/document.xml.rels`` as one transaction.
Neither copy, its dirty flag, nor the new part is published until both the
relationship and matching ``Override`` are complete. An allocation failure
while preparing this attachment leaves the package unchanged and the same
``Document`` may retry safely. A part first attached to an existing source
archive is marked dirty immediately, so a later content-mutation failure cannot
save the relationship and ``Override`` without also writing the new XML part.

The main document, headers, footers, styles, numbering, settings, footnotes,
endnotes, and comments are likewise resolved by expanded QName. The
transitional WordprocessingML namespace may use the default namespace or any
valid UTF-8 NCName prefix, including a Chinese prefix. On load, FeatherDoc
atomically canonicalizes WordprocessingML element and attribute names to its
internal ``w:`` spelling while preserving namespace declarations, foreign
markup, and prefix-sensitive attribute values such as MCE and data-binding
QNames. Normal saves always reserialize the main document and active
header/footer parts with canonical ``w:`` names. Clean, lazily loaded singleton
parts (styles, numbering, settings, footnotes, endnotes, and comments) remain
eligible for byte-for-byte source copying; once one becomes dirty, it is
serialized canonically. An explicit
``xmlns:w`` binding to any other namespace, an unbound/malformed QName, or a
duplicate expanded attribute is rejected rather than treated as
WordprocessingML. For the main document, a wrong root is rejected in strict
mode and retained only as a structural diagnostic in tolerant mode. Eagerly
loaded header/footer roots fail ``open()`` in both modes. Lazy singleton roots
are checked on first access and fail with ``invalid_package_structure`` in both
modes; opening the package itself may already have succeeded. ``commentsExtended``
uses another schema and never enters this canonicalizer. The iterative pass is
bounded to 65,536 levels, 1,000,000 elements, and 1,000,000 attributes.

The public ``archive_limits`` aggregate retains its original five configurable
members: 10,000 entries, 64 MiB per XML part, 256 MiB per binary part, 512 MiB
total uncompressed data, and a compression ratio of 200 by default. Raise these
limits only when the application has a concrete need and a trusted input
boundary. Separate fixed internal metadata limits cap the central directory at
64 MiB, one physical entry name at 511 bytes, and all physical entry names at
8 MiB. These metadata limits are applied before allocating or reading the
complete central directory and are intentionally not additional
``archive_limits`` fields.

Relationships MCE sanitization also caps the temporary output DOM at 1,000,000
elements and 262,144 attributes, with at most 4,096 effective namespace bindings
hoisted onto any one element. Namespace token resolution, copying, comparisons,
context push/pop, sorting, and binding hoists share a cumulative 64 MiB work
budget. These fixed internal bounds stop
``ProcessContent`` wrapper removal from multiplying inherited namespace
declarations into an unbounded result. A limit failure leaves the original DOM
unchanged and is reported as ``archive_limit_exceeded``. Allocation failure is
reported as ``std::errc::not_enough_memory`` and is never downgraded to a
tolerant-mode package diagnostic.

Physical entry names must be valid UTF-8 relative package paths and must not
contain an embedded NUL. Strict mode additionally requires the OPC physical
name to be canonical ASCII: UTF-8 bytes outside ASCII and spaces are represented
with uppercase ``%HH`` escapes. Tolerant mode can read a legacy raw-Unicode or
otherwise non-canonical spelling, but both modes reject traversal, malformed
escapes, duplicates, ASCII case-equivalent names, and raw/percent-equivalent
logical parts. Saving writes the canonical percent-encoded physical name. This
keeps Chinese, Japanese, emoji, and other Unicode logical part names portable
without depending on a ZIP implementation's raw-name behavior. Internal OPC
relationship targets are resolved as UTF-8 package paths relative to their
source part, without conversion through the host file-system code page.

Package PartNames use the RFC 3986 ``pchar`` repertoire and additionally obey
the OPC segment rules: no empty or all-dot segment, no segment ending in a dot,
and no two parts where one name is derived from the other by appending path
segments. Encoded ``#`` and ``?`` bytes (``%23`` and ``%3F``) and a literal
colon remain valid. Content-type ``Override`` PartNames and ``Default``
extensions are compared without ASCII case sensitivity, and duplicate logical
declarations fail closed. ``Default/@Extension`` follows the OPC
``ST_Extension`` lexical form; encode a Unicode extension as UTF-8 ``%HH``
bytes rather than raw non-ASCII XML text.

The internal metadata limits do not extend ``archive_limits`` or ``Document``.
``Document`` keeps its 1.13 object layout by placing the additional package
validation and relationship bookkeeping in a sidecar owned through the existing
opaque handle-lifetime state. Its move constructor and move assignment remain
``noexcept``. Existing five-value ``archive_limits`` aggregate initializers and
code that depends on the 1.13 ``Document`` layout therefore keep their original
source and binary mapping.

Every lazy source-archive reopen first revalidates the entire current archive:
entry count, per-entry and total sizes, compression ratio, UTF-8 path validity,
canonical form, and uniqueness. The selected semantic XML part is then checked
against the XML limit according to its relationship and content semantics, not
only its file-name extension. ``save()`` and ``save_as()`` perform the same
full-source validation before copying preserved entries. A reader-close failure
returns ``archive_close_failed`` before a lazy result is committed or a save
target is replaced.

``open()`` also records an ordered, metadata-only source fingerprint for every
ZIP entry: physical name, canonical name, logical identity, CRC32, compressed
and uncompressed sizes, and the directory flag. Lazy reads and save-source
copying compare the reopened archive against that exact snapshot. Any changed,
added, removed, or reordered entry normally changes one or more recorded
fingerprint fields; any such fingerprint difference returns
``source_archive_changed`` before mixing the loaded DOM with a different
package generation. When ``save()``
replaces its own source path, it repeats the comparison immediately before the
atomic replacement, reopens the finalized output under the limits originally
used by ``open()``, and refreshes the snapshot only after replacement succeeds.
An output that exceeds those original limits is rejected before FeatherDoc
replaces the source path.
Source-path identity follows directory-entry semantics: Windows resolves
case/8.3/Win32 aliases while respecting case-sensitive directories, and POSIX
resolves symlinked parent directories. Distinct hard-link names remain distinct
on both platforms because atomic replacement publishes one directory entry,
not every link to the same file object.

This fingerprint is a consistency guard, not an authentication mechanism:
ZIP CRC32 and central-directory metadata are not cryptographic integrity
proofs. The final check is path-based and does not lock out another process in
the small interval between closing the check handle and atomic replacement.
Applications that require multi-writer coordination must provide an external
file lock or higher-level version protocol.

Explicit Package Repair
-----------------------

The default repair policy handles only deterministic changes that preserve
unknown metadata: create ``w:body`` under a valid ``w:document``, create missing
root or main-document relationships, and create missing content types or correct
the main-document MIME. Malformed XML, invalid roots or namespaces, duplicate
main-document declarations, and external main-document relationships return
``package_repair_not_possible`` without applying other changes.

After a successful repair, ``save()`` and ``save_as()`` write a sibling temporary
archive and reopen it with strict validation before replacing the target. A
failed check returns ``package_repair_validation_failed`` and preserves the
original. The CLI exposes ``inspect-package <input.docx> --json`` and
``repair-package <input.docx> --output <repaired.docx> --json``.

Handle Invalidation
-------------------

``Paragraph``, ``Run``, ``Table``, ``TableRow``, ``TableCell``, and
``TemplatePart`` are tracked, non-owning handles into the current DOM. Each
handle records a package generation and a node epoch, so it neither extends
the ``Document`` lifetime nor dereferences a pugixml node after the package or
node has been retired.

* ``set_path(...)``, ``open(...)``, and ``create_empty()`` reset the package and
  invalidate every XML-backed handle previously returned by that ``Document``.
* Move construction and move assignment invalidate handles previously returned
  by the source object. Move assignment also invalidates handles from the old
  destination state. The moved-from ``Document`` is closed and may safely be
  assigned a path, opened, or initialized with ``create_empty()`` again.
* A successful ``repair_package(...)`` that changes the package replaces repaired
  DOM state, so previously retained XML-backed handles must be reacquired.
* Removing a node invalidates handles to that node and all descendants.
* ``Paragraph::set_text(...)`` and ``TableCell::set_text(...)`` prepare complete
  replacement content first. Failure preserves the original XML and every
  handle. On success, the paragraph or cell and its ancestor handles remain
  valid, while old paragraph/run handles into replaced content are invalid.
  ``Run::set_text(...)`` preserves the ``Run`` handle itself on success.
  Paragraph/run/table append and insertion failures leave no empty placeholder.
* Table insertion, row/column mutation, merge, and unmerge prepare ``tblGrid``,
  cell properties, and removal roots as one operation. Failure preserves the
  whole table and its handles. Success invalidates only removed or replaced
  subtrees; unremoved table, row, cell, and sibling handles remain valid.
* ``fill_bookmarks(...)`` and ``apply_bookmark_block_visibility(...)`` treat the
  full bindings list as one transaction. A duplicate or invalid binding,
  malformed structure, or allocation failure publishes none of the earlier
  bindings and preserves the XML, error detail, and handles. Success invalidates
  only handles into replaced or removed ranges.
* Footnote, endnote, comment, and revision mutations prepare affected body/story
  XML, relationships, Content Types, review sidecars, and dirty state as one
  transaction. Allocation failure returns ``std::errc::not_enough_memory``
  without publishing partial package state or advancing a handle epoch. On
  success, old handles into each published body, header, or footer story become
  invalid and must be reacquired from ``Document``; handles into stories that
  were not published remain valid.
* A successful ``remove_header_part(...)`` or ``remove_footer_part(...)``
  publishes the related section-reference cleanup and destroys a physical XML
  part. Because ``TemplatePart`` tracks that part at document generation scope,
  the operation invalidates every XML-backed handle returned by the document;
  a rejected or failed removal preserves the current generation and its handles.
* ``move_section(...)`` prepares the reordered body in isolation. A failed
  reorder preserves the body, settings metadata, and every existing handle; a
  successful reorder invalidates body-backed handles while unrelated header and
  footer handles remain valid.
* ``append_section(...)``, ``insert_section(...)``, and ``remove_section(...)``
  also prepare structural body changes in isolation. Failure preserves the
  published body and all handles; success invalidates body-backed handles while
  loaded header/footer handles remain valid.
* ``set_section_page_setup(...)`` and non-structural section reference creation,
  assignment, copy, or removal publish only a fully prepared ``w:sectPr``.
  Failure preserves all state and handles. On a normally formed package,
  success preserves body and related-part handles; a tolerant package whose
  existing ``xmlns:r`` binding is incorrect may require full-document
  publication when an operation must repair that binding, which invalidates
  body-backed handles.
* ``move_header_part(...)`` and ``move_footer_part(...)`` prepare relationship
  order in isolation and reorder ownership without allocation at publication.
  Both failure and success preserve existing body and related-part handles.
* Replacing text in an already resolved header/footer part is transactional.
  Allocation failure preserves the part and every existing handle; success
  invalidates handles in that part subtree while body and other-story handles
  remain valid. Creating a previously missing part also prepares its package
  relationship and Content Types attachment transactionally and preserves
  previously returned handles on a normally formed package.
* ``sync_content_controls_from_custom_xml()`` is transactional across the body
  and every loaded header/footer. Allocation failure leaves the published DOM,
  unsaved edits, and existing handles unchanged. A successful synchronization
  that updates at least one control publishes the isolated DOM copies and
  invalidates all XML-backed handles; a zero-update result preserves them.
* ``rename_style(...)``, ``merge_style(...)``, a clean non-empty
  ``apply_style_refactor(...)`` batch, and a successful mutating
  ``restore_style_refactor(...)`` publish styles, body, header/footer, and
  styles-attachment package parts as one transaction. Allocation failure
  preserves the complete published DOM, dirty state, and existing handles;
  ``apply_style_refactor(...)`` never commits an earlier operation when a later
  operation fails. A rejected restore entry does not mutate that entry, although
  other valid entries in the same restore result may still be applied. Any
  successful mutation invalidates all XML-backed handles, which must then be
  reacquired from ``Document``.
* Rebuilding a table, paragraph, content control, or template-part structure
  invalidates handles into the replaced subtree.

Use ``valid()`` for ``Paragraph``, ``Run``, ``Table``, ``TableRow``, and
``TableCell``; use the explicit ``bool`` conversion for ``TemplatePart``.
Reads through an invalid handle return an empty result, while mutations return
``false`` or an empty handle. Unaffected sibling subtrees remain valid unless an
operation above explicitly advances the complete document generation.
Reacquire affected handles from ``Document`` or an unaffected parent.

Breaking API migration
~~~~~~~~~~~~~~~~~~~~~~

Older releases exposed two-``pugi::xml_node`` constructors and public
``set_parent`` / ``set_current`` methods for the XML-backed handle classes.
Those entry points could not carry lifetime metadata and have been removed.
Obtain handles only from ``Document``, ``TemplatePart``, or a parent handle;
application code no longer needs direct pugixml DOM access through the public
API.

Template Part Access
--------------------

Use template parts when the same operation should work on body, headers, or
footers.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``body_template()``
     - None.
     - ``TemplatePart``
     - Access the document body through the template-part API.
   * - ``header_template(std::size_t index = 0U)``
     - ``index``: physical header part index.
     - ``TemplatePart``
     - Access a physical header part by index.
   * - ``footer_template(std::size_t index = 0U)``
     - ``index``: physical footer part index.
     - ``TemplatePart``
     - Access a physical footer part by index.
   * - ``section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: zero-based section index. ``reference_kind``:
       default, first-page, or even-page header reference.
     - ``TemplatePart``
     - Access the resolved header for a section/reference kind.
   * - ``section_footer_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: zero-based section index. ``reference_kind``:
       default, first-page, or even-page footer reference.
     - ``TemplatePart``
     - Access the resolved footer for a section/reference kind.

Sections And Inspection
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``append_section(bool inherit_header_footer = true)``
     - ``inherit_header_footer``: copy the previous section references when
       ``true``. When ``false``, no explicit references are copied; under the
       WordprocessingML model the new section therefore remains linked to the
       previous section rather than being forced to use blank parts.
     - ``bool``
     - Add a section to the end of the document.
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``: insertion point. ``inherit_header_footer``: copy
       surrounding header/footer references when ``true``. ``false`` leaves
       the inserted section linked-to-previous through missing local references.
     - ``bool``
     - Insert a section before an existing section.
   * - ``remove_section(std::size_t section_index)``
     - ``section_index``: section to remove.
     - ``bool``
     - Remove a section.
   * - ``move_section(std::size_t source_section_index, std::size_t target_section_index)``
     - ``source_section_index``: section to move. ``target_section_index``:
       destination index after removal.
     - ``bool``
     - Reorder sections without recreating the whole document.
   * - ``section_count() const noexcept``
     - None.
     - ``std::size_t``
     - Return the number of sections.
   * - ``header_count() const noexcept`` / ``footer_count() const noexcept``
     - None.
     - ``std::size_t``
     - Return physical header/footer part counts.
   * - ``inspect_sections()``
     - None.
     - ``sections_inspection_summary``
     - Return section/header/footer inspection data.
   * - ``inspect_header_parts()`` / ``inspect_footer_parts()``
     - None.
     - ``std::vector<related_part_inspection_summary>``
     - Return each physical related part and the sections/reference kinds that
       refer to it.
   * - ``inspect_section(std::size_t section_index)``
     - ``section_index``: zero-based section index.
     - ``std::optional<section_inspection_summary>``
     - Inspect one section; empty means the section is not available.
   * - ``get_section_page_setup(std::size_t section_index) const``
     - ``section_index``: section whose setup should be read.
     - ``std::optional<section_page_setup>``
     - Read page size, margins, orientation, and related setup.
   * - ``set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``: target section. ``setup``: page setup values.
     - ``bool``
     - Apply page setup to a section.
   * - ``replace_section_header_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``replacement_text``: full header
       text. ``reference_kind``: resolved header kind.
     - ``bool``
     - Replace section header text through the resolved header part.
   * - ``replace_section_footer_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``replacement_text``: full footer
       text. ``reference_kind``: resolved footer kind.
     - ``bool``
     - Replace section footer text through the resolved footer part.
   * - ``remove_section_header_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: header reference
       kind to detach.
     - ``bool``
     - Remove the section reference only; it does not delete the physical part.
   * - ``remove_section_footer_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``: target section. ``reference_kind``: footer reference
       kind to detach.
     - ``bool``
     - Remove the footer reference only; it does not delete the physical part.
   * - ``inspect_body_blocks()``
     - None.
     - ``std::vector<body_block_inspection_summary>``
     - Inspect paragraph/table order in the body.
   * - ``inspect_paragraphs()`` / ``inspect_paragraphs_with_options(const paragraph_inspection_options &options)``
     - ``options``: controls optional metadata resolution.
     - ``std::vector<paragraph_inspection_summary>``
     - Inspect body paragraphs, including raw numbering IDs and optionally
       resolved numbering-definition metadata.
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``other``: document to compare against. ``options``: semantic diff
       toggles.
     - ``std::optional<document_semantic_diff_result>``
     - Compare semantic document content.

``paragraph_inspection_options::resolve_numbering_metadata`` defaults to
``true``. In that mode, paragraph inspection resolves ``numId`` to its
numbering definition and propagates a source-reopen or numbering-lookup failure
through ``last_error()`` instead of returning a partially enriched result. Set
it to ``false`` when only the raw ``num_id`` and ``level`` are needed and the
inspection must not trigger numbering-part resolution. ``TemplatePart`` exposes
the same option through ``inspect_paragraphs_with_options(options)`` and
``inspect_paragraph_with_options(index, options)`` for body, header, and footer
parts. Distinct ``*_with_options`` names preserve unambiguous source
compatibility for existing code that takes the address of the original
``inspect_paragraphs`` or ``inspect_paragraph`` member.

Template Filling Shortcuts
--------------------------

These methods operate from ``Document`` directly. Use them for common template
fills without first taking a ``TemplatePart``.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``replace_bookmark_text(const std::string &bookmark_name, const std::string &replacement)``
     - ``bookmark_name``: bookmark to fill. ``replacement``: text to insert.
     - ``std::size_t``
     - Replace matching bookmark text and return the replacement count.
   * - ``fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``: bookmark/text pairs to fill in one call.
     - ``bookmark_fill_result``
     - Fill multiple bookmarks and report requested, matched, replaced, and
       missing bookmarks.
   * - ``fill_bookmarks(std::initializer_list<bookmark_text_binding> bindings)``
     - ``bindings``: inline bookmark/text pairs.
     - ``bookmark_fill_result``
     - Fill a small bookmark set without constructing a separate container.
   * - ``list_bookmarks() const`` / ``find_bookmark(std::string_view bookmark_name) const``
     - ``bookmark_name``: bookmark to locate for the find overload.
     - ``std::vector<bookmark_summary>`` / ``std::optional<bookmark_summary>``
     - Inspect available bookmark slots before filling.
   * - ``replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``: content-control tag. ``replacement``: text to insert.
     - ``std::size_t``
     - Replace matching content controls and return the replacement count.
   * - ``replace_content_control_text_by_alias(std::string_view alias, std::string_view replacement)``
     - ``alias``: content-control alias/title. ``replacement``: text to insert.
     - ``std::size_t``
     - Fill controls addressed by alias/title.
   * - ``list_content_controls() const``
     - None.
     - ``std::vector<content_control_summary>``
     - Inspect content-control tags, aliases, lock state, and data binding
       metadata before filling.
   * - ``find_content_controls_by_tag(std::string_view tag) const`` / ``find_content_controls_by_alias(std::string_view alias) const``
     - ``tag`` or ``alias``: lookup key for structured document tags.
     - ``std::vector<content_control_summary>``
     - Locate fill targets before mutating the document.
   * - ``replace_content_control_with_paragraphs_by_tag(std::string_view tag, const std::vector<std::string> &paragraphs)``
     - ``tag``: content-control tag. ``paragraphs``: replacement paragraph
       text.
     - ``std::size_t``
     - Replace matching controls with generated paragraphs.
   * - ``replace_content_control_with_table_rows_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``: content-control tag. ``rows``: row/cell text matrix appended
       as table rows.
     - ``std::size_t``
     - Replace matching controls with rows in the surrounding table context.
   * - ``replace_content_control_with_table_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``: content-control tag. ``rows``: generated table matrix.
     - ``std::size_t``
     - Replace matching controls with generated tables.
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path)``
     - ``tag``: content-control tag. ``image_path``: replacement image path.
     - ``std::size_t``
     - Replace matching controls with images.
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``tag``: content-control tag. ``image_path``: replacement image path.
       ``width_px`` / ``height_px``: explicit image size.
     - ``std::size_t``
     - Replace matching controls with sized images.

Body, Tables, And Images
------------------------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``paragraphs()``
     - None.
     - ``Paragraph &``
     - Return the body paragraph iterator/editing entry.
   * - ``tables()``
     - None.
     - ``Table &``
     - Return the body table iterator/editing entry.
   * - ``append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` and ``column_count``: initial table dimensions.
     - ``Table``
     - Append a table and return the created table handle.
   * - ``append_image(const std::filesystem::path &image_path)``
     - ``image_path``: image to append inline.
     - ``bool``
     - Append an inline image using natural dimensions.
   * - ``append_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``image_path``: image to append. ``width_px`` / ``height_px``:
       explicit image size.
     - ``bool``
     - Append an inline image with an explicit size.
   * - ``append_floating_image(const std::filesystem::path &image_path, floating_image_options options = {})``
     - ``image_path``: image to append. ``options``: floating position/wrap
       options.
     - ``bool``
     - Append a floating image.
   * - ``append_floating_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px, floating_image_options options = {})``
     - ``image_path``: image to append. ``width_px`` / ``height_px``:
       explicit size. ``options``: floating position/wrap options.
     - ``bool``
     - Append a sized floating image.

Notes, Revisions, And Links
---------------------------

The detailed object pages cover these families, but the root ``Document`` API
also exposes document-wide entry points.

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - Method
     - Parameters
     - Returns
     - Purpose
   * - ``append_comment(std::string_view selected_text, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - ``selected_text``: text to annotate. ``comment_text``: comment body.
       ``author`` / ``initials`` / ``date``: optional metadata.
     - ``std::size_t``
     - Add a comment; returns ``1`` on success and ``0`` when no matching text
       can be annotated.
   * - ``append_paragraph_text_comment(std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length, std::string_view comment_text, ...)``
     - ``paragraph_index``: target paragraph. ``text_offset`` and
       ``text_length``: annotated range.
     - ``std::size_t``
     - Add a comment to a precise paragraph text range.
   * - ``append_text_range_comment(std::size_t start_paragraph_index, std::size_t start_text_offset, std::size_t end_paragraph_index, std::size_t end_text_offset, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - Start/end paragraph indexes, text offsets, comment body, and optional
       metadata.
     - ``std::size_t``
     - Add a comment that spans a multi-paragraph text range.
   * - ``list_revisions() const``
     - None.
     - ``std::vector<revision_summary>``
     - Inspect tracked revisions.
   * - ``accept_revision(std::size_t revision_index)`` / ``reject_revision(std::size_t revision_index)``
     - ``revision_index``: revision returned by ``list_revisions()``.
     - ``bool``
     - Accept or reject one tracked revision.
   * - ``append_footnote(std::string_view reference_text, std::string_view note_text)``
     - ``reference_text``: marker text. ``note_text``: footnote body.
     - ``std::size_t``
     - Append a footnote.
   * - ``list_hyperlinks() const``
     - None.
     - ``std::vector<hyperlink_summary>``
     - Inspect hyperlink text and targets before editing.
   * - ``append_hyperlink(std::string_view text, std::string_view target)``
     - ``text``: visible text. ``target``: URL or relationship target.
     - ``std::size_t``
     - Append a hyperlink to the body.
   * - ``replace_hyperlink(std::size_t hyperlink_index, std::string_view text, std::string_view target)``
     - ``hyperlink_index``: item from ``list_hyperlinks()``. ``text`` and
       ``target``: replacement display text and URL/relationship target.
     - ``bool``
     - Replace an existing hyperlink.

Related API Families
--------------------

The root ``Document`` type also exposes these public API families. Use the
focused pages for the full method sets and examples:

* :doc:`sections`: section paragraphs, header/footer assignment, physical
  header/footer part removal, and section page setup.
* :doc:`fields_links_reviews`: comments, comment replies, resolved state,
  footnotes, endnotes, hyperlinks, and tracked revision mutations.
* :doc:`template_part`: template validation, schema scanning, onboarding,
  custom XML synchronization, and content-control form state helpers.
* :doc:`images`: inline/floating image listing, extraction, replacement, and
  removal.
* :doc:`paragraph_run` and :doc:`table`: paragraph, run, table, row, and cell
  editing entry points returned from ``Document``.
* :doc:`styles_numbering` and :doc:`enums`: style/numbering catalogs, shared
  options, and inspection summary types.

Examples
--------

Open, fill content controls, and save:

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

Fill directly from ``Document`` and inspect failures:

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (auto error = doc.open()) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   const auto replaced =
       doc.replace_content_control_text_by_tag("customer", "Ada");
   if (replaced == 0) {
       return 1;
   }

   if (auto error = doc.save_as("filled.docx")) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   return 0;

Create a new document with a table and field-update setting:

.. code-block:: cpp

   featherdoc::Document doc;
   if (doc.create_empty()) {
       return 1;
   }

   doc.enable_update_fields_on_open();
   auto table = doc.append_table(2, 3);
   if (auto customer = table.find_cell(0, 0)) {
       customer->set_text("Customer");
   }
   if (auto status = table.find_cell(0, 1)) {
       status->set_text("Status");
   }

   return doc.save_as("report.docx") ? 1 : 0;
