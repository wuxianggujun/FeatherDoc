# Changelog

All notable FeatherDoc changes should be recorded in this file.

This project follows a pragmatic `MAJOR.MINOR.PATCH` release model oriented
around clear API behavior, MSVC buildability, diagnostics quality, and core-path
performance.

## [Unreleased]

### Fixed

- Fixed DOCX functional smoke readiness so default runs can fall back to the
  current release asset visual gallery and release evidence ZIPs when legacy
  Word visual smoke directories are missing or stale, while still preserving
  explicit `-VisualSmokeRoots` behavior.

## [1.12.4] - 2026-06-21

### Added

- Added missing business document type entry details to the schema patch
  confidence calibration report Markdown and summary JSON so reviewers can
  inspect incomplete business-dimension metadata without reopening raw
  artifacts.
- Added missing source entry details to the schema patch confidence
  calibration report Markdown and summary JSON so reviewers can inspect
  incomplete source metadata without reopening raw artifacts.

## [1.12.3] - 2026-06-19

### Added

- Added project-template workflow dashboard evidence to release candidate
  summaries and release-facing materials so reviewers can see readiness status,
  blocker and warning counts, grouped next actions, and source report paths
  without reopening intermediate JSON files.
- Added project-template business corpus metadata to schema confidence and
  smoke-manifest flows, including registered/planned corpus profiles for
  invoice, contract, policy, report, notice, and tender template categories.
- Added a dedicated `project-report-schema-baseline-smoke` corpus entry to the
  project-template smoke manifest so report coverage is tracked separately from
  the policy baseline entry while reusing the resolved schema fixture.
- Added schema approval reviewer-action metadata across approval history,
  delivery readiness, release blocker rollups, governance handoff, release
  notes, reviewer checklists, and release asset manifests.
- Added content-control repair action classification for release-blocking,
  auto-repair candidate, and manual-confirmation-required actions, with the
  same action identifiers preserved in release materials.
- Added stricter release asset manifest and GitHub Release upload evidence
  checks for ZIP asset selection, remote asset URLs, release URLs, upload host
  consistency, release path prefixes, and integer metrics.
- Added bilingual Word workflow entrypoint documentation and a long-running
  task board/backlog for continuing release governance work on the `dev`
  branch.

### Changed

- Hardened release refresh and publish workflows so generated release artifacts
  are written under a fixed output root and stale workspace-local output is
  cleaned safely before upload.
- Tightened project-template smoke runner and template schema CLI build
  helpers so configure/build output stays quiet and build-mode CLI discovery
  resolves to an absolute binary path.
- Strengthened release material safety tests so `START_HERE.md`,
  `ARTIFACT_GUIDE.md`, `REVIEWER_CHECKLIST.md`, `release_handoff.md`, release
  body, and release summary keep the same machine-readable governance fields.

### Fixed

- Fixed release entry material safety regressions where ready or blocked
  project-template dashboard action groups could lose `blocker`, `entries`,
  source, or action markers in reviewer-facing materials.
- Fixed project-template smoke manifest validation so single-entry manifest
  properties stay enumerable and the report corpus anchor resolves to the
  dedicated `project-report-schema-baseline-smoke` entry.
- Fixed release asset manifest validation so `upload.remote_assets` cannot
  include the manifest itself, unrelated assets, malformed remote URLs,
  mismatched release tags, query or fragment polluted URLs, or non-integer size
  and download-count metrics.
- Fixed a selector-based CLI test output collision by giving the bookmark
  template-table patch case a unique JSON filename, so parallel runs no longer
  race on the same artifact.

## [1.12.2] - 2026-06-16

### Added

- Added `featherdoc_cli export-pdf --expand-header-footer-page-placeholders`
  so header and footer `{{page}}` / `{{total_pages}}` placeholders can be
  expanded during PDF export, with the effective export switches reported in
  both `--json` and `--summary-json` output.
- Added the bilingual `docs/en/api/pdf_workflow.rst` and
  `docs/zh-CN/api/pdf_workflow.rst` pages plus README links for the
  experimental PDF export CLI entry point, supported options, JSON summary
  shape, and current scope limits.
- Documented the `export-pdf --json` failure result contract, including parse,
  open, export, and summary failure stages.
- Added a stable schema marker to the raw PDF visual release-gate preflight
  summary so downstream governance and readiness evidence can identify it
  without relying on path conventions.
- Added PDF floating table regression coverage for preservation-only wrapping
  metadata so `leftFromText`, `rightFromText`, non-paragraph `topFromText`, and
  `tblOverlap` stay layout-neutral until full Word-compatible table wrapping is
  implemented.
- Added PDF floating table support-boundary evidence to the table layout
  delivery report, rollup, and governance summaries so release reviewers can
  distinguish the stable PDF geometry subset from metadata-only wrapping scope.
- Added table layout governance Markdown details for metadata-only PDF floating
  table fields and reviewer-required wrapping boundaries.
- Added machine-readable PDF floating table field lists to table layout rollup,
  governance, handoff, and release bundle materials so reviewers can see the
  concrete metadata-only and review-required boundaries without reopening the
  source layout JSON.
- Added a table layout governance `-FailOnIssue` regression and CTest
  registration so issue-triggered release-gate failures stay covered.
- Added release metadata docs checker coverage for the documentation
  maintenance overview so the release metadata pipeline and checklist remain
  linked from the maintenance entrypoint.
