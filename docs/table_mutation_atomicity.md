# Table Mutation Atomicity

This document records the transaction contract and validation boundary for
direct WordprocessingML table-property mutations.

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

Use an isolated directory under `.codex-temp` when no reusable Windows build
exists. After validation, confirm that no compiler, build, or test process still
references it, remove only that isolated directory, and verify the Git worktree
is clean.
