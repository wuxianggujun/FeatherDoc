# Page Number Fields Visual Review Task

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

请严格按下面顺序执行，不要只看 JSON 结构就下结论：

1. 目标是检查页码字段在 Word 最终渲染中的可见效果，而不是只看 CLI
   返回值、XML 结构或字段计数。
2. 优先使用当前任务目录里的现成 bundle 证据；只有在关键证据缺失、
   路径失效或明显过期时，才允许执行上面的 refresh command。
3. 一切结论必须基于截图证据，尤其是 aggregate contact sheet、
   `aggregate-contact-sheets\\*.png`、`bundle_review_manifest.json` 和每个
   case 的 `field_summary.json`。
4. 必须先做总览，再分别检查 `api-sample` 与 `cli-append`，不允许只看
   一个 case。
5. 只允许做视觉检查和问题定位，不要修改代码、文档或任务包内容。
6. 必须把检查结论与截图依据回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 验证以下文件存在：
   - `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`
   - `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`
   - `{{TASK_BUNDLE_SUMMARY_PATH}}`
   - `{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}\\*.png`
2. 先查看 `{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}`，快速判断两个 case 的
   页眉、页脚与页面几何是否整体正常。
3. 读取 `{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}`，逐 case 使用
   `cases[*].expected_visual_cues`、`docx_path`、`field_summary_json`
   作为检查断言和定位线索。
4. 至少逐一检查 `{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}\\*.png`；如需更细节，
   再根据 manifest 里的 `visual_artifacts.contact_sheet`、
   `visual_artifacts.pages_dir` 打开原始证据。
5. 若关键证据缺失或 manifest 指向无效路径，执行：
   `{{BUNDLE_REFRESH_COMMAND}}`
   若刷新仍失败，则结论只能是“无法判定”。
6. 将最终结论与问题列表回写到：
   - `{{REPORT_DIR}}\\review_result.json`
   - `{{REPORT_DIR}}\\final_review.md`

重点检查项：

- `api-sample`：页眉中 `Current page` 后必须能看到 `PAGE` 字段的实际渲染值。
- `api-sample`：页脚中 `Total pages` 后必须能看到 `NUMPAGES` 字段的实际渲染值。
- `cli-append`：两页都必须能看到 header/footer 中新增字段的实际渲染结果。
- `cli-append`：第一页与第二页的当前页码必须不同，总页数必须保持一致。
- `cli-append`：追加字段后，原有 portrait/landscape section 几何不能被破坏。
- 两个 case 都不能出现页眉页脚裁切、错位、内容消失或分页异常。

输出格式：

A. 总结：通过 / 不通过 / 无法判定

B. Case 结果：
- Case：
- 结论：
- 问题描述：
- 截图依据：
- 置信度：

C. 共性风险

D. 若无问题，也要明确说明“未发现明显页码字段视觉回归”
