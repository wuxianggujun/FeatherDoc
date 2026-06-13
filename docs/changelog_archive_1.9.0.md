# Changelog archive: 1.9.0

This file continues `../CHANGELOG.md`.

## [1.9.0] - 2026-04-24

### Added

- Added direct existing-document edit workflows through
  `scripts/edit_document_from_plan.ps1`, letting JSON edit plans apply ordered
  changes to a real `.docx` and write a new `.docx` plus machine-readable
  summary without converting the whole document into JSON.
- Added direct-edit coverage for common Word editing needs: bookmark text,
  bookmark paragraphs, bookmark table rows, ordinary text replacement,
  paragraph alignment and spacing, run style metadata, table cell text, row
  height, column width, vertical/horizontal cell alignment, fill color, borders,
  and table cell merge/unmerge operations.
- Added `scripts/run_edit_document_from_plan_visual_regression.ps1` so the
  direct-edit workflow can be verified through Microsoft Word PDF export,
  rendered PNG pages, and a before/after contact sheet.
- Added template render-data workspace scripts and samples so users can prepare
  an editable JSON data workspace from a template `.docx`, validate the edited
  data/mapping contract, and render the final business `.docx` from
  `-WorkspaceDir`; prepared workspaces now include `START_HERE.zh-CN.md`.
- Added a complete render-data mapping pipeline through
  `scripts/export_render_data_mapping_draft.ps1`,
  `scripts/export_render_data_skeleton.ps1`,
  `scripts/lint_render_data_mapping.ps1`,
  `scripts/validate_render_data_mapping.ps1`,
  `scripts/convert_render_data_to_patch_plan.ps1`,
  `scripts/render_template_document_from_data.ps1`, and
  `scripts/render_template_document_from_workspace.ps1`, so business JSON can
  drive template rendering without hand-authoring raw render-plan patches.
- Added focused PowerShell regression coverage for render-data mapping,
  generated patch plans, workspace-backed rendering, and direct existing-
  document editing so the new template-data toolchain stays release-ready.

- Added CLI `merge-template-schema` so multiple reusable template-schema JSON
  files can now be combined into one normalized contract, with later files
  upserting matching targets and replacing same-bookmark slot definitions
  before the merged result is written or printed.
- Added CLI `lint-template-schema` so committed template-schema JSON can now be
  gated for duplicate target identities, duplicate slot names,
  non-canonical ordering, and leaked `entry_name` metadata during review or CI.
- Added CLI `repair-template-schema` so those canonical template-schema
  maintenance issues can now be rewritten automatically by stripping
  `entry_name`, merging duplicate target identities, folding duplicate slot
  names, and normalizing the final schema.
- Added CLI `patch-template-schema` so a committed template-schema JSON can now
  be maintained through explicit `upsert_targets`, `remove_targets`,
  `remove_slots`, and `rename_slots` operations before the patched result is
  normalized and written back out.
- Added CLI `build-template-schema-patch` so a reviewed left/right schema pair
  can now be converted into a reusable patch JSON, including `{}` no-op output
  for already-equivalent schemas and slot-level `remove_slots` /
  `rename_slots` generation when a target keeps the same full identity.
- Added floating-image anchor z-order control through
  `floating_image_options::z_order`, threaded that metadata through image
  inspection, CLI append/replace flows, and round-trip parsing of
  `wp:anchor/@relativeHeight`, and added a dedicated
  `sample_floating_image_z_order_visual` sample plus
  `run_floating_image_z_order_visual_regression.ps1` / release-gate bundle
  coverage for Word-backed overlap validation.
- Added high-level header/footer part reordering through
  `move_header_part()` / `move_footer_part()` plus CLI
  `move-header-part` / `move-footer-part`, keeping section references bound to
  the same relationship ids across save/reopen while exposing the reordered
  logical part indexes.
- Added CLI `inspect-default-run-properties`,
  `set-default-run-properties`, and `clear-default-run-properties` so
  `docDefaults` font/language/RTL metadata can now be inspected and mutated
  from scripts instead of only through the C++ API.
- Added CLI `inspect-style-run-properties`,
  `set-style-run-properties`, and `clear-style-run-properties` so existing
  styles can now have run font/language/RTL and paragraph bidi metadata
  inspected and updated from scripts instead of only through the C++ API.
- Added `resolve_style_properties()` plus CLI `inspect-style-inheritance` so
  style `basedOn` chains can now be inspected as effective inherited
  font/language/RTL/paragraph-bidi values with per-property source style ids.
- Added `materialize_style_run_properties()` plus CLI
  `materialize-style-run-properties` so supported inherited
  font/language/RTL/paragraph-bidi style metadata can now be frozen onto the
  child style before later parent-style rewrites, and extended the existing
  Word-backed ensure-style visual regression bundle with a dedicated
  child-style freeze case to verify that parent rewrites no longer leak through
  after materialization.
- Added direct paragraph style metadata accessors and mutators for
  `basedOn`/`next`/`outlineLvl`, plus CLI
  `inspect-paragraph-style-properties`,
  `set-paragraph-style-properties`, and `clear-paragraph-style-properties`,
  so heading-flow metadata can now be updated without routing through the more
  destructive `ensure_paragraph_style(...)` path.
