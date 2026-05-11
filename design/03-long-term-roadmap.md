# 长期渲染愿景：自研 PDF 渲染路线

> 本文档汇总 FeatherDoc Core **自研 PDF 渲染（Word → PDF 正向）**的长期研究方向。
>
> ⚠️ **本文不在当前产品 roadmap**，也不阻塞 Core 现有特性发布。
>
> 当前主路线见 [02-current-roadmap.md](02-current-roadmap.md)（依赖优先，0–6 个月）。
> 长期决策依据见 [00-vision.md](00-vision.md)。
>
> 本文合并自原 `pdf-rendering-strategy.md` / `pdf-rendering-architecture.md` /
> `pdf-rendering-roadmap.md` 三篇文档（已在重整中合并）。

## 1. 当前定位

| 项目 | 状态 |
|---|---|
| Core 现有能力 | DOCX 内容操作（pugixml + zip）— 填模板、批量替换、修订处理 |
| 本研究方向 | 在 Core 中增加 DOCX 子集 Layout 引擎 + PDF 字节输出后端 |
| 本研究阶段 | **依赖优先**：先用 PDFio / PDFium 跑通端到端，再优化翻译层 |
| 与反向转换关系 | 本文只覆盖 Word → PDF；PDF → Word 由 PDFium → AST 适配层另行研究 |
| 启动门槛 | 见本文 §10「启动前置条件」 |

## 2. 战略意图

### 2.1 为什么 Core 可能要做这件事

Core 当前只做 DOCX 内容操作，输出仍然是 DOCX。下游需要 PDF 时，必须依赖外部 Office
（LibreOffice / Microsoft Word / Aspose）来完成 DOCX→PDF 转换。

外部依赖带来三个长期问题：

1. **性能瓶颈**：外部 Office 启动开销大（5–10 秒），批量处理被拖慢两个数量级
2. **依赖膨胀**：需要 bundle 或要求安装 ~250 MB 的 LibreOffice
3. **可控性弱**：渲染结果由外部决定，Core 无法精准控制输出

如果 Core 自带渲染能力：

- 简单文档（合同、论文）启动即输出，无外部依赖
- 复杂文档仍可降级到外部 Office
- Core 二进制保持轻量，PDF 子模块可选编译

### 2.2 为什么这件事很难

OOXML 排版引擎是 30–50 人年级别的工程。LibreOffice 用了 22 年仍在迭代。完整覆盖 OOXML
不现实，本研究方向只追求**子集覆盖**。详细难度分析见 [00-vision.md](00-vision.md)。

## 3. 范围限定

### 3.1 本研究方向**包括**

- OOXML 子集解析（已有部分能力，但渲染场景需要更多语义信息）
- Layout 引擎：把 OOXML 结构翻译成"画在第几页第几行第几个坐标"的具体指令
- PDF writer 后端：把 Layout 结果输出成 PDF object / page / stream 字节
- 字体子集嵌入与文本可选中性

### 3.2 本研究方向**不包括**

- ❌ 通用 OOXML 全特性支持（永远做不完）
- ❌ 完整替代 LibreOffice / Word（业务上没必要）
- ❌ 交互式编辑（Core 不是 Office 软件）
- ❌ 反向：PDF → DOCX（属于 PDFium → AST 的另一条研究线）
- ❌ 数学公式（OMML）、图表（chart）、SmartArt 等极复杂特性的渲染
- ❌ 把 LibreOffice 的排版引擎或 PDF 写出器"挖"进项目（详见 [00-vision.md](00-vision.md)
  「学习 LibreOffice ≠ 替换开源依赖」一节）

## 4. 核心架构原则：Layout 与 Renderer 解耦

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
│ Layout Engine                        │
│   - 纯算法，不依赖任何渲染后端         │
│   - 输入：DocumentModel              │
│   - 输出：LayoutResult（纯数据）       │
└──────────────────────────────────────┘
   │
   ▼
LayoutResult (纯数据)
   │
   ├──────────────┬───────────────┬──────────────┐
   ▼              ▼               ▼              ▼
