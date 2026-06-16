# PDF E7 进展存档：组合合并、塑形与 RTL

本文件从 `04-pdf-execution-plan.md` 拆分而来，保留 E7 历史进展记录。

原始行号范围：2494-2973。

---

2026-05-15 继续推进（内部 2x2 组合合并）：

- 已补充内部 2x2 组合合并样本：
  `featherdoc-pdf-import-center-merged-table-source.pdf` 与
  `featherdoc-pdf-import-center-merged-table.pdf`。
  样本使用 4x4 规则网格，中间 `(row=1, col=1)` 单元格同时横跨 2 列和 2 行，
  用于覆盖非左上角锚点的组合合并路径。
- 已补 parser 侧回归：
  `PDFium parser detects center two-by-two merged table candidate spans`
  断言表格候选仍为 4 行 x 4 列，中心锚点 `column_span = 2`、
  `row_span = 2`，右侧和下方覆盖单元格保持为空文本占位。
- 已补导入侧回归：
  `PDF text importer preserves center two-by-two merged table cells`
  断言 opt-in 导入后仍保持 `paragraph / table / paragraph` 顺序，
  写入 4 行 x 4 列真实 `Document` 表格，保存重开后中心锚点为
  `vertical_merge = restart`，下方覆盖单元格为 `continue_merge`。
  Word 实际 cell 节点数为 14，避免把 2x2 合并错误展开成 16 个独立单元格。
- 已完成构建与测试验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_(structure|failure|table_heuristic)$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-center-merged-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-center-merged-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-center-merged-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  组合合并现在覆盖规则网格里的左上角和内部 2x2 锚点；仍不处理任意嵌套合并、
  扫描/OCR、缺失线条后的视觉推断或需要图像理解的表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（矩形 2x3 组合合并）：

- 已补充更大的内部矩形组合合并样本：
  `featherdoc-pdf-import-rectangular-merged-table-source.pdf` 与
  `featherdoc-pdf-import-rectangular-merged-table.pdf`。
  样本使用 5x5 规则网格，中间 `(row=1, col=1)` 单元格横跨 2 列和 3 行，
  用于确认组合合并不是 2x2 特例。
- 已补 parser 侧回归：
  `PDFium parser detects center two-by-three merged table candidate spans`
  断言表格候选为 5 行 x 5 列，中心锚点 `column_span = 2`、
  `row_span = 3`，两行下方覆盖区域都保持为空文本占位。
- 已补导入侧回归：
  `PDF text importer preserves center two-by-three merged table cells`
  断言 opt-in 导入后仍保持 `paragraph / table / paragraph` 顺序，
  写入 5 行 x 5 列真实 `Document` 表格，保存重开后中心锚点为
  `vertical_merge = restart`，两行下方覆盖单元格均为 `continue_merge`。
  Word 实际 cell 节点数为 22，避免把 2x3 合并错误展开成 25 个独立单元格。
- 已完成构建与测试验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_import_table_heuristic_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_import_table_heuristic$" --output-on-failure --timeout 60`
  通过。
- 已完成视觉验证：
  源 PDF 渲染产物为
  `output/pdf-e7-rectangular-merged-table-visual/source-pdf/contact-sheet.png`；
  导入后 DOCX 的 Word smoke 产物为
  `output/pdf-e7-rectangular-merged-table-visual/merged-docx/evidence/contact_sheet.png`
  和
  `output/pdf-e7-rectangular-merged-table-visual/merged-docx/table_visual_smoke.pdf`，
  视觉报告 verdict 为 `pass`。
- 已知限制更新：
  组合合并现在覆盖规则网格里的左上角 2x2、内部 2x2 和内部 2x3 矩形锚点；
  仍不处理任意嵌套合并、扫描/OCR、缺失线条后的视觉推断或需要图像理解的表格。
- 下一阶段入口保留：
  更复杂的嵌套合并、扫描件和 OCR 场景。

2026-05-15 继续推进（样式映射状态同步）：