- Added `rebase_paragraph_style_based_on()` plus CLI
  `rebase-paragraph-style-based-on` so a paragraph style can now switch to a
  new parent while freezing the currently resolved inherited
  font/language/RTL/paragraph-bidi fields that would otherwise drift, and
  extended the Word-backed ensure-style visual regression bundle with a rebased
  child-style preservation case.
- Added `rebase_character_style_based_on()` plus `set_character_style_based_on()`
  / `clear_character_style_based_on()` and CLI
  `rebase-character-style-based-on` so a character style can now switch to a
  new parent while freezing the currently resolved inherited font/RTL fields
  that would otherwise drift, and extended the Word-backed ensure-style visual
  regression bundle with an inherited-run preservation case.
- Added `ensure_style_linked_numbering()` and CLI
  `ensure-style-linked-numbering` so one custom numbering definition can now be
  created or refreshed and then bound to multiple paragraph styles in a single
  shared outline-numbering workflow, and extended the Word-backed paragraph
  style numbering visual regression bundle with a batched multi-style case.
- Added `scripts/run_project_template_smoke.ps1` plus the runnable
  `samples/project_template_smoke.manifest.json` example so a manifest can now
  orchestrate project-level `validate-template`, `validate-template-schema`,
  template-schema baseline drift checks, and optional Word-backed visual smoke
  evidence across multiple templates while aggregating per-entry artifacts into
  `summary.json` and `summary.md`. Added the companion
  `check_project_template_smoke_manifest.ps1` preflight plus
  `project_template_smoke.manifest.schema.json` so malformed manifest entries
  can now be rejected before the harness starts preparing fixtures or launching
  visual smoke work. Added
  `describe_project_template_smoke_manifest.ps1` and
  `register_project_template_smoke_manifest_entry.ps1` so the same manifest can
  now be inspected and updated through scriptable maintenance entry points
  instead of hand-editing JSON.
- Added `scripts/sync_project_template_smoke_visual_verdict.ps1` so manual
  edits to project-template-smoke `review_result.json` files can now be folded
  back into `summary.json` / `summary.md`, including refreshed per-entry
  `review_status` and `review_verdict`, top-level `visual_verdict`, and
  explicit pending or undetermined visual-review counts. The manifest describe
  helper now surfaces the same latest visual verdict details in maintenance
  output. The same sync script can now also update a linked
  release-candidate `summary.json`, rewrite `final_review.md`, and refresh the
  generated release bundle through `write_release_note_bundle.ps1`.
- Extended `run_release_candidate_checks.ps1` with an optional
  `-ProjectTemplateSmokeManifestPath` gate so release-preflight can now execute
  `run_project_template_smoke.ps1`, record real-template smoke counts plus the
  aggregated project-template `visual_verdict` in `report/summary.json`, and
  thread the same status through generated release artifacts such as
  `START_HERE.md`, `ARTIFACT_GUIDE.md`, `REVIEWER_CHECKLIST.md`,
  `release_handoff.md`, and `release_body.zh-CN.md`. The same gate now also
  supports `-ProjectTemplateSmokeRequireFullCoverage`, which writes
  `project-template-smoke/candidate_discovery.json`, records
  registered/unregistered/excluded DOCX/DOTX candidate counts, and fails the
  release preflight when strict coverage finds an unregistered candidate.
- Added `scripts/discover_project_template_smoke_candidates.ps1` so tracked
  repository `.docx` / `.dotx` files can be compared against the current
  project-template-smoke manifest before real-template onboarding. The report
  marks already registered inputs, filters generated build/output documents by
  default, and emits ready-to-run registration commands with unique suggested
  entry names for unregistered candidates. It also supports
  `-FailOnUnregistered` for CI-style coverage gating and manifest-level
  `candidate_exclusions` for tracked DOCX fixtures that should be intentionally
  excluded from real-template smoke coverage.
- Added `scripts/new_project_template_smoke_onboarding_plan.ps1` so real
  project-template onboarding can start from a non-mutating `plan.json` /
  `plan.md` bundle that combines candidate discovery with schema-baseline
  freeze commands, manifest registration commands, smoke rerun commands, and a
  strict release-preflight command.
- Registered `samples/chinese_invoice_template.docx` in
  `samples/project_template_smoke.manifest.json` as the first committed
  real-template smoke entry, combining schema validation, schema-baseline
  checking, and Word-backed visual smoke evidence.
- Expanded `validate_template(...)` into a stronger schema pass that now also
  reports unexpected bookmarks, kind mismatches, and occurrence mismatches,
  extended CLI `validate-template` parsing with `count=` / `min=` / `max=`
  slot constraints, and kept the new result shape covered by unit and CLI
  regression tests.
