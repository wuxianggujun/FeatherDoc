# 当前主路线：Word ⇄ PDF 转换器（依赖优先）

> 本文档定义 FeatherDoc Core **当前 0–6 个月**的 Word ⇄ PDF 双向转换推进路线。
>
> 这是**当前主路线**——优先用好 PDFio / PDFium，不是自研引擎。
>
> 阅读前置：[01-architecture.md](01-architecture.md)。
> 长期愿景（自研排版引擎，6 个月后才考虑）见 [03-long-term-roadmap.md](03-long-term-roadmap.md)。

## 路线原则

当前阶段的重点不是自研 PDF 引擎，而是**好好使用 PDFio / PDFium**。

FeatherDoc 应维护的是：

- docs 库的 Word 语义模型
- `AST → PDFio` 翻译层
- `PDFium → AST` 翻译层
- 测试集和回归质量

PDFio 和 PDFium 本身应尽量作为外部基础设施使用，不把维护成本搬进 Core。

## 阶段总览

```text
Phase 1: 能跑起来          （现在 - 3 个月）
   ↓
Phase 2: 稳定 + 优化       （3 - 6 个月）
   ↓
Phase 3: 选择性自研        （6 个月+）
```

每个阶段都必须保持默认构建轻量：不显式打开 PDF 开关时，不拉取 PDFio / PDFium。

## Phase 1：能跑起来（现在 - 3 个月）

### 目标

用 PDFio + PDFium 跑通端到端流程，确认三件套架构可行。

这个阶段的关键不是 PDF 质量做到多高，而是把整体链路接起来：

```text
Word → docs 库 → AST → PDFio → PDF
PDF  → PDFium → AST → docs 库 → Word
```

### 重点

Phase 1 的重点是：

- docs 库的 AST / DOM 是否足够表达文档语义
- 两个翻译层的边界是否清楚
- PDFio / PDFium 是否能稳定作为底层依赖
- 测试集是否能持续暴露退化问题

### 子任务

1. **接好 PDFio 写出链路**

   - 保持 `FeatherDoc::Pdf` 默认关闭
   - 让 probe 从手写内容升级为 `LayoutResult → PDFio`
   - 支持基础文本、矩形、页面尺寸和 metadata

2. **接好 PDFium 读入链路**

   - 新增实验性 PDF 读入模块设计
   - 验证 PDFium 能提取页面、文本、坐标和字体信息
   - 输出调试用中间结构，不急着生成高质量 Word

3. **写抽象接口**

   初始接口只表达 Core 需要的能力，不暴露第三方类型：

   ```cpp
   class IPdfGenerator {
   public:
     virtual ~IPdfGenerator() = default;
     virtual PdfWriteResult write(const LayoutResult &layout,
                                  const PdfWriteOptions &options) = 0;
   };

   class IPdfParser {
   public:
     virtual ~IPdfParser() = default;
     virtual PdfParseResult parse(const std::filesystem::path &input,
                                  const PdfParseOptions &options) = 0;
   };
   ```

   `IPdfGenerator` 背后第一版是 PDFio，`IPdfParser` 背后第一版是 PDFium。

4. **建立测试集**

   准备一组 10+ 个样本（当前 manifest 已扩展到 39 个样本，`pdf_regression_*`
   覆盖 40 个 CTest，包含 manifest 校验），覆盖：

   - 纯文本 PDF
   - 中文 PDF
   - 简单表格 PDF
   - 多页 PDF
   - 带图片 PDF
   - 不同字体和字号
   - FeatherDoc 自己生成的 DOCX / PDF 对

   每个样本都要能进入回归测试，至少验证：

   - 不崩溃
   - 页数正确
   - 文本提取结果可比对
   - 坐标信息可序列化
   - 生成 PDF 能被常见阅读器打开

### 验收标准

- [ ] 默认构建仍不启用 PDF 能力
- [ ] PDFio 写出 probe 可通过 `LayoutResult` 生成 PDF
- [ ] PDFium 读入 probe 可提取文本和坐标
- [ ] `IPdfGenerator` / `IPdfParser` 不暴露 PDFio / PDFium 类型
- [x] 首批 PDF 样本进入回归测试（当前 manifest 已扩展到 39 个样本，
      `pdf_regression_*` 覆盖 40 个 CTest，包含 manifest 校验）

### 当前实现进展

