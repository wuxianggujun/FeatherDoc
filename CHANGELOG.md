# Changelog

All notable FeatherDoc changes should be recorded in this file.

This project follows a pragmatic `MAJOR.MINOR.PATCH` release model oriented
around clear API behavior, MSVC buildability, diagnostics quality, and core-path
performance.

## [Unreleased]

### Added

- Documented the `sync_visual_review_verdict.ps1 -RefreshReleaseBundle` provenance flow so release reviewers know which internal handoff files surface `reviewed_at` / `review_method` and why public `release_body.zh-CN.md` stays concise.
- Added visual review provenance sync for release gate and release-candidate final reviews so `reviewed_at` / `review_method` metadata survives verdict refreshes.
- Added release handoff, artifact guide, start-here, and reviewer checklist visual review provenance output for `reviewed_at` / `review_method` metadata while keeping those internal sign-off details out of public release notes.
- Added release handoff, artifact guide, start-here, and reviewer checklist review-note output for visual gate sign-off metadata while keeping free-form notes out of public release notes.
- Added release metadata bundle review-status output for smoke, fixed-grid, section/page setup, page-number fields, and curated visual regression verdicts.
- Added smoke and fixed-grid seeded visual verdicts to release-preflight `release_handoff.md` so the release handoff matches the rest of the metadata bundle.
- Added smoke and fixed-grid seeded visual verdicts to release-preflight `REVIEWER_CHECKLIST.md` so human review prompts match the rest of the release metadata bundle.
- Added smoke and fixed-grid seeded visual verdicts to release-preflight `START_HERE.md` and `ARTIFACT_GUIDE.md` so metadata entry points match the full release-note bundle.
- Added smoke and fixed-grid seeded visual verdicts to release-preflight `release_body.zh-CN.md` and `release_summary.zh-CN.md` so the release note bundle covers the full visual gate flow.
- Added `run_release_candidate_checks.ps1` passthrough for the visual gate `*-ReviewVerdict/-ReviewNote` parameters so release-preflight can seed same-run screenshot sign-off metadata and copy per-flow verdicts into `report/summary.json` and `report/final_review.md`.
- Added `run_word_visual_release_gate.ps1 -SectionPageSetupReviewVerdict/-SectionPageSetupReviewNote` and `-PageNumberFieldsReviewVerdict/-PageNumberFieldsReviewNote` support so those generated review tasks can be seeded with same-run screenshot sign-off metadata.
- Added `run_word_visual_release_gate.ps1 -CuratedVisualReviewVerdict/-CuratedVisualReviewNote` support so curated visual regression bundle review tasks can be seeded with screenshot-backed sign-off metadata in the gate summary and final review.
- Added `prepare_word_review_task.ps1 -ReviewVerdict/-ReviewNote` and `run_word_visual_release_gate.ps1 -FixedGridReviewVerdict/-FixedGridReviewNote` support so fixed-grid review tasks can be seeded with screenshot-backed sign-off metadata.
- Added `run_word_visual_release_gate.ps1 -SmokeReviewVerdict/-SmokeReviewNote` passthrough so the standard smoke flow can stamp same-run screenshot verdicts into smoke reports and the gate summary.
- Added `run_word_visual_smoke.ps1 -ReviewVerdict/-ReviewNote` support so screenshot-backed pass/fail/undetermined sign-off can be written directly to `review_result.json` and `final_review.md` with review metadata.
- Added `preview-template-schema-patch --output-patch` support so schema patch previews can copy an explicit patch file or generate a reusable left/right schema patch while reporting `output_patch_path` in JSON summaries for automation.
- Added template-schema patch preview and slot mutation helpers in the public API, including preview, replace-target, upsert-slot, remove-target, remove-slot, rename-slot, and update-slot operations.
- Added template-schema patch `update_slots` support in CLI patch files for targeted slot kind, required, and occurrence metadata changes without replacing full targets.
- Updated `build_template_schema_patch(...)` and `build-template-schema-patch` to emit `update_slots` for same-slot metadata drift and unique rename-plus-metadata drift, keeping generated patches smaller and more reviewable.
- Added a one-stop project template onboarding script that prepares schema candidates, temporary smoke manifests, render-data workspaces, completeness reports, and review checklists without mutating committed manifests by default.
- Added `convert_render_data_to_patch_plan.ps1` `output_patch_path` summaries and stricter render-data selector validation before generated patches are written.
- Added `export_template_render_plan.ps1 -TargetMode resolved-section-targets` plus `-ExportTargetMode` passthrough in the from-patch/from-data render wrappers so render-plan drafts can target effective `section-header` / `section-footer` references directly.
- Added `-ExportTargetMode` support to render-data workspace preparation, workspace validation, and workspace rendering so prepared workspaces can preserve effective section header/footer selectors end to end, including the generated render recommendation.

- Added numbering catalog export/import APIs and CLI automation for stable
  numbering governance, including catalog lint/check/diff/patch/import flows,
  baseline and manifest check scripts, and validation for definitions,
  instances, `lvlOverride`, and `startOverride` round-trips.
- Added style-numbering audit and repair improvements so CLI workflows can
  inspect, gate, plan, apply, and catalog-preload style numbering fixes while
  reporting safer command templates and post-apply cleanliness.
- Added content-control inspection, plain-text replacement, and template-schema
  slot support with `list_content_controls()` /
  `TemplatePart::list_content_controls()`,
  `replace_content_control_text_by_tag(...)` /
  `replace_content_control_text_by_alias(...)`,
  `inspect-content-controls --tag/--alias`,
  `replace-content-control-text --tag/--alias`, and
  `content_control_tag` / `content_control_alias` slots for schema
  export/validation/patch workflows across body/header/footer template parts.
- Added style refactor governance features for duplicate-style merge plans:
  `suggest-style-merges --confidence-profile recommended|strict|review|exploratory`,
  `--min-confidence <0-100>`, source/target suggestion filters,
  `--fail-on-suggestion`, gate diagnostic
  JSON fields, top-level `suggestion_confidence_summary`,
  `recommended_min_confidence`, merge restore
  dry-run / plan-only auditing, repeated `--entry`, source/target rollback
  filters, actionable restore suggestions, and issue summaries.
- Added render-data workspace guidance to the project-template smoke onboarding
  plan so newly discovered templates now include prepare/validate render-data
  commands, workspace paths, validation report paths, and Markdown/JSON review
  hints before manifest registration.
- Added repository assessment documentation in
  `docs/project_score_assessment_zh.rst` to capture the current project score,
  capability assessment, and maintainability priorities.

### Changed