- Added document-level `validate_template_schema(...)`, exposed the same
  multi-part schema contract through CLI `validate-template-schema`, added a
  runnable `sample_template_schema_validation` sample, added reusable
  `--schema-file <path>` JSON input for CLI schema validation, taught schema
  files to accept both compact slot strings and structured slot objects, and
  added `export-template-schema` so existing templates can now be turned into
  starter schema files. The export flow now also supports `--section-targets`
  for direct `section-header` / `section-footer` target generation and
  `--resolved-section-targets` for effective linked-to-previous section views;
  schema validation now resolves section header/footer inheritance the same way,
  and CLI schema tooling now also includes `normalize-template-schema` plus
  `diff-template-schema` for canonicalization and revision-to-revision
  comparison, including `--fail-on-diff` for CI-style gating, and the new
  `check-template-schema` command for one-step baseline verification against a
  live document. Added repository-level wrapper scripts
  `freeze_template_schema_baseline.ps1` and
  `check_template_schema_baseline.ps1` so baseline freeze/check workflows can be
  reused locally or in CI. The local release-preflight wrapper
  `run_release_candidate_checks.ps1` can now optionally run the same template
  schema baseline gate and record it in `report/summary.json`, with the parser
  and export flow covered by focused CLI regression tests.
- Added a repository-level template schema baseline registry under
  `baselines/template-schema/manifest.json`, including mixed support for
  committed source templates plus build-relative generated fixtures,
  `prepare_sample_target` fixture preparation, `check_template_schema_manifest.ps1`
  manifest gating, `describe_template_schema_manifest.ps1` maintenance
  reporting, and `register_template_schema_manifest_entry.ps1` for one-step
  baseline freeze plus manifest registration. The same manifest gate is now
  surfaced through release-preflight summaries and generated release metadata
  entry points such as `START_HERE.md`, `REVIEWER_CHECKLIST.md`,
  `release_handoff.md`, `release_body.zh-CN.md`, and `ARTIFACT_GUIDE.md`.
- Extended `scripts/prepare_word_review_task.ps1` with a generic
  `visual-regression-bundle` source so screenshot-backed Word review tasks can
  now be packaged from any curated visual regression bundle that provides
  `summary.json` plus `aggregate-evidence/`, including support for either
  `before_after_contact_sheet.png` or the legacy `contact_sheet.png`, copied
  aggregate evidence, bundle-specific latest-task pointers, and dedicated
  review/review-and-repair task prompt templates.
- Extended `scripts/run_word_visual_release_gate.ps1` so the release gate can
  now execute and package curated visual regression bundles for
  `bookmark-floating-image`, `fill-bookmarks`, `append-image`, `table-row`,
  and `replace-remove-image`, recording each flow plus its generated review
  task in `gate_summary.json` and `gate_final_review.md`.
- Extended `scripts/run_word_visual_release_gate.ps1` again so the curated
  visual regression stage also covers `bookmark-image`,
  `bookmark-block-visibility`, `template-bookmark-paragraphs`, and
  `bookmark-table-replacement`, keeping those screenshot-backed bundles on the
  same generic review-task packaging path as the rest of the release gate.
- Extended `scripts/run_word_visual_release_gate.ps1` further so the curated
  visual regression stage also covers `paragraph-list`,
  `paragraph-numbering`, `paragraph-run-style`, and
  `paragraph-style-numbering`, keeping those paragraph-focused Word evidence
  bundles on the same generic review-task packaging path as the rest of the
  release gate.
- Extended `scripts/run_word_visual_release_gate.ps1` once more so the curated
  visual regression stage also covers `table-row-height`,
  `table-row-cant-split`, `table-row-repeat-header`, `table-cell-fill`,
  `table-cell-border`, and `table-cell-width`, keeping those table-focused
  Word evidence bundles on the same generic review-task packaging path as the
  rest of the release gate.
- Extended `scripts/run_word_visual_release_gate.ps1` again so the curated
  visual regression stage also covers `table-cell-margin`,
  `table-cell-vertical-alignment`, `table-cell-text-direction`, and
  `table-cell-merge`, keeping those additional table cell evidence bundles on
  the same generic review-task packaging path as the rest of the release gate.
- Extended `scripts/run_word_visual_release_gate.ps1` further so the curated
  visual regression stage also covers `template-bookmark-multiline`,
  `section-text-multiline`, `remove-bookmark-block`,
  `template-bookmark-paragraphs-pagination`, and `section-order`, keeping
  those text/template Word evidence bundles on the same generic review-task
  packaging path as the rest of the release gate.
- Extended `scripts/run_word_visual_release_gate.ps1` again so the curated
  visual regression stage also covers `section-part-refs`,
  `run-font-language`, `ensure-style`, `template-table-cli-bookmark`,
  `template-table-cli-column`, and `template-table-cli-direct-column`, and
  normalized the generated bundle refresh command so review-task packaging for
  those flows now consistently honors a custom `TaskOutputRoot` instead of
  falling back to the default task directory when the bundle output path is a
  directory.
- Extended `scripts/run_word_visual_release_gate.ps1` further so the curated
  visual regression stage also covers `template-table-cli`,
  `template-table-cli-merge-unmerge`, `template-table-cli-direct`,
  `template-table-cli-direct-merge-unmerge`,
  `template-table-cli-section-kind`, and
  `template-table-cli-section-kind-row`, and hardened child PowerShell
  execution plus the new flow-specific default build directories so first-run
  CMake warnings and Windows object-path limits no longer derail release-gate
  packaging for those template-table CLI bundles.
