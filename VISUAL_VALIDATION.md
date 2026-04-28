# Visual Validation Reproduction Guide

This note maps the preview images shipped in `docs/assets/readme/` and
`share/FeatherDoc/visual-validation/` back to the repository-side scripts that
generated them.

If you only need the shortest installed-package jump from preview PNGs to
commands, start with `VISUAL_VALIDATION_QUICKSTART.md` or
`VISUAL_VALIDATION_QUICKSTART.zh-CN.md` first.

The installed package mirrors the preview PNGs and this guide for release
review, but reproducing the screenshots still requires a source checkout, a
local Windows environment, and a real `Microsoft Word` install.

## Preview Mapping

- `visual-smoke-contact-sheet.png`
- `visual-smoke-page-05.png`
- `visual-smoke-page-06.png`

These come from the Word smoke flow:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1
```

Expected output root: `<repo-root>\output\word-visual-smoke\`

Key evidence paths:

- `evidence\contact_sheet.png`
- `evidence\pages\*.png`
- `report\summary.json`
- `report\review_checklist.md`
- `report\review_result.json`
- `report\final_review.md`

When you finish the screenshot review in the same run, add
`-ReviewVerdict pass` (or `fail` / `undetermined`) and `-ReviewNote` to stamp
machine-readable `status=reviewed`, `verdict`, and `reviewed_at` metadata.

- `fixed-grid-aggregate-contact-sheet.png`

This comes from the fixed-grid merge/unmerge quartet flow:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1
```

Expected output root: `<repo-root>\output\fixed-grid-merge-unmerge-regression\`

Key evidence paths:

- `aggregate_contact_sheet.png`
- `bundle_review_manifest.json`
- `review_checklist.md`
- `final_review.md`

## Screenshot-to-Task Handoff

For a fixed-grid bundle plus a ready-to-review task package in one run:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

For the full local release-preflight visual chain:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1

# Same gate, but stamp same-run smoke, fixed-grid, section/page, and curated verdicts.
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 `
    -SmokeReviewVerdict pass `
    -SmokeReviewNote "Smoke contact sheet reviewed." `
    -FixedGridReviewVerdict pass `
    -FixedGridReviewNote "Fixed-grid contact sheet reviewed." `
    -SectionPageSetupReviewVerdict pass `
    -SectionPageSetupReviewNote "Section page setup contact sheet reviewed." `
    -PageNumberFieldsReviewVerdict pass `
    -PageNumberFieldsReviewNote "Page number fields contact sheet reviewed." `
    -CuratedVisualReviewVerdict pass `
    -CuratedVisualReviewNote "Curated visual bundles reviewed."

# Same gate, but also refresh docs/assets/readme/ in one shot.
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 `
    -RefreshReadmeAssets
```

That wrapper regenerates the smoke screenshots, runs the fixed-grid quartet,
and prepares both document and fixed-grid review tasks.

To inspect the newest generated tasks:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt
```

After the screenshot-backed verdicts are written into those task reports,
including any curated visual-regression bundle tasks emitted by the release
gate, promote them back into the gate summary with the shortest auto-detected
path:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

If you ran the full release-preflight and also want to update
`output/release-candidate-checks/report/summary.json` plus refresh
`START_HERE.md`, `release_handoff.md`, `release_body.zh-CN.md`, and
`release_summary.zh-CN.md`, the same command will refresh the detected
release bundle automatically. Those refreshed entry points now surface not only
the top-level `visual verdict`, but also the `section page setup` verdict, the
`page number fields` verdict, and each curated visual-regression bundle
verdict. Only fall back to the explicit command below when you need to
override the inferred paths:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json `
    -ReleaseCandidateSummaryJson <repo-root>\output\release-candidate-checks\report\summary.json `
    -RefreshReleaseBundle
```

For the longer workflow and repair loop, see `README.md`,
`README.zh-CN.md`, and `docs/automation/word_visual_workflow_zh.rst`.

## Refreshing The Repository Preview Gallery

After a new visual run, refresh the repository-side preview PNGs under
`docs/assets/readme/` with:

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\refresh_readme_visual_assets.ps1
```

By default the script resolves the newest document task and fixed-grid task
from `output/word-visual-smoke/tasks/`. When you rendered into a different
task root, pass `-TaskOutputRoot <path>` explicitly.
