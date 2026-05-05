# Word ⇄ PDF 转换器路线

> 本文档定义 FeatherDoc Core 的 Word ⇄ PDF 双向转换推进路线。
>
> 阅读前置：[Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)。

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

   准备 10-20 个样本，覆盖：

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
- [ ] 10-20 个 PDF 样本进入回归测试

### 当前实现进展

- [x] 默认构建保持 PDFio / PDFium 全部关闭
- [x] 已定义 `IPdfGenerator` / `IPdfParser`
- [x] 已定义最小 PDF layout 纯数据结构
- [x] `PdfioGenerator` 已作为 `IPdfGenerator` 的第一版实现
- [x] `featherdoc_pdfio_probe` 已改为通过接口写出 PDF
- [x] 已新增 `PdfiumParser` 作为 `IPdfParser` 的第一版实现
- [x] `FEATHERDOC_BUILD_PDF_IMPORT` 默认关闭，开启时默认使用 PDFium 源码 checkout
- [x] 已新增 `cmake/FeatherDocPdfium.cmake`，通过 GN/Ninja 构建 PDFium 源码
- [x] 已新增 [PDFium 源码构建接入](pdfium-source-build.md)
- [x] 已接入本机 PDFium 源码 checkout 并验证 `featherdoc_pdfium_probe`
- [x] 已用 PDFio probe 生成的 PDF 验证 PDFium 可提取页面和 text spans
- [x] 同时开启 PDFio / PDFium 时，已有 `pdfium_parser_probe` CTest 覆盖写出后再读入
- [ ] 10-20 个 PDF 样本尚未建立

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