- Added release metadata docs checker summary-label assertions and checklist
  entrypoint regression coverage for the documentation maintenance overview.
- Documented and guarded strict release metadata gate switches for blocker,
  missing-report, and warning failures in release rollup and handoff flows.
- Added release governance handoff `-FailOnBlocker` regression coverage and
  CTest registration so blocker-triggered handoff exits stay guarded.
- Added content-control data-binding governance report CTest registration for
  aggregate, `-FailOnBlocker`, and `-FailOnWarning` scenarios.
- Added style-merge restore audit CTest registration for clean and issue
  scenarios, with the style confidence route contract guarding the entries.
- Added style-merge suggestion review and reviewed-apply CTest registration
  for success and review-gate failure scenarios.
- Added DOCX functional smoke readiness `-FailOnWarning` regression coverage
  and CTest registration for pending visual-review warnings.
- Added project-template smoke candidate discovery `-FailOnUnregistered`
  regression coverage and CTest registration.
- Added project-template smoke manifest description regression coverage and
  bounded CTest registration for adjacent onboarding tests.
- Added project-template smoke manifest checker regression coverage for
  manifest structure, duplicate entries, disabled checks, invalid slots, and
  `-CheckPaths` failures.
- Added project-template smoke manifest entry registration regression coverage
  for JSON snippet merging, duplicate rejection, `-ReplaceExisting`, and path
  normalization.
- Added project-template smoke visual verdict sync regression coverage for
  refreshed review results, undetermined visual-review state, summary markdown,
  and release-candidate summary propagation.
- Added project-template smoke visual verdict failure regression coverage so
  explicit `review_verdict=fail` updates smoke summaries, release summaries,
  and the script exit code as a blocking failure.
- Added lightweight parser coverage for release metadata, script index, Word
  visual preflight, and DOCX functional smoke readiness guard scripts.
- Added script task index regression coverage for reporting unindexed
  repository scripts without treating them as missing indexed scripts.
- Added stable public evidence-path display handling for packaged release
  manifests so project-template and manifest-signoff evidence keeps source
  identity even when packaging runs from an out-of-repository working directory.
- Documented and guarded the release metadata docs checker
  `checked_documents[]`, `checked_document_count`, `checked_document_labels`,
  `checked_document_relative_paths`, and `required_marker_count` summary
  contracts so automation can verify document coverage without inferring it
  from marker counts.
- Added release metadata docs checker guards for the manifest signoff and
  project-template readiness checklist trace markers.
- Added duplicate-reference line diagnostics to the script task index checker
  so repeated `scripts/*.ps1` or `scripts/*.py` entries point reviewers to the
  exact source lines and groups.
- Added script task index regression coverage for duplicate script references
  that span multiple documentation groups.
- Added a stable `featherdoc.project_template_smoke_onboarding_plan.v1` schema
  marker to project-template smoke onboarding plans.
- Added a stable `featherdoc.project_template_onboarding_summary.v1` schema
  marker to project-template onboarding summaries and taught governance readers
  to prefer explicit onboarding evidence schemas.
- Added release-asset packaging regression coverage for warning-only
  project-template delivery readiness contracts.
- Added release-note bundle and docs-contract regression coverage for
  warning-only project-template delivery readiness `needs_review` handoff
  semantics.
- Added packaged release manifest signoff prompts for project-template
  readiness `release_blocker_count` and `warning_count` so warning-only handoff
  evidence remains reviewer-visible before publish.
- Documented the same manifest signoff count fields in the project-template
  readiness checklist and release metadata pipeline route docs.
- Added packaged manifest signoff regression coverage for the 7 required
  project-template governance fields.

### Fixed

- Fixed `export-pdf --json` and `--summary-json` success output so the outer
  JSON result object is closed after the nested `options` object.
- Fixed PDF table layout so floating table horizontal specs and reliable
  page/margin vertical specs affect rendered table placement instead of being
  preserved only in DOCX metadata.
- Fixed PDF table layout so paragraph-anchored floating tables honor
  `topFromText` as spacing above the table.
- Fixed Debug PDF CTest runtime-path resolution so `pdf_cli_export` uses Debug
  vcpkg DLL directories instead of release-only runtime paths.
- Clarified historical PDF visual validation changelog wording so old
  reusable-build and baseline-output blocker notes do not read as current
  release status.
- Fixed project-template delivery readiness so warning-only evidence gaps report
  `needs_review` instead of `ready`, and direct onboarding summary/plan evidence
  keeps its stable source schema in propagated blockers and action items.
- Fixed release-note bundle warning-only readiness regression fixture selection
  so the coverage runs consistently under PowerShell 7 CI.
- Fixed release short summaries so project-template readiness governance lines
  preserve `release_blocker_count` and `warning_count`.

## [1.12.1] - 2026-06-03

### Added

- Added the bundled `stb_image` raster probe for PNG, JPEG, GIF, and BMP
  dimension detection, with install/include handling and license notice
  coverage.
- Added raster image regression coverage for PNG, JPEG, GIF, BMP, corrupted
  image bytes, and the package metadata boundary when raster bytes and file
  extensions do not match.

### Changed

- Clarified the CI-artifact release boundary so GitHub release publishing can
  proceed from explicit CI evidence while preserving the skipped Word visual
  gate status in release-facing notes.
