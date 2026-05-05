# PDF 渲染策略

> 本文档定义 FeatherDoc Core **自研 PDF 渲染能力**的高层策略。
>
> 该方向是 Core 的长期研究目标之一，**当前不在产品 roadmap**，也不阻塞 Core 现有特性的发布。
>
> 上层总览见 [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)，当前主路线见
> [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)。

## 当前定位

| 项目 | 状态 |
|---|---|
| Core 现有能力 | DOCX 内容操作（pugixml + zip）— 填模板、批量替换、修订处理 |
| 本研究方向 | 在 Core 中增加 DOCX 子集 Layout 引擎 + PDF 字节输出后端 |
| 本研究阶段 | **依赖优先**：先用 PDFio / PDFium 跑通端到端，再优化翻译层 |
| 与反向转换关系 | 本文只覆盖 Word → PDF；PDF → Word 由 PDFium → AST 适配层另行研究 |
| 启动门槛 | 见本文档"启动前置条件" |

## 战略意图

### 为什么 Core 可能要做这件事

Core 当前只做"DOCX 内容操作"，输出仍然是 DOCX。下游需要 PDF 时，必须依赖外部 Office（LibreOffice / Microsoft Word / Aspose）来完成 DOCX→PDF 转换。

外部依赖带来三个长期问题：

1. **性能瓶颈**：外部 Office 启动开销大（5–10 秒），Core 在批量处理场景下被拖慢两个数量级
2. **依赖膨胀**：需要 bundle 或要求安装 ~250 MB 的 LibreOffice
3. **可控性弱**：渲染结果由外部决定，Core 无法精准控制输出

如果 Core 自带渲染能力：

- 简单文档（合同、论文等）启动即输出，无外部依赖
- 复杂文档仍可降级到外部 Office
- Core 二进制保持轻量，PDF 子模块可选编译

### 为什么这件事很难

OOXML 排版引擎是一个 30–50 人年级别的工程。LibreOffice 用了 22 年到现在仍在迭代。完整覆盖 OOXML 是不现实的；本研究方向只追求**子集覆盖**。

详细难度分析见 [自研还是外购](why-not-buy.md)。

## 范围限定

### 本研究方向**包括**

- OOXML 子集解析（已有部分能力，但渲染场景需要更多语义信息）
- Layout 引擎：把 OOXML 结构翻译成"画在第几页第几行第几个坐标"的具体指令
- PDF writer 后端：把 Layout 结果输出成 PDF object / page / stream 字节
- 字体子集嵌入与文本可选中性

### 本研究方向**不包括**

- ❌ 通用 OOXML 全特性支持（永远做不完）
- ❌ 完整替代 LibreOffice / Word（业务上没必要）
- ❌ 交互式编辑（Core 不是 Office 软件）
- ❌ 反向：PDF → DOCX（属于 PDFium → AST 的另一条研究线，不在本文档范围）
- ❌ 数学公式（OMML）、图表（chart）、SmartArt 等极复杂特性的渲染

## 设计原则

### 原则 1：Layout 与 Renderer 解耦

Layout 引擎是纯算法，不依赖任何渲染后端。Renderer 是可替换的输出层。

详见 [PDF 渲染架构](pdf-rendering-architecture.md)。

### 原则 2：渐进式子集覆盖

不追求一次到位，按使用场景从小到大覆盖：

```text
合同子集 → 论文子集 → 项目文档子集 → 通用子集
```

每个子集独立可发布。详见 [OOXML 子集覆盖](oxml-subset-coverage.md)。

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

当前第一步详见 [PDFio 字节写出层](pdfio-byte-writer.md)。Skia 只保留为未来复杂绘制后端的评估方向，详见 [Skia PDF 后端](skia-pdf-backend.md)。

### 原则 4：保持 Core 二进制轻量

PDF 渲染能力作为**可选编译模块**，不强制链接到 Core 主二进制。下游可以选择：

- 仅使用 Core 内容操作（现状，保持轻量）
- 链接实验性 PDF writer 模块（先接入 PDFio，仍保持轻量）

### 原则 5：永远不阻塞产品

Core 当前 CLI、Core 当前下游（如 FeatherDoc Studio）的产品节奏**不依赖**本方向完成。

如果 5 年内本方向没出成果，Core 仍然按现有路线持续输出价值。

## 成功指标

本方向被认为"成功"的标志：

- [ ] 至少一个真实业务场景（如合同生成）的 PDF 输出可由 Core 自身完成
- [ ] 该场景下 Core 渲染速度比 LibreOffice 快至少 5 倍
- [ ] 用户视觉接受度 ≥ 80%（A/B 测试 vs Word 输出）
- [ ] Core 二进制不强制变重（可选模块）

## 启动前置条件

本研究方向**不会**因为"想做"就启动实施。只有以下条件**全部**满足时才进入实施阶段：

- [ ] Core 现有能力（DOCX 内容操作）已稳定
- [ ] Core 有持续的下游商业用户
- [ ] 团队规模到 3+ 人，且至少 1 人能专职 C++ / 排版工程
- [ ] LibreOffice / Aspose 等外部依赖明确成为业务瓶颈
- [ ] 有持续 1–2 年的工程投入预算

## 放弃条件

满足以下任一时，本方向应被**正式归档**而非半死不活地拖着：

- [ ] 主要下游用户都不再需要 PDF 输出
- [ ] LibreOffice headless 已经能完美满足下游场景，性能不再是瓶颈
- [ ] 团队规模长期不到 3 人
- [ ] 投入 2 年仍未做出最小可用版本

## 相关文档

- [PDF 渲染路线](pdf-rendering-roadmap.md)
- [PDF 渲染架构](pdf-rendering-architecture.md)
- [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)
- [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)
- [PDFio 字节写出层](pdfio-byte-writer.md)
- [Skia PDF 后端](skia-pdf-backend.md)
- [OOXML 子集覆盖](oxml-subset-coverage.md)
- [自研还是外购](why-not-buy.md)

## Owner

本方向负责人：wuxianggujun

每次评估本方向状态时，必须更新本文档的"当前定位"和"启动前置条件"段落。