- [x] 默认构建保持 PDFio / PDFium 全部关闭
- [x] 已定义 `IPdfGenerator` / `IPdfParser`
- [x] 已定义最小 PDF layout 纯数据结构
- [x] 已新增最小 `Document` 段落 → `PdfDocumentLayout` 适配层
- [x] `PdfioGenerator` 已作为 `IPdfGenerator` 的第一版实现
- [x] `featherdoc_pdfio_probe` 已改为通过接口写出 PDF
- [x] 已新增 `featherdoc_pdf_document_probe`，验证 docs 段落可经 layout 写出 PDF
- [x] 已新增 `PdfiumParser` 作为 `IPdfParser` 的第一版实现
- [x] `FEATHERDOC_BUILD_PDF_IMPORT` 默认关闭，开启时默认使用 PDFium 源码 checkout
- [x] 已新增 `cmake/FeatherDocPdfium.cmake`，通过 GN/Ninja 构建 PDFium 源码
- [x] 已新增 [PDFium 源码构建接入](dependencies/pdfium.md)
- [x] 已接入本机 PDFium 源码 checkout 并验证 `featherdoc_pdfium_probe`
- [x] 已用 PDFio probe 生成的 PDF 验证 PDFium 可提取页面和 text spans
- [x] 同时开启 PDFio / PDFium 时，已有 `pdfium_parser_probe` CTest 覆盖写出后再读入
- [x] 同时开启 PDFio / PDFium 时，已有 `pdfium_document_parser_probe` CTest 覆盖 docs 段落写出后再读入
- [x] 已支持基础段落、表格和基础样式写出
- [x] 已接入 FreeType 字体度量，Layout 换行和行高改为真实字体测量
- [x] 已有 CJK / Unicode 字体专项回归，覆盖字体解析、CJK 回退、PDFio 子集嵌入、
      `/ToUnicode` 写出和 PDFium 文本回读
- [x] 已打通基础样式映射，`PdfTextRun`、`layout_document_paragraphs()` 和
      PDFio writer 已覆盖字体族/字体文件、字号、颜色、粗体、斜体和下划线，
      并由 `pdf_document_adapter_font` 覆盖段落、继承样式、表格单元格和 writer 回归
- [x] 已建立首批 39 个 PDF regression manifest 样本，覆盖纯文本、多页文本、中文路径、中英混排标点、Latin ligature 文本、样式、字号、颜色、横向页面、标点、边框框体、基础线条、固定坐标表格外观、合同样式、页眉页脚、多栏文本、发票网格、图片说明文字、metadata 长标题，以及 sectioned/list/long report、image report、CJK report、CJK image report、document east-asian style probe、document image semantics、document table semantics、document long flow 和 document invoice table 这几个更接近真实文档流的生成型样本
- [ ] 仍需继续扩展复杂视觉回归和发布门禁；真实文档样本集已经进入 manifest，但还没有把文件大小、图片数量/尺寸和 PNG baseline 全部自动化，CJK 字体也还没有进入发行包捆绑策略

## 当前下一步任务（按优先级）

> 本节是 Phase 1 → Phase 2 之间的**具体执行清单**，把"链路已通"推进到"输出可用"。
>
> 进展应直接更新本节的 checkbox 状态，而不是另开文档。

当前 PDF 输出已经跑通端到端，而且基础样式、表格、字体度量以及 CJK / Unicode 回环
都已有专项回归。下一步主要是把发行级字体策略、复杂视觉门禁和 HarfBuzz 文字塑形继续收口。
下面四步按依赖顺序推进，**每一步是后一步的前置**。

### 优先级 1：FreeType 集成 + 字体度量（约 1 个月）

**问题**：当前 `layout_document_paragraphs()` 用"字符数 × 估算宽度"算换行。这是后续所有
"和 Word 视觉一致" 目标的瓶颈。

**子任务**：

- [x] 把 FreeType 接入 `FeatherDoc::Pdf` 构建（默认仍 OFF，只在 `FEATHERDOC_BUILD_PDF=ON`
      时拉取）
- [x] 实现最小 `FontMetrics`：从字体文件读 ascent / descent / line gap / 每字形的
      advance width
- [x] 在 `PdfDocumentLayout` 里把字符宽度从估算改为真实 advance
- [x] 行高从固定值改为 `ascent + descent + line gap`
- [x] 给 `featherdoc_pdf_document_probe` 加一个真实字体测试用例

**验收**：换行位置和行高与字体度量一致，不再是估算。

### 优先级 2：CJK 字体嵌入 + ToUnicode CMap（约 1 个月）

**问题**：FeatherDoc 是中文项目，没有 CJK 字体嵌入就没有合格的 PDF 输出。当前 PDFio probe
没有写 ToUnicode，输出的中文 PDF**无法复制粘贴**。

**子任务**：

- [ ] 选定一个可随发行包分发的开源 CJK TTF（思源黑体 / 思源宋体 / Noto CJK 之一），列出许可证义务；
      当前测试先使用 `FEATHERDOC_TEST_CJK_FONT`、`FEATHERDOC_PDF_CJK_FONT` 或系统字体候选，
      不把字体文件重新分发进仓库