- Updated the bilingual Images API reference to document that PNG/JPEG/GIF/BMP
  dimensions are parsed by `stb_image`, while SVG/WebP/TIFF keep FeatherDoc
  format-specific dimension readers.

### Fixed

- Removed FeatherDoc's hand-written PNG/JPEG/GIF/BMP signature gate from the
  image append/replace path so supported raster dimensions are resolved by
  `stb_image` instead of duplicate parser logic.

## [1.12.0] - 2026-05-28

### Added

- Added release metadata docs checks that keep the document-governance
  acceptance page wired to template readiness, content-control repair,
  numbering real-corpus alignment, and table-layout delivery quality evidence.
- Added content-control governance repair-plan `open_command` passthrough so
  reviewer-facing repair feasibility rows can link back to the report rebuild
  command when they originate from action items.
- Added a stale `codex/*` branch inventory that records which remote branches
  are not fully merged and why they should not be deleted without an explicit
  archive or discard decision.
- Added a stale `codex/*` branch inventory documentation contract so the
  reference-branch retention, preflight readiness, and deletion safeguards stay
  wired into lightweight regression checks.
- Added conservative PDF repeated-header abbreviation and same-token word-order
  matching for common cross-page table headers such as `Qty`/`Quantity`,
  `Amt`/`Amount`, `Project Status`/`Status Project`, and parenthesized unit
  labels such as `Amount USD`/`Amount (USD)`, while keeping semantic header
  changes split into separate editable tables.
- Added a PDF visual validation status documentation contract that keeps the
  memory preflight guard, current blocker counts, and "do not run the full
  visual gate yet" boundary visible in release-facing docs.
- Added PDF repeated-header continuation diagnostics that expose whether a
  matched source header used exact, normalized text, plural variant, canonical
  abbreviation, or token-set word-order matching.
- Added `featherdoc_cli set-table-width`, `clear-table-width`,
  `set-table-layout-mode`, and `clear-table-layout-mode` so body-table total
  width and layout mode can be edited without dropping to the C++ API, including
  `edit_document_from_plan.ps1` operation support for table width and layout
  mode edits.
- Added `featherdoc_cli set-table-alignment`, `clear-table-alignment`,
  `set-table-indent`, `clear-table-indent`, `set-table-cell-spacing`, and
  `clear-table-cell-spacing` plus edit-plan operations for body-table placement
  and spacing metadata.
- Added `featherdoc_cli set-table-default-cell-margin`,
  `clear-table-default-cell-margin`, `set-table-border`, and
  `clear-table-border` plus edit-plan operations for table-level default cell
  margins and `w:tblBorders` edges.
- Added `featherdoc_cli set-table-style-id` and `clear-table-style-id` plus
  edit-plan operations for assigning and removing body-table `w:tblStyle`
  references.
- Added `featherdoc_cli set-table-style-look` and `clear-table-style-look` plus
  edit-plan operations for assigning and removing body-table `w:tblLook`
  style-routing flags.
- Added `featherdoc_cli remove-table` and `remove_table` / `delete_table`
  edit-plan operations for deleting a selected body table.
- Added `featherdoc_cli insert-table-before` and `insert-table-after` plus
  edit-plan operations for creating empty sibling body tables around an
  existing table.
- Added `featherdoc_cli insert-table-like-before` and
  `insert-table-like-after` plus edit-plan operations for cloning a selected
  body table's structure and formatting while clearing copied cell text.
- Added `featherdoc_cli insert-paragraph-after-table` plus an edit-plan
  operation for inserting follow-up body paragraphs immediately after a
  selected table.
- Added `set_table_position` and `clear_table_position` edit-plan operations
  that reuse the existing floating table `w:tblpPr` CLI placement controls.
- Added edit-plan operations for body-table row edits: `append_table_row`,
  `insert_table_row_before`, `insert_table_row_after`, `remove_table_row`, and
  `delete_table_row`.
- Added edit-plan operations for template-part table row edits:
  `append_template_table_row`, `insert_template_table_row_before`,
  `insert_template_table_row_after`, `remove_template_table_row`, and
  `delete_template_table_row`.
- Added edit-plan operations for template-part table column edits:
  `insert_template_table_column_before`, `insert_template_table_column_after`,
  `remove_template_table_column`, and `delete_template_table_column`.
- Added `set_template_table_cell_text` and
  `set_template_table_cell_block_texts` edit-plan operations for template-part
  table cell text updates.
- Added `set_template_table_row_texts` and `set_template_table_rows_texts`
  edit-plan operations for contiguous template-part table row text updates.
- Added `merge_template_table_cells` / `merge_template_table_cell` and
  `unmerge_template_table_cells` / `unmerge_template_table_cell` edit-plan
  operations for template-part table cell merge and split workflows.
- Added `set_template_table_from_json` / `patch_template_table_from_json` and
  `set_template_tables_from_json` / `patch_template_tables_from_json`
  edit-plan operations for single-table and batch template-table JSON patches.
- Added content-control edit-plan operations for text, paragraph, table,
  table-row, image, form-state, and Custom XML sync workflows by tag or alias.
- Added bookmark rich-replacement edit-plan operations for full-table
  replacement, block removal, inline image replacement, floating image
  replacement, and single or batched block-visibility updates.