- Refactored release visual verdict metadata collection into a shared helper so release handoff, artifact guide, start-here, reviewer checklist, and release-note writers resolve standard and curated visual verdicts consistently.
- Updated release metadata bundle writers so curated visual regression entries also use same-run `review_verdict` values when no legacy `verdict` field is present.
- Updated template schema patch generation to preserve slot source selectors in generated `remove_slots` and `rename_slots` entries, keeping content-control and bookmark-oriented schemas round-trippable.

- Updated README, Chinese README, Sphinx docs, current-direction notes, and
  feature-gap analysis to describe numbering catalog governance, content
  control inspection and plain-text replacement, style refactor confidence
  filtering, source/target
  suggestion filtering, and the remaining real-corpus confidence calibration
  work.

### Tests

- Added `sync_latest_visual_review_verdict_no_release_summary_test.ps1` to verify the shortest sign-off helper updates only the gate when no matching release-candidate summary is found.
- Extended `sync_latest_visual_review_verdict_test.ps1` to cover auto-detected release summaries, automatic release-bundle refreshes, and public release-body provenance filtering.
- Extended `sync_visual_review_verdict_section_page_setup_test.ps1` to cover `-RefreshReleaseBundle` provenance refreshes and assert public release-body notes stay free of operator-only metadata.
- Extended visual verdict sync regression coverage to verify review provenance is copied into gate summaries, release summaries, and final reviews.
- Extended release metadata bundle regression coverage to verify visual review provenance is emitted only in handoff-oriented artifacts.
- Extended release metadata bundle regression coverage to verify visual review notes are emitted only in handoff-oriented artifacts.
- Extended release note bundle regression coverage to verify visual review-status metadata is rendered beside verdicts.
- Updated release visual verdict metadata consistency coverage to pin the shared helper and its generated bundle behavior.
- Added release note bundle fallback coverage for visual gate `review_verdict` metadata across handoff, guide, checklist, start-here, and Chinese release-note outputs.
- Added release visual verdict metadata consistency coverage so release handoff, artifact guides, reviewer checklist, release notes, and preflight summaries keep smoke, fixed-grid, section/page setup, page-number, and curated visual verdict fields aligned.
- Added CLI regression coverage for `preview-template-schema-patch --output-patch`, including patch-file copy output, left/right schema generated output, and JSON `output_patch_path` reporting.
- Added unit coverage for high-level template schema mutation helpers, including preview-only summaries, target replacement, slot occurrence updates, source-aware slot rename, and target/slot removal.
- Added unit and CLI regression coverage for template-schema `update_slots` patch application, parse errors, preview summaries, output-patch serialization, generated patch output, generated occurrence-clear updates, rename-plus-occurrence-clear updates, source-aware rename-plus-update patch generation, ambiguous rename fallback behavior, cross-source rename fallback behavior, cross-target rename fallback behavior, and changed target identity replacement.
- Split shared CLI test support and style-focused CLI coverage into dedicated helpers/files so the full CLI suite can keep focused regression coverage without duplicating harness code.
- Added PowerShell coverage for the project template onboarding workflow.
- Added PowerShell regression coverage for render-data conversion summary paths and malformed render-plan selector rejection.
- Added PowerShell regression coverage for resolved section header/footer render-plan export and from-data section header/footer rendering.
- Added PowerShell regression coverage for resolved section header/footer workspace preparation, workspace validation inference, workspace rendering, and Word-backed visual review.

- Added unit and CLI coverage for numbering catalog import/export, catalog
  patch/lint/diff/check flows, style-numbering repair with catalog preload,
  content-control API/CLI inspection and replacement, duplicate-style suggestion confidence
  summaries, recommended confidence profile, min-confidence filtering,
  source/target suggestion filtering,
  suggestion gate exit-code and JSON diagnostic behavior,
  merge restore dry-run/selection behavior, and the
  new PowerShell catalog and
  onboarding-plan scripts.

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

## [1.8.0] - 2026-04-15

### Added

- Added `featherdoc_cli ensure-paragraph-style`,
  `featherdoc_cli ensure-character-style`, and
  `featherdoc_cli ensure-table-style` so CLI workflows can create or update
  paragraph, character, and table style definitions with style catalog flags,
  style inheritance, and optional run-language / font / bidi metadata without
  dropping to the C++ API.

## [1.7.0] - 2026-04-15

### Added

- Added `featherdoc_cli replace-bookmark-image` and
  `featherdoc_cli replace-bookmark-floating-image` so CLI workflows can replace
  standalone bookmark placeholder paragraphs inside body/header/footer/
  section-scoped template parts with inline or anchored drawing images,
  including scaling, floating layout flags, and regression coverage.
- Added `featherdoc_cli extract-image` and `featherdoc_cli remove-image` so
  CLI workflows can export or delete existing drawing-backed images from
  body/header/footer/section-scoped template parts using the same
  `--image` / `--relationship-id` / `--image-entry-name` selection semantics
  as `inspect-images`, with regression coverage and ignored CLI-generated
  `.png` test artifacts.
- Added `featherdoc_cli append-image` so CLI workflows can append new inline or
  floating images into body/header/footer/section-scoped template parts, with
  explicit scaling, floating layout flags, missing-part materialization, and
  regression coverage for inline and anchored image insertion.
- Added explicit per-section page setup read/write support through
  `Document::get_section_page_setup(...)` and
  `Document::set_section_page_setup(...)`, including round-trip coverage for
  implicit final sections, section-local `w:pgSz` / `w:pgMar` updates, and
  optional `page_number_start` handling.
- Added `featherdoc_sample_section_page_setup`, `featherdoc_cli
  inspect-page-setup`, and `featherdoc_cli set-section-page-setup`, together
  with CLI regression coverage for partial updates and implicit-final-section
  materialization.
- Added `TemplatePart::append_page_number_field()` and
  `TemplatePart::append_total_pages_field()`, the
  `featherdoc_sample_page_number_fields` sample, and matching
  `featherdoc_cli append-page-number-field` /
  `featherdoc_cli append-total-pages-field` commands so page-number fields can
  be appended to body, header, footer, and section-scoped template parts with
  regression coverage.
- Added `scripts/run_section_page_setup_visual_regression.ps1` plus a
  compatibility wrapper at `scripts/run_section_page_setup_regression.ps1`,
  promoting the existing page-setup bundle into the repository's standard
  visual-regression entry-point naming.
- Added `scripts/run_page_number_fields_visual_regression.ps1` plus a
  compatibility wrapper at `scripts/run_page_number_fields_regression.ps1`,
  promoting the existing `PAGE` / `NUMPAGES` bundle into the repository's
  standard visual-regression entry-point naming while keeping the
  `field_summary.json` artifacts intact.
- Added richer style inspection coverage through `Document::find_style(...)`,
  `Document::find_style_usage(...)`, and `featherdoc_cli inspect-styles`,
  including style-linked numbering summaries, body/header/footer usage
  breakdowns, and per-hit `references` metadata that reports which sections
  reuse a shared header/footer part via `default` / `first` / `even`
  relationships.