- 已核对样式映射的当前真实实现状态，确认基础链路已经落地：
  `PdfTextRun` 已表达字体族/字体文件、字号、填充色、粗体、斜体、下划线、
  Unicode 标记和旋转角度；`layout_document_paragraphs()` 经 paragraph/table
  adapter 把直接 run 属性和继承样式翻译到 layout fragment；PDFio writer 已在
  content stream 中写出标准字体粗斜体变体、字号、颜色和下划线。
- 已同步 `design/02-current-roadmap.md`，把优先级 3 从“字段尚未串通”的过期描述
  调整为“基础样式映射已完成，剩余为合同级视觉 baseline、非标准字体粗斜体质量和
  发布门禁”。
- 已同步 `BUILDING_PDF.md`，在当前可用状态中补充 document adapter → PDFio writer
  的样式映射能力，并把正式可用前的限制收窄到复杂分页、图片锚点/裁剪/环绕、
  发布级合同样式视觉 baseline、字体捆绑和 PNG baseline 门禁。
- 本轮不新增生产代码，目标是避免后续继续推进时重复实现已完成的样式映射能力。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_document_adapter_font$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口保留：
  优先推进发布级视觉门禁或 HarfBuzz 文字塑形；如果继续补样式相关能力，应先补
  合同 sample 的视觉 baseline，而不是重复扩展已存在的 `PdfTextRun` 字段。

2026-05-15 继续推进（样式视觉 baseline 门禁）：

- 已把 `run_pdf_visual_release_gate.ps1` 的核心 PNG baseline 渲染从“合同、发票、
  长文档、图片语义、CLI font-map”扩展到样式专用样本：
  `styled-text`、`mixed-style-text`、`underline-text`、
  `document-contract-cjk-style` 和 `document-eastasia-style-probe`。
- 已给每个样式视觉样本补 `style_focus` 摘要，发布门禁的 `summary.json` 可以直接标出
  当前 contact sheet 需要人工关注的字号、颜色、粗斜体、下划线、CJK 字体映射和
  文档样式继承点。
- 已新增 `pdf_visual_release_gate_style_baselines` 轻量回归，静态解析
  `run_pdf_visual_release_gate.ps1` 并断言样式样本和 `style_focus` 标记不会被误删。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`，把合同级样式视觉 baseline
  从“待补”推进到“已纳入 PDF 视觉发布门禁”。
- 已完成验证：
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File test/pdf_visual_release_gate_style_baselines_test.ps1 -RepoRoot . -WorkingDir .bpdf-roundtrip-msvc/pdf_visual_release_gate_style_baselines`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_visual_release_gate_style_baselines|pdf_regression_(styled|mixed|underline|document-contract|document-eastasia)|pdf_document_adapter_font|pdf_regression_manifest" --output-on-failure --timeout 60`
  通过 8/8；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-style-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成 `output/pdf-style-visual-release-gate/report/aggregate-contact-sheet.png`。
- 下一阶段入口保留：
  非标准字体缺少 bold / italic 变体时的质量策略，或继续进入 HarfBuzz 文字塑形。

2026-05-15 继续推进（非标准字体 synthetic 粗斜体兜底）：

- 已把非标准字体缺少 bold / italic 变体时的策略从文档 TODO 落到代码：
  `PdfFontResolver` 仍优先选择 style-specific 映射；当显式映射、默认字体或 fallback
  只能提供 regular 字体时，会在 `PdfResolvedFont` 上标记 `synthetic_bold` /
  `synthetic_italic`。
- 已把 synthetic 标记经 document adapter 传入 `PdfTextRun`，PDFio writer 对 synthetic
  bold 使用 `PDFIO_TEXTRENDERING_FILL_AND_STROKE` 和小 stroke width，对 synthetic italic
  使用 12 度斜切 text matrix。这样不会重复 `TextShow`，避免 PDFium 回读出现重复文本。
- 已补回归：
  `pdf_font_resolver` 断言 style-specific 映射不会误标 synthetic，regular fallback 会标记
  synthetic；
  `pdf_document_adapter_font` 断言 synthetic 标记进入 layout，并覆盖 writer 输出后 PDFium
  回读不重复文本。