- Added edit-plan operations for body-table column edits:
  `insert_table_column_before`, `insert_table_column_after`,
  `remove_table_column`, and `delete_table_column`.
- Added `set_table_row_cant_split`, `clear_table_row_cant_split`,
  `set_table_row_repeat_header`, and `clear_table_row_repeat_header`
  edit-plan operations, with clear-row-height coverage for row layout metadata.
- Added `set_table_cell_margin`, `clear_table_cell_margin`,
  `set_table_cell_text_direction`, and `clear_table_cell_text_direction`
  edit-plan operations for per-cell spacing and text-flow metadata.
- Added `set_table_cell_width` and `clear_table_cell_width` edit-plan
  operations, plus regression coverage for clearing direct table-column widths.
- Expanded edit-plan regression coverage for clearing direct table-cell fill,
  border, vertical alignment, and horizontal alignment overrides.
- Added a free-memory guard to `scripts/run_reused_build_check.ps1` so reused
  local build/test passes refuse to start on memory-starved workstations unless
  the operator explicitly skips the guard.
- Added the same low-memory preflight guard to the PDF visual release gate so
  visual validation refuses to start when the workstation is already
  memory-starved.
- Added PDF visual preflight governance reporting for free-memory guard
  outcomes so release-blocker summaries explain whether resource pressure, not
  missing PDF artifacts alone, is blocking visual validation.
- Added memory-guard parameter passthrough to the PDF visual preflight
  governance report entry point so auto-generated preflight summaries use the
  same workstation resource threshold as the visual gate.
- Added PDF visual preflight memory-guard details to release blocker runbooks
  so reviewer checklists show whether low memory is an active blocker before
  preparing build outputs or running the full gate.
- Documented the PDF visual release-blocker runbook memory fields so the
  status page matches the reviewer checklist output.
- Recorded the latest lightweight PDF preflight and governance-report result
  so the status page distinguishes the current build/baseline blockers from
  the passing memory guard.
- Tightened PDF visual preflight build-dir auto-detection so generic `build`
  directories are not treated as reusable CMake builds unless they contain
  `CMakeCache.txt` or `CTestTestfile.cmake`.
- Added a plain-build regression case for PDF visual preflight so a repository
  with only `build/tmp` still reports the requested `.bpdf-roundtrip-msvc`
  directory as missing instead of silently selecting an unrelated build folder.
- Added top-level `output_gap_count` and `missing_output_count` fields to the
  PDF visual preflight summary so release governance and status docs can use
  the same missing-output totals without recomputing them.
- Added the same PDF preflight output-gap totals to release blocker runbooks
  and release metadata pipeline docs so reviewers see the grouped and total
  missing-output counts before preparing build outputs.
- Recorded the updated PDF visual preflight blocker count after stricter
  reusable-build auto-detection stopped selecting plain `build` directories.
- Documented the stricter PDF preflight reusable-build selection in the stale
  branch inventory so old `auto:build` evidence does not drive branch cleanup
  decisions.
- Documented the then-current PDF visual validation boundary, including the
  free-memory preflight check, governance passthrough, and the reusable-build
  and baseline-output blockers that were active before later full-gate closure.

### Fixed

- Fixed Windows runtime DLL copying for CLI-oriented test executables so
  vcpkg FreeType/Harfbuzz companions such as zlib, bzip2, and Brotli DLLs are
  available without an ad hoc `PATH`.

## [1.11.0] - 2026-05-12

### Added

- Added `PdfDocumentImportOptions::min_table_continuation_confidence` so
  callers can keep low-confidence cross-page PDF table continuations as
  separate editable Word tables while retaining diagnostics for the actual
  confidence and blocker.
- Added `scripts/run_reused_build_check.ps1` as a guarded local validation
  entry point that reuses an existing build directory, refuses concurrent
  FeatherDoc build/test processes, and enforces timeouts to avoid stale local
  build workers.

## [1.10.0] - 2026-05-11

### Added

- Added experimental PDF export/import plumbing behind the opt-in PDF build,
  including PDFio/PDFium integration, document-to-PDF layout adaptation,
  parser probes, and focused import failure classification.
- Added PDF text import into editable `Document` content with preserved page
  text structure, geometry-aware reading order recovery, and regression
  coverage for out-of-order PDF content streams.
- Added conservative PDF table candidate detection and opt-in table import,
  including body-order preservation for mixed paragraph/table content,
  table-first imports without leading empty paragraphs, and negative coverage
  for two-column prose, aligned lists, and invoice-like forms.
- Added PDF table import support for detected column/row spans, merged header
  cells, and repeated source headers, with editable Word table output verified
  by focused parser/import tests and visual evidence.
- Added cross-page PDF table continuation handling with diagnostics, repeated
  header skipping, compatible table merging, and conservative blockers for
  width mismatches, low page starts, intervening paragraphs, and semantic
  header changes.
- Added `build_document_skeleton_governance_report.ps1` with PowerShell
  coverage to export an exemplar numbering catalog, collect style usage, run
  style-numbering audit, and write unified JSON/Markdown governance reports.
- Added `build_document_skeleton_governance_rollup_report.ps1` with PowerShell
  coverage to aggregate multiple document skeleton governance summaries into a
  multi-template numbering catalog exemplar, style-numbering issue, release
  blocker, and action item handoff.