- Extended style-usage hit reporting so body matches now carry their owning
  `section_index`, while header/footer hits continue to expose shared-part
  section linkage through `references` metadata.
- Extended floating image placement so `floating_image_options` now supports
  `wrap_mode` plus per-edge wrap distances for square and top/bottom text
  flow around anchored drawings written through `Document`,
  `TemplatePart`, and bookmark-replacement helpers.
- Extended `floating_image_options` with optional per-edge
  `floating_image_crop` values so anchored images can write `a:srcRect`
  trimming through `Document`, `TemplatePart`, and bookmark-replacement
  helpers, with validation that rejects crop values that would remove the
  visible image area.
- Extended `drawing_image_info` so `drawing_images()` now reads back anchored
  `floating_options` metadata, including reference targets, offsets, wrap
  distances, overlap flags, and crop percentages from existing `wp:anchor`
  drawings.
- Added `featherdoc_cli inspect-images` so body, header, footer, and
  section-scoped template parts can expose inline versus anchored drawing
  metadata from the CLI, with JSON/text output and regression coverage.
- Extended `featherdoc_cli inspect-images` with `--relationship-id` and
  `--image-entry-name` filters so image inspection can target a stable drawing
  relationship or resolved media package path before emitting list/single-image
  output.
- Added `featherdoc_cli replace-image` so existing body/header/footer/
  section-scoped drawing images can be replaced from the CLI while preserving
  their current size and inline versus anchored placement metadata.

### Fixed

- Added `scripts/assert_release_material_safety.ps1`, wired it into
  `write_release_note_bundle.ps1`, and taught `package_release_assets.ps1` to
  sanitize repo-local absolute paths inside staged text/JSON metadata before
  auditing and zipping public release materials.
- Allowed `package_release_assets.ps1 -AllowIncomplete` to package a CI preview
  bundle when `visual_gate` is explicitly `skipped`, recording the gate status,
  adding a placeholder README instead of fake Word evidence, and uploading the
  sanitized preview bundle from `windows-msvc.yml`.
- Removed remaining public-facing placeholder wording from the release quickstart /
  template docs, and added regression coverage so `release_body.zh-CN.md` /
  `release_summary.zh-CN.md` do not fall back to the old
  `write_release_body_zh.ps1` placeholder boilerplate.
- Added `sync_github_release_notes.ps1` so the audited
  `release_body.zh-CN.md` can safely sync into the matching GitHub Release,
  with an explicit `-Publish` path that only makes the release public after
  final local Word signoff.
- Added `publish_github_release.ps1` as the one-shot wrapper for uploading the
  public ZIP assets and syncing the audited GitHub Release notes against the
  same tag before an optional final publish step.
- Added zero-input GitHub Actions workflows, `Release Refresh`
  (`.github/workflows/release-refresh.yml`) and `Release Publish`
  (`.github/workflows/release-publish.yml`), for self-hosted Windows runners
  that already have the validated local release bundle on disk.
- Simplified the generated release metadata so reviewers can choose the correct
  workflow button in `Actions` without manually editing workflow form fields.
- Taught `write_release_body_zh.ps1` to normalize absolute-path examples such
  as Windows absolute path examples into public-safe wording before they can flow into
  generated release notes.

### Documentation

- Added a Chinese historical-release backfill playbook, linked it from the
  Sphinx docs homepage and release policy, and recorded the current
  `2026-04-12` recovery baseline for the `v1.0.0` to `v1.6.4` release set.
- Cleaned the Sphinx documentation build by adding a hidden docs toctree,
  fixing existing reStructuredText syntax issues, adding the docs static
  directory placeholder, and ignoring `docs/_build/` in Git.

## [1.6.4] - 2026-04-11

### Added

- Added `scripts/package_release_assets.ps1` so a single release-candidate
  summary can now stage and package the public MSVC install ZIP, the
  screenshot gallery ZIP, and the release-evidence ZIP, with optional
  `gh release upload --clobber` publishing to the matching GitHub release.

### Fixed

- Release-facing metadata bundles now emit repository-relative paths across
  `START_HERE.md`, `ARTIFACT_GUIDE.md`, `REVIEWER_CHECKLIST.md`,
  `release_handoff.md`, `final_review.md`, and `gate_final_review.md` instead
  of leaking machine-local Windows absolute paths back into public release
  notes and reviewer handoff material.
- Visual-verdict sync and release-candidate final-review regeneration now keep
  those reviewer-facing evidence paths relative even after a later screenshot
  review refreshes the bundle.

## [1.6.3] - 2026-04-11

### Changed

- Refreshed the public README and Sphinx gallery so the repository now shows
  standalone Word-rendered `merge_right()` and `merge_down()` fixed-grid
  closure pages alongside the aggregate quartet signoff bundle and the Chinese
  invoice template output.
- Rebuilt `README.zh-CN.md` as a readable UTF-8 Simplified Chinese entry point
  instead of shipping a garbled document.
- Expanded release-facing quickstarts, templates, and README-gallery refresh
  scripts so installed `visual-validation/` assets now include the standalone
  fixed-grid closure pages and the Chinese business-template preview.

## [1.6.2] - 2026-04-11

### Fixed

- Public Chinese release-note generation now emits install-tree relative paths
  such as `share/FeatherDoc/...` plus repository-relative evidence paths instead
  of leaking machine-local absolute paths into `release_body.zh-CN.md` and
  `release_summary.zh-CN.md`.
- Replaced the release-note relative-path helper with a PowerShell-compatible
  implementation so local release-preflight runs no longer depend on
  `System.IO.Path.GetRelativePath()` availability.
- Added regression coverage for the public release-note bundle so the generated
  Chinese release body now fails tests if it contains the local install prefix
  or local working directory.

## [1.6.1] - 2026-04-11

### Added

- Added standalone fixed-grid merge/unmerge samples plus a bundled Word-rendered
  regression flow for `merge_right()`, `merge_down()`, `unmerge_right()`, and
  `unmerge_down()` so reopened tables now have screenshot-backed width-closure
  repros and review tasks.
- Added `fixed_grid_regression_bundle` and `docs_conf_version_sync` checks so
  the release candidate suite now covers reopened fixed-layout table workflows
  and guards Sphinx docs metadata drift from `CMakeLists.txt`.

### Changed

- Release metadata generation now persists `release_version` in the preflight
  summary and refreshes `release_handoff.md` / `release_body.zh-CN.md` against
  the matching `CHANGELOG.md` release section when available, instead of always
  falling back to `Unreleased`.
- Refreshed `README.md`, `README.zh-CN.md`, `docs/index.rst`, and installed
  visual-validation assets to highlight the reopened fixed-layout column-width
  Word rendering and fixed-grid aggregate evidence more directly.

