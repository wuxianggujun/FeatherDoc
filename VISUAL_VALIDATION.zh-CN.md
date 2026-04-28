# 可视化验证复现指南

这份说明把仓库 `docs/assets/readme/` 和安装产物
`share/FeatherDoc/visual-validation/` 里的预览图，映射回生成它们的仓库脚本。

如果你现在只想从安装包里的预览 PNG 直接跳到命令入口，建议先看
`VISUAL_VALIDATION_QUICKSTART.md` 或
`VISUAL_VALIDATION_QUICKSTART.zh-CN.md`。

安装产物会镜像保存这些 PNG 和本文，方便 release 评审直接查看；但如果要真正
复现截图，仍然需要源码仓库、本地 Windows 环境，以及真实的 `Microsoft Word`
安装。

## 预览图对应关系

- `visual-smoke-contact-sheet.png`
- `visual-smoke-page-05.png`
- `visual-smoke-page-06.png`

这 3 张图来自 Word smoke 流程：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1
```

预期输出根目录：`<repo-root>\output\word-visual-smoke\`

关键证据路径：

- `evidence\contact_sheet.png`
- `evidence\pages\*.png`
- `report\summary.json`
- `report\review_checklist.md`
- `report\review_result.json`
- `report\final_review.md`

如果同一轮已经完成截图审查，可以追加 `-ReviewVerdict pass`（或 `fail` /
`undetermined`）和 `-ReviewNote`，让报告直接写入机器可读的
`status=reviewed`、`verdict` 与 `reviewed_at`。

- `fixed-grid-aggregate-contact-sheet.png`

这张图来自 fixed-grid merge/unmerge 四联回归流程：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1
```

预期输出根目录：`<repo-root>\output\fixed-grid-merge-unmerge-regression\`

关键证据路径：

- `aggregate_contact_sheet.png`
- `bundle_review_manifest.json`
- `review_checklist.md`
- `final_review.md`

## 从截图走到 Review Task

如果想把 fixed-grid 截图证据和 review task 一次性产出，可以直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

如果想把 smoke 截图、fixed-grid 四联回归，以及两条 review task 打包成一条
完整本地链路，可以运行：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1

# 同一条 gate，把本轮 smoke、fixed-grid、section/page 与 curated 结论写进报告。
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

# 同一条 gate，但顺手刷新 docs/assets/readme/ 里的展示图。
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 `
    -RefreshReadmeAssets

# 完整 release-preflight 也接受同一组 visual verdict 透传参数。
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\run_release_candidate_checks.ps1 `
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
```

这个总控脚本会重新生成 smoke 截图、执行 fixed-grid quartet，并同时准备
document task 与 fixed-grid task。

如果要查看最新生成的任务入口，可以运行：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt
```

当这些 task 的截图级 verdict 已经写回各自 `report/` 后，包括 release gate
产生的 curated visual-regression bundle task，优先用下面这条最短命令自动
识别最新 task / gate / release 路径，并把结果正式上卷回 gate summary：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

如果你跑的是完整 release-preflight，并且还想把 verdict 同步回
`output/release-candidate-checks/report/summary.json`，同时重刷
`START_HERE.md`、`report/final_review.md`、`release_handoff.md`、
`release_body.zh-CN.md` 和 `release_summary.zh-CN.md`，这条默认命令也会一并刷新检测到的 release bundle。
刷新后的入口页除了总 `visual verdict` 外，还会同步显示
`section page setup`、`page number fields` 和每个 curated visual
regression bundle 的 verdict。
只有在你需要手动覆盖推断路径时，才继续执行下面这条显式命令：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json `
    -ReleaseCandidateSummaryJson <repo-root>\output\release-candidate-checks\report\summary.json `
    -RefreshReleaseBundle
```

更完整的执行流、任务打包方式和修复闭环，请继续查看 `README.md`、
`README.zh-CN.md` 与 `docs/automation/word_visual_workflow_zh.rst`。

## 刷新仓库里的预览图

如果一次新的可视化验证已经跑完，并且想把仓库 `docs/assets/readme/`
里的预览 PNG 更新成最新结果，可以执行：

```powershell
powershell -ExecutionPolicy Bypass -File <repo-root>\scripts\refresh_readme_visual_assets.ps1
```

这个脚本默认会从 `output/word-visual-smoke/tasks/` 解析最新的 document task
和 fixed-grid task；如果你用了别的任务根目录，记得显式传入
`-TaskOutputRoot <path>`。
