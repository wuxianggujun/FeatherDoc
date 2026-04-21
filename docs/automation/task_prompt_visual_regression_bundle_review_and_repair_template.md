# {{VISUAL_BUNDLE_LABEL}} Review And Repair Task

- Task id: `{{TASK_ID}}`
- Generated at: `{{GENERATED_AT}}`
- Mode: `{{MODE}}`
- Workspace: `{{WORKSPACE}}`
- Source kind: `{{SOURCE_KIND}}`
- Source bundle root: `{{BUNDLE_ROOT}}`
- Local aggregate evidence dir: `{{TASK_BUNDLE_AGGREGATE_EVIDENCE_DIR}}`
- Local aggregate contact sheet: `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
- Local bundle summary: `{{TASK_BUNDLE_SUMMARY_PATH}}`
- Local review manifest (if any): `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
- Task directory: `{{TASK_DIR}}`
- Evidence directory: `{{EVIDENCE_DIR}}`
- Report directory: `{{REPORT_DIR}}`
- Repair directory: `{{REPAIR_DIR}}`
- Refresh command for stale source evidence: `{{BUNDLE_REFRESH_COMMAND}}`

请先完成严格视觉检查；若检查不通过，再继续修复生成逻辑并重新做完整回归。
不要手工改 Word 成品，也不要只修单个 case 不做全套回归。

强制约束：

1. 一切结论必须基于 Word 最终截图证据，而不是 sample 退出码、JSON
   摘要、XML 或单个结构断言。
2. 第一轮优先使用当前任务目录里已经复制好的 bundle 证据；只有关键证据
   缺失、路径失效或明显过期时，才执行 `{{BUNDLE_REFRESH_COMMAND}}`。
3. 第一轮必须先查看 aggregate contact sheet，再根据 `bundle_summary.json`
   的 `cases[*].expected_visual_cues` 逐 case 检查。
4. 不得覆盖原始 source bundle。
5. 每一轮修复都必须输出到 `{{REPAIR_DIR}}` 下新的子目录，例如
   `fix-01\bundle-regression`。
6. 每次修复后都必须重新执行该 bundle 对应的完整 visual regression，
   不允许只挑一个 case 重跑。
7. 如果任一轮回归脚本失败、Word 导出失败、PNG 证据缺失，或 aggregate
   contact sheet 未生成，就不能宣称修复成功。
8. 最终报告必须回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 完成一轮完整视觉检查：
   - 验证 `{{TASK_BUNDLE_SUMMARY_PATH}}`、
     `{{TASK_BUNDLE_AGGREGATE_EVIDENCE_DIR}}`、
     `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}` 都存在；
   - 查看 aggregate contact sheet；
   - 按 `bundle_summary.json` 的 `expected_visual_cues` 与
     `visual_artifacts` 逐 case 检查；
   - 输出“通过 / 不通过 / 无法判定”。
2. 若结论为“不通过”，先根据截图判断回归更像是布局错位、元素缺失、
   顺序异常、换行漂移、表格几何漂移、图片定位错误，还是样式渲染异常。
3. 修改生成逻辑，而不是手工改 Word 成品。
4. 每轮修复后重新执行该 bundle 对应回归脚本，输出到
   `repair\fix-XX\bundle-regression` 下的新目录。
5. 基于新目录中的 aggregate contact sheet、summary.json 和 case 证据，
   再做一轮完整检查。
6. 必须将本轮结果同步回写到：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

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