- Extended `scripts/run_word_visual_release_gate.ps1` once more so the curated
  visual regression stage now also covers
  `template-table-cli-section-kind-column`,
  `template-table-cli-section-kind-merge-unmerge`, and
  `template-table-cli-selector`, and updated
  `scripts/run_template_table_cli_selector_visual_regression.ps1` to
  self-configure its dedicated NMake build directory so the selector bundle no
  longer depends on a pre-existing shared build tree before release-gate
  packaging can run.
- Extended `scripts/sync_visual_review_verdict.ps1` and
  `scripts/sync_latest_visual_review_verdict.ps1` so verdict refresh now also
  tracks curated `visual-regression-bundle` review tasks, refreshes matching
  gate/release summary task metadata from bundle-specific latest pointers, and
  preserves curated bundle flow plus verdict details when rewriting
  `gate_final_review.md`.
- Extended `scripts/find_superseded_review_tasks.ps1` so superseded-task audits
  now scan every resolved `latest_*_task.json` pointer under the chosen task
  root instead of only the legacy fixed source kinds, allowing bundle-specific
  curated visual regression tasks to report pointer alignment against the
  newest task in their source group.
- Extended `scripts/write_release_artifact_handoff.ps1`,
  `scripts/write_release_artifact_guide.ps1`, and
  `scripts/write_release_reviewer_checklist.ps1` plus
  `scripts/write_release_metadata_start_here.ps1` so generated release
  artifacts now surface curated visual regression bundle verdicts, review-task
  paths, and bundle-specific `open_latest_word_review_task.ps1 -SourceKind
  <bundle-key>-visual-regression-bundle` entry points alongside the existing
  section/page-number visual signoff details.
- Extended `scripts/write_release_metadata_start_here.ps1` again so
  `START_HERE.md` now also surfaces the section-page-setup / page-number-fields
  review task paths plus each curated visual bundle task path and matching
  `open_latest_word_review_task.ps1 -SourceKind <bundle-key>-visual-regression-bundle`
  shortcut, turning the summary-root note into a true first-click handoff for
  fine-grained visual signoff.
- Extended `scripts/write_release_body_zh.ps1` so both
  `release_body.zh-CN.md` and `release_summary.zh-CN.md` now surface
  section-page-setup, page-number-fields, and curated visual regression
  bundle verdict details instead of only the top-level `visual verdict`,
  keeping the short release summary aligned with the richer release handoff
  and checklist outputs.
- Aligned `scripts/run_section_page_setup_visual_regression.ps1` and
  `scripts/run_page_number_fields_visual_regression.ps1` with the repository's
  standard aggregate visual-evidence layout by adding
  `aggregate-evidence/selected-pages/` and
  `aggregate-evidence/before_after_contact_sheet.png`, while preserving the
  legacy `contact_sheet.png` and `contact-sheets/` paths for downstream
  review-task compatibility, and ensured case-level `rendered_pages` metadata
  stays array-shaped even when Word only renders a single page.

### Documentation

- Updated `README.md`, `README.zh-CN.md`, and `docs/v1_7_roadmap_zh.rst` to
  reflect that first-pass style catalog inspection, numbering definition,
  template validation, and floating-image wrap/crop/z-order controls are
  already available, while clarifying the remaining gaps.
- Updated `README.md`, `README.zh-CN.md`, `docs/release_policy_zh.rst`, and
  `baselines/template-schema/README.md` so the repository-level template
  schema maintenance workflow now documents manifest registration, manifest
  inspection, and release-preflight manifest gating from the top-level entry
  points instead of only the low-level helper scripts.
- Updated `scripts/run_word_visual_smoke.ps1` so custom `-InputDocx` runs now
  emit generic review notes and checklist wording instead of the repository's
  fixed table-smoke checklist, keeping ad-hoc visual validation output aligned
  with the document actually under review.
- Updated `README.md`, `README.zh-CN.md`, `VISUAL_VALIDATION*.md`,
  `docs/index.rst`, and `docs/automation/word_visual_workflow_zh.rst` so the
  latest-pointer and verdict-sync workflow now documents curated
  `visual-regression-bundle` tasks explicitly, including
  `open_latest_word_review_task.ps1 -SourceKind <bundle-key>-visual-regression-bundle`
  examples and the fact that `sync_latest_visual_review_verdict.ps1` now scans
  all resolved `latest_*_task.json` pointers under the chosen task root.
- Aligned the legacy template-table CLI visual scripts so their aggregate
  evidence now also records `aggregate-evidence/selected-pages/` plus
  per-case `selected_pages` metadata, while preserving existing
  `first-pages`/`baseline_first_page` compatibility fields, and taught
  `scripts/run_template_table_cli_direct_visual_regression.ps1` to reuse any
  existing `build*` directory that already contains `featherdoc_cli` and the
  matching sample target when `-SkipBuild` is used.
