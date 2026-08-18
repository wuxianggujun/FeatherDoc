# Table Mutation Atomicity

This document records the transaction contract and validation boundary for
direct WordprocessingML table-property and structural table mutations.

## Current Milestone

The table-property atomicity batch completed on 2026-08-16 at `ca9d3041`.
The batch covers these public mutations:

| Owner | APIs completed in this batch |
| --- | --- |
| `Table` | `clear_width`, `clear_layout_mode`, `clear_alignment`, `clear_indent`, `clear_cell_spacing`, `clear_position`, `clear_cell_margin`, `clear_style_id`, `clear_style_look`, `clear_border` |
| `TableRow` | `set_height_twips`, `clear_height`, `set_cant_split`, `clear_cant_split`, `set_repeats_header`, `clear_repeats_header` |
| `TableCell` | `set_vertical_alignment`, `clear_vertical_alignment`, `set_text_direction`, `clear_text_direction`, `set_margin_twips`, `clear_margin`, `clear_width`, `clear_fill_color`, `clear_border` |

This milestone does not declare all structural table operations complete.
Row/column insertion and removal, merge/unmerge, grid-span mutation, and
vertical-merge mutation have separate multi-node transaction rules and must be
reviewed independently.

## Structural Mutation Checkpoint

The Windows-validated structural checkpoint advanced on 2026-08-18 at
`6042090c`. The unmerge transaction commits are `970b6ae6`, `23b27d63`,
`38559de6`, and `8e8265a5`; row-removal hardening is `81c17777`, row insertion
hardening is `caaff31d`, column insertion hardening is `d919a7ad`, and column
removal hardening is `333ba3fa`; horizontal-merge hardening is `6042090c`. It
covers:

| Owner | APIs completed in this checkpoint |
| --- | --- |
| `TableCell` | `unmerge_right`, `unmerge_down`, `insert_cell_before`, `insert_cell_after`, `remove`, `merge_right` |
| `TableRow` | `remove`, `insert_row_before`, `insert_row_after` |

`unmerge_right` now stages inserted sibling cells, the anchor `w:tcPr`, and
fixed-layout table/grid/cell-width updates before retiring any published
subtree. `unmerge_down` stages every `w:tcPr` in the planned vertical merge
chain and removes empty staged property containers before commit. A failure
before publication rolls back all staged replacements and inserted cells.
Structural mutations retain the existing exception behavior: a
`std::bad_alloc` is rolled back and rethrown, while ordinary checked XML
failures return `false`.

`TableCell::remove` rejects malformed or oversized table geometry before
mutation. Before batch retirement it validates current-cell ownership, unique
row/cell removal targets, the surviving wrapper target, every staged `w:tcPr`,
and the replacement `w:tblPr` / `w:tblGrid` parent relationships. Publication
then commits the staged layout and removes the retired cells without further
allocation. Rejection preserves the original XML and existing cell, paragraph,
run, row, and table handles.

`TableCell::merge_right` preserves a zero-count no-op, but validates actual
merges before publication. It rejects malformed, oversized, or duplicate
`w:tcPr` / `w:gridSpan` geometry; verifies anchor and removed-sibling ownership;
stages the anchor span, fixed-layout cell widths, `w:tblPr`, and `w:tblGrid`;
and validates every staged parent relationship and exclusion before batch
retirement. Commit and sibling removal then use only the validated,
allocation-free publication path. Rejection preserves serialized XML and all
anchor, removed-cell, paragraph, run, row, and table handles.

`TableRow::remove` now rejects malformed or oversized table geometry before
staging. Vertical-merge promotions validate their source and target rows,
unique target ownership, staged `w:tcPr` parent relationships, and exactly one
replacement `w:vMerge` before the removed row or old cell contents are retired.

`TableRow::insert_row_before` and `insert_row_after` validate complete table and
source-row geometry plus vertical-merge guards before mutation. Their checked
deep clone propagates thrown exceptions, and any failure while cloning or
clearing cloned cell bodies removes the unpublished row before returning or
rethrowing. The caller wrapper moves to the inserted row only after the clone is
complete.

`TableCell::insert_cell_before` and `insert_cell_after` now share one column
insertion transaction. It inserts and clears cloned cells across every target
row, stages fixed-layout cell widths plus replacement `w:tblPr` and `w:tblGrid`
subtrees, validates every staged parent relationship, and batch-retires all old
property/grid roots immediately before allocation-free publication. Checked XML
failures remove all unpublished cells and staged nodes; thrown allocation
failures perform the same rollback and propagate. Invalid or oversized column
geometry is rejected before mutation.

This checkpoint does not yet cover `merge_down` or independent grid-span and
vertical-merge setters. Those APIs remain in the structural review queue.

## Transaction Contract

Property setters and clear operations must follow this order:

1. Reject an invalid owner handle before mutation.
2. Return success without writing XML when the requested clear is already a
   no-op.
3. Stage a checked clone of the owning property subtree.
4. Mutate only the staged replacement.
5. Validate the replacement node name, parent, cardinality, attributes, and
   every removal result that the operation depends on.
6. Remove an empty nested container only when it has neither children nor
   attributes and the previous API behavior removed that container.