- [x] 通过 PDFio 实现字体子集嵌入（不嵌入整个 20 MB 字体）
- [x] 写 /ToUnicode CMap，使 PDFium 能回读正确 Unicode 文本
- [x] 中文 sample → PDF → 用 PDFium 反向提取文本，断言文本一致
- [x] 加 CTest：`pdf_unicode_font_roundtrip`、`pdf_document_adapter_font` 和
      `pdf_font_resolver` 覆盖 CJK 字体解析、子集化、ToUnicode 和回读

**关键依赖检查**：如果 PDFio 在 CJK 字体子集 + ToUnicode 上**不可用或不稳定**，触发
[dependencies/pdfio.md](dependencies/pdfio.md) §"放弃条件"中的"无法满足字体嵌入和 ToUnicode 的最低要求"，进入评估替代方案。**不允许**用"挖 LibreOffice"作为替代，详见
[00-vision.md](00-vision.md) §"学习 LibreOffice ≠ 替换开源依赖"。

**验收**：中文 PDF 在 Adobe Reader / Foxit / SumatraPDF 中文本可复制、可搜索。

### 优先级 3：样式映射（约 2–3 周）

**状态 / 问题**：基础样式映射已经贯通，`Run.bold` / `italic` / `font_size` /
`color` / `underline` 已能进入 layout 并由 PDFio writer 写出。剩余问题不再是字段断链，
而是合同级视觉 baseline、非标准字体粗斜体质量和发布门禁还没有收口。

**子任务**：

- [x] 扩展 `PdfDocumentLayout` 的 Run 表达：font weight、font style、font size、color、
      下划线
- [x] `layout_document_paragraphs()` 把 `Run` 字段和继承样式翻译进 layout
- [x] PDFio backend 在 content stream 里展开标准字体粗斜体、字号、颜色和下划线
- [x] 给 PDF document adapter / writer 加样式 sample 和 `pdf_document_adapter_font`
      回归
- [x] 补合同级样式视觉 baseline，把粗体 / 斜体 / 字号 / 颜色 / 下划线纳入
      `run_pdf_visual_release_gate.ps1` 的核心 PNG 渲染和聚合 contact sheet
- [x] 明确非标准字体缺少 bold / italic 变体时的质量策略：resolver 优先找
      style-specific 字体，缺失时显式标记 synthetic bold / italic，writer 使用
      fill+stroke 和斜切矩阵做最小视觉兜底

**验收**：基础段落和表格单元格的粗体 / 斜体 / 字号 / 颜色 / 下划线已有专项
CTest 覆盖；发布级视觉门禁已覆盖合同样式和样式矩阵样本；非标准字体缺少
bold / italic 变体时已有合成兜底和回归覆盖。优先级 3 基础验收完成，下一步进入
HarfBuzz 文字塑形。

### 优先级 4：HarfBuzz 文字塑形（约 1 个月）

**问题**：到优先级 3 完成后，简单中英文输出已可用。但 CJK 标点挤压、阿拉伯数字和中文混排
间距、连字（fi / fl）等场景需要 HarfBuzz。

**子任务**：

- [x] HarfBuzz 接入 `FeatherDoc::Pdf` 构建
- [x] 实现 `shaper_bridge`：输入 Unicode 字符串 + 字体，输出 GlyphRun（glyphId / xAdvance /
      xOffset / yOffset）；当前已落到独立 `pdf_text_shaper`，还未接入 layout / writer
- [x] `PdfDocumentLayout` 改为消费 GlyphRun 而非字符串；当前 `PdfTextRun` 已可携带
      `PdfGlyphRun`，layout 宽度、后续 run 坐标和受控 writer 路径已优先使用 glyph advance；
      原始字符串仍保留为 ToUnicode / ActualText 和 fallback 语义
- [x] PDFio backend 用 glyph ID 写出 content stream；当前实现为 file-backed shaped run
      创建独立 Type0 / CIDFontType2 资源，用私有 CID、CIDToGIDMap、ToUnicode 和 HarfBuzz
      advance 写出，并已用对象级 CMap 解压回归覆盖 shaped cluster 到 ToUnicode 的映射；
      非零 x/y offset 或 y advance 会逐 glyph 写 `Tm` + `Tj`；字号不匹配、cluster 越界或
      倒序、RTL / 竖排仍走字符串 fallback 或后续专项
- [x] 视觉回归 sample 集扩展到中英混排、CJK 标点用例；当前
      `mixed-cjk-punctuation-text` 和 `latin-ligature-text` 已进入 regression manifest
      和 PDF 视觉发布门禁 baseline

**验收**：CJK 标点挤压、连字、混排间距与 LibreOffice 输出对比可接受。

### 完成后的里程碑

四步全部完成时，应能拿出以下 demo：

