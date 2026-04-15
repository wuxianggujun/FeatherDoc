# Section Page Setup Visual Review Task

- Task id: `{{TASK_ID}}`
- Generated at: `{{GENERATED_AT}}`
- Mode: `{{MODE}}`
- Workspace: `{{WORKSPACE}}`
- Source kind: `{{SOURCE_KIND}}`
- Source bundle root: `{{BUNDLE_ROOT}}`
- Local aggregate contact sheet: `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
- Local aggregate case contact sheets: `{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}`
- Local review manifest: `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
- Local bundle summary: `{{TASK_BUNDLE_SUMMARY_PATH}}`
- Task directory: `{{TASK_DIR}}`
- Evidence directory: `{{EVIDENCE_DIR}}`
- Report directory: `{{REPORT_DIR}}`
- Repair directory: `{{REPAIR_DIR}}`
- Refresh command (only when evidence is missing or stale): `{{BUNDLE_REFRESH_COMMAND}}`

请严格执行以下规则，不要跳步，也不要只看结构 JSON 就直接下结论：

1. 目标是检查 section page setup 的最终肉眼可见效果，而不是只看
   CLI inspect 输出、sample 退出码或 XML 结构。
2. 优先使用当前任务目录里的现成 bundle 证据；只有在关键证据缺失、
   路径失效或明显过期时，才允许执行上面的 refresh command。
3. 一切结论必须基于截图证据，尤其是 aggregate contact sheet、
   `aggregate-contact-sheets\\*.png` 和 `bundle_review_manifest.json`
   中的 `expected_visual_cues`。
4. 必须先做总览，再分别检查 `api-sample` 与 `cli-rewrite`，不允许
   只挑一个 case 下结论。
5. 只允许做视觉检查和问题定位，不要修改代码、文档或任务包内容。
6. 必须把检查结论与截图依据回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 验证以下文件存在：
   - `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
   - `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
   - `{{TASK_BUNDLE_SUMMARY_PATH}}`
   - `{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}\\*.png`
2. 先查看 `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`，快速判断两个 case
   的页面方向和页面宽高是否落在预期区间。
3. 读取 `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`，逐 case 使用
   `cases[*].expected_visual_cues` 作为检查断言。
4. 至少逐一检查 `{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}\\*.png`；如需更细节，
   再根据 manifest 里的 `visual_artifacts.contact_sheet`、
   `visual_artifacts.page_01`、`visual_artifacts.page_02` 打开原始证据。
5. 若关键证据缺失或 manifest 指向无效路径，执行：
   `{{BUNDLE_REFRESH_COMMAND}}`
   若刷新仍失败，则结论只能是“无法判定”。
6. 将最终结论与问题列表回写到：
   - `{{REPORT_DIR}}\\review_result.json`
   - `{{REPORT_DIR}}\\final_review.md`

重点检查项：

- `api-sample`：第 1 页必须保持竖版，第 2 页必须切换为横版。
- `api-sample`：两页文本必须保持可读，不能出现裁切、错页或异常边距漂移。
- `cli-rewrite`：第 1 页和第 2 页都必须渲染为横版。
- `cli-rewrite`：CLI 改写后仍需保持两页输出，不能出现内容溢出或分页损坏。
- 两个 case 都不能出现页边距明显错误、页眉页脚位置异常或方向与预期不符。

输出格式：

A. 总结：通过 / 不通过 / 无法判定

B. Case 结果：
- Case：
- 结论：
- 问题描述：
- 截图依据：
- 置信度：

C. 共性风险

D. 若无问题，也要明确说明“未发现明显 section page setup 视觉回归”