### Fixed

- `docs/conf.py` now resolves `version` / `release` directly from the root
  `CMakeLists.txt`, preventing stale documentation headers during release prep.

## [1.6.0] - 2026-04-11

### Added

- Added `README.zh-CN.md` as a repository-level Simplified Chinese README,
  linked it from `README.md`, documented the bilingual entry in
  `docs/index.rst`, refreshed the docs homepage with current Word-rendered
  validation previews, added `scripts/refresh_readme_visual_assets.ps1` for
  repository-gallery refresh, taught `run_word_visual_release_gate.ps1` and
  `run_release_candidate_checks.ps1` to optionally trigger that refresh in the
  same run, and installed both README files under `share/FeatherDoc`.
- Added `VISUAL_VALIDATION.md` and `VISUAL_VALIDATION.zh-CN.md`, linked the
  preview galleries back to those repro notes, and installed the same
  screenshot assets plus repro guides under `share/FeatherDoc` so release
  artifacts now keep the screenshot -> script -> review-task entry visible.
- Added `VISUAL_VALIDATION_QUICKSTART.md` and
  `VISUAL_VALIDATION_QUICKSTART.zh-CN.md` as install-facing shortcut guides so
  `share/FeatherDoc` now exposes a direct preview PNG -> repro command ->
  review-task path without making users scan the longer README first, and now
  also points directly at the README-gallery refresh step.
- Added `RELEASE_ARTIFACT_TEMPLATE.md`,
  `RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`, and
  `scripts/write_release_artifact_handoff.ps1` so release-preflight output can
  be turned into a release-facing handoff note that points back to installed
  preview PNGs, repro commands, and evidence files.
- Updated `scripts/run_release_candidate_checks.ps1` to reuse an already active
  MSVC developer environment, and updated `windows-msvc.yml` to upload a
  `windows-msvc-release-metadata` artifact carrying `share/FeatherDoc` plus a
  CI-side `release_handoff.md` / summary bundle for release-note preparation.
- Added `scripts/write_release_body_zh.ps1` so the release-preflight summary
  can also emit a directly editable Chinese release-body preview
  (`release_body.zh-CN.md`) alongside `release_handoff.md`, with the initial
  "highlights" section seeded from `CHANGELOG.md` `Unreleased` entries.
- Updated `scripts/write_release_body_zh.ps1`,
  `scripts/run_release_candidate_checks.ps1`, and
  `scripts/write_release_artifact_handoff.ps1` so release-preflight now also
  emits a shorter `release_summary.zh-CN.md` preview, wires that path into the
  report bundle, and carries the short summary through handoff metadata and
  installed release-note guidance.
- Added `scripts/write_release_note_bundle.ps1` as the one-shot post-review
  refresh entry, and updated the quickstarts, templates, README/docs release
  notes, and handoff regeneration path to refresh `release_handoff.md`,
  `release_body.zh-CN.md`, and `release_summary.zh-CN.md` together after a
  later screenshot-backed visual verdict update.
- Added `scripts/sync_visual_review_verdict.ps1` so screenshot-backed task
  verdicts can now be promoted back into `gate_summary.json`,
  release-preflight `summary.json`, `gate_final_review.md`, `final_review.md`,
  and the regenerated release-note bundle without rerunning the full Windows
  preflight.
- Added `scripts/sync_latest_visual_review_verdict.ps1` as the shortest
  post-review helper so the newest document/fixed-grid task pointers can now
  auto-resolve the matching gate summary and release-preflight summary before
  delegating to the explicit verdict-sync path.
- Added a dedicated fixed-grid merge/unmerge regression bundle around
  `scripts/run_fixed_grid_merge_unmerge_regression.ps1`, covering
  `merge_right()`, `merge_down()`, `unmerge_right()`, and `unmerge_down()` via
  standalone runnable samples, aggregate Word-rendered contact sheets,
  screenshot-backed review-task packaging, and repository README gallery
  refresh for the fixed-grid quartet.
- Added `scripts/write_release_artifact_guide.ps1` so the generated report
  bundle now includes an `ARTIFACT_GUIDE.md` index that points artifact
  browsers at the installed `share/FeatherDoc` entry points plus the release
  summary/body/handoff files before they dive into the raw evidence set.
- Added `scripts/write_release_reviewer_checklist.ps1` so the report bundle now
  also emits a `REVIEWER_CHECKLIST.md` with a fixed three-step reviewer flow
  covering release-note review, verification state, and publish-or-refresh handoff.
- Added `scripts/write_release_metadata_start_here.ps1`, a summary-root
  `START_HERE.md`, and an artifact-root `RELEASE_METADATA_START_HERE.md`, then
  updated the README/docs/templates so release metadata now has one consistent
  start-here entry before `ARTIFACT_GUIDE.md` and `REVIEWER_CHECKLIST.md`.
- Added `table_style_look`, `Table::style_look()`, `set_style_look(...)`, and
  `clear_style_look()` for editing `w:tblLook` on existing tables so first/last
  row or column emphasis plus row/column banding can be retuned without
  dropping to raw XML.
- Added `Document::restart_paragraph_list(...)` so managed bullet or decimal
  numbering can restart from a fresh `w:num` instance while adjacent list items
  continue the restarted sequence instead of falling back to the shared default
  instance.
- Added `TableCell::remove()` so lightweight table editing can delete one
  standalone column across the whole table while refusing to remove the last
  remaining column or any column that intersects a horizontal merge span.
- Added `TableCell::insert_cell_before()` and `insert_cell_after()` so
  lightweight table editing can insert one safe cloned column across the whole
  table while expanding `w:tblGrid`, clearing copied merge markup on the new
  cells, and refusing insertion points that land inside horizontal merge spans.
- Added `TableCell::unmerge_right()` and `unmerge_down()` so lightweight table
  editing can split one pure horizontal merged cell back into standalone
  siblings and remove one full vertical merge chain while keeping restored
  cells editable through the same wrappers.