- 已同步 `BUILDING_PDF.md` 的字体选择说明，明确真实 bold / italic 字体优先，
  synthetic 只是防止样式完全丢失的视觉兜底。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_font_resolver_tests pdf_document_adapter_font_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(font_resolver|document_adapter_font)$" --output-on-failure --timeout 60`
  通过 2/2；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_(styled-text|mixed-style-text|underline-text|document-contract-cjk-style|document-eastasia-style-probe|contract-cjk-style)$" --output-on-failure --timeout 60`
  通过 6/6；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-style-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  优先级 3 基础样式映射收口完成，下一步进入优先级 4 HarfBuzz 文字塑形。

2026-05-15 继续推进（HarfBuzz shaper bridge 最小接口）：

- 已新增独立 `pdf_text_shaper` 桥接层，先不改现有 writer/layout 行为：
  `shape_pdf_text()` 输入 UTF-8 文本、字体文件路径和字号，输出 `PdfGlyphRun`，
  包含 glyph id、cluster、x/y advance 和 x/y offset。
- 已在 `FeatherDoc::Pdf` 的 HarfBuzz 可用构建中定义
  `FEATHERDOC_ENABLE_PDF_TEXT_SHAPER=1`，并显式链接 `harfbuzz::harfbuzz`。
  `pdf_text_shaper_has_harfbuzz()` 可用于测试和诊断当前构建是否启用 HarfBuzz。
- 已补 `pdf_text_shaper` 单测，覆盖空文本、非法字号、缺字体路径，以及 HarfBuzz 可用时
  `office` 能产出非空 glyph run、正向 advance 和合法 cluster。
- 已同步 `design/02-current-roadmap.md` 和 `BUILDING_PDF.md`：
  优先级 4 的“接入 HarfBuzz 构建”和“最小 shaper_bridge”已完成；
  已知限制是 `PdfDocumentLayout` 仍消费字符串 `PdfTextRun`，PDFio writer 还没有用 glyph id
  写 content stream。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_text_shaper$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  把 GlyphRun 接入 `PdfDocumentLayout` 的内部文本片段，先保留字符串回退，再让 PDFio writer
  增加 glyph id 写出路径。

2026-05-15 继续推进（GlyphRun 接入 PdfTextRun）：

- 已把 glyph run 数据结构拆成独立 `pdf_glyph_run.hpp`，供 shaper bridge 和 layout
  共同使用，避免让纯数据 layout 头直接承担 HarfBuzz 调用接口。
- `PdfTextRun` 现在可以携带 `PdfGlyphRun`。document adapter 在 file-backed text fragment
  上调用 `shape_pdf_text()`，仅在 HarfBuzz 成功产出非空 glyph run 时保留结果；
  标准 PDF base font、缺字体和坏字体仍保持原字符串路径。
- 已补 `pdf_document_adapter_font` 回归，断言 file-backed `office` 文本在 HarfBuzz 可用时
  会进入 `PdfTextRun::glyph_run`，并检查 glyph advance、cluster、字体路径和字号。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`：
  当前状态是 layout 已能携带 GlyphRun，但 line wrapping 和 PDFio writer 仍然消费字符串。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 3/3。
- 下一阶段入口：
  研究 PDFio Unicode CID 字体当前的 `CIDToGIDMap` / `ToUnicode` 写法，再决定 writer
  是直接写 CID/glyph hex string，还是先扩展 PDFio-side helper。

2026-05-15 继续推进（GlyphRun advance 接入 layout 宽度）：

- 已确认 PDFio 当前 Unicode 字体写法是：content stream 写 UTF-16 codepoint，
  font resource 的 `CIDToGIDMap` 再把 Unicode CID 映射到 glyph id；因此不能直接把
  HarfBuzz glyph id 当 CID 写入，否则会和现有 `CIDToGIDMap` / `ToUnicode` 语义错配。
- 已先推进低风险前置：`measure_text()` 在有字体文件时会优先调用 `shape_pdf_text()`，
  并用 `glyph_run_x_advance_points()` 作为 layout 宽度。HarfBuzz 不可用、缺字体、
  坏字体或塑形失败时仍回退到 FreeType / 估算宽度。
- 已扩展 `pdf_document_adapter_font` 回归：同一段落两个 file-backed run 使用不同字体族
  保持拆分，第二个 run 的 baseline x 坐标必须等于第一个 run 的 baseline x 加上
  `PdfGlyphRun` 的 x advance。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(font_resolver|text_metrics|text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 5/5。
- 下一阶段入口：
  若继续推进 writer，需要先设计新的 glyph-id font resource：要么让 CIDToGIDMap 变为
  identity 并为 clusters 生成 ToUnicode，要么继续借助 ActualText 做复制语义兜底。

2026-05-15 继续推进（HarfBuzz 相关 regression 样本扩展）：

- 已新增两个导出回归样本：
  `mixed-cjk-punctuation-text` 覆盖中英混排、中文逗号/分号/句号、中文括号和弯引号；
  `latin-ligature-text` 覆盖 `office`、`affinity`、`flow`、`file`、`fixture` 等
  fi/fl 相关文本。
- 已把 `pdf_regression_manifest.json` 从 37 个样本扩展到 39 个样本；
  `pdf_regression_*` 当前覆盖 40 个 CTest，包含 manifest 校验。
- 已更新 `test/CMakeLists.txt` 的 CJK skip 规则，让 `mixed_cjk_punctuation_text`
  和其他 CJK 样本一样，在缺少 CJK 字体时以 77 跳过。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md` 的样本数量与覆盖范围。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^(pdf_regression_manifest|pdf_regression_(mixed-cjk-punctuation-text|latin-ligature-text))$" --output-on-failure --timeout 60`
  通过 3/3；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过 40/40。