- Updated `docs/release_policy_zh.rst` and
  `docs/automation/word_visual_workflow_zh.rst` so the Chinese release /
  automation docs now describe the finer-grained Word visual signoff flow,
  including section-page-setup, page-number-fields, curated visual regression
  bundle verdicts, and the fact that release bundle entry points surface both
  those verdicts and bundle-specific
  `open_latest_word_review_task.ps1 -SourceKind <bundle-key>-visual-regression-bundle`
  shortcuts after verdict sync.
- Updated the repository entry docs (`README.md`, `README.zh-CN.md`,
  `docs/index.rst`, `VISUAL_VALIDATION.md`, and
  `VISUAL_VALIDATION.zh-CN.md`) so they now explain that verdict-sync
  refreshes `START_HERE.md`, the release bundle notes, and the reviewer entry
  points with finer-grained section-page-setup, page-number-fields, and
  curated visual regression bundle verdicts instead of only the top-level Word
  visual verdict.
- Added `featherdoc_cli inspect-tables` and
  `featherdoc_cli inspect-table-cells` so table and table-cell inspection
  metadata can be queried from the CLI without writing a C++ integration.
- Added `featherdoc_cli set-table-cell-text` so a specific body-table cell can
  be rewritten from the CLI while preserving the existing cell container and
  its layout metadata.
- Added `featherdoc_cli merge-table-cells` so body-table cells can be merged to
  the right or downward from the CLI with explicit direction and span control.
- Added `featherdoc_cli unmerge-table-cells` so existing horizontal or vertical
  body-table merges can be split from the CLI using the same directional
  targeting model as merges.
- Added `featherdoc_cli set-table-cell-fill` and
  `clear-table-cell-fill` so body-table cell shading can be edited from the
  CLI without dropping into raw XML.
- Added `featherdoc_cli set-table-cell-vertical-alignment`,
  `clear-table-cell-vertical-alignment`, `set-table-cell-text-direction`, and
  `clear-table-cell-text-direction` so body-table cell layout metadata can be
  edited from the CLI alongside text replacement and merge operations.
- Added `featherdoc_cli set-table-cell-width`,
  `clear-table-cell-width`, `set-table-cell-margin`,
  `clear-table-cell-margin`, `set-table-cell-border`, and
  `clear-table-cell-border` so body-table cell width, margin, and border
  metadata can also be edited from the CLI without dropping into raw XML.
- Added `featherdoc_cli set-table-row-height`,
  `clear-table-row-height`, `set-table-row-cant-split`,
  `clear-table-row-cant-split`, `set-table-row-repeat-header`, and
  `clear-table-row-repeat-header` so body-table row height, page-splitting,
  and repeating-header behavior can also be edited from the CLI.
- Added `featherdoc_cli inspect-table-rows`, `append-table-row`,
  `insert-table-row-before`, `insert-table-row-after`, and
  `remove-table-row` so CLI workflows can inspect row-level table metadata and
  grow or shrink body tables without dropping to the C++ API.
- Added Word-backed visual regression coverage for `append-table-row`,
  `insert-table-row-before`, `insert-table-row-after`, and
  `remove-table-row`, including body-table growth/shrink cases with
  `inspect-table-rows` / `inspect-table-cells` assertions plus real
  Word-rendered before/after evidence.
- Added granular clear helpers for run/default/style eastAsia font and
  eastAsia/bidi language metadata so callers can drop one override without
  wiping unrelated run formatting.
- Added `featherdoc_cli set-paragraph-list`,
  `featherdoc_cli restart-paragraph-list`, and
  `featherdoc_cli clear-paragraph-list` so CLI workflows can attach managed
  bullet/decimal numbering to body paragraphs, restart list sequences at a
  target paragraph, or remove paragraph list markers without dropping to the
  C++ API.
- Added `featherdoc_cli ensure-numbering-definition` and
  `featherdoc_cli set-paragraph-numbering` so CLI workflows can create or
  update custom numbering definitions and then apply those definitions to body
  paragraphs by definition id, including explicit nesting levels, without
  dropping to the C++ API.
- Added `featherdoc_cli inspect-paragraphs` so CLI workflows can enumerate
  body paragraph indices together with section ownership, paragraph style ids,
  direct numbering metadata, and section-break markers before issuing
  paragraph-targeted mutations.
- Added `featherdoc_cli replace-bookmark-paragraphs` and
  `remove-bookmark-block` so CLI workflows can replace or delete standalone
  bookmark placeholder paragraphs in body/header/footer/section-scoped
  template parts without dropping to the C++ API.
- Added Word-backed visual regression coverage for bookmark/template paragraph
  mutation flows, including multiline bookmark text replacement, section
  header/footer multiline replacement, `replace-bookmark-paragraphs`
  single-page and cross-page expansion, and `remove-bookmark-block`
  placeholder removal across body/header/footer template parts.
- Added Word-backed visual regression coverage for
  `set-bookmark-block-visibility` and `apply-bookmark-block-visibility`,
  including keep/remove body block mutations with real Word-rendered
  before/after evidence.
- Added `featherdoc_cli replace-bookmark-table` and
  `replace-bookmark-table-rows` so CLI workflows can replace standalone
  bookmark placeholder paragraphs with generated tables or expand
  bookmark-backed template rows inside existing tables, including empty
  replacement lists for removing template rows.