- Added `Table::column_width_twips()`, `set_column_width_twips(...)`, and
  `clear_column_width()` so existing tables can edit `w:tblGrid/w:gridCol`
  widths directly and persist explicit per-column twip widths without dropping
  to raw XML. Fixed-layout tables now also mirror complete grid widths back
  into cell `w:tcW` values, including when `set_layout_mode(fixed)` is applied
  after the grid widths are already in place, while clearing one grid column
  width removes stale `w:tcW` hints from cells that still cover that column.
  Switching back to `autofit` or clearing `tblLayout` keeps the saved
  `w:gridCol` / `w:tcW` widths as preferred widths instead of discarding them.
  `set_width_twips(...)` and `clear_width()` likewise only change `w:tblW`
  and leave the saved grid/cell preferred widths intact.
  Manual `TableCell::set_width_twips(...)` / `clear_width()` edits remain
  possible, but the next fixed-grid synchronization pass rewrites those cells
  back to the grid-derived widths.
  Reopened legacy fixed-layout tables with stale `w:tcW` values can be
  normalized the same way by reapplying `set_layout_mode(fixed)`.
  The same normalization also runs when those reopened fixed-layout tables go
  through explicit span or column edits such as `merge_right()`,
  `unmerge_right()`, `set_column_width_twips(...)`, `insert_cell_before()` /
  `insert_cell_after()`, or `TableCell::remove()`.
  `clear_column_width()` follows the same reopened fixed-layout path for the
  cleared grid column, but only clears `w:tcW` on cells that still cover that
  column instead of re-normalizing unrelated stale cell widths.
  Plain non-grid edits such as `TableCell::set_text(...)`,
  `set_fill_color(...)`, and `set_border(...)` do not trigger that
  normalization on their own.
  The same is true for row-only structural edits:
  `TableRow::insert_row_before()`, `insert_row_after()`, and `remove()` keep
  reopened stale `w:tcW` values as-is instead of re-normalizing them from
  `w:tblGrid`.
- Added `samples/sample_edit_existing_table_style_look.cpp` as a runnable
  workflow that reopens a saved `.docx`, updates `tblLook` on an existing
  table, and preserves the underlying style reference.
- Added `samples/sample_restart_paragraph_list.cpp` as a runnable workflow that
  reopens a saved `.docx`, restarts an existing decimal list at `1.`, and
  continues the restarted sequence in later paragraphs.
- Added `samples/sample_remove_table_column.cpp` as a runnable workflow that
  reopens a saved `.docx`, removes a temporary middle column from an existing
  table, and continues editing the surviving result column through the same
  wrapper.
- Added `samples/sample_insert_table_column.cpp` as a runnable workflow that
  reopens a saved `.docx`, inserts one cloned column after the base column,
  inserts another before the result column, and keeps editing the new cells.
- Added `samples/sample_unmerge_table_cells.cpp` as a runnable workflow that
  reopens a saved `.docx`, splits one horizontal merge and one vertical merge,
  and continues editing the restored cells.
- Added `samples/sample_edit_existing_table_column_widths.cpp` as a runnable
  workflow that reopens a saved `.docx`, rewrites `w:tblGrid` column widths on
  an existing table, and makes the resulting narrow/medium/wide split easy to
  verify in Word.

### Fixed

- Fixed PowerShell parsing in `scripts/run_word_visual_smoke.ps1` and
  `scripts/run_fixed_grid_merge_unmerge_regression.ps1` so the generated review
  checklist text no longer trips `pwsh` on Markdown backticks during real local
  Word-rendered validation runs.

### Validation

- Validated table style-look edits with MinGW build + `ctest` and a Word visual
  smoke pass for `featherdoc_sample_edit_existing_table_style_look`, including
  a final XML check that the saved table keeps `w:tblStyle` and writes the
  expected `w:tblLook`.
- Validated paragraph-list restart edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_restart_paragraph_list`, confirming
  the rendered document shows two decimal sequences that both begin at `1.`.
- Validated table-column removal edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_remove_table_column`, confirming
  the rendered document keeps only the surviving two columns and the saved
  table shrinks to two `w:gridCol` entries while dropping the removed
  column's saved grid width.
- Validated safe table-column insertion edits with targeted `unit_tests` cases
  plus the Word visual smoke sample, confirming inserted columns stay aligned,
  inherit the source-side `w:gridCol` width after
  `insert_cell_before()` / `insert_cell_after()`, and still behave correctly
  after insertion at a merged-block boundary.
- Validated table-cell unmerge edits with targeted `unit_tests` cases plus the
  Word visual smoke sample, confirming `unmerge_right()` and `unmerge_down()`
  restore standalone cells without leftover merge markup in the saved XML.
- Validated table grid-column width edits with targeted `unit_tests` cases plus
  the Word visual smoke sample, confirming `Table::set_column_width_twips(...)`
  updates `w:tblGrid/w:gridCol`, `clear_column_width()` removes one saved
  `w:w` attribute, and the rendered layout keeps a visible narrow/medium/wide
  split.

## [1.5.0] - 2026-04-09

### Added

- Added `TemplatePart::paragraphs()`, `tables()`, and `append_table(...)` so
  existing body, header, and footer parts can use direct paragraph/table
  traversal and lightweight table creation/editing without dropping back to the
  owning `Document`.
- Added `TemplatePart::append_image(...)` and
  `TemplatePart::append_floating_image(...)` so existing body, header, and
  footer parts can append new inline or anchored images without dropping back
  to the owning `Document`.
- Added `TemplatePart::append_paragraph(...)` so existing body, header, and
  footer parts can append a new paragraph directly and continue editing the
  returned `Paragraph` handle without dropping back to paragraph cursor
  bookkeeping.
- Added `Paragraph::insert_paragraph_before()` so existing body, header,
  footer, and table-cell paragraph handles can prepend a new sibling paragraph
  directly before the current anchor without dropping to manual XML ordering.
- Added `Paragraph::insert_paragraph_like_before()` and
  `Paragraph::insert_paragraph_like_after()` so existing body, header, footer,
  and table-cell paragraph handles can clone paragraph-level properties such as
  styles, bidi flags, and list numbering into new sibling paragraphs without
  copying the anchor paragraph's body content.
- Added `Run::insert_run_before()` and `Run::insert_run_after()` so existing
  body, header, footer, and table-cell paragraphs can insert sibling runs
  around the current anchor run while keeping the anchor handle usable.
- Added `Run::insert_run_like_before()` and `Run::insert_run_like_after()` so
  existing body, header, footer, and table-cell paragraphs can clone the
  current anchor run's formatting properties into a new empty sibling run
  without copying the anchor's body content.
- Added `TableRow::insert_row_before()` for cloning the current row structure
  into a new empty row directly above it, while conservatively refusing rows
  that participate in vertical merge chains.
- Added `TableRow::insert_row_after()` for cloning the current row structure
  into a new empty row directly below it, while conservatively refusing rows
  that participate in vertical merge chains.
- Added `Table::insert_table_before()` and `Table::insert_table_after()` for
  inserting new sibling tables directly around an existing body/header/footer
  table while retargeting the wrapper to the inserted table.
- Added `Table::insert_table_like_before()` and `Table::insert_table_like_after()`
  for cloning an existing body/header/footer table's structure and
  table/row/cell formatting into a new empty sibling table while retargeting
  the wrapper to the inserted clone.