┌────────┐   ┌────────┐    ┌────────┐    ┌────────┐
│ PDF    │   │ PNG/   │    │ SVG    │    │ Future │
│Backend │   │ Image  │    │Backend │    │Backend │
│(PDFio) │   │Backend │    │        │    │        │
└────────┘   └────────┘    └────────┘    └────────┘
```

### 4.1 关键约束

- **LayoutResult 是纯数据**：不引用 `Canvas`、不引用 `SkCanvas`、不引用任何 UI 框架类型
- **Renderer 可插拔**：实现 `IDocumentRenderer` 接口即可接入新后端
- **Layout 只跑一次**：同一棵 layout 树可被多个 Renderer 消费，保证一致性
- **Layout 可序列化**：调试时可 dump 为 JSON / 二进制，方便回放

## 5. 设计原则

### 原则 1：Layout 与 Renderer 解耦
见 §4。

### 原则 2：渐进式子集覆盖

不追求一次到位，按使用场景从小到大覆盖：

```text
合同子集 → 论文子集 → 项目文档子集 → 通用子集
```

每个子集独立可发布。详见 [references/oxml-subset.md](references/oxml-subset.md)。

### 原则 3：复用现成基础库

不重写已经有人做好的东西：

| 能力 | 借用 | 自写 |
|---|---|---|
| ZIP 解压 | miniz / libzip | — |
| XML 解析 | pugixml | — |
| 字体度量 | FreeType | — |
| 文字塑形 | HarfBuzz | — |
| 国际化 / Unicode | ICU | — |
| PDF 对象 / 字节写出 | PDFio | — |
| 未来复杂绘制后端 | Skia + SkPDF（待评估） | — |
| **OOXML 语义层** | — | ✅ |
| **Layout 算法** | — | ✅ |

第一步详见 [dependencies/pdfio.md](dependencies/pdfio.md)。Skia 评估方向详见
[dependencies/skia.md](dependencies/skia.md)。

### 原则 4：保持 Core 二进制轻量

PDF 渲染能力作为**可选编译模块**，不强制链接到 Core 主二进制。

### 原则 5：永远不阻塞产品

Core 当前 CLI 和当前下游产品节奏**不依赖**本方向完成。如果 5 年内本方向没出成果，Core 仍按现有路线持续输出价值。

## 6. 数据模型

### 6.1 LayoutResult 结构草案

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

### 6.2 GlyphRun 必带 sourceText

PDF 输出时如果只有字形 ID 没有原始 Unicode，文本就**不可被选中或搜索**。这是自研 PDF 路线
的核心卖点之一，必须从 Layout 阶段就保留。

## 7. 模块划分

```text
src/
└── render/                  ← 新增，PDF 渲染全部在此目录下
    ├── layout/              ← Layout 引擎
    │   ├── line_breaker.cpp
    │   ├── paragraph_layout.cpp
    │   ├── table_layout.cpp
    │   ├── page_breaker.cpp
    │   └── shaper_bridge.cpp ← 调 HarfBuzz
    ├── font/                ← 字体管理
    │   ├── font_manager.cpp
    │   ├── font_metrics.cpp
    │   └── freetype_loader.cpp
    ├── backend/             ← 各种 Renderer 后端
    │   ├── pdf_pdfio.cpp    ← 第一阶段：PDFio 字节写出
    │   ├── pdf_skia.cpp     ← 未来评估
    │   └── png_skia.cpp     ← 可选
    └── api/                 ← 对外 API
        └── render_engine.cpp

include/featherdoc/render/   ← 公开头文件（不暴露后端类型）
```

PDF 模块默认**不编译**：

```cmake
option(FEATHERDOC_BUILD_PDF "Build experimental PDF byte writer module" OFF)
```

## 8. 公开 API（草案）

### 8.1 C++ API

```cpp
#include <featherdoc/render/render_engine.hpp>

featherdoc::Document doc = featherdoc::open("contract.docx");

featherdoc::render::RenderOptions options;
options.profile = featherdoc::render::Profile::Contract;
options.output_format = featherdoc::render::Format::Pdf;
options.embed_fonts = true;

featherdoc::render::RenderEngine engine;
auto result = engine.render(doc, "out.pdf", options);
```

### 8.2 CLI

```bash
featherdoc_cli render-pdf input.docx \
  --output out.pdf \
  --profile contract \
  --embed-fonts \
  --json
