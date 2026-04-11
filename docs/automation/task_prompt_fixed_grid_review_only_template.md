# Fixed-grid Merge/Unmerge Visual Review Task

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
- Refresh command (only when evidence is missing or stale): `{{BUNDLE_REFRESH_COMMAND}}`

请严格执行以下规则，不要跳步，不要脑补成功：

1. 目标是检查 fixed-grid quartet
   (`merge_right` / `merge_down` / `unmerge_right` / `unmerge_down`)
   的最终肉眼可见效果，而不是只看 sample 退出码、XML 或结构树。
2. 优先使用当前任务目录里的现成 bundle 证据；只有在关键证据缺失、
   路径失效或明显过期时，才允许执行上面的 refresh command。
3. 一切结论必须基于截图证据，尤其是 aggregate contact sheet、
   `aggregate-first-pages\*.png` 和 `bundle_review_manifest.json`
   中的 `expected_visual_cues`。
4. 必须先做总览，再逐 case 检查，不允许只挑单个 case 下结论。
5. 只允许做视觉检查和问题定位，不要修改代码、文档或任务包内容。
6. 必须把检查结论与截图依据回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 验证以下文件存在：
   - `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
   - `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
   - `{{TASK_BUNDLE_SUMMARY_PATH}}`
   - `{{TASK_BUNDLE_FIRST_PAGES_DIR}}\*.png`
2. 先查看 `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`，快速判断四个 case
   是否都处于预期布局区间。
3. 读取 `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`，逐 case 使用
   `cases[*].expected_visual_cues` 作为检查断言。
4. 至少逐一检查 `{{TASK_BUNDLE_FIRST_PAGES_DIR}}\*.png`；如需更细节，
   再根据 manifest 里的 `visual_artifacts.first_page`、
   `visual_artifacts.contact_sheet`、`visual_artifacts.pages_dir`
   打开原始证据。
5. 若关键证据缺失或 manifest 指向无效路径，执行：
   `{{BUNDLE_REFRESH_COMMAND}}`
   若刷新仍失败，则结论只能是“无法判定”。
6. 将最终结论与问题列表回写到：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

重点检查项：

- `merge-right`：蓝色 merged cell 必须宽于下方灰色 `1000` 基列，
  但仍窄于绿色 `4100` tail 列。
- `merge-down`：橙色首列 pillar 必须跨两行，同时保持窄首列宽度；
  右侧 `1900/4100` 列不能漂移。
- `unmerge-right`：蓝色和黄色单元格必须恢复成两个独立单元格，
  且满足 `1000 < 1900 < 4100` 的视觉层次。
- `unmerge-down`：左上橙色和左下蓝色必须恢复成两个独立 `1000`
  单元格，中间横向边界清晰。
- 所有 case 都不能出现边框断裂、填充错位、单元格塌缩或 reopen 后漂移。

输出格式：

A. 总结：通过 / 不通过 / 无法判定

B. Case 结果：
- Case：
- 结论：
- 问题描述：
- 截图依据：
- 置信度：

C. 共性风险

D. 若无问题，也要明确说明“未发现明显 fixed-grid 视觉回归”