- Added `Table::cell_spacing_twips()`, `set_cell_spacing_twips()`, and
  `clear_cell_spacing()` for editing `w:tblCellSpacing` on existing tables
  without dropping to raw XML.
- Added `samples/sample_remove_table.cpp` as a runnable workflow that reopens
  a saved `.docx`, removes a temporary middle table, and continues editing the
  following table through the same wrapper.
- Added `samples/sample_insert_table_around_existing.cpp` as a runnable
  workflow that reopens a saved `.docx`, inserts new tables directly before and
  after an existing anchor table, and continues editing the surrounding tables
  through the returned wrappers.
- Added `samples/sample_insert_table_like_existing.cpp` as a runnable workflow
  that reopens a saved `.docx`, duplicates an existing table's layout and
  styling into new empty sibling tables, fills the clones, and keeps the
  original anchor table editable.
- Added `samples/sample_edit_existing_table_spacing.cpp` as a runnable workflow
  that reopens a saved `.docx`, adds visible inter-cell spacing to an existing
  table, and saves the updated layout back out.
- Added `samples/sample_edit_existing_part_tables.cpp` as a runnable workflow
  that creates header tables, reopens the saved `.docx`, removes a temporary
  middle header table, and continues editing the following table through the
  same wrapper.
- Added `samples/sample_edit_existing_part_append_images.cpp` as a runnable
  workflow that seeds body/header/footer content, reopens the saved `.docx`,
  and appends new inline or floating images through `TemplatePart` handles.
- Added `samples/sample_edit_existing_part_paragraphs.cpp` as a runnable
  workflow that seeds body/header/footer content, reopens the saved `.docx`,
  and appends new paragraphs through `TemplatePart` handles.
- Added `samples/sample_insert_paragraph_before.cpp` as a runnable workflow
  that seeds body/header/footer anchor paragraphs, reopens the saved `.docx`,
  and inserts new paragraphs directly before those existing anchors.
- Added `samples/sample_insert_paragraph_like_existing.cpp` as a runnable
  workflow that seeds styled body/header/footer anchor paragraphs, reopens the
  saved `.docx`, and clones paragraph formatting into new sibling paragraphs
  around those existing anchors.
- Added `samples/sample_insert_run_around_existing.cpp` as a runnable workflow
  that seeds body/header/footer anchor runs, reopens the saved `.docx`, and
  inserts new runs directly around those existing anchors.
- Added `samples/sample_insert_run_like_existing.cpp` as a runnable workflow
  that seeds styled body/header/footer anchor runs, reopens the saved `.docx`,
  and clones run formatting into new sibling runs around those existing
  anchors.
- Added `samples/sample_insert_table_row_before.cpp` as a runnable workflow
  that reopens a saved `.docx`, inserts a formatted row above an existing row,
  and saves the edited result back out.
- Added `samples/sample_insert_table_row.cpp` as a runnable workflow that
  reopens a saved `.docx`, inserts a formatted row in the middle of an
  existing table, and saves the edited result back out.
- Added `remove_bookmark_block(...)` across `Document` and `TemplatePart` as a
  high-level helper for deleting standalone bookmark placeholder paragraphs in
  body, header, or footer parts without having to pass an explicit empty
  replacement list.
- Added `samples/sample_remove_bookmark_block.cpp` as a runnable template edit
  flow for visually confirming that a standalone bookmark paragraph is removed
  cleanly from the rendered output.
- Added `remove_drawing_image(...)` and `remove_inline_image(...)` across
  `Document` and `TemplatePart` so existing inline or anchored images can be
  deleted while automatically pruning orphaned media parts on the next save.
- Added `samples/sample_remove_images.cpp` as a runnable workflow that creates,
  removes, and re-saves existing drawing-backed images for manual review.

### Validation

- Revalidated the table-removal and row-insertion editing flows with MSVC
  build + `ctest` and Word visual smoke passes for
  `featherdoc_sample_remove_table`,
  `featherdoc_sample_insert_table_around_existing`,
  `featherdoc_sample_edit_existing_part_tables`, and
  `featherdoc_sample_insert_table_row_before` before cutting `1.5.0`.
- Revalidated styled table-clone insertion with MSVC build + `ctest` and a
  Word visual smoke pass for `featherdoc_sample_insert_table_like_existing`
  before cutting `1.5.0`.
- Revalidated table cell-spacing edits with MinGW build + `ctest` and a Word
  visual smoke pass for `featherdoc_sample_edit_existing_table_spacing`
  before cutting `1.5.0`.
- Revalidated template-part image append flows with MSVC build + `ctest` and a
  Word visual smoke pass for
  `featherdoc_sample_edit_existing_part_append_images`, including a final
  manual review of the generated PDF/PNG evidence after correcting header
  floating-image placement before cutting `1.5.0`.
- Revalidated template-part paragraph append flows with MSVC build + `ctest`
  and a Word visual smoke pass for
  `featherdoc_sample_edit_existing_part_paragraphs` before cutting `1.5.0`.
- Revalidated paragraph insertion-before flows with MSVC build + `ctest` and a
  Word visual smoke pass for `featherdoc_sample_insert_paragraph_before`
  before cutting `1.5.0`.
- Revalidated paragraph property-clone insertion flows with MinGW build +
  `ctest` and a Word visual smoke pass for
  `featherdoc_sample_insert_paragraph_like_existing` before cutting `1.5.0`.
- Revalidated run insertion-around-anchor flows with MinGW build + `ctest` and
  a Word visual smoke pass for `featherdoc_sample_insert_run_around_existing`
  before cutting `1.5.0`.
- Revalidated run property-clone insertion flows with MinGW build + `ctest`
  and a Word visual smoke pass for
  `featherdoc_sample_insert_run_like_existing` before cutting `1.5.0`.

## [1.4.0] - 2026-04-08

### Added

- Expanded existing `.docx` editing workflows across paragraphs, runs,
  tables, bookmarks, headers/footers, and drawing-backed images, including
  safe `TableRow::remove()` behavior that preserves vertical merge restarts
  when deleting the first row of a merged block.
- Added `floating_image_horizontal_reference`,
  `floating_image_vertical_reference`, `floating_image_options`,
  `append_floating_image(...)`, and
  `replace_bookmark_with_floating_image(...)` so body/header/footer parts can
  create or replace anchored `wp:anchor` drawings without dropping to manual
  XML editing.

### Documentation

- Added `samples/sample_edit_existing.cpp`,
  `samples/sample_edit_existing_part_images.cpp`, and
  `samples/sample_floating_images.cpp`, then documented the related edit,
  drawing-image, and floating-image workflows in `README.md` and
  `docs/index.rst`.
- Added a minimal Chinese/CJK quickstart example and runnable Chinese template
  samples to `README.md` and `docs/index.rst`, and refreshed the README
  screenshot gallery plus sponsor layout.

