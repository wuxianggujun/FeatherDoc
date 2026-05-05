# FeatherDoc Core 设计文档

> 本目录存放 FeatherDoc Core 的**前瞻性设计文档**，记录尚未实施或处于研究阶段的方向。
>
> 已实施特性的开发者文档在 `docs/`（Sphinx 系统）。本目录与 `docs/` 解耦，避免把"还在想"和"已经做"混在一起。

## 当前在研方向

### Word ⇄ PDF 转换器

Core 长期方向之一是探索 **Word ⇄ PDF 双向转换器**。整体方案采用三件套：

- 自研 docs 库：负责 Word 文档解析 / 生成，并定义内部 AST / DOM
- PDFio：负责 PDF 生成（写出方向）
- PDFium：负责 PDF 解析（读入方向）

当前重点不是一上来重写 PDF 引擎，而是**好好使用 PDFio / PDFium**，先跑通端到端流程。

FeatherDoc 真正需要长期打磨的是 docs 库和两个翻译层：`AST → PDFio`、`PDFium → AST`。
选择性自研应放到 6 个月之后，并且优先从 PDF 写出层开始。

当前第一步不是引入完整渲染器，而是先接入轻量 PDF 字节写出层：`FeatherDoc::Pdf` 默认关闭，底层先使用 PDFio 生成 PDF object / page / stream / font / image 字节。

相关文档：

- [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md) — docs 库 + PDFio + PDFium 的双向转换总览
- [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md) — 依赖优先、接口隔离、后期选择性自研
- [PDFium 源码构建接入](pdfium-source-build.md) — PDFium 源码 checkout、GN/Ninja 和 CMake provider
- [PDF 构建手册](../BUILDING_PDF.md) — PDFio / PDFium 本地编译、smoke 测试和不提交生成物清单
- [PDF 渲染策略](pdf-rendering-strategy.md) — 高层定位、范围、决策原则
- [PDF 渲染路线](pdf-rendering-roadmap.md) — 分阶段落地计划
- [PDF 渲染架构](pdf-rendering-architecture.md) — Layout / Renderer 解耦原则
- [PDFio 字节写出层](pdfio-byte-writer.md) — 轻量 PDF writer 的第一步接入
- [Skia PDF 后端](skia-pdf-backend.md) — Skia + SkPDF 集成细节
- [OOXML 子集覆盖](oxml-subset-coverage.md) — 各阶段覆盖的 OOXML 特性
- [自研还是外购](why-not-buy.md) — 为什么不直接用 LibreOffice/Aspose

## 设计文档约束

放进本目录的文档必须满足：

1. **前瞻性**：尚未实施或在研究阶段（实施完毕的迁到 `docs/`）
2. **Core 视角**：从 Core 自身的角度描述，不依赖任何下游消费者
3. **可演进**：每个研究方向的主控文档必须写明"启动条件"和"放弃条件"；支撑文档必须能回链到主控文档
4. **小步可验证**：阶段切分能让阶段产出独立可用，不要"做完才能用"

## 不在本目录的内容

以下不属于 design/ 范围：

- ✗ Core 的 API 文档（在 `docs/`）
- ✗ Core 的发布说明（CHANGELOG.md）
- ✗ Core 的构建说明（README.md / 各种 BUILDING）
- ✗ 第三方系统的设计文档（如 FeatherDoc Studio 自己的 PDF 策略，那在 Studio 仓库）

## 维护者

本目录文档由 Core 长期方向负责人维护。每个研究方向至少要有一个具名 owner，避免文档变成"无主的愿景"。
