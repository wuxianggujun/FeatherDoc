# Fixed-grid Merge/Unmerge Review And Repair Task

- Task id: `{{TASK_ID}}`
- Generated at: `{{GENERATED_AT}}`
- Mode: `{{MODE}}`
- Workspace: `{{WORKSPACE}}`
- Source kind: `{{SOURCE_KIND}}`
- Source bundle root: `{{BUNDLE_ROOT}}`
- Local aggregate contact sheet: `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
- Local aggregate first pages: `{{TASK_BUNDLE_FIRST_PAGES_DIR}}`
- Local review manifest: `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
- Local bundle summary: `{{TASK_BUNDLE_SUMMARY_PATH}}`
- Task directory: `{{TASK_DIR}}`
- Evidence directory: `{{EVIDENCE_DIR}}`
- Report directory: `{{REPORT_DIR}}`
- Repair directory: `{{REPAIR_DIR}}`
- Refresh command for stale source evidence: `{{BUNDLE_REFRESH_COMMAND}}`
- Repair rerun example: `{{BUNDLE_REPAIR_COMMAND_EXAMPLE}}`

请先做严格视觉检查；若检查不通过，再继续修复生成逻辑并重新做完整回归。
不要手工改 Word 成品，也不要只修单个 case 不做全套回归。

强制约束：

1. 一切结论必须基于 Word 最终截图证据，而不是 sample 返回值、XML
   或单个 API 自检。
2. 第一轮优先使用当前任务目录里的现成 bundle 证据；只有在关键证据缺失、
   路径无效或明显过期时，才执行 `{{BUNDLE_REFRESH_COMMAND}}`。
3. 第一轮必须先查看 aggregate contact sheet，再根据
   `bundle_review_manifest.json` 的 `expected_visual_cues` 逐 case 检查。
4. 不得覆盖原始 source bundle。
5. 每一轮修复都必须写到 `{{REPAIR_DIR}}` 下的新子目录，例如
   `fix-01\bundle-regression`。
6. 每次修复后都必须重新执行完整 fixed-grid quartet 回归，不允许只挑一个
   case 重跑。
7. 如果任一轮回归脚本失败、Word 导出失败、PNG 证据缺失，或总览 contact
   sheet 未生成，就不能宣称修复成功。
8. 最终报告必须回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 完成一轮完整视觉检查：
   - 验证 `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`、
     `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`、
     `{{TASK_BUNDLE_SUMMARY_PATH}}`、
     `{{TASK_BUNDLE_FIRST_PAGES_DIR}}\*.png` 都存在；
   - 查看 aggregate contact sheet；
   - 按 manifest 的 `expected_visual_cues` 逐 case 检查；
   - 输出“通过 / 不通过 / 无法判定”。
2. 若结论为“不通过”，根据截图判断问题更像是：
   - `merge_right()` 宽度同步回归；
   - `merge_down()` 纵向合并/边界回归；
   - `unmerge_right()` 恢复单元格宽度或边框回归；
   - `unmerge_down()` 恢复单元格、横边界或行高回归；
   - 通用表格渲染、填充或边框回归。
3. 修改生成逻辑，而不是手工改 Word 成品。
4. 每轮修复后执行一套新的 quartet 回归，输出到 `repair\fix-XX` 下的
   repo-relative 目录。例如：
   `{{BUNDLE_REPAIR_COMMAND_EXAMPLE}}`
5. 基于新回归目录中的 aggregate contact sheet、review manifest 和
   case 证据重新做完整检查。
6. 必须将本轮结果同步回写到：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

重点检查项：

- `merge-right`：蓝色 merged cell 是否仍宽于 `1000` 基列且窄于 `4100`
  tail 列。
- `merge-down`：橙色 pillar 是否仍跨两行且保持窄首列宽度。
- `unmerge-right`：拆分后蓝/黄两格是否恢复独立且宽度层次正确。
- `unmerge-down`：拆分后左上/左下是否恢复独立首列单元格且横边界清晰。
- 四个 case 是否都没有边框断裂、填充漂移、单元格塌缩、错列或 reopen 后
  视觉漂移。

最终输出至少包含：

A. 本轮检查结论

B. Case 问题列表
- Case：
- 问题描述：
- 严重程度：
- 截图依据：
- 置信度：

C. 本轮修复动作

D. 修复后回归目录路径

E. 回归检查结论

F. 若仍失败，说明剩余阻塞点和下一轮建议