### Validation

- Added `scripts/run_install_find_package_smoke.ps1` together with a minimal
  `find_package(FeatherDoc CONFIG REQUIRED)` consumer smoke project for MSVC
  install/export validation.
- Revalidated the release candidate with MSVC build + `ctest`, install smoke,
  and Word visual smoke for the floating image sample before cutting `1.4.0`.

## [1.3.0] - 2026-04-07

### Added

- `Run::font_family()`, `east_asia_font_family()`, `set_font_family()`,
   `set_east_asia_font_family()`, and `clear_font_family()` for explicit
   `w:rFonts` editing with `eastAsia` coverage for Chinese/CJK text.
- `default_run_font_family()`, `default_run_east_asia_font_family()`,
  `set_default_run_font_family()`, `set_default_run_east_asia_font_family()`,
  `clear_default_run_font_family()`, and matching `style_run_*` APIs for
  editing `word/styles.xml` `docDefaults` plus style-level `w:rFonts`
  inheritance for Chinese/CJK text.
- `Run::language()`, `east_asia_language()`, `set_language()`,
  `set_east_asia_language()`, `clear_language()`, plus matching
  `default_run_*` and `style_run_*` language APIs for editing `w:lang`
  inheritance in `document.xml` and `styles.xml`.
- `scripts/run_word_visual_smoke.ps1` now validates generated smoke DOCX
  archives before opening them in Word and cross-checks PDF page count against
  `summary.json` plus rendered PNG output so stale build artifacts fail fast.
- Matching `bidi_language()` / `set_bidi_language()` plus `default_run_*` and
  `style_run_*` bidi language APIs for editing `w:lang/@w:bidi` alongside
  existing `val` and `eastAsia` language inheritance.

### Validation

- Extended `samples/visual_smoke_tables.cpp` with explicit Chinese/CJK review
  text rendered through `docDefaults` and `Strong` style inheritance
  (`Microsoft YaHei` for `eastAsia`, `zh-CN` / `ar-SA` for `w:lang`) so the
  Word-to-PDF-to-PNG smoke flow checks mixed-script and bidi layout as well as
  borders and table geometry.
- `prepare_word_review_task.ps1` now seeds collision-safe task packages from
  reusable review templates and emits script-oriented prompts that explicitly
  require AI reviewers to write findings back into `report/`.

## [1.2.0] - 2026-04-07

### Added

- `append_table(row_count, column_count)` for creating new body tables on
  editable documents created from either `open()` or `create_empty()`.
- `Table::append_row()` and `TableRow::append_cell()` for growing existing or
  newly created tables without dropping down to manual XML editing.
- `TableCell::set_width_twips()`, `width_twips()`, and `clear_width()` for
  explicit cell width editing.
- `TableCell::merge_right()` and `column_span()` for lightweight horizontal
  cell merging and span inspection.
- `Table::set_width_twips()`, `clear_width()`, `style_id()`,
  `set_style_id()`, `clear_style_id()`, and `set_border()` /
  `clear_border()` plus `TableCell::merge_down()` and
  `TableCell::set_border()` / `clear_border()` for table-level width/style
  references, vertical merges, and table/cell border editing.
- `Table::layout_mode()`, `set_layout_mode()`, and `clear_layout_mode()` for
  switching tables between `autofit` and fixed layout modes.
- `Table::alignment()`, `set_alignment()`, `clear_alignment()`,
  `indent_twips()`, `set_indent_twips()`, and `clear_indent()` for table
  placement editing.
- `Table::cell_margin_twips()`, `set_cell_margin_twips()`, and
  `clear_cell_margin()` for table-level default cell margin editing.
- `TableRow::height_twips()`, `height_rule()`, `set_height_twips()`, and
  `clear_height()` for explicit row height editing.
- `TableRow::cant_split()`, `set_cant_split()`, and `clear_cant_split()` for
  row pagination control.
- `TableRow::repeats_header()`, `set_repeats_header()`, and
  `clear_repeats_header()` for repeat-header row editing.
- `TableCell::fill_color()`, `set_fill_color()`, `clear_fill_color()`,
  `margin_twips()`, `set_margin_twips()`, and `clear_margin()` for basic cell
  shading and per-edge margin editing.
- `TableCell::vertical_alignment()`, `set_vertical_alignment()`, and
  `clear_vertical_alignment()` for cell vertical alignment editing.
- `samples/visual_smoke_tables.cpp` plus `scripts/run_word_visual_smoke.ps1`
  and `scripts/render_pdf_pages.py` for Word-driven visual smoke checks that
  export generated `.docx` output to PDF and page PNGs.
- `append_image(path)` and `append_image(path, width_px, height_px)` for
  appending inline body images backed by managed `word/media/*` parts.
- `set_paragraph_list(paragraph, kind, level)` and
  `clear_paragraph_list(paragraph)` for attaching and removing managed bullet
  and decimal list numbering on paragraphs.
- `set_paragraph_style(paragraph, style_id)` /
  `clear_paragraph_style(paragraph)` plus `set_run_style(run, style_id)` /
  `clear_run_style(run)` for attaching and removing paragraph/run style
  references.
- `fill_bookmarks(...)` plus `bookmark_text_binding` /
  `bookmark_fill_result` for batch bookmark text filling as a first high-level
  template API.
- `replace_bookmark_with_paragraphs(...)` for replacing standalone bookmark
  paragraphs with zero or more generated plain-text paragraphs.
- `replace_bookmark_with_table_rows(...)` for expanding a standalone bookmark
  paragraph inside a template table row into zero or more cloned rows whose
  cell bodies are rewritten from plain-text data.
- `replace_bookmark_with_table(...)` and `replace_bookmark_with_image(...)`
  for replacing standalone bookmark paragraphs with generated table/image
  blocks.
- `TemplatePart` plus `body_template()`, `header_template()`,
  `footer_template()`, `section_header_template()`, and
  `section_footer_template()` for reusing bookmark template APIs across
  existing body, header, and footer parts, including bookmark-based image
  replacement.
- `set_bookmark_block_visibility(...)` and
  `apply_bookmark_block_visibility(...)` plus
  `bookmark_block_visibility_binding` /
  `bookmark_block_visibility_result` for high-level conditional block handling
  backed by bookmark marker paragraphs across body, header, and footer
  template parts.

### Changed

- Newly created or structurally extended tables now gain minimal `w:tblPr`,
  `w:tblGrid`, and `w:tcPr` scaffolding automatically so saved `.docx` output
  remains valid when appending rows or cells.
- Table column counting now honors existing `w:gridSpan` values, which keeps
  later structural edits aligned after horizontal cell merges.
- Saving now emits newly appended image media parts together with matching
  `document.xml.rels` and `[Content_Types].xml` updates.