- Added `build_numbering_catalog_governance_report.ps1` with PowerShell
  coverage to combine document skeleton governance rollups with numbering
  catalog manifest checks into one exemplar catalog, style-numbering, drift,
  dirty-baseline, release blocker, and action item gate.
- Added schema approval state, release blockers, action items, and manual
  review recommendations to project-template onboarding summaries and
  onboarding-plan candidates.
- Added `build_project_template_onboarding_governance_report.ps1` with
  PowerShell coverage to aggregate onboarding summaries, onboarding plans, and
  project-template smoke schema approval evidence into one JSON/Markdown
  release-readiness report.
- Added `build_project_template_delivery_readiness_report.ps1` with PowerShell
  coverage to join onboarding governance evidence with schema approval history
  into a final project-template delivery readiness gate.
- Added `build_release_blocker_rollup_report.ps1` with PowerShell coverage to
  normalize release blockers and action items from multiple governance reports
  into one JSON/Markdown release handoff.
- Added optional release blocker rollup inputs to `run_release_candidate_checks.ps1`
  so existing numbering catalog, table layout, and project-template governance
  summaries can be folded into the release-candidate summary and final review.
- Added `write_schema_patch_confidence_calibration_report.ps1` with PowerShell
  coverage to aggregate schema patch review size, approval outcomes, optional
  confidence metadata, bucket summaries, and conservative threshold
  recommendations from existing smoke/history evidence.
- Added table layout delivery report scripts and PowerShell coverage to collect
  table style quality, tblLook repair planning, floating table preset planning,
  dry-run replay, and visual-regression handoff entries.
- Added `build_table_layout_delivery_rollup_report.ps1` with PowerShell
  coverage to aggregate multiple table layout delivery summaries into a
  multi-template table style quality, safe tblLook repair, floating table plan,
  release blocker, and action item handoff.
- Added `build_table_layout_delivery_governance_report.ps1` with PowerShell
  coverage to turn table layout delivery rollups into a final table style,
  safe tblLook repair, floating table review, visual-regression, blocker, and
  action item gate.
- Added release rollup-compatible `action_items` and `next_steps` mirrors to
  table layout delivery governance summaries so final release blocker handoff
  reports retain table style and floating table follow-up actions.
- Added cross-platform CTest registration for the release-candidate blocker
  rollup smoke when `pwsh` is available outside Windows.
- Added release-candidate blocker rollup auto-discovery for the default
  numbering catalog, table layout, content-control data-binding, and
  project-template delivery governance
  reports.
- Added a release governance handoff report that summarizes the default
  numbering catalog, table layout, content-control data-binding, and
  project-template delivery readiness
  reports before final release blocker rollup.
- Added optional nested release blocker rollup output to the release governance
  handoff report for one-command reviewer handoff plus final blocker summary.
- Added release-candidate governance handoff integration so
  `run_release_candidate_checks.ps1` can archive the handoff, optional nested
  blocker/action rollup, and mirrored counts in the release summary/final
  review.
- Added `build_release_governance_pipeline_report.ps1` with PowerShell
  coverage to compose the four final governance reports, release handoff, and
  final blocker/action rollup from existing summary artifacts.
- Added `build_content_control_data_binding_governance_report.ps1` with
  PowerShell coverage to aggregate content-control inspection and Custom XML
  sync evidence into release blockers, action items, and a pipeline-ready
  JSON/Markdown data-binding governance report.
- Added content-control data-binding governance to the release governance
  pipeline so Custom XML sync issues, bound placeholders, lock review, and
  duplicate binding review can reach the final release handoff.
- Added content-control data-binding governance to the default release
  governance handoff and release-candidate auto-discovery so the reviewer-facing
  summary now includes the fourth governance line automatically.
- Added cross-platform CTest registration for the release governance handoff
  smoke when `pwsh` is available outside Windows.
- Added a standalone CLI JSON helper module with focused CTest coverage so
  common CLI output escaping can be validated without running the full CLI
  regression suite.
- Added a standalone CLI parse helper module with focused CTest coverage for
  shared flag, integer, and boolean option parsing.
- Added a standalone CLI usage module with focused CTest coverage for the
  command help text emitted by parse errors and empty CLI invocations.
- Split the CLI styles regression suite into its own `cli_styles` CTest target
  so style-focused CLI coverage can run independently from the full CLI suite.
- Added CTest labels for split CLI/core smoke and heavy suites, and made the
  Linux/macOS workflows run the dedicated `cli_smoke` label after the full test
  pass.
- Added Linux/macOS workflow steps for the dedicated `release_smoke` label so
  release governance handoff and blocker rollup smoke coverage runs in the
  minimal cross-platform CI.
- Added Linux/macOS release governance smoke artifact uploads for blocker
  rollup, governance handoff, and governance pipeline evidence.
- Added a minimal Linux CMake CI workflow with GCC/Clang matrix coverage for
  the core and CLI test build.
- Added a minimal macOS CMake CI workflow and README badges so the project has
  visible Windows, Linux, and macOS build/test coverage.
