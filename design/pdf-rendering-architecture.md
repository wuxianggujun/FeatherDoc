# PDF 渲染架构

> 本文档定义 FeatherDoc Core **自研 PDF 渲染**的整体架构原则。
>
> 阅读前置：[PDF 渲染策略](pdf-rendering-strategy.md)、
> [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)。

## 核心原则：Layout 与 Renderer 解耦

这是整个 PDF 渲染能力能演进的地基。

```text
DOCX 文件
   │
   ▼
OOXML 解析 (复用 Core 现有 pugixml + zip)
   │
   ▼
DocumentModel (Core 已有, 可能扩展)
   │
   ▼
┌──────────────────────────────────────┐
│ Layout Engine                         │
│   - 纯算法，不依赖任何渲染后端          │
│   - 输入：DocumentModel               │
│   - 输出：LayoutResult（纯数据）       │
└──────────────────────────────────────┘
   │
   ▼
LayoutResult (纯数据, 见下文结构)
   │
   ├──────────────┬───────────────┬──────────────┐
   ▼              ▼               ▼              ▼
┌────────┐   ┌────────┐    ┌────────┐    ┌────────┐
│ PDF     │   │ PNG/    │    │ SVG     │    │ Future  │
│ Backend │   │ Image   │    │ Backend │    │ Backend │
│(PDFio)  │   │ Backend │    │         │    │         │
└────────┘   └────────┘    └────────┘    └────────┘
```

### 关键约束

- **LayoutResult 是纯数据**：不引用 `Canvas`、不引用 `SkCanvas`、不引用任何 UI 框架类型
- **Renderer 可插拔**：实现 `IDocumentRenderer` 接口即可接入新后端
- **Layout 只跑一次**：同一棵 layout 树可被多个 Renderer 消费，保证一致性
- **Layout 可序列化**：调试时可 dump 为 JSON / 二进制，方便回放

## 数据模型

### LayoutResult 结构草案

```cpp
// include/featherdoc/render/layout_result.hpp

namespace featherdoc::render {

struct LayoutResult {
  std::vector<PageBox> pages;
  DocumentMetadata metadata;
};

struct PageBox {
  Size size;                    // 页面尺寸（pt）
  HeaderArea header;
  ContentArea content;
  FooterArea footer;
};

struct ContentArea {
  Rect bounds;
  std::vector<BlockBox> blocks;
};

struct BlockBox {
  enum class Kind { Paragraph, Table, Image, List };
  Kind kind;
  Rect bounds;
  // ... 子节点
};

struct LineBox {
  std::vector<GlyphRun> runs;
  float baseline;
  Rect bounds;
};

struct GlyphRun {
  std::vector<Glyph> glyphs;    // glyphId, xAdvance, xOffset, yOffset
  FontHandle font;
  Color color;
  std::string sourceText;       // 原始 Unicode，用于 PDF 文本提取
};

}  // namespace featherdoc::render
```

### GlyphRun 必带 sourceText

PDF 输出时如果只有字形 ID 没有原始 Unicode，文本就**不可被选中或搜索**。这是自研 PDF 路线的核心卖点之一，必须从 Layout 阶段就保留。

## 模块划分

### Core 现有模块（不动）

```text
src/
├── parsing/      ← OOXML 解析（已有）
├── doc/          ← DocumentModel（已有）
├── content/      ← 内容操作（已有：fill, replace, revision）
└── ...
```

### 新增模块（PDF 渲染）

```text
src/
└── render/                   ← 新增，全部在此目录下
    ├── layout/               ← Layout 引擎
    │   ├── line_breaker.cpp
    │   ├── paragraph_layout.cpp
    │   ├── table_layout.cpp
    │   ├── page_breaker.cpp
    │   └── shaper_bridge.cpp ← 调 HarfBuzz
    ├── font/                 ← 字体管理
    │   ├── font_manager.cpp
    │   ├── font_metrics.cpp
    │   └── freetype_loader.cpp
    ├── backend/              ← 各种 Renderer 后端
    │   ├── pdf_pdfio.cpp     ← 第一阶段：PDFio 字节写出
    │   ├── pdf_skia.cpp      ← 未来复杂绘制后端评估
    │   └── png_skia.cpp      ← 可选
    └── api/                  ← 对外 API
        ├── render_options.cpp
        └── render_engine.cpp ← 入口

include/
└── featherdoc/render/        ← 公开头文件
    ├── layout_result.hpp
    ├── render_options.hpp
    ├── render_engine.hpp
    └── i_document_renderer.hpp
```

### 编译开关

PDF writer / render 模块默认**不编译**，必须显式启用：

```cmake
option(FEATHERDOC_BUILD_PDF "Build experimental PDF byte writer module" OFF)

if(FEATHERDOC_BUILD_PDF)
  # 构建 FeatherDoc::Pdf，底层当前使用 PDFio。
endif()
```

这样 Core 不带 PDF 渲染时仍然轻量，下游按需启用。

### 第一阶段目标：PDFio writer

第一阶段先接入 PDFio，不直接接入 Skia / PDFium / LibreOffice。

目标是先验证 Core 可以自己写出 PDF 字节：

