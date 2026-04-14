# Word Visual Review And Repair Task

- Task id: `{{TASK_ID}}`
- Generated at: `{{GENERATED_AT}}`
- Mode: `{{MODE}}`
- Workspace: `{{WORKSPACE}}`
- Target document: `{{DOCX_PATH}}`
- First-pass render command: `powershell -ExecutionPolicy Bypass -File "{{WORKSPACE}}\scripts\run_word_visual_smoke.ps1" -InputDocx "{{DOCX_PATH}}" -OutputDir "{{TASK_DIR}}"`
- Evidence directory: `{{EVIDENCE_DIR}}`
- Report directory: `{{REPORT_DIR}}`
- Repair directory: `{{REPAIR_DIR}}`

请先做严格的视觉检查；若检查不通过，再继续修复生成逻辑并重新做完整回归。不要手工直接改原始 Word 文档。
如果任务包里已经有 `{{REPORT_DIR}}\summary.json`、`{{REPORT_DIR}}\review_checklist.md`、`{{EVIDENCE_DIR}}\contact_sheet.png` 和 `{{EVIDENCE_DIR}}\pages\*.png`，优先复用这些现成证据；只有在证据缺失、明显过期或与当前文档不一致时，才执行首轮渲染命令。

强制约束：

1. 一切结论必须基于 Word 中最终肉眼可见效果和截图证据。
2. 只允许使用脚本产出的 PDF/PNG 证据做结论，不要再直接打开 Word 查看最终效果。
3. 第一轮必须先执行上面的渲染命令，并确认脚本成功生成 PDF、分页 PNG、联系图和报告骨架。
4. 原始文档 `{{DOCX_PATH}}` 不得覆盖。
5. 每一轮修复都必须写到 `{{REPAIR_DIR}}` 下的新子目录，例如 `fix-01\candidate.docx`。
6. 每修完一轮都要重新执行完整脚本渲染与视觉检查，不能只看单个问题页。
7. 如果任一轮脚本执行失败、导出失败、分页 PNG 缺失，或证据不完整，就不能宣称修复成功。
8. 最终报告必须回写到 `{{REPORT_DIR}}`。

执行顺序：

1. 完成一轮完整视觉检查，步骤与检查模式一致：
   - 验证文档路径；
   - 运行首轮渲染脚本；
   - 校验 `summary.json`、联系图和分页 PNG 已生成；
   - 按页检查截图并记录证据；
   - 输出“通过 / 不通过 / 无法判定”。
2. 若结论为“不通过”，根据截图和问题报告判断问题更可能落在哪个代码层：
   - 段落/标题/正文问题；
   - 表格布局问题，包括边框、宽度、跨页、单元格纵排/旋转文本、窄格混排；
   - 页眉页脚/页码问题；
   - 图片插入与环绕问题；
   - 自动化误抓或视图问题。
3. 修改生成逻辑或模板输入，而不是手工改 Word 成品。
4. 将修复后的候选文档输出到 `{{REPAIR_DIR}}` 下的新轮次目录。
5. 对候选文档执行：`powershell -ExecutionPolicy Bypass -File "{{WORKSPACE}}\scripts\run_word_visual_smoke.ps1" -InputDocx "<candidate.docx>" -OutputDir "<对应 fix-XX 目录>"`，并基于该目录下的证据重新做完整视觉检查。
6. 必须将本轮结果同步回写到 `{{REPORT_DIR}}` 下现有报告文件：
   - `{{REPORT_DIR}}\review_result.json`
   - `{{REPORT_DIR}}\final_review.md`

最终输出至少包含：

A. 本轮检查结论

B. 问题列表
- 页码：
- 元素类型：
- 问题描述：
- 严重程度：
- 截图依据：
- 置信度：

检查问题时要特别关注：
- 单元格 `w:textDirection` 纵排/旋转文本是否可读、是否有裁切或边框错位；
- 窄单元格中的 RTL/LTR/CJK 混排是否出现顺序错乱、异常换行、字体漂移、重叠或标点反向。

C. 本轮修复动作

D. 修复后候选文档路径

E. 回归检查结论

F. 若仍失败，说明剩余阻塞点和下一轮建议