> **拿一个真实中文合同 DOCX，用 `featherdoc_pdf_document_probe`（或新 CLI）输出 PDF，
> PDF 里的中文能复制粘贴、字体粗细对、字号对、行距对、CJK 标点正确。**

这个 demo 稳定后，Phase 1 正式收尾，进入 Phase 2「稳定 + 优化」。

### 不在本清单内的任务

以下任务**明确不在**当前下一步范围，不要被诱惑去做：

- ❌ 排版引擎（断行 / 分页 / 表格）的真实算法——属于 [03-long-term-roadmap.md](03-long-term-roadmap.md) 的 LR-1
- ❌ 自研 PDF writer 替换 PDFio——同上
- ❌ 从 LibreOffice 挖代码——任何时候都不允许，详见 [00-vision.md](00-vision.md)
- ❌ 接 Skia——见 [dependencies/skia.md](dependencies/skia.md)，长期评估方向不是当前任务
- ❌ 默认开启 PDF 模块——`FEATHERDOC_BUILD_PDF` 必须保持 OFF

## Phase 2：稳定 + 优化（3 - 6 个月）

### 目标

把 Phase 1 跑通的链路变得稳定，开始面对真实文档。

### 重点

Phase 2 的重点是：**能不能复现各种真实文档**。

这里的"复现"不是完全等同 Word / Adobe，而是在目标子集内稳定、可解释、可回归。

### 子任务

1. **修复边界问题**

   - 空页面
   - 大页面尺寸
   - 缺字体
   - CJK 文本
   - 多页文本流
   - 表格坐标误差
   - 图片尺寸和 DPI

2. **性能调优**

   - 避免重复解析字体
   - 避免整文档不必要拷贝
   - 大 PDF 分页处理
   - 输出路径和临时文件清理

3. **真实文档回归**

   - 持续增加样本
   - 每个 bug 都沉淀成测试
   - 保留中间 AST / Layout dump，方便定位问题

4. **翻译层质量提升**

   - Word → PDF：提升 LayoutResult 的表达能力
   - PDF → Word：提升行、段落、表格识别启发式
   - 记录不支持能力，不做静默失败

### 验收标准

- [ ] 常见样本无崩溃
- [ ] 文本不丢失
- [ ] 中文文本路径可跑通
- [ ] 主要边界问题有测试覆盖
- [ ] 性能瓶颈有基准测试记录

## Phase 3：选择性自研（6 个月+）

### 目标

只有当 PDFio / PDFium 的某个部分成为明确瓶颈时，才考虑选择性自研。

自研不是为了替换而替换，而是为了学习和掌握关键能力。

### 推荐替换顺序

优先考虑替换 PDFio。

理由：

- PDF 写出方向风险更小
- 输入数据由 Core 自己控制
- 更容易做 A/B 对比
- 可以学到 PDF object、xref、stream、font、ToUnicode 等核心知识
- 替换失败也可以继续保留 PDFio 实现

PDFium 不建议早期替换。PDF 解析和渲染生态复杂，自己维护成本高，学习收益不如先做写出层可控。

### 子任务

1. **保留接口**

   `IPdfGenerator` / `IPdfParser` 必须继续存在。自研实现只能作为新实现挂到接口后面。

2. **实现共存**

   ```text
   IPdfGenerator
      ├── PdfioGenerator
      └── NativePdfGenerator   ← Phase 3 才出现
   ```

3. **A/B 对比**

   同一个 `LayoutResult` 同时输出：

   - PDFio 版本
   - 自研版本

   对比：

   - 文件能否打开
   - 文本能否复制和搜索
   - 字体是否正确
   - 视觉是否退化
   - 文件体积是否异常

4. **逐步替换**

   不要一次性替换全部 PDF writer。推荐顺序：

   - PDF header / trailer / xref
   - page tree
   - content stream
   - base font
   - TrueType 字体嵌入
   - CJK 字体子集
   - ToUnicode

### 验收标准

- [ ] PDFio 和自研 writer 可共存
- [ ] 自研 writer 不影响默认构建
- [ ] 自研版本在测试集上不退化
- [ ] 可随时切回 PDFio 实现
- [ ] 学习结论沉淀到设计文档

## 不提前自研的内容

以下内容在 Phase 1 / Phase 2 不自研：

- PDF parser
- PDF renderer
- 字体引擎
- 文字塑形引擎
- 图像解码器
- 通用 Office 排版引擎

这些都应优先使用成熟库。Core 当前真正需要打磨的是文档模型和翻译层。

## 默认关闭要求

PDF 能力必须长期保持 opt-in：

- 默认不构建 `FeatherDoc::Pdf`
- 默认不拉取 PDFio
- 默认不拉取 PDFium
- 默认不安装实验性 PDF 头文件
- 默认不改变 `FeatherDoc::FeatherDoc` 依赖集合

## Owner

本方向负责人：wuxianggujun