```

## 9. 阶段划分（长期）

为避免和当前主路线（[02-current-roadmap.md](02-current-roadmap.md)）的 `Phase 1/2/3`
撞车，本路线的阶段统一加 `LR-` 前缀（Long-term Rendering）。

```text
LR-0: PDF writer 基础接入       ← 已并入主路线 Phase 1，本文不再单独追踪
LR-1: 合同子集 — 最小可用版本    （启动后 3–6 个月）
LR-2: 论文子集 — 结构化文档     （+3–6 个月）
LR-3: 项目文档子集 — 图表/列表  （+3–6 个月）
LR-4: 通用 OOXML 子集          （长期）
LR-5: 性能与质量打磨            （持续）
```

每个阶段都是**独立可发布的**，覆盖一类业务场景。

### 9.1 LR-1：合同子集（启动后 3–6 个月，1 人专职）

**目标**：让 Core 能渲染**自身生成的简单合同 DOCX**为 PDF。

支持：

- 段落（对齐、缩进、行距、字体、字号、加粗、斜体、颜色、下划线）
- 简单表格（无嵌套、无跨页拆分、无合并单元格）
- 段落级项目符号（无多级编号）
- 静态页眉、页脚（不含字段域）
- 基础图片嵌入（PNG / JPEG，固定大小、不浮动环绕）
- 书签

不支持：复杂表格、浮动图文环绕、多级列表、字段域、修订标记、复杂分节、数学公式、SmartArt /
Chart。

**子任务**：

- [ ] OOXML 子集语义解析（`include/featherdoc/render/`）
- [ ] Layout 引擎合同子集（`src/render/layout/`）
- [ ] 字体集成：FreeType + HarfBuzz
- [ ] PDF 输出：先使用 PDFio 写出 content stream
- [ ] CLI 命令 `render-pdf`
- [ ] 集成测试：用 sample 合同模板填充后渲染，对比 Word/LibreOffice 输出
- [ ] 视觉回归测试：差异低于阈值才允许 merge

**验收标准**：

- 渲染 sample 合同 DOCX 不丢任何字符
- 字体度量与 Word 输出差异 ≤ 5%
- 渲染速度比 LibreOffice 快 ≥ 5 倍
- A/B 视觉测试用户接受度 ≥ 80%

**工时估算**：9–14 个月（1 人专职），2 人专职可压到 6–10 个月。

### 9.2 LR-2 / LR-3 / LR-4 / LR-5（提纲）

| 阶段 | 增量范围 | 工时 |
|---|---|---|
| LR-2 论文子集 | 标题样式 / 多级列表 / 脚注 / 目录占位符 / 简单分节 | +3–6 月 |
| LR-3 项目文档子集 | 复杂表格（合并/跨页）/ 列表表格嵌套 / EMF/WMF 矢量 / OMML 简化公式 | +3–6 月 |
| LR-4 通用 OOXML 子集 | 浮动图文环绕 / 复杂分节 / 字段域 / 修订标记可视化 | 长期 |
| LR-5 性能与质量打磨 | 100+ 页 < 5 秒；内存 < 200 MB；Word 视觉一致性 > 95% | 持续 |

## 10. 启动前置条件

本研究方向**不会**因为"想做"就启动实施。只有以下条件**全部**满足才进入实施阶段：

- [ ] Core 现有能力（DOCX 内容操作）已稳定
- [ ] Core 有持续的下游商业用户
- [ ] 团队规模到 3+ 人，且至少 1 人能专职 C++ / 排版工程
- [ ] LibreOffice / Aspose 等外部依赖明确成为业务瓶颈
- [ ] 有持续 1–2 年的工程投入预算
- [ ] 已完成主路线 [02-current-roadmap.md](02-current-roadmap.md) 的 Phase 1 / Phase 2

## 11. 放弃条件

满足以下任一时，本方向应被**正式归档**而非半死不活地拖着：

- [ ] 主要下游用户都不再需要 PDF 输出
- [ ] LibreOffice headless 已经能完美满足下游场景，性能不再是瓶颈
- [ ] 团队规模长期不到 3 人
- [ ] 投入 2 年仍未做出最小可用版本

## 12. 风险与应对

### 风险 1：基础库集成成本超预期

Skia 用 GN + Ninja 构建，与 Core 的 CMake 不直接兼容。HarfBuzz / FreeType / ICU 在 Windows
上的工具链匹配也复杂。

**应对**：先在主路线接入 PDFio writer 验证最小字节写出链路；Skia 只保留为复杂绘制后端
评估；遇到无法解决的问题再考虑自研 writer 或其他轻量后端。

### 风险 2：与 Word 视觉差异不可接受

排版细节差异（行距、字距、CJK 标点处理）可能让用户觉得"不像 Word 排的"。

**应对**：LR-1 验收前必须做 A/B 测试；接受一定差异作为代价；提供 fallback 到 LibreOffice 的开关。

### 风险 3：维护成本失控

OOXML 标准持续演进，子集覆盖容易陷入"修不完的 bug"。

**应对**：严格控制子集边界；用户反馈触发新特性而非主动追求；每阶段必须有明确收尾标准。

### 风险 4：人手不足，进度停滞

1 人长期投入 9–14 个月，期间无法兼顾其他工作。

**应对**：启动前必须确认资源；不允许"业余时间慢慢做"；如人手不足直接归档不启动。

## 13. 错误处理

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

不支持特性的处理策略由 `RenderOptions::unsupported_strategy` 控制：

| 策略 | 行为 |
|---|---|
| `Strict` | 立即报错，返回 `UnsupportedFeature` |
| `BestEffort`（默认） | 跳过该特性，记录 warning，继续渲染其他内容 |
| `Fallback` | 整个文档降级，建议下游用 LibreOffice |

## 14. 测试策略

1. **单元测试**：每个 Layout 子算法（断行、分页、表格列宽）独立测试
2. **集成测试**：完整 OOXML→PDF 端到端，输出 PDF 文件
3. **视觉回归**：把 PDF 渲染成 PNG，与基线图对比像素差异

每个 Profile 维护一份 sample DOCX 集，同时跑：Core 自研渲染 / LibreOffice headless /
Microsoft Word（可选）三方输出对比。

## 15. 性能目标

| 指标 | LR-1 | LR-5 |
|---|---|---|
| 1 页 docx → pdf | < 200 ms | < 50 ms |
| 100 页 docx → pdf | < 5 s | < 2 s |
| 内存占用（典型） | < 300 MB | < 200 MB |
| 与 LibreOffice 性能比 | ≥ 5× 快 | ≥ 10× 快 |

## 16. 不允许的设计

- ❌ Layout 引擎里直接调 SkCanvas
- ❌ Renderer 反向修改 LayoutResult
- ❌ render 模块依赖任何 Core 内容操作之外的现有模块
- ❌ render 模块强制成为 Core 主二进制依赖
- ❌ 公开 API 泄漏 `pdfio_file_t` / `pdfio_stream_t` 等后端类型
- ❌ 公开 API 泄漏 `FPDF_DOCUMENT`、`FPDF_PAGE` 等 PDFium 后端类型
- ❌ 测试用例直接对比 PDF 字节（应对比 LayoutResult + 视觉）
- ❌ 把 LibreOffice 的排版/PDF 子系统挖进项目（详见 [00-vision.md](00-vision.md)）

## 17. 成功指标

本方向被认为"成功"的标志：

- [ ] 至少一个真实业务场景（如合同生成）的 PDF 输出可由 Core 自身完成
- [ ] 该场景下 Core 渲染速度比 LibreOffice 快至少 5 倍
- [ ] 用户视觉接受度 ≥ 80%（A/B 测试 vs Word 输出）
- [ ] Core 二进制不强制变重（可选模块）

## 18. 与当前主路线的关系

| 维度 | 当前主路线（02-current-roadmap） | 本文（长期愿景） |
|---|---|---|
| 时间窗口 | 0–6 个月 | 6 个月+ |
| 阶段编号 | Phase 1 / 2 / 3 | LR-1 / 2 / 3 / 4 / 5 |
| 策略 | 依赖优先（PDFio / PDFium 用好） | 选择性自研（先 writer，再视情况） |
| 阻塞性 | 阻塞当前 PDF 能力发布 | 不阻塞 |
| 文档地位 | 滚动更新 | 启动前置满足才推进，否则归档 |

主路线把"用好 PDFio / PDFium" 跑通，本路线才有意义。**先有当前主路线，再有本文。**

## 19. 相关文档

- [00-vision.md](00-vision.md) — 自研还是外购，及"为什么不挖 LibreOffice"
- [01-architecture.md](01-architecture.md) — 三件套（docs + PDFio + PDFium）总体架构
- [02-current-roadmap.md](02-current-roadmap.md) — 当前主路线
- [dependencies/pdfio.md](dependencies/pdfio.md) — PDFio 接入和能力评估
- [dependencies/pdfium.md](dependencies/pdfium.md) — PDFium 源码构建接入
- [dependencies/skia.md](dependencies/skia.md) — Skia + SkPDF 评估
- [references/oxml-subset.md](references/oxml-subset.md) — OOXML 各阶段子集覆盖矩阵

## 20. 文档维护

每次本研究方向状态变化（启动 / 推进阶段 / 归档）必须同步更新本文。

启动 LR-N 时，对应章节要补"实际启动日期"和"实际负责人"。

## Owner

本方向负责人：wuxianggujun

每次评估本方向状态时，必须更新 §1「当前定位」和 §10「启动前置条件」。
