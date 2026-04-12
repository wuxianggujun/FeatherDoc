# 可视化验证快速入口

如果你现在看到的是安装产物里的 `share/FeatherDoc` 目录，并且想从预览 PNG
最快跳到本地复现命令，就先看这份文件。

## 这个文件旁边有什么

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

这些 PNG 是面向 release 评审的证据快照。要真正复现它们，仍然需要源码仓库、
Windows 环境，以及真实安装的 `Microsoft Word`。

## 最短复现命令

把下面的 `<repo-root>` 替换成你本地的 FeatherDoc 源码仓库根目录。

```powershell
# 只跑可视化 gate：重新生成 smoke 图集、fixed-grid 四联图，以及两条 review task。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1

# 同一条 gate，但顺手刷新仓库 README / docs 的预览 PNG。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_release_gate.ps1 -RefreshReadmeAssets

# 跑完整 release-preflight：MSVC 构建 + 测试 + install smoke + visual gate。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_release_candidate_checks.ps1

# 跑完整 release-preflight，并同步刷新 README / docs 图集。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_release_candidate_checks.ps1 -RefreshReadmeAssets

# 上面任一命令完成后，打开最新的 review task。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_word_review_task.ps1
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\open_latest_fixed_grid_review_task.ps1 -PrintPrompt

# 两条 task report 都写完后，最短的一键签收入口。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_latest_visual_review_verdict.ps1

# 把截图级 verdict 写回 task report 后，同步回 gate summary。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json

# 如果你跑的是完整 release-preflight，再把 verdict 上卷到 summary.json，
# 并顺手刷新 START_HERE.md 与整套 release 说明。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\sync_visual_review_verdict.ps1 `
    -GateSummaryJson <repo-root>\output\word-visual-release-gate\report\gate_summary.json `
    -ReleaseCandidateSummaryJson <repo-root>\output\release-candidate-checks\report\summary.json `
    -RefreshReleaseBundle

# 从最新 task 里刷新仓库 README / docs 用的预览 PNG。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\refresh_readme_visual_assets.ps1
```

如果你跑的是完整 `release-preflight`，报告目录还会同时生成
`release_handoff.md`、`release_body.zh-CN.md` 和 `release_summary.zh-CN.md`。
上面这条短命令会自动解析最新 task pointer 和匹配的 release summary。
如果你需要手动覆盖路径，再继续使用显式同步命令。
如果 `summary.json` 已经带上最终 verdict，只是后面又改了 release 文案，
想重刷这几份说明而不重跑 preflight，也可以继续用更窄的入口：

```powershell
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\write_release_note_bundle.ps1 `
    -SummaryJson <repo-root>\output\release-candidate-checks\report\summary.json
```

## 更窄的入口

```powershell
# 只生成 smoke 图集。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_word_visual_smoke.ps1

# 只生成 README / docs 公共展示面用的三张独立样例页。
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

# 只生成 fixed-grid 四联回归图，并顺带准备 review task。
pwsh -ExecutionPolicy Bypass -File <repo-root>\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
    -PrepareReviewTask `
    -ReviewMode review-only
```

## 下一步看哪里

- `VISUAL_VALIDATION.md`：预览图和脚本、输出路径的一一对应。
- `README.md`：安装结构、`find_package` smoke 和 CLI 总览。
- `README.zh-CN.md`：中文入口说明。