- Added `run_word_visual_smoke.ps1 -ShowRevisions` and review mutation visual runner passthrough so Word PDF/PNG evidence can render tracked insertion and deletion markup when validating unresolved revisions.
- Added typed insertion/deletion revision authoring APIs and matching CLI commands so review workflows can generate Word revision markup before accepting or rejecting it.
- Added in-place run revision authoring APIs and CLI commands for inserting, deleting, and replacing body runs with Word revision markup.
- Added paragraph text range revision authoring APIs and CLI commands for inserting, deleting, and replacing body paragraph text across run boundaries.
- Added cross-paragraph text range revision authoring APIs and CLI commands for inserting at a body text position and deleting or replacing half-open ranges across body paragraphs.
- Added paragraph text range revision evidence to the review mutation visual regression sample and runner.
- Added cross-paragraph range revision evidence to the review mutation visual regression sample and runner.
- Added in-place paragraph and cross-paragraph text range comment authoring APIs and CLI commands.
- Added comment range retargeting APIs and CLI commands so existing comments can move to paragraph or cross-paragraph text ranges.
- Added comment resolved-state authoring through `commentsExtended.xml`, with API, CLI, inspection, and visual-regression coverage.
- Added comment reply threading through `commentsExtended.xml`, including API, CLI, inspection metadata, recursive reply cleanup, and visual-regression evidence.
- Added comment creation date authoring for comments, text-range comments, and threaded replies through API and CLI.
- Added comment metadata update authoring so existing comments and replies can set or clear author, initials, and date fields.
- Added revision metadata update authoring so existing revisions can set or clear author and date fields.
- Added text range preview diagnostics so API and CLI callers can verify selected body text ranges before writing comments or revisions.
- Added `find_text_ranges(...)` and `find-text-ranges` so automation can locate exact body text matches and reuse returned paragraph offsets for review mutation plans.
- Added `build-review-mutation-plan` so callers can resolve `find_text` requests into guarded review mutation plan JSON before previewing or applying batch revisions.
- Added `before_text` / `after_text` context filters to `build-review-mutation-plan` requests so repeated body text can be resolved by surrounding text instead of brittle occurrence-only selection.
- Added `require_unique` to `build-review-mutation-plan` requests so ambiguous filtered matches fail before a guarded review mutation plan is written.
- Added insertion revision operations to review mutation JSON plans, including `build-review-mutation-plan` insertion before or after matched body text.
- Added paragraph and cross-paragraph comment operations to review mutation JSON plans so `build-review-mutation-plan` can batch author guarded review comments from found body text.
- Added nested/overlapping comment-anchor coverage so `inspect-review` preserves full anchor text for outer comments while review mutation plans still reject overlapping text-changing operations.
- Added `set_comment_resolved` review mutation plan operations with comment index, anchor text, and resolved-state preflight guards.
- Added `append_comment_reply` review mutation plan operations with parent comment index, anchor text, and resolved-state preflight guards.
- Added `set_comment_metadata` review mutation plan operations with comment body, parent index, anchor text, and resolved-state preflight guards.
- Added `replace_comment` and `remove_comment` review mutation plan operations with comment body, parent index, anchor text, and resolved-state preflight guards.
- Added API-level `expected_text` safeguards for paragraph and cross-paragraph text range revision delete/replace authoring.
- Added `--expected-text` safeguards to paragraph and cross-paragraph text range revision CLI delete/replace commands so automation can fail before mutating when selected text has drifted.
- Extended `expected_text` mismatch diagnostics with selected range offsets and segment previews for API and CLI revision authoring.
- Expanded CLI regression coverage for paragraph `--expected-text` mismatch and parse failures.
- Added `preview-review-mutation-plan` so automation can preflight JSON batches of paragraph and cross-paragraph revision delete/replace operations before mutating DOCX files.
- Added `apply-review-mutation-plan` for guarded JSON batch revision authoring with full-plan preflight, overlap rejection, and descending-position application to reduce offset drift.
- Added in-place comment range evidence to the review mutation visual regression sample and runner.
- Added `review_note_summary::anchor_text` for comments and surfaced it in `inspect-review` text/JSON output so review automation can audit the selected text behind each comment.
- Added structured failure fields such as `failure_kind`, `failure_relative_path`, and `failure_expected_text` to release metadata docs check JSON summaries.
- Added `check_release_metadata_docs.ps1 -Quiet` so automation can rely on JSON summaries without success-banner output.
- Added `summary_json_relative_path` to release metadata docs check JSON summaries for portable automation logs.
- Added PowerShell edition and version fields to release metadata docs check JSON summaries for environment diagnostics.
- Added `checker_name` and `checked_at_utc` audit fields to release metadata docs check JSON summaries.
- Added `summary_schema_version` and document/marker count fields to release metadata docs check JSON summaries.
- Added `summary_json_path` to release metadata docs check JSON summaries so automation can record the resolved output path.
- Extended `check_release_metadata_docs.ps1 -SummaryJson` to write failed summaries with `error_message` while preserving non-zero exits.
- Added `check_release_metadata_docs.ps1 -SummaryJson` so release metadata documentation checks can emit machine-readable pass summaries.
- Added a lightweight release metadata documentation checker for pipeline, maintenance checklist, and release policy cross-reference coverage.
- Added a Chinese release metadata maintenance checklist covering field contracts, targeted tests, visual review, and commit hygiene.
- Added Chinese release metadata pipeline documentation covering visual gate, preflight, verdict sync, and release-note bundle metadata contracts.
- Added `write_release_note_bundle.ps1 -SkipMaterialSafetyAudit` for focused regression tests that verify generated bundle content without rerunning the full release material audit.
- Added release artifact guide and start-here rendering for visual gate review task counts so every internal metadata entry point shows total, standard, and curated review scope.
- Added release metadata handoff and reviewer checklist rendering for visual gate review task counts so internal sign-off bundles show total, standard, and curated review scope.
- Added release preflight propagation for visual gate review task counts so `run_release_candidate_checks.ps1` records total, standard, and curated scope in release summaries and final reviews.
- Added release summary synchronization for visual gate review task counts so `sync_visual_review_verdict.ps1` carries total, standard, and curated scope into release `summary.json` and `final_review.md`.
- Added `run_word_visual_release_gate.ps1` review task summary counts to `gate_summary.json` and `gate_final_review.md` so automation and reviewers can see total, standard, and curated review-task scope at a glance.
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

