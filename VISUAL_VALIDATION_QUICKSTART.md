# Visual Validation Quickstart

Start here when you are browsing an installed `share/FeatherDoc` tree and want
the shortest path from the shipped preview PNGs to the local reproduction
commands.

## What Is Included Next To This File

- `visual-validation/visual-smoke-contact-sheet.png`
- `visual-validation/reopened-fixed-layout-column-widths-page-01.png`
- `visual-validation/fixed-grid-merge-right-page-01.png`
- `visual-validation/fixed-grid-merge-down-page-01.png`
- `visual-validation/visual-smoke-page-05.png`
- `visual-validation/visual-smoke-page-06.png`
- `visual-validation/fixed-grid-aggregate-contact-sheet.png`
- `visual-validation/sample-chinese-template-page-01.png`
- `VISUAL_VALIDATION.md`
- `VISUAL_VALIDATION.zh-CN.md`

The PNGs are release-facing evidence only. Reproducing them still requires a
source checkout, Windows, and a real `Microsoft Word` install.

## Fastest Reproduction Commands

Replace `<repo-root>` with your local FeatherDoc checkout.

```powershell
# Visual-only gate: regenerate the smoke gallery, fixed-grid quartet, and both review tasks.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1

# Same gate, but stamp same-run smoke, fixed-grid, and curated screenshot verdicts.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 `
    -SmokeReviewVerdict pass `
    -SmokeReviewNote "Smoke contact sheet reviewed." `
    -FixedGridReviewVerdict pass `
    -FixedGridReviewNote "Fixed-grid contact sheet reviewed." `
    -CuratedVisualReviewVerdict pass `
    -CuratedVisualReviewNote "Curated visual bundles reviewed."

# Same gate, but also refresh the repository README / docs preview PNGs.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 -RefreshReadmeAssets

# Full release-preflight: MSVC build + tests + install smoke + visual gate.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_release_candidate_checks.ps1

# Full release-preflight, plus README / docs gallery refresh.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_release_candidate_checks.ps1 -RefreshReadmeAssets

# Open the newest review tasks after either command finishes.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt

# Shortest sign-off step after both task reports are updated.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1

# After writing screenshot-backed verdicts into the task reports, sync them back into the gate summary.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json

# Same sync step, but also promote the verdict into release-preflight summary.json
# and refresh START_HERE.md plus the release-facing notes in one shot.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json `
    -ReleaseCandidateSummaryJson <repo-root>\output\release-candidate-checks\report\summary.json `
    -RefreshReleaseBundle

# Refresh the repository-side README / docs preview PNGs from the newest tasks.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\refresh_readme_visual_assets.ps1
```

If you ran the full `release-preflight`, the same report directory also gets
`release_handoff.md`, `release_body.zh-CN.md`, and `release_summary.zh-CN.md`.
The short helper above auto-resolves the newest task pointers and matching
release summary. If you need to override paths manually, keep using the longer
commands below. If `summary.json` already carries the final verdict and you
only need to regenerate those release notes after later editorial edits, the narrower
fallback is:

```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\write_release_note_bundle.ps1 `
    -SummaryJson <repo-root>\output\release-candidate-checks\report\summary.json
```

## Narrower Entry Points

```powershell
# Smoke gallery only.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1

# Standalone pages that feed the public README / docs gallery.
<build-dir>\featherdoc_sample_merge_right_fixed_grid.exe `
    <repo-root>\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1 `
    -InputDocx <repo-root>\output\sample-merge-right-fixed-grid\merge_right_fixed_grid.docx `
    -OutputDir <repo-root>\output\word-visual-sample-merge-right-fixed-grid

<build-dir>\featherdoc_sample_merge_down_fixed_grid.exe `
    <repo-root>\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1 `
    -InputDocx <repo-root>\output\sample-merge-down-fixed-grid\merge_down_fixed_grid.docx `
    -OutputDir <repo-root>\output\word-visual-sample-merge-down-fixed-grid

<build-dir>\featherdoc_sample_chinese_template.exe `
    <repo-root>\samples\chinese_invoice_template.docx `
    <repo-root>\output\sample-chinese-template\sample_chinese_invoice_output.docx
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1 `
    -InputDocx <repo-root>\output\sample-chinese-template\sample_chinese_invoice_output.docx `
    -OutputDir <repo-root>\output\word-visual-sample-chinese-template

# Fixed-grid quartet only, plus a ready-to-review task package.
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

## Where To Look Next

- `VISUAL_VALIDATION.md`: screenshot-to-script mapping plus expected output paths.
- `README.md`: install layout, `find_package` smoke, and CLI overview.
- `README.zh-CN.md`: same entry in Simplified Chinese.