- Added Word-backed visual regression coverage for
  `replace-bookmark-table` and `replace-bookmark-table-rows`, including
  standalone bookmark-to-table insertion plus populated and empty
  bookmark-backed table-row replacement cases with real Word-rendered
  before/after evidence.
- Added `featherdoc_cli set-bookmark-block-visibility` and
  `apply-bookmark-block-visibility` so CLI workflows can keep or remove
  bookmark-guarded template blocks in body/header/footer/section-scoped
  template parts without dropping to the C++ API.
- Added `featherdoc_cli replace-bookmark-text` and `fill-bookmarks` so CLI
  workflows can rewrite one bookmark range or batch-fill multiple bookmark
  text slots across body/header/footer/section-scoped template parts without
  dropping to the C++ API.
- Added Word-backed visual regression coverage for
  `replace-bookmark-image`, including inline body bookmark replacement with
  structure assertions from `inspect-images` / `extract-image` plus real
  Word-rendered before/after evidence.
- Added Word-backed visual regression coverage for
  `replace-bookmark-floating-image`, including anchored body bookmark
  replacement with `inspect-images` / `extract-image` assertions plus real
  Word-rendered before/after evidence for floating layout, wrap distances, and
  retained paragraph order.
- Added Word-backed visual regression coverage for `fill-bookmarks`,
  including batch body bookmark replacement with `inspect-template-paragraphs`
  / `inspect-bookmarks` assertions plus real Word-rendered before/after
  evidence that labels, filled values, and paragraph order remain stable.
- Added Word-backed visual regression coverage for `append-image`, including
  both inline body insertion and section-header floating insertion with
  `inspect-images`, `extract-image`, and `inspect-header-parts` assertions
  plus real Word-rendered before/after evidence.
- Added Word-backed visual regression coverage for `replace-image` and
  `remove-image`, including anchored body-image replacement/removal with
  `inspect-images` / `extract-image` assertions plus real Word-rendered
  before/after evidence that the remaining inline image stays in normal flow.
- Added `featherdoc_cli inspect-template-paragraphs`,
  `inspect-template-runs`, `inspect-template-tables`, and
  `inspect-template-table-cells` so CLI workflows can inspect paragraph/run/
  table metadata inside body/header/footer/section-scoped template parts
  before issuing bookmark or template mutations.
- Added `featherdoc_cli inspect-template-table-rows` so template-part table
  rows can also be inspected from the CLI with the same
  `--part/--index/--section/--kind` target model as the rest of the template
  inspection commands.
- Added `featherdoc_cli set-template-table-cell-text` so table-cell text can
  now be rewritten inside body/header/footer/section-scoped template parts
  without changing the existing body-only `set-table-cell-text` command
  semantics.
- Added `featherdoc_cli set-template-table-row-texts` so a contiguous row
  range inside a template-part table can be overwritten from either a
  positional table index or `--bookmark <name>` with the existing
  `--row/--cell` input shape.
- Added `featherdoc_cli set-template-table-cell-block-texts` so a
  bookmark-targeted template table can overwrite a rectangular cell block
  from a starting row/cell position without dropping to the C++ API.
- Added `featherdoc_cli append-template-table-row`,
  `insert-template-table-row-before`,
  `insert-template-table-row-after`, and `remove-template-table-row` so
  template-part tables can now grow or shrink with the same
  `--part/--index/--section/--kind` target model used by the rest of the
  template CLI mutations.
- Added `featherdoc_cli merge-template-table-cells` and
  `unmerge-template-table-cells` so template-part tables can also merge or
  split horizontal/vertical cell spans across `body`, `header`, `footer`,
  `section-header`, and `section-footer` selections without dropping into the
  C++ API.
- Added bookmark-backed template-table selectors across the template-table CLI
  surface plus `TemplatePart::find_table_index_by_bookmark(...)` and
  `TemplatePart::find_table_by_bookmark(...)`, so table inspection and
  mutation commands can target a named bookmark instead of a fragile
  positional table index when you need to reach a specific page-local table.
- Added indexed table editing helpers on `Table` / `TableRow`, including
  `find_row(...)`, `find_cell(...)`, `set_cell_text(...)`, and
  `set_row_texts(...)`, so bookmark-targeted tables can be updated directly in
  C++ without hand-written iterator walks.
- Added batch table editing helpers `set_rows_texts(...)` and
  `set_cell_block_texts(...)`, so page-local tables can be updated from a
  row range or rectangular data block in one call instead of per-cell loops.
- Added `scripts/run_template_table_cli_visual_regression.ps1` so the new
  template-table CLI row/cell commands can be verified through dedicated
  `body`, `section-header`, and `section-footer` samples plus Word-rendered
  before/after screenshot evidence, not just structural inspection or unit
  tests.
- Added `scripts/run_template_table_cli_bookmark_visual_regression.ps1` plus
  a dedicated sample generator so bookmark-targeted
  `set-template-table-row-texts` and
  `set-template-table-cell-block-texts` flows now emit Word-rendered
  before/after evidence for both a bookmark paragraph immediately before the
  target table and a bookmark embedded inside the table itself.
