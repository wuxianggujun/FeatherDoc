# 发布产物说明模板

当你需要把安装包入口、本地 preflight 结果，以及可视化证据复现命令整理成一份
对外发布说明时，就从这份模板开始。

如果已经跑过 `run_release_candidate_checks.ps1`，可以直接用下面的命令把最新
document task / fixed-grid task 的截图结论回灌到 `gate_summary.json`、
`summary.json`、`release_handoff.md`、`release_body.zh-CN.md` 和
`release_summary.zh-CN.md`：

```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

这条命令适合在人工补完 visual verdict 之后再次刷新，避免重跑整条
preflight。只有在你需要手动覆盖推断出来的 gate / release 路径时，
才回退到 `sync_visual_review_verdict.ps1`。

## 安装包里的入口

- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.md`
- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`
- `share/FeatherDoc/VISUAL_VALIDATION.md`
- `share/FeatherDoc/VISUAL_VALIDATION.zh-CN.md`
- `share/FeatherDoc/visual-validation/visual-smoke-contact-sheet.png`
- `share/FeatherDoc/visual-validation/reopened-fixed-layout-column-widths-page-01.png`
- `share/FeatherDoc/visual-validation/fixed-grid-merge-right-page-01.png`
- `share/FeatherDoc/visual-validation/fixed-grid-merge-down-page-01.png`
- `share/FeatherDoc/visual-validation/fixed-grid-aggregate-contact-sheet.png`
- `share/FeatherDoc/visual-validation/sample-chinese-template-page-01.png`

## 可直接复制的首屏短摘要

````md
- 用 5 到 8 条 bullet 概括：可视化验证入口、安装包入口、CI metadata、
  fixed-grid / merge / unmerge / 列编辑能力、单功能 `merge_right()` /
  `merge_down()` 宽度闭环页、fixed-grid 四件套联系图、中文模板预览页，
  以及当前验证结论。
- 如果已经有 `output/release-candidate-checks/report/release_summary.zh-CN.md`，
  优先从它开始微调。
````

## 可直接复制的发布说明骨架

````md
# FeatherDoc v<版本号>

## 核心变化
- 根据 `CHANGELOG.md` 总结本次 API、行为边界、文档展示面或打包层面的主要变化。

## 验证结果
- MSVC configure/build：<completed|skipped|failed>
- ctest：<completed|skipped|failed>
- install + find_package smoke：<completed|skipped|failed>
- Word visual release gate：<completed|skipped|failed>
- Visual verdict：<pass|fail|pending_manual_review>

## 安装包内的复现入口
- `share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md`
- `share/FeatherDoc/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md`
- `share/FeatherDoc/visual-validation/`
- `share/FeatherDoc/visual-validation/fixed-grid-merge-right-page-01.png`
- `share/FeatherDoc/visual-validation/fixed-grid-merge-down-page-01.png`
- `share/FeatherDoc/visual-validation/fixed-grid-aggregate-contact-sheet.png`
- `share/FeatherDoc/visual-validation/sample-chinese-template-page-01.png`

## 如何复现截图
```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1
```

## 证据文件
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
- `output/word-visual-sample-merge-right-fixed-grid/evidence/pages/page-01.png`
- `output/word-visual-sample-merge-down-fixed-grid/evidence/pages/page-01.png`
- `output/word-visual-sample-chinese-template/evidence/pages/page-01.png`
````

## 备注

- 如果 `Visual verdict` 仍是 `pending_manual_review`，先完成截图级人工复核，
  再重跑 `sync_latest_visual_review_verdict.ps1`。
- 如果你是第一次打开本地 summary 输出，先看 `START_HERE.md`。
- 如果你是第一次打开 CI metadata artifact，先看
  `RELEASE_METADATA_START_HERE.md`。
- 看完 start-here 入口后，再进入 `ARTIFACT_GUIDE.md`。
- 如果你正在做 release 评审，直接照 `REVIEWER_CHECKLIST.md` 逐项打勾即可。
- 想走最短路径时先看 `VISUAL_VALIDATION_QUICKSTART*.md`；需要更完整的
  图 -> 脚本 -> 输出目录映射时，再看 `VISUAL_VALIDATION*.md`。