- Refactored the top-level C++ test registration in `test/CMakeLists.txt`
  through a small helper while preserving the existing `unit` and `iterator`
  test behavior.
- Split the `featherdoc_cli` process entry point into a small
  `featherdoc_cli_main.cpp` wrapper while keeping the existing command
  implementation in `featherdoc_cli.cpp`.
- Extended semantic diff review-note fingerprints so comment anchor text changes are reported alongside comment body changes.
- Added `failure_rule_id` to release metadata docs checker JSON failures for stable CI aggregation.
- Extended release metadata docs checker diagnostics so whitespace failures include `failure_excerpt`.
- Extended release metadata docs checker diagnostics so whitespace failures report `failure_column_number`.
- Extended release metadata docs checker diagnostics so `tab_character` failures report `failure_line_number`.
- Linked the release policy documentation to the Chinese release metadata pipeline guide for maintainers updating gate, preflight, sync, and bundle metadata flows.
- Hardened word visual gate review task counting so empty standard or curated task placeholders are ignored in generated review scope totals.
- Hardened release preflight summary generation so incomplete visual review task counts are skipped before they enter `summary.json`.
- Hardened visual verdict sync so incomplete review task counts are dropped from release summaries and omitted from refreshed `final_review.md` output.
- Hardened release preflight review task-count rendering so incomplete visual gate metadata is omitted instead of producing blank counts in `final_review.md`.
- Hardened release metadata review task count rendering so incomplete release-summary metadata falls back to the gate summary instead of emitting blank counts.
- Mirrored `sync_latest_visual_review_verdict.ps1` release summary discovery metadata into selected release summaries and surfaced it in release `final_review.md` for artifact-only review handoffs.
- Added a release summary discovery section to `gate_final_review.md` so reviewers can see whether `sync_latest_visual_review_verdict.ps1` used an explicit summary, auto-detected one, or ran gate-only.
- Added `selected_release_summary_path` and `release_summary_discovery` metadata to `sync_latest_visual_review_verdict.ps1` gate summaries so automation can tell whether release artifacts were refreshed.
- Added deterministic release-summary discovery ordering so `sync_latest_visual_review_verdict.ps1` breaks equal timestamp candidates by path and keeps repeated syncs stable.
- Validated explicit `sync_latest_visual_review_verdict.ps1 -ReleaseCandidateSummaryJson` JSON before mutating gate summaries, preventing half-written sign-off state when the release summary is unreadable.
- Refactored release visual verdict metadata collection into a shared helper so release handoff, artifact guide, start-here, reviewer checklist, and release-note writers resolve standard and curated visual verdicts consistently.
- Updated release metadata bundle writers so curated visual regression entries use the current `review_verdict` field without accepting the removed `verdict` shape.
- Removed old README visual asset refresh source-bundle fallbacks so refreshed gallery images must come from the reviewed task-local evidence.
- Removed the visual regression bundle `contact_sheet.png` aggregate contact-sheet fallback; curated bundle review tasks now require `aggregate-evidence/before_after_contact_sheet.png`.
- Updated template schema patch generation to preserve slot source selectors in generated `remove_slots` and `rename_slots` entries, keeping content-control and bookmark-oriented schemas round-trippable.

- Updated README, Chinese README, Sphinx docs, current-direction notes, and
  feature-gap analysis to describe numbering catalog governance, content
  control inspection and plain-text replacement, style refactor confidence
  filtering, source/target
  suggestion filtering, and the remaining real-corpus confidence calibration
  work.

### Fixed

- Fixed MSVC unit-test compilation for existing UTF-8 literals by enabling
  `/utf-8` on `unit_tests` and using explicit UTF-8 byte literals for checkbox
  content-control fallback text.
- Fixed the template-table selector visual regression aggregate contact sheet
  filename so release-gate review task packaging finds
  `aggregate-evidence/before_after_contact_sheet.png`.
- Fixed CLI `--text-file` replacements to strip a leading UTF-8 BOM before
  writing section header/footer text, with CLI and section text multiline
  visual regression coverage.
- Fixed release preflight MSVC command launches to use UTF-8 code pages before
  running CTest so PowerShell render-plan tests can parse CLI JSON containing
  CJK text consistently.

### Tests

- Added PowerShell regression coverage for document skeleton governance,
  table layout delivery reports, and schema-approval onboarding summaries.