- Added `scripts/run_template_table_cli_direct_visual_regression.ps1` plus a
  dedicated sample generator so the same template-table row/cell CLI flows can
  also emit screenshot-backed baseline/mutated evidence for direct `header`,
  direct `footer`, and `body` scenarios.
- Added `scripts/run_template_table_cli_section_kind_visual_regression.ps1`
  plus a dedicated sample generator so section-scoped template-table row/cell
  CLI flows can also emit screenshot-backed baseline/mutated evidence for
  `--kind default`, `--kind even`, and `--kind first`, including page-specific
  Word screenshots beyond `page-01`.
- Added `scripts/run_template_table_cli_section_kind_row_visual_regression.ps1`
  plus a dedicated sample generator so section-scoped template-table row CLI
  flows can also emit screenshot-backed baseline/mutated evidence for
  `section-header --kind default` insert-row-before,
  `section-header --kind even` append-row,
  `section-footer --kind first` remove-row, and
  `section-footer --kind default` insert-row-after, including page-specific
  Word screenshots beyond `page-01`.
- Added `scripts/run_template_table_cli_section_kind_column_visual_regression.ps1`
  plus a dedicated sample generator so section-scoped template-table column
  CLI flows can also emit screenshot-backed baseline/mutated evidence for
  `section-header --kind default` insert-before,
  `section-header --kind even` remove-column,
  `section-footer --kind first` insert-after, and
  `section-footer --kind default` remove-column, including page-specific Word
  screenshots beyond `page-01`.
- Added `scripts/run_template_table_cli_section_kind_merge_unmerge_visual_regression.ps1`
  plus a dedicated sample generator so section-scoped template-table
  merge/unmerge CLI flows can also emit screenshot-backed baseline/mutated
  evidence for `section-header --kind default` merge-right,
  `section-header --kind even` unmerge-right,
  `section-footer --kind first` merge-down, and
  `section-footer --kind default` unmerge-down, including page-specific Word
  screenshots beyond `page-01`.
- Added `scripts/run_template_table_cli_merge_unmerge_visual_regression.ps1`
  plus a dedicated sample generator so template-table merge/unmerge CLI flows
  can now emit screenshot-backed baseline/mutated evidence for
  `section-header` merge-right, `section-footer` merge-down, and `body`
  unmerge-right / unmerge-down scenarios.
- Added `scripts/run_template_table_cli_column_visual_regression.ps1` plus a
  dedicated sample generator so template-table column CLI flows can now emit
  screenshot-backed baseline/mutated evidence for `section-header`
  insert-before, `section-footer` insert-after, and `body` remove-column
  scenarios.
- Added `scripts/run_template_table_cli_direct_column_visual_regression.ps1`
  plus a dedicated sample generator so the same template-table column CLI
  flows can also emit screenshot-backed baseline/mutated evidence for direct
  `header` insert-before, direct `footer` insert-after, and `body`
  remove-column scenarios.
- Added `scripts/run_template_table_cli_direct_merge_unmerge_visual_regression.ps1`
  plus a dedicated sample generator so template-table merge/unmerge CLI
  flows can also emit screenshot-backed baseline/mutated evidence for direct
  `header` merge-right, direct `footer` unmerge-down, and `body`
  merge-down scenarios.
- Added `featherdoc_cli set-paragraph-style` and
  `featherdoc_cli clear-paragraph-style` so CLI workflows can assign or remove
  paragraph styles from specific body paragraphs by paragraph index without
  dropping to the C++ API.
- Added `featherdoc_cli set-run-style` and `featherdoc_cli clear-run-style` so
  CLI workflows can assign or remove character styles from specific body runs
  by paragraph index and run index without dropping to the C++ API.
- Added Word-backed visual regression coverage for `set-paragraph-style`,
  `clear-paragraph-style`, `set-run-style`, and `clear-run-style`, including
  screenshot-backed before/after evidence for paragraph growth/shrink and
  inline emphasis toggles on body content, plus baseline JSON assertions for
  `inspect-template-runs` against the same shared sample document.
- Added `featherdoc_cli inspect-template-runs` to the top-level command
  reference surfaces in `README.md`, `README.zh-CN.md`, and `docs/index.rst`
  so template-part run inspection is exposed alongside `inspect-runs`.
- Expanded the top-level CLI reference surfaces in `README.md`,
  `README.zh-CN.md`, and `docs/index.rst` with grouped examples for
  paragraph/run/list, body-table, bookmark/image, and template-table command
  families so every currently shipped `featherdoc_cli` subcommand is now
  surfaced somewhere in the primary docs.
- Added Word-backed visual regression coverage for
  `set-table-cell-fill`, `clear-table-cell-fill`,
  `set-table-cell-border`, `clear-table-cell-border`,
  `set-table-cell-width`, `clear-table-cell-width`,
  `set-table-cell-margin`, `clear-table-cell-margin`,
  `set-table-cell-text-direction`, `clear-table-cell-text-direction`,
  `set-table-cell-vertical-alignment`, and
  `clear-table-cell-vertical-alignment`, including real Word-rendered
  before/after evidence plus structural assertions from
  `inspect-table-cells` and `word/document.xml`.
