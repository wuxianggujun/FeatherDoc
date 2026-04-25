# {{VISUAL_BUNDLE_LABEL}} Visual Review Task

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
- Refresh command (only when evidence is missing or stale): `{{BUNDLE_REFRESH_COMMAND}}`

请严格执行以下规则，不要跳步，不要把结构断言当成视觉结论：

1. 目标是检查 Word 最终截图是否符合预期，而不是只看 CLI 成功、JSON
   摘要、XML 或 reopen 结构。
2. 优先使用当前任务目录里已经复制好的 bundle 证据；只有关键证据缺失、
   路径失效或明显过期时，才允许执行 refresh command。
3. 必须先看 aggregate contact sheet，再结合 `bundle_summary.json`
   的 `cases[*].expected_visual_cues` 与 `cases[*].visual_artifacts`
   逐 case 检查。
4. 如果 bundle 里还有 review manifest，它只能作为补充，不得替代截图判断。
5. 只允许做视觉检查和问题定位，不要修改代码、文档或任务包内容。
6. 必须把检查结论回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 验证以下路径存在：
   - `{{TASK_BUNDLE_SUMMARY_PATH}}`
   - `{{TASK_BUNDLE_AGGREGATE_EVIDENCE_DIR}}`
   - `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
2. 先查看 `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`，快速确认所有 case 的
   before/after 是否都具备可读差异。
3. 读取 `{{TASK_BUNDLE_SUMMARY_PATH}}`，逐 case 使用
   `expected_visual_cues` 作为检查断言，并根据 `visual_artifacts`
   打开 case 级 contact sheet 或页面截图。
4. 若 `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}` 存在，可额外读取其中的路径与
   说明，但最终结论仍必须基于截图。
5. 若关键证据缺失或路径失效，才允许执行：
   `{{BUNDLE_REFRESH_COMMAND}}`
   如果刷新仍失败，则结论只能是“无法判定”。
6. 将最终结论与截图依据回写到：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

输出格式：

A. 总结：通过 / 不通过 / 无法判定

B. Case 结果：
- Case：
- 结论：
- 问题描述：
- 截图依据：
- 置信度：

C. 共性风险

D. 若无问题，也要明确说明“未发现明显视觉回归”
