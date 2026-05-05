# PDFio 字节写出层

> 本文档定义 FeatherDoc Core 自研 PDF 路线中的**第一层落地目标**：
> 先拥有轻量、可替换、可商业分发的 PDF 字节生成能力。
>
> 阅读前置：[PDF 渲染策略](pdf-rendering-strategy.md)、
> [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)、
> [PDF 渲染架构](pdf-rendering-architecture.md)。

## 定位

PDFio 只负责 PDF 文件结构和字节写出，不负责 Word 排版，也不负责 PDF 预览。

它在 FeatherDoc PDF 路线中的位置是：

```text
DOCX 语义解析
   ↓
Layout Engine（后续自研）
   ↓
LayoutResult（纯数据）
   ↓
FeatherDoc PDF writer facade
   ↓
PDFio（PDF object / page / stream / font / image 写出）
   ↓
PDF 字节
```

因此，PDFio 是**输出容器层**，不是排版引擎。

在 Word ⇄ PDF 三件套中，PDFio 只负责 Word → PDF 的写出方向；PDF → Word 的读入方向由
PDFium 承担。

在当前路线里，PDFio 应挂在抽象接口后面：

```text
IPdfGenerator
   └── PdfioGenerator
```

也就是说，PDFio 是 Phase 1 的默认 PDF 生成实现，而不是 FeatherDoc 公开 API 的一部分。

## 为什么选择 PDFio

选择 PDFio 的主要原因：

1. **轻量**：它是 C 库，不是完整 PDF 渲染器或 Office 转换器。
2. **职责准确**：它处理 PDF object、page、stream、xref、字体和图片写入。
3. **许可证友好**：Apache-2.0 路线适合商业使用和再分发。
4. **可替换**：FeatherDoc 公开 API 不暴露 PDFio 类型，后续可替换为自研 writer。
5. **工程风险低**：比 Skia / PDFium / LibreOffice 更适合作为 Core 的第一步 PDF 依赖。

## 不选择大后端作为第一步

当前阶段不把以下项目作为 Core 的第一层 PDF 字节生成依赖：

- Skia / SkPDF：适合未来图形后端或复杂绘制，但对“先写 PDF 字节”偏重。
- PDFium：主要价值在 PDF 渲染、预览和解析，不是轻量生成层。
- LibreOffice：是完整 Office 转换器，不应嵌入 Core。
- MuPDF / Poppler：许可证和分发约束不适合当前目标。
- libharu：许可证友好，但 API 和维护状态不如 PDFio 更适合作为第一选择。

## 集成原则

### 原则 1：默认不启用

PDFio 通过独立 CMake 开关启用：

```cmake
option(FEATHERDOC_BUILD_PDF "Build experimental PDF byte writer module" OFF)
```

默认构建仍然只包含 FeatherDoc DOCX Core。

### 原则 2：不污染 Core 主库

启用 PDF 模块时，构建独立目标：

```text
FeatherDoc::FeatherDoc  ← 现有 DOCX Core
FeatherDoc::Pdf         ← 实验性 PDF 字节写出模块
```

`FeatherDoc::FeatherDoc` 不链接 PDFio。

### 原则 3：公开 API 不暴露 PDFio

公开头文件只暴露 FeatherDoc 自己的类型：

```cpp
namespace featherdoc::pdf {

struct PdfWriterOptions;
struct PdfWriteResult;

PdfWriteResult write_pdfio_probe_document(
    const std::filesystem::path &output_path,
    const PdfWriterOptions &options = {});

}  // namespace featherdoc::pdf
```

`pdfio_file_t`、`pdfio_stream_t` 等类型只允许出现在 `.cpp` 文件。

### 原则 4：先验证字节写出，再接 Layout

当前第一步只做 probe：

- 创建 PDF 文件
- 创建页面
- 写入基础字体
- 写入文本和简单图形
- 关闭 stream / document

这一步完成后，再把 `LayoutResult` 翻译成 PDFio content stream。

## 当前接入边界

当前只承诺：

- 可选拉取 / 指定 PDFio 源码
- 构建实验性 `FeatherDoc::Pdf`
- 输出一个最小 probe PDF
- 验证 PDFio 字节写出链路能接入 FeatherDoc 工程
- `PdfioGenerator` 已作为 `IPdfGenerator` 的第一版实现
- probe 已改为通过最小 PDF layout 纯数据结构写出

当前不承诺：

- DOCX 到 PDF 的真实排版
- CJK 字体嵌入和 ToUnicode CMap 完整处理
- Word 视觉一致性
- PDF/A
- PDF 内容编辑
- PDF 预览或渲染

## 后续演进

推荐按下面顺序推进：

1. PDFio probe 输出可稳定构建。
2. 定义 `LayoutResult` 的最小页面 / 文本 / 矩形模型。
3. 实现 `LayoutResult -> PDFio content stream`。
4. 接入字体文件嵌入和 ToUnicode。
5. 接入图片对象。
6. 再开始 DOCX 合同子集 Layout。

## 启动条件

进入真实 DOCX-to-PDF 实施前，至少满足：

- [ ] `FeatherDoc::Pdf` 可在 Windows / Linux / macOS 配置和构建
- [ ] probe PDF 可被常见 PDF 阅读器打开
- [ ] PDFio 许可证和 NOTICE 已进入 FeatherDoc 第三方依赖清单
- [ ] 已定义最小 `LayoutResult` 数据结构
- [ ] 已明确第一阶段只覆盖合同子集

## 放弃条件

满足以下任一时，应停止 PDFio 路线并评估自研 writer 或其他后端：

- [ ] PDFio 无法满足字体嵌入和 ToUnicode 的最低要求
- [ ] PDFio 的构建和分发成本超过自研最小 writer
- [ ] PDFio API 迫使 FeatherDoc 公开 API 泄漏后端类型
- [ ] PDFio 上游维护状态不再满足长期使用要求

## Owner

本方向负责人：wuxianggujun

状态：实验性接入，默认关闭。