- Added `featherdoc_cli inspect-runs` so CLI workflows can enumerate run
  indices, character style ids, direct run font/language tags, and text
  content for a specific body paragraph before issuing run-targeted mutations.
- Added `featherdoc_cli set-run-font-family` and
  `featherdoc_cli clear-run-font-family` so CLI workflows can assign or remove
  direct run font families on specific body runs by paragraph index and run
  index without dropping to the C++ API.
- Added `featherdoc_cli set-run-language` and
  `featherdoc_cli clear-run-language` so CLI workflows can assign or remove
  direct run language tags on specific body runs by paragraph index and run
  index without dropping to the C++ API.
- Added Word-backed visual regression coverage for `set-run-font-family`,
  `clear-run-font-family`, `set-run-language`, and `clear-run-language`,
  including screenshot-backed monospaced before/after evidence plus
  no-layout-drift language-tag metadata checks on body runs.
- Added Word-backed visual regression coverage for `set-paragraph-list`,
  `restart-paragraph-list`, and `clear-paragraph-list`, including
  screenshot-backed before/after evidence for bullet insertion, decimal list
  restart-at-1 behavior, and list-marker removal on body paragraphs.
- Added Word-backed visual regression coverage for
  `ensure-numbering-definition` and `set-paragraph-numbering`, including
  screenshot-backed custom outline `(3)` / `(3.1)` evidence plus custom `>>`
  bullet application on body paragraphs.
- Added Word-backed visual regression coverage for
  `set-paragraph-style-numbering` and `clear-paragraph-style-numbering`,
  including screenshot-backed visible body paragraphs whose numbering is
  driven by paragraph style definitions, nested heading style numbering, and a
  clear-numbering assertion that keeps `w:bidi` in `word/styles.xml`.
- Added Word-backed visual regression coverage for `ensure-paragraph-style`,
  `ensure-character-style`, and `ensure-table-style`, including
  screenshot-backed evidence that existing body paragraphs, runs, and
  ReviewTable-bound body tables pick up rewritten style definitions without
  rebinding, plus `inspect-tables` / `word/styles.xml` assertions for
  paragraph, character, and table-style metadata changes.
- Added Word-backed visual regression coverage for `set-table-cell-text`,
  `merge-table-cells`, and `unmerge-table-cells`, including screenshot-backed
  evidence for body-cell text replacement, horizontal header-cell merges, and
  merged-banner restoration back to visible three-column body grids.
- Added Word-backed visual regression coverage for `set-table-cell-fill` and
  `clear-table-cell-fill`, including screenshot-backed evidence that body-cell
  shading can be applied and removed without changing surrounding table
  geometry, plus `word/document.xml` shading assertions for both seeded and
  cleared targets.
- Added Word-backed visual regression coverage for `set-table-cell-border` and
  `clear-table-cell-border`, including screenshot-backed evidence that direct
  top-edge cell borders can be applied and cleared on body tables without
  shifting geometry, plus `word/document.xml` assertions for the targeted
  `w:tcBorders/w:top` edge.
- Added Word-backed visual regression coverage for
  `set-table-cell-text-direction` and `clear-table-cell-text-direction`,
  including screenshot-backed evidence that body-table cell text can switch
  between normal horizontal flow and `top_to_bottom_right_to_left` vertical
  flow, plus `word/document.xml` assertions for the targeted
  `w:textDirection` node.
- Added Word-backed visual regression coverage for `set-table-row-height` and
  `clear-table-row-height`, including real Word-rendered before/after evidence
  for exact-height growth and auto-height restoration on body-table rows, plus
  `inspect-table-rows` and `word/document.xml` assertions for seeded and
  cleared `w:trHeight` metadata.
- Added Word-backed visual regression coverage for
  `set-table-row-repeat-header` and `clear-table-row-repeat-header`, including
  real Word-rendered page-2 before/after evidence for repeated-header gain and
  removal on body tables, plus `inspect-table-rows` and `word/document.xml`
  assertions for seeded and cleared `w:tblHeader` metadata.
- Added Word-backed visual regression coverage for
  `set-table-row-cant-split` and `clear-table-row-cant-split`, including real
  Word-rendered page-2/page-3 before/after evidence for row-splitting versus
  intact-row forwarding on body tables, plus `inspect-table-rows` and
  `word/document.xml` assertions for seeded and cleared `w:cantSplit` metadata.
- Added Word-backed visual regression coverage for `insert-section`,
  `copy-section-layout`, `move-section`, and `remove-section`, including real
  Word-rendered multi-page before/after evidence for inserted, reordered, and
  removed sections, plus `inspect-sections`, `show-section-header`, and
  `show-section-footer` assertions for inherited versus detached header/footer
  layouts.
- Added Word-backed visual regression coverage for `assign-section-header`,
  `assign-section-footer`, `remove-section-header`, `remove-section-footer`,
  `remove-header-part`, and `remove-footer-part`, including real Word-rendered
  before/after evidence for shared-part rebinding and part removal, plus
  `inspect-header-parts`, `inspect-footer-parts`, `show-section-header`, and
  `show-section-footer` assertions for section-to-part reference changes.