- Added revision authoring coverage for C++ APIs, CLI commands, in-place run revision flows, paragraph text range flows, and review mutation visual regression evidence.
- Added review comment anchor coverage for C++ APIs, CLI inspection/mutation flows, semantic diff, and review inspection/mutation visual regressions.
- Extended release metadata docs checker regressions to assert stable `failure_rule_id` values for structured failures.
- Extended release metadata docs checker regressions to assert failure excerpts for tab and trailing-whitespace failures.
- Extended release metadata docs checker regressions to assert column numbers for tab and trailing-whitespace failures.
- Extended release metadata docs checker regressions to assert line numbers for `tab_character` failures.
- Extended release metadata docs checker regressions to cover `missing_file`, `trailing_whitespace`, and `tab_character` structured failure kinds.
- Extended release metadata docs checker regressions to assert structured failure metadata for missing markers and UTF-8 BOM rejection.
- Extended release metadata docs checker regressions to verify quiet mode still writes JSON while suppressing the success marker.
- Extended release metadata docs checker regressions to assert portable summary JSON relative paths.
- Extended release metadata docs checker regressions to assert PowerShell environment fields in JSON summaries.
- Extended release metadata docs checker regressions to assert checker identity and UTC timestamp summary fields.
- Extended release metadata docs checker regressions to assert JSON schema version and count fields for passing and failing summaries.
- Extended release metadata docs checker regressions to assert `summary_json_path` for passing and failing summaries.
- Extended release metadata docs checker regressions to cover failed JSON summaries for missing markers and UTF-8 BOM rejection.
- Extended release metadata docs checker coverage to assert JSON summary output and UTF-8 without BOM encoding.
- Added focused regression coverage for the release metadata docs checker, including missing checklist markers and UTF-8 BOM rejection.
- Documented the release metadata docs checker in the maintenance checklist so doc-only changes have a focused validation path.
- Updated release note bundle visual verdict metadata coverage to skip the material safety audit while preserving generated artifact assertions.
- Optimized release note bundle visual verdict metadata assertions with cached file reads to reduce Windows CTest timeout risk.
- Added README visual asset refresh coverage to reject missing task-local evidence even when source manifests point to older evidence bundles.
- Added visual regression bundle review-task coverage to require `before_after_contact_sheet.png` and reject old aggregate `contact_sheet.png`-only bundles.
- Added word visual gate review task-count helper coverage for dictionary and object-shaped task metadata with empty placeholders.
- Added release preflight helper coverage for complete versus incomplete visual review task-count metadata.
- Extended visual verdict sync coverage to verify incomplete review task counts do not survive release-summary refreshes.
- Added release preflight review task-count coverage so incomplete visual gate summaries do not render empty count lines in `final_review.md`.
- Added missing visual review task-count coverage so incomplete release metadata without `review_task_summary` stays clean instead of rendering empty count lines.
- Added release `final_review.md` assertions for explicit, auto-detected, and `-SkipReleaseBundle` release summary discovery metadata.
- Added gate final-review assertions for release summary discovery metadata, including explicit, auto-not-found, and `-SkipReleaseBundle` paths.
- Extended `sync_latest_visual_review_verdict_curated_visual_bundle_test.ps1` to verify `-SkipReleaseBundle` still syncs release summaries while leaving release-note bundle files untouched and marking `release_bundle_refresh_requested=false`.
- Added release-summary discovery metadata assertions for explicit, auto-detected, and gate-only `sync_latest_visual_review_verdict.ps1` runs.
- Added equal-timestamp release-summary discovery coverage for the path tie-breaker used by `sync_latest_visual_review_verdict.ps1`.
- Added latest-match release-summary discovery coverage so `sync_latest_visual_review_verdict.ps1` refreshes the newest valid matching summary when multiple candidates point at the same gate.
- Added malformed-only release-summary discovery coverage so `sync_latest_visual_review_verdict.ps1` continues gate-only synchronization without writing release artifacts when no valid matching summary exists.
- Added auto-discovery coverage so `sync_latest_visual_review_verdict.ps1` skips malformed release-summary candidates and still refreshes the matching valid release bundle.
- Added missing explicit release-summary coverage so `sync_latest_visual_review_verdict.ps1` reports absent `-ReleaseCandidateSummaryJson` paths before gate summaries or release artifacts are written.
- Added malformed explicit release-summary coverage so `sync_latest_visual_review_verdict.ps1` reports unreadable `-ReleaseCandidateSummaryJson` files before gate summaries or release artifacts are written.
- Added malformed latest-task pointer coverage so `sync_latest_visual_review_verdict.ps1` reports the unreadable JSON path before gate inference or audit artifacts are written.
- Added `sync_latest_visual_review_verdict_unsupported_source_kind_test.ps1` to verify unsupported latest-task `source.kind` values fail clearly before gate inference or audit artifacts are written.
- Added `sync_latest_visual_review_verdict_missing_source_path_test.ps1` to verify latest-task pointers without `source.path` fail clearly before gate inference or audit artifacts are written.
- Stabilized `sync_latest_visual_review_verdict*` regressions with isolated short `output/` run directories so repeated CTest runs stay within the 60-second test budget.
- Added `sync_latest_visual_review_verdict_conflicting_gates_test.ps1` to ensure mixed latest-task pointers from different visual gates fail before mutating summaries or audit artifacts.
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


## Historical archives

- [1.9.0](docs/changelog_archive_1.9.0.md)
- [1.8.0 and earlier](docs/changelog_archive_1.8.0_and_earlier.md)
