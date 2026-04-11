# Release Artifact Template

Use this note when you want one release-facing summary that ties together the
installed package entry points, the local preflight result, and the visual
evidence reproduction commands.

If you already ran `run_release_candidate_checks.ps1`, you can sync the latest
screenshot-backed document/fixed-grid task verdicts back into
`gate_summary.json`, `summary.json`, `release_handoff.md`,
`release_body.zh-CN.md`, and `release_summary.zh-CN.md` in one shot with:

```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

This is the fastest way to refresh the release-facing drafts after a later
screenshot-backed visual verdict update. When you need to override the inferred
gate or release paths, fall back to `sync_visual_review_verdict.ps1`.

## Installed Entry Points

- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.md`
- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`
- `share/FeatherDoc/VISUAL_VALIDATION.md`
- `share/FeatherDoc/VISUAL_VALIDATION.zh-CN.md`
- `share/FeatherDoc/visual-validation/*.png`

## Copy-Paste Release Summary

````md
- Use 5 to 8 bullets that cover the visual-validation entry points, installed
  package entry points, CI metadata artifact, fixed-grid / merge / unmerge /
  column-editing capability, the fixed-grid quartet contact sheet, and the
  current validation result.
- If `output/release-candidate-checks/report/release_summary.zh-CN.md` already
  exists, start by trimming that draft.
````

## Copy-Paste Release Body

````md
# FeatherDoc v<version>

## Highlights
- Summarize the main API, behavior, or packaging changes from `CHANGELOG.md`.

## Validation
- MSVC configure/build: <completed|skipped|failed>
- ctest: <completed|skipped|failed>
- install + find_package smoke: <completed|skipped|failed>
- Word visual release gate: <completed|skipped|failed>
- Visual verdict: <pass|fail|pending_manual_review>

## Installed Package Entry Points
- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`
- `share/FeatherDoc/visual-validation/`

## Reproduce The Screenshots
```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

## Evidence Files
- `output/release-candidate-checks/START_HERE.md`
- `RELEASE_METADATA_START_HERE.md`
- `output/release-candidate-checks/report/ARTIFACT_GUIDE.md`
- `output/release-candidate-checks/report/REVIEWER_CHECKLIST.md`
- `output/release-candidate-checks/report/summary.json`
- `output/release-candidate-checks/report/final_review.md`
- `output/release-candidate-checks/report/release_handoff.md`
- `output/release-candidate-checks/report/release_body.zh-CN.md`
- `output/release-candidate-checks/report/release_summary.zh-CN.md`
- `output/word-visual-release-gate/report/gate_summary.json`
- `output/word-visual-release-gate/report/gate_final_review.md`
- `output/fixed-grid-merge-unmerge-regression/aggregate-evidence/contact_sheet.png`
````

## Notes

- If the visual verdict is still `pending_manual_review`, finish the screenshot
  review first and rerun `sync_latest_visual_review_verdict.ps1`.
- If you are opening the local summary output for the first time, start with
  `START_HERE.md`.
- If you are opening the CI metadata artifact for the first time, start with
  `RELEASE_METADATA_START_HERE.md`.
- After either start-here note, continue into
  `ARTIFACT_GUIDE.md`.
- If you are performing the release review itself, walk through
  `REVIEWER_CHECKLIST.md` line by line.
- Prefer `VISUAL_VALIDATION_QUICKSTART*.md` for the shortest screenshot ->
  command -> review-task path, and `VISUAL_VALIDATION*.md` for the longer
  mapping and expected output locations.