- 下一阶段入口：
  继续考虑是否把这两个 HarfBuzz 相关样本纳入视觉发布门禁 baseline。

2026-05-15 继续推进（文字塑形样本纳入视觉发布门禁）：

- 已把 `mixed-cjk-punctuation-text` 和 `latin-ligature-text` 加入
  `run_pdf_visual_release_gate.ps1` 的核心 PNG baseline 渲染列表。
- 已新增 `pdf_visual_release_gate_text_shaping_baselines` 静态回归，解析视觉门禁脚本并断言
  两个文字塑形样本的 `name`、PDF 路径和 baseline 输出目录不会被误删。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`，把优先级 4 的
  “视觉回归 sample 集扩展到中英混排、CJK 标点用例”推进为已完成。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  通过 2/2；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-text-shaping-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成文字塑形样本 baseline 与 aggregate contact sheet。
- 下一阶段入口：
  优先级 4 剩余核心风险集中在 PDFio writer 的 glyph-id content stream 与自定义
  CIDToGIDMap / ToUnicode 设计。

2026-05-15 继续推进（PDFio writer glyph-id content stream）：

- 已在 `pdf_writer.cpp` 增加 shaped glyph font resource 路径：对满足安全条件的
  file-backed `PdfTextRun::glyph_run` 分配私有 CID，创建独立 Type0 / CIDFontType2 字体资源，
  用 `CIDToGIDMap` 映射到 HarfBuzz glyph id，用 `W` 数组写入 HarfBuzz x advance，并保留
  ToUnicode / ActualText 文本提取语义。
- 已保留字符串 fallback：HarfBuzz 不可用、缺少字体文件、glyph offset / y advance 当前无法安全表达、
  或 glyph id 超出 16-bit CID 范围时仍走原 `pdfioContentTextShow()` 路径。
- 已补 `pdf_unicode_font_roundtrip` writer 回归：构造 shaped Latin 文本，断言 PDF 中出现
  `FeatherDocGlyph` / `CIDToGIDMap` / `ToUnicode`，并用 PDFium 回读确认原文只出现一次。
- 已完成验证：
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过 3/3；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过 40/40；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_visual_release_gate_(style|text_shaping)_baselines$" --output-on-failure --timeout 60`
  通过 2/2；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-writer-visual-release-gate -SkipUnicodeBaseline`
  通过，并生成 aggregate contact sheet。
- 下一阶段入口：
  后续若扩大 writer 塑形能力，优先处理 x/y offset、RTL / 竖排和
  多 glyph 同 cluster 的 ToUnicode 映射策略。

2026-05-15 继续推进（shaped glyph writer 安全边界与 CMap 回归）：

- 已收紧 `pdf_writer.cpp` 的 shaped glyph writer 入口：要求 `PdfGlyphRun` 字号与
  `PdfTextRun` 一致，cluster 必须落在原始 UTF-8 文本范围内，并且 glyph 顺序中的 cluster
  不能倒序。倒序 cluster 代表 RTL 或更复杂重排语义，当前仍回退到字符串写出路径。
- 已扩展 `pdf_unicode_font_roundtrip`：
  解压 PDF Flate stream，定位 `/FeatherDocGlyphToUnicode` CMap，并逐项断言 shaped glyph
  私有 CID 到 cluster Unicode 文本的 `beginbfchar` 映射；同时新增倒序 cluster 的 fallback
  回归，确认不会生成 `/FeatherDocGlyph` 字体资源，PDFium 仍能回读原文。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-positioned-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  继续扩展 shaped writer 前，先补齐 glyph offset / y advance 的定位模型；RTL 和竖排需要
  独立设计基线、方向和 text matrix 语义，不能直接复用当前 LTR `Tj` 路径。

2026-05-15 继续推进（shaped glyph offset 定位写出）：

- 已放开 shaped glyph writer 对 x/y offset 和 y advance 的硬 fallback：入口仍要求 file-backed
  font、HarfBuzz 成功、字号一致、cluster 有界且非倒序、glyph id 可放入 16-bit CID，并新增
  advance / offset 的 finite 检查。
- 已新增定位写出分支：普通无 offset run 保持原来的整段 `<cid...> Tj`；一旦 shaped run
  带有 x/y offset 或 y advance，就按 HarfBuzz pen position 逐 glyph 写 `Tm` + `<cid> Tj`。
  该分支复用当前 text run 的旋转矩阵和 synthetic italic 斜切矩阵，避免定位语义和既有
  `PdfTextRun` 样式分叉。
- 已扩展 `pdf_unicode_font_roundtrip`：构造带非零 offset / y advance 的 shaped Latin run，
  解压 PDF content stream 断言逐 glyph `Tm` 与 `Tj` 数量，并用 PDFium 回读确认原文只出现一次。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-cluster-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  shaped writer 剩余风险主要是 RTL / 竖排的方向模型，以及多 glyph 同 cluster 在定位和
  文本选择中的更细粒度验收。

2026-05-15 继续推进（shaped glyph cluster 提取语义收口）：

- 已收紧 shaped glyph writer 的 cluster 安全条件：cluster offset 除了必须有界、非倒序，
  还必须落在 UTF-8 codepoint 边界上；如果 HarfBuzz 数据或上游构造的数据把 cluster 指到
  多字节字符中间，writer 会保留字符串 fallback，避免把无效 UTF-8 切片写进 ToUnicode。
- 已固定多 glyph 同 cluster 的 ToUnicode 策略：第一个 CID 映射完整 cluster Unicode 文本，
  后续同 cluster CID 不写 ToUnicode entry，避免复制/提取时重复输出同一个文本 cluster。
  `ActualText` 仍包裹完整 run，作为阅读器文本提取兜底。
- 已扩展 `pdf_unicode_font_roundtrip`：
  手写 `a + combining acute + b` 的 repeated-cluster shaped run，解压
  `/FeatherDocGlyphToUnicode` CMap 断言 `<0001>` 映射 `<00610301>`、`<0002>` 不映射、
  `<0003>` 映射 `<0062>`；同时补 cluster 落在 UTF-8 continuation byte 时的 fallback 回归。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_unicode_font_roundtrip$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  shaped writer 的 LTR 安全子集已覆盖 glyph id、advance、offset、cluster 边界和重复
  cluster 提取语义；继续向外扩展时应优先单独设计 RTL / 竖排方向模型。

2026-05-15 继续推进（shaped glyph direction 元数据与非 LTR 回退）：

- 已在 `PdfGlyphRun` 增加 `PdfGlyphDirection`，`shape_pdf_text()` 会在 HarfBuzz
  `hb_buffer_guess_segment_properties()` 后记录 buffer direction；document adapter 保留该
  direction，避免后续 writer 只能靠 cluster 倒序推断 RTL / 竖排。
- 已收紧 shaped glyph writer 入口：当前 glyph-id CID content stream 只接受
  `left_to_right` 方向；`right_to_left`、`top_to_bottom`、`bottom_to_top` 或 unknown direction
  都回退到原字符串写出路径，继续保留 ToUnicode / ActualText 文本提取语义。
- 已扩展回归：
  `pdf_text_shaper` 断言 Latin `office` 的 HarfBuzz direction 为 `left_to_right`；
  `pdf_document_adapter_font` 断言 adapter 携带该 direction；`pdf_unicode_font_roundtrip`
  手写一个其它条件都满足但 direction 为 `right_to_left` 的 shaped run，确认不会生成
  `/FeatherDocGlyph` 字体资源且 PDFium 仍只回读一次原文。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过；
  `powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File scripts/run_pdf_visual_release_gate.ps1 -BuildDir .bpdf-roundtrip-msvc -OutputDir output/pdf-glyph-direction-visual-release-gate -SkipUnicodeBaseline`
  通过。
- 下一阶段入口：
  RTL / 竖排不再是 writer 入口的隐式边界条件，但真正支持它们还需要单独设计基线方向、
  text matrix、glyph order 和文本选择语义；在那之前保持字符串 fallback。

2026-05-15 继续推进（RTL direction 真实文本 smoke）：

- 已给 `pdf_text_shaper` 增加 Hebrew RTL smoke：通过 `FEATHERDOC_TEST_RTL_FONT` 或 Windows
  Arial / Segoe UI / Times 候选字体，对 `שלום` 验证 HarfBuzz direction 映射为
  `right_to_left`。
- 已给 `pdf_document_adapter_font` 增加 RTL direction metadata 回归：Document run 经字体映射
  进入 `PdfDocumentLayout` 后，`PdfTextRun::glyph_run.direction` 仍保留 `right_to_left`，
  证明 adapter 不会丢掉 HarfBuzz 的方向元数据。
- 已同步 `BUILDING_PDF.md`，记录 `FEATHERDOC_TEST_RTL_FONT` 这个可选测试字体入口。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font)$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  已确认真实 RTL 文本能进入 metadata 层；下一步如果继续推进 RTL，需要先定义 layout
  的逻辑顺序 / 视觉顺序边界，再决定 writer 是否用逐 glyph positioned CID 流承接。

2026-05-15 继续推进（HarfBuzz script tag 元数据）：

- 已在 `PdfGlyphRun` 增加 `script_tag`，`shape_pdf_text()` 会把 HarfBuzz
  `hb_buffer_get_script()` 转成 ISO 15924 tag；Latin 文本记录 `Latn`，Hebrew RTL 文本
  记录 `Hebr`。
- 已扩展 `pdf_text_shaper` 和 `pdf_document_adapter_font` 回归：分别断言 shaper 输出与
  document adapter layout 中的 `PdfGlyphRun` 都保留 direction 和 script tag，避免后续
  设计 RTL / Arabic / vertical 策略时丢失脚本上下文。
- 已同步 `BUILDING_PDF.md`，把 `PdfGlyphRun` 当前 metadata 更新为 direction / script。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font)$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  script tag 现在可以作为后续方向模型的输入；真正改变 layout / writer 前，仍需先定义
  每种脚本的受控验收样本和 fallback 规则。

2026-05-15 继续推进（shaper 显式 direction / script override）：

- 已扩展 `PdfTextShaperOptions`：调用方可以显式传入 `PdfGlyphDirection` 和 ISO 15924
  `script_tag`；未传入时仍由 `hb_buffer_guess_segment_properties()` 自动推断。
- 已在 `shape_pdf_text()` 中先写入调用方指定的 direction / script，再让 HarfBuzz 补齐未指定
  的 segment properties；非法 script tag 会返回明确诊断，不继续生成 glyph run。
- 已扩展 `pdf_text_shaper` 回归：验证 Latin 文本可被显式塑形为 RTL / `Hebr` metadata，
  并验证超长非法 script tag 会被拒绝。
- 已同步 `BUILDING_PDF.md`，记录 `PdfTextShaperOptions` 的显式 override 能力。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font)$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  可以开始把上游文档语义中的 RTL / bidi / language 信息映射到 shaper options；在 writer
  支持前，非 LTR 仍保持字符串 fallback。

2026-05-15 继续推进（Document run RTL -> shaper direction）：

- 已把 `run_inspection_summary::rtl` / style `run_rtl` / 默认 run RTL 解析进
  `ResolvedRunStyle`，并在 token、fragment、测宽和最终 `shape_fragment_text()` 之间保留
  `shaping_direction`。
- `rtl=true` 的 run 会把 `PdfTextShaperOptions::direction` 设置为 `right_to_left`；没有 RTL
  语义时仍保持 unknown，让 HarfBuzz 继续按文本自动 guess。
- 已扩展 `pdf_document_adapter_font`：Latin `office` run 显式 `set_rtl()` 后，layout 中的
  `PdfGlyphRun` 会记录 `right_to_left` direction 和 `Latn` script，证明上游 Word run
  格式已经进入 shaper options。
- 已同步 `BUILDING_PDF.md`，记录 run 级 `rtl` 到 shaper direction 的映射，以及 writer
  仍保持非 LTR 字符串 fallback。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  可以继续把 `bidi_language` 映射到 shaper language / script，或开始定义 RTL writer 的
  glyph order、text matrix 和文本提取验收；在 writer 语义明确前仍不直接写非 LTR CID 流。

2026-05-15 继续推进（HarfBuzz language tag 元数据）：

- 已在 `PdfGlyphRun` 和 `PdfTextShaperOptions` 增加 `language_tag`；调用方可以显式设置
  HarfBuzz language，shaper 会从 buffer 回填最终 language tag。
- 已扩展 `pdf_text_shaper` override 回归：显式设置 RTL / `Hebr` / `he` 后，glyph run 会同时
  记录 direction、script tag 和 language tag。
- 已同步 `BUILDING_PDF.md`，把当前 `PdfGlyphRun` 元数据更新为 direction / script /
  language。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_text_shaper_tests pdf_document_adapter_font_tests pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  shaper 已能承载 language metadata；下一步可以把 Document 的 `language` /
  `bidi_language` 解析结果传入 `PdfTextShaperOptions`。

2026-05-15 继续推进（Document run language -> shaper language）：

- 已把 `ResolvedRunStyle` / `TextToken` / `TextFragment` 扩展为携带
  `shaping_language_tag`，并在测宽与最终 `shape_fragment_text()` 之间保持同一组
  HarfBuzz segment options。
- Document adapter 现在会把 run/style/default 的 `language` / `bidi_language` 映射到
  `PdfTextShaperOptions::language_tag`；LTR / unknown direction 优先使用 `language`，
  RTL run 优先使用 `bidi_language`，缺失时再回退到另一侧语言。
- 已扩展 `pdf_document_adapter_font`：覆盖普通 run `language=fr` 进入
  `PdfGlyphRun::language_tag`，以及 `rtl=true` 时 `bidi_language=he` 优先于
  `language=en`；随后补充 default run `language=de` 和 inherited character style
  `language=it` 的 adapter 回归，确认直接 run、样式继承和默认 run 三条语言来源都能进入
  shaper options。
- 已同步 `BUILDING_PDF.md` 和 `design/02-current-roadmap.md`，记录 document adapter
  已能把 run 级 RTL 与语言语义传给 HarfBuzz shaper。
- 已完成验证：
  `cmd /c 'call "D:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && cmake --build .bpdf-roundtrip-msvc --target pdf_document_adapter_font_tests pdf_text_shaper_tests pdf_unicode_font_roundtrip_tests'`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "^pdf_(text_shaper|document_adapter_font|unicode_font_roundtrip)$" --output-on-failure --timeout 60`
  通过；
  `ctest --test-dir .bpdf-roundtrip-msvc -R "pdf_regression_" --output-on-failure --timeout 60`
  通过。
- 下一阶段入口：
  language metadata 已贯通到 layout；后续若继续推进 RTL writer，需要单独设计
  glyph order、baseline advance、text matrix 和文本选择语义，不能直接复用 LTR
  glyph-id stream。