7. Retire the original subtree only after the replacement is complete.
8. Commit the staged replacement exactly once.

Rollback must remove every staged replacement. `std::bad_alloc` returns
`false` after rollback; other exceptions roll back and propagate. A failed
operation must leave the published XML and unrelated content unchanged.

The paragraph above is the contract for table-property setters and clear
operations. Structural table mutations have a separate compatibility rule:
they must roll back all staged/publication work and rethrow `std::bad_alloc`
unless an API-specific contract is deliberately changed and documented.

Use the existing helpers instead of introducing an independent transaction
framework:

- `stage_table_child`, `rollback_staged_table_child`, and
  `commit_staged_table_child` for `w:tblPr` and `w:trPr`;
- `stage_cell_properties`, `rollback_staged_cell_properties`, and
  `commit_staged_cell_properties` for `w:tcPr`;
- checked XML creation, cloning, attribute, and insertion helpers from the
  table XML helper modules.

## Container Deletion

Clearing the last nested margin or border removes the now-empty
`w:tblCellMar`, `w:tblBorders`, `w:tcMar`, or `w:tcBorders` container when it
has no attributes.

Row clears require an additional deletion transaction. If removing
`w:trHeight`, `w:cantSplit`, or `w:tblHeader` leaves the staged `w:trPr`
completely empty:

1. remove the staged replacement from the row;
2. reset the staged replacement handle;
3. retire the original `w:trPr`;
4. commit removal of the original node.

Keep the local row node mutable (`auto row`, not `const auto row`) because the
empty-container path calls `remove_child()`.

## Source Ownership

- `src/table_properties.cpp` owns `Table` property behavior.
- `src/table_row.cpp` owns `TableRow` property behavior.
- `src/table_cell.cpp` owns `TableCell` property behavior.
- `src/table_column_edit_helpers.*` owns the shared staging primitives.

When several agents work on this area, assign at most one agent to each source
file. Agents should not commit or run builds; integrate and review one API per
commit from the coordinating task.

## Validation Boundary

During a planned batch, use `git diff --check` after each API and defer
compilation until the batch closes. At the batch boundary, run the affected
Windows/MSVC targets once with build concurrency limited to one:

```powershell
cmake --build <windows-build-dir> --target `
  table_properties_unit_tests `
  table_layout_unit_tests `
  table_style_unit_tests `
  table_presentation_unit_tests `
  table_structure_unit_tests `
  xml_handle_retirement_tests `
  --parallel 1

ctest --test-dir <windows-build-dir> `
  -R "^(table_properties_unit|table_layout_unit|table_style_unit|table_presentation_unit|table_structure_unit|xml_handle_retirement)$" `
  --output-on-failure -j 1
```

The 2026-08-16 Windows/MSVC run passed all six tests. No local WSL/Linux test
was required. Linux, sanitizer, allocation-failure, and fuzz validation remain
CI or explicit release/integration work as defined in `CONTRIBUTING.md`.

The 2026-08-18 row-insertion checkpoint at `caaff31d` used an isolated
Release/NMake MSVC build with concurrency one. Both focused tests passed:

- `table_structure_unit`
- `xml_handle_retirement`

No local WSL/Linux, sanitizer, fuzz, or allocation-failure suite was run.

The 2026-08-18 column-insertion checkpoint at `d919a7ad` used the same focused
Windows boundary in the isolated `.codex-temp/column-insertion-windows-msvc`
Release/NMake build. Both focused tests passed again:

- `table_structure_unit`
- `xml_handle_retirement`

The ordinary Windows build type-checked but did not register the CI-only global
allocation-failure sweep. No local WSL/Linux, sanitizer, fuzz, or
allocation-failure suite was run.

The 2026-08-18 column-removal checkpoint at `333ba3fa` used the isolated
`.codex-temp/column-removal-windows-msvc` Release/NMake build with concurrency
one. Both focused tests passed:

- `table_structure_unit`
- `xml_handle_retirement`

The regression covers invalid and oversized `w:gridSpan` geometry plus handle
and serialized-XML preservation. No local WSL/Linux, sanitizer, fuzz, or
allocation-failure suite was run.

The 2026-08-18 horizontal-merge checkpoint at `6042090c` used the isolated
`.codex-temp/merge-right-windows-msvc` Release/NMake build with concurrency one.
Both focused tests passed:

- `table_structure_unit`
- `xml_handle_retirement`

The ordinary Windows regression covers invalid, oversized, duplicate, and
missing span/property geometry plus handle and serialized-XML preservation. No
local WSL/Linux, sanitizer, fuzz, or allocation-failure suite was run.

Use the following focused commands for the next structural batch boundary:

```powershell
cmake --build <windows-build-dir> --target `
  table_structure_unit_tests `
  xml_handle_retirement_tests `
  --parallel 1

ctest --test-dir <windows-build-dir> `
  -R "^(table_structure_unit|xml_handle_retirement)$" `
  --output-on-failure -j 1
```

Use an isolated directory under `.codex-temp` when no reusable Windows build
exists. After validation, confirm that no compiler, build, or test process still
references it, remove only that isolated directory, and verify the Git worktree
is clean.