- 创建 PDF document / page / stream
- 写入基础字体、文本和简单图形
- 后续把 `LayoutResult` 翻译成 PDF content stream

详见 [PDFio 字节写出层](pdfio-byte-writer.md)。

### 与 PDFium 读入方向的关系

本文只描述 Word → PDF 的正向渲染架构。

PDFium 用于 PDF → Word 的读入方向，后续应放在独立模块中，例如：

```text
src/
└── pdf_import/
    ├── pdfium_document.cpp
    ├── text_extractor.cpp
    ├── layout_analyzer.cpp
    └── ast_builder.cpp
```

该模块输出的仍然应是 Core 的 AST / DOM，不能让 `FPDF_*` 类型泄漏到公开 API。

## 公开 API

### C++ API（同进程嵌入）

```cpp
#include <featherdoc/render/render_engine.hpp>

featherdoc::Document doc = featherdoc::open("contract.docx");

featherdoc::render::RenderOptions options;
options.profile = featherdoc::render::Profile::Contract;  // 用合同子集
options.output_format = featherdoc::render::Format::Pdf;
options.embed_fonts = true;

featherdoc::render::RenderEngine engine;
auto result = engine.render(doc, "out.pdf", options);

if (!result.success) {
  std::cerr << result.error_message << std::endl;
}
```

### CLI 命令

```bash
featherdoc_cli render-pdf input.docx \
  --output out.pdf \
  --profile contract \
  --embed-fonts \
  --json
```

输出 JSON 格式：

```json
{
  "success": true,
  "output_path": "out.pdf",
  "duration_ms": 234,
  "pages": 3,
  "warnings": []
}
```

## 与 Core 现有架构的集成

### 不影响现有命令

`render-pdf` 是新增子命令，**不修改**任何现有命令：

- `fill-bookmarks` 仍然只输出 DOCX
- `find-text-ranges` 不变
- `build-review-mutation-plan` 等修订命令不变
- 测试用例不破坏

### DocumentModel 可能扩展

Layout 需要的语义信息可能比现有 DocumentModel 更细：

- 段落的 keep-with-next / keep-together 属性
- 表格的 widow / orphan 控制
- 字符级的 east-asian punctuation 提示

这些扩展应**保持向后兼容**，仅在需要时加字段，不删旧字段。

## 错误处理

### 失败类别

```cpp
enum class RenderFailureKind {
  ParseError,         // OOXML 解析失败
  UnsupportedFeature, // 当前 Profile 不支持的特性
  FontMissing,        // 缺字体且无回退
  LayoutFailed,       // Layout 算法异常
  BackendFailed,      // PDF 后端写入失败
  OutputIoError,      // 文件 I/O 错误
};
```

### 不支持特性的处理策略

遇到当前 Profile 不支持的 OOXML 特性时（如 Phase 1 的合同子集遇到了 SmartArt）：

| 策略 | 行为 |
|---|---|
| `Strict` | 立即报错，返回 `UnsupportedFeature` |
| `BestEffort`（默认） | 跳过该特性，记录 warning，继续渲染其他内容 |
| `Fallback` | 整个文档降级，建议下游用 LibreOffice |

由 `RenderOptions::unsupported_strategy` 控制。

## 测试策略

### 三层测试

1. **单元测试**：每个 Layout 子算法（断行、分页、表格列宽）独立测试
2. **集成测试**：完整 OOXML→PDF 端到端，输出 PDF 文件
3. **视觉回归**：把 PDF 渲染成 PNG，与基线图对比像素差异

### 视觉回归触发

任何 Layout 或 Renderer 修改都必须跑视觉回归。差异超过阈值（如 1%）必须人工 review 后才能 merge。

### 与 Word 对比

每个 Profile（合同 / 论文 / 项目）维护一份 sample DOCX 集，分别用：

- Core 自研渲染
- LibreOffice headless
- Microsoft Word（可选）

三方输出对比，记录差异点。

## 性能目标

| 指标 | Phase 1 | Phase 5 |
|---|---|---|
| 1 页 docx → pdf | < 200 ms | < 50 ms |
| 100 页 docx → pdf | < 5 s | < 2 s |
| 内存占用（典型） | < 300 MB | < 200 MB |
| 与 LibreOffice 性能比 | ≥ 5× 快 | ≥ 10× 快 |

## 不允许的设计

- ❌ Layout 引擎里直接调 SkCanvas
- ❌ Renderer 反向修改 LayoutResult
- ❌ render 模块依赖任何 Core 内容操作之外的现有模块
- ❌ render 模块强制成为 Core 主二进制依赖
- ❌ 公开 API 泄漏 `pdfio_file_t` / `pdfio_stream_t` 等后端类型
- ❌ 公开 API 泄漏 `FPDF_DOCUMENT`、`FPDF_PAGE` 等 PDFium 后端类型
- ❌ 测试用例直接对比 PDF 字节（应对比 LayoutResult + 视觉）

## 相关文档

- [PDF 渲染策略](pdf-rendering-strategy.md)
- [PDF 渲染路线](pdf-rendering-roadmap.md)
- [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)
- [PDFio 字节写出层](pdfio-byte-writer.md)
- [Skia PDF 后端](skia-pdf-backend.md)
- [OOXML 子集覆盖](oxml-subset-coverage.md)
