# Word Visual Review Task

- Task id: `{{TASK_ID}}`
- Generated at: `{{GENERATED_AT}}`
- Mode: `{{MODE}}`
- Workspace: `{{WORKSPACE}}`
- Target document: `{{DOCX_PATH}}`
- Render command: `powershell -ExecutionPolicy Bypass -File "{{WORKSPACE}}\scripts\run_word_visual_smoke.ps1" -InputDocx "{{DOCX_PATH}}" -OutputDir "{{TASK_DIR}}"`
- Evidence directory: `{{EVIDENCE_DIR}}`
- Report directory: `{{REPORT_DIR}}`
- Repair directory: `{{REPAIR_DIR}}`

请严格执行以下规则，不要跳步，不要脑补成功：

1. 目标是检查 `{{DOC_NAME}}` 的最终肉眼可见效果，而不是只看 XML、结构树、单元测试或程序返回值。
2. 只允许使用脚本产出的 PDF/PNG 证据做结论，不要再直接打开 Word 查看最终效果。
3. 如果任务包里已经有 `{{REPORT_DIR}}\summary.json`、`{{REPORT_DIR}}\review_checklist.md`、`{{EVIDENCE_DIR}}\contact_sheet.png` 和 `{{EVIDENCE_DIR}}\pages\*.png`，优先复用这些现成证据；只有在证据缺失、明显过期或与当前文档不一致时，才执行上面的渲染命令。
4. 若脚本执行失败、Word 导出失败、PDF 缺失、分页 PNG 缺失，或证据不完整，结论只能是“无法判定”，不能输出“通过”。
5. 只允许做视觉检查和问题定位，不要修改文档。
6. 必须把截图依据和结论回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 验证 `{{DOCX_PATH}}` 存在，并在输出中明确写出绝对路径。
2. 先检查任务包里是否已经存在以下产物；若缺失、明显过期或不对应当前文档，再执行渲染命令：`powershell -ExecutionPolicy Bypass -File "{{WORKSPACE}}\scripts\run_word_visual_smoke.ps1" -InputDocx "{{DOCX_PATH}}" -OutputDir "{{TASK_DIR}}"`。
3. 验证以下产物已经生成且可读：
   - `{{REPORT_DIR}}\summary.json`
   - `{{REPORT_DIR}}\review_checklist.md`
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`
   - `{{EVIDENCE_DIR}}\contact_sheet.png`
   - `{{EVIDENCE_DIR}}\pages\*.png`
4. 先查看 `{{EVIDENCE_DIR}}\contact_sheet.png` 快速确认总页数和整体版式，再逐页检查 `{{EVIDENCE_DIR}}\pages\*.png`。
5. 重点检查：
   - 一级标题、二级标题位置是否正确；
   - 正文是否有异常空行、错位、截断、重叠、丢字；
   - 表格边框、列宽、行高、跨页、分页断裂、单元格 `w:textDirection` 纵排/旋转文本是否异常；
   - 窄单元格里的 RTL/LTR/CJK 混排是否出现顺序错乱、异常换行、裁切、重叠、标点翻转或字体漂移；
   - 图片、图注、文字环绕是否异常；
   - 页码、页眉、页脚是否正常；
   - 是否有元素重叠、超出页面、分页突变、大片空白。
6. 必须把截图依据和结论回写到 `{{REPORT_DIR}}`。优先直接更新以下现有文件：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

输出格式：

A. 总结：通过 / 不通过 / 无法判定

B. 问题列表：
- 页码：
- 元素类型：
- 问题描述：
- 严重程度：
- 截图依据：
- 置信度：

C. 建议修复方式

D. 若无问题，也要明确说明“未发现明显版式问题”