- Saving now emits managed `word/numbering.xml` content together with matching
  `document.xml.rels` and `[Content_Types].xml` updates when paragraph lists
  are used.
- Saving now emits managed `word/styles.xml` content together with matching
  `document.xml.rels` and `[Content_Types].xml` updates when paragraph/run
  style references are used.
- Bookmark replacement logic is now split out of `document.cpp` into a
  dedicated template-focused translation unit.
- Bookmark template operations now target any loaded XML content part, which
  lets existing header/footer parts reuse the same text, paragraph, table-row,
  table, and image bookmark replacement pipeline as `document.xml`.
- Header/footer image insertion now writes per-part `.rels` entries together
  with managed `word/media/*` parts so those images survive save/reopen
  roundtrips.

## [1.1.0] - 2026-04-06

### Added

- `create_empty()` for creating a new in-memory `.docx` document without an
  existing template archive.
- `save_as(path)` semantics that keep `Document::path()` unchanged.
- `header_count()` / `footer_count()` and indexed `header_paragraphs()` /
  `footer_paragraphs()` accessors for editing existing header/footer parts.
- `ensure_header_paragraphs()` / `ensure_footer_paragraphs()` for creating and
  attaching default header/footer parts to the body-level section properties.
- `section_count()` and section-aware `section_header_paragraphs()` /
  `section_footer_paragraphs()` accessors for resolving existing
  header/footer references per section.
- `ensure_section_header_paragraphs()` / `ensure_section_footer_paragraphs()`
  for creating missing default/first/even header/footer references on a
  specific section.
- `assign_section_header_paragraphs()` / `assign_section_footer_paragraphs()`
  for rebinding a section to an already loaded header/footer part.
- `remove_section_header_reference()` / `remove_section_footer_reference()`
  for detaching one section-specific header/footer reference kind.
- `remove_header_part()` / `remove_footer_part()` for dropping one loaded
  header/footer part and detaching every section reference that points to it.
- `copy_section_header_references()` / `copy_section_footer_references()` for
  making one section reuse another section's current header/footer reference
  layout in bulk.
- `append_section(inherit_header_footer)` for appending a new final section and
  optionally inheriting the previous section's header/footer reference layout.
- `insert_section(section_index, inherit_header_footer)` for inserting a new
  section after an existing section while optionally inheriting that section's
  header/footer reference layout.
- `remove_section(section_index)` for collapsing one section boundary while
  preserving the surrounding document content.
- `move_section(source_section_index, target_section_index)` for reordering
  whole sections while keeping their header/footer reference layouts attached.
- `featherdoc_cli` with `inspect-sections`, `insert-section`, `remove-section`,
  `move-section`, and `copy-section-layout` commands for minimal section layout
  inspection and editing from the command line.
- `replace_section_header_text()` / `replace_section_footer_text()` for
  rewriting one section-specific header/footer part as plain paragraphs,
  creating the requested reference when needed.
- `featherdoc_cli` `show-section-header`, `show-section-footer`,
  `set-section-header`, and `set-section-footer` commands for section-level
  text inspection and replacement.
- `featherdoc_cli inspect-sections --json` and `show-section-header/footer
  --json` output for machine-readable section layout and paragraph inspection.
- `--json` output for mutating `featherdoc_cli` section commands, including
  `insert-section`, `remove-section`, `move-section`,
  `copy-section-layout`, `set-section-header`, and `set-section-footer`.
- `featherdoc_cli` commands for rebinding and removing section header/footer
  references plus deleting loaded header/footer parts:
  `assign-section-header`, `assign-section-footer`,
  `remove-section-header`, `remove-section-footer`,
  `remove-header-part`, and `remove-footer-part`.
- `inspect-header-parts` / `inspect-footer-parts` commands for listing loaded
  header/footer part indexes, their relationship ids, referenced sections, and
  paragraph text in either plain text or JSON.
- Automatic `w:titlePg` / `w:evenAndOddHeaders` enablement plus
  `word/settings.xml` read/write support when first-page or even-page
  header/footer references are created.
- Automatic pruning of unreferenced header/footer parts, corresponding
  `document.xml.rels` relationships, and `[Content_Types].xml` overrides
  during `save()` / `save_as()` output.
- Removing the last first-page or even-page section reference now also clears
  stale `w:titlePg` / `w:evenAndOddHeaders` markers.
- `replace_bookmark_text(name, replacement)` for rewriting text enclosed by a
  named bookmark range.
- Structured error reporting through `last_error()` with `detail`,
  `entry_name`, and `xml_offset`.
- `NOTICE`, `LEGAL.md`, Chinese licensing guidance, project identity notes, and
  release policy documentation.

### Changed

- Project branding, exported CMake target names, installed headers, and package
  metadata were migrated from DuckX naming to FeatherDoc naming.
- `Document` now uses `std::filesystem::path`.
- `open()` and `save()` now report failures through `std::error_code`.
- Formatting flags now use `enum class`.
- Installed package metadata now includes legal and project-facing repository
  files under `share/FeatherDoc`.
- README and Sphinx docs now document the current unsupported feature surface
  more explicitly, including encrypted `.docx`, section header/footer
  part-reordering limits,
  equations, and table creation.
- Removed the leftover upstream DuckX image asset directory from the repository.

- `open()` now preserves a single custom `std::error_category` instance across
  Windows shared-library boundaries, so `document_errc` compares correctly in
  MSVC DLL builds.
- Windows shared builds now export/link correctly and copy runtime DLLs next to
  tests and samples, allowing MSVC `ctest` runs to execute without missing-DLL
  failures.
- `open()` now supports reading `.docx` files while Microsoft Word keeps the
  source file open on Windows.
- Paragraph and run traversal now stay on matching WordprocessingML siblings
  instead of drifting into unrelated nodes.
- Appending a paragraph to `w:body` now inserts it before `w:sectPr`, which
  avoids invalid document ordering and fixes `insert_paragraph_after()` save
  regressions.
- `open()` now reports password-protected or encrypted `.docx` files through an
  explicit FeatherDoc error code instead of a generic XML read failure.

### Performance

- `open()` now keeps ZIP-owned XML buffers on the FeatherDoc side before
  parsing, which avoids cross-library allocator mismatches in shared-library
  builds.
- `save()` / `save_as()` now stream `document.xml` and copy ZIP entries
  chunk-by-chunk instead of materializing whole archive entries in memory.

### Dependencies

- Updated bundled `pugixml` to `1.15`.
- Updated bundled `kuba--/zip` to `0.3.8`.
- Updated bundled `doctest` to `2.5.1`.

### Validation

- MSVC configuration, build, and test flow is documented and currently used as
  the main verification baseline.
- Added end-to-end CLI coverage for section inspection plus insert/copy/move/
  remove workflows.
