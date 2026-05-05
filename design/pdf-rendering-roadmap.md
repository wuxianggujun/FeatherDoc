# PDF 渲染专项路线

> 本文档定义 FeatherDoc Core **自研 PDF 渲染**的分阶段实施计划。
>
> 当前 Word ⇄ PDF 主路线见 [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)。
> 本文只保留 Word → PDF 正向渲染专项的长期参考。
>
> 阅读前置：[PDF 渲染策略](pdf-rendering-strategy.md)、
> [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)。

## 阶段总览

```text
Phase 0: PDF writer 基础接入           （进行中，默认关闭）
   ↓
Phase 1: 合同子集 — 最小可用版本       （3–6 个月，启动后）
   ↓
Phase 2: 论文子集 — 加上结构化文档      （+3–6 个月）
   ↓
Phase 3: 项目文档子集 — 加上图表与列表  （+3–6 个月）
   ↓
Phase 4: 通用 OOXML 子集               （长期，按用户反馈推进）
   ↓
Phase 5: 性能与质量打磨                 （持续）
```

每个 Phase 都是**独立可发布的**，覆盖一类业务场景。

注意：在当前 0-6 个月阶段，优先执行 [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)
中的 Phase 1 / Phase 2，也就是先使用 PDFio / PDFium 跑通和稳定端到端流程。本文的自研排版
深水区不作为当前阻塞任务。

本路线只覆盖 Word → PDF 正向渲染。PDF → Word 属于 PDFium → AST 适配层研究，不进入本路线的
Phase 0–5。

## Phase 0：PDF writer 基础接入（当前阶段）

### 目标

确认本研究方向值得启动 Phase 1，并先完成最小 PDF 字节写出层：

- 评估 OOXML 排版的真实复杂度
- 评估借用基础库（PDFio / HarfBuzz / FreeType / Skia）能省多少工作量
- 接入 PDFio，验证 FeatherDoc 能直接生成 PDF object / page / stream 字节
- 保持 `FeatherDoc::Pdf` 默认关闭，不影响 Core 主库
- 估算 Phase 1 的真实工时

### 交付

- 本目录所有 design 文档
- 一份 PDFio 接入说明：[PDFio 字节写出层](pdfio-byte-writer.md)
- 一个最小 probe：启用 `FEATHERDOC_BUILD_PDF` 后可生成基础 PDF
- 启动 Phase 1 的去/不去决策

### 不做事项

- 不写真实的 OOXML→Layout 代码
- 不修改 Core 现有 CLI / API
- 不引入默认启用的新运行时依赖

### 当前下一步

Phase 0 后续不应继续堆 PDF 功能，而是先把边界打稳：

1. **确认默认关闭策略**

   `FEATHERDOC_BUILD_PDF` 必须保持 `OFF`。默认构建不拉 PDFio、不构建 `FeatherDoc::Pdf`、
   不安装 PDF 实验头文件。

2. **定义 PDF 抽象接口**

   先定义 `IPdfGenerator` / `IPdfParser`，把 PDFio / PDFium 隔离在实现层。

3. **定义最小 AST 输入**

   从 docs 库已有能力里挑出合同子集需要的语义字段：段落、Run、样式、表格、图片和页面设置。

4. **定义最小 LayoutResult**

   先只表达一页 A4、若干文本行、基础字体和矩形。这个结构必须是纯数据，不依赖 PDFio。

5. **实现坐标和单位工具**

   集中处理 twip / pt / px、PDF 左下角坐标系和 Word 常见左上角思维之间的换算。

6. **让 probe 消费 LayoutResult**

   当前 probe 是手写 PDF 内容。下一步应改成手写一个最小 `LayoutResult`，再由 PDFio 后端输出。

7. **再接真实 DOCX 输入**

   只有当 `LayoutResult → PDFio` 稳定后，才把 docs 库解析出来的真实段落接进来。

## Phase 1：合同子集

### 目标

让 Core 能渲染**自身生成的简单合同 DOCX**为 PDF。这是最小、最有商业价值、最可控的子集。

### 范围（合同子集）

支持：

- 段落（对齐、缩进、行距、字体、字号、加粗、斜体、颜色、下划线）
- 简单表格（无嵌套、无跨页拆分、无合并单元格）
- 段落级的项目符号（无多级编号）
- 静态页眉、页脚（不含字段域）
- 基础图片嵌入（PNG / JPEG，固定大小、不浮动环绕）
- 书签（保留位置信息，不影响渲染）

不支持：

- 复杂表格（嵌套 / 合并 / 跨页拆分）
- 浮动图文环绕
- 多级列表
- 字段域（页码 / 日期 / 目录）
- 修订标记可视化
- 复杂分节
- 数学公式
- SmartArt / Chart

### 子任务

- [ ] OOXML 子集语义解析（`include/featherdoc/render/`）
- [ ] Layout 引擎合同子集（`src/render/layout/`）
- [ ] 字体集成：FreeType + HarfBuzz
- [ ] PDF 输出：先使用 PDFio 写出 content stream；复杂绘制后端后续再评估
- [ ] CLI 命令：`featherdoc_cli render-pdf --input contract.docx --output out.pdf --profile contract`
- [ ] 集成测试：用 Core 的 sample 合同模板填充后渲染，对比 Word/LibreOffice 输出
- [ ] 视觉回归测试：差异低于阈值才允许 merge

### 验收标准

- 渲染 Core sample 合同 DOCX 不丢任何字符
- 字体度量与 Word 输出差异 ≤ 5%
- 渲染速度比 LibreOffice 快 ≥ 5 倍
- A/B 视觉测试用户接受度 ≥ 80%

### 工时估算

| 子任务 | 工时 |
|---|---|
| OOXML 子集解析 | 2–3 个月 |
| Layout 引擎（段落 + 表格） | 4–6 个月 |
| 字体集成（FreeType + HarfBuzz） | 1–2 个月 |
| Skia / SkPDF 集成 | 2–3 个月 |
| 测试与调优 | 持续 |
| **合计** | **9–14 个月（1 人专职）** |

如果有 2 人专职：**6–10 个月**。

## Phase 2：论文子集

### 增量范围

在合同子集基础上加：

- 标题样式（heading 1–6）
- 多级列表（OOXML numbering）
- 脚注（footnote）
- 目录占位符（不自动更新，但保留位置）
- 简单分节（节首页、奇偶页页眉页脚）

### 工时估算

3–6 个月。

## Phase 3：项目文档子集

### 增量范围

- 复杂表格（合并单元格、跨页拆分）
- 列表与表格嵌套
- 矢量图（EMF/WMF 基础回放）
- 内联公式（OMML 简化形态）

### 工时估算

3–6 个月。

## Phase 4：通用 OOXML 子集

按真实下游反馈优先级推进。无固定时间。

### 可能加入的特性

- 浮动图文环绕
- 复杂分节
- 字段域（页码、日期、交叉引用）
- 修订标记可视化
- Chart / SmartArt（需另起子模块）

## Phase 5：性能与质量打磨

持续阶段，目标：

- 大文档（100+ 页）渲染 < 5 秒
- 内存占用 < 200 MB（典型场景）
- 视觉与 Word 一致性 > 95%

## 阶段间依赖

```text
Phase 0 ─► Phase 1 ─► Phase 2 ─► Phase 3 ─► Phase 4 ─► Phase 5
            │
            └─► 每个 Phase 都对外可用，下游可以
                "用到 Phase N 就停在 Phase N"
```

## 风险与应对

### 风险 1：基础库集成成本超预期

Skia 用 GN + Ninja 构建，与 Core 的 CMake 不直接兼容。HarfBuzz / FreeType / ICU 在 Windows 上的工具链匹配也复杂。

**应对**：Phase 0 先接入 PDFio writer，验证最小字节写出链路；Skia 只保留为复杂绘制后端评估；遇到无法解决的问题再考虑自研 writer 或其他轻量后端。

### 风险 2：与 Word 视觉差异不可接受

排版细节差异（行距、字距、CJK 标点处理）可能让用户觉得"不像 Word 排的"。

**应对**：Phase 1 验收前必须做 A/B 测试；接受一定差异作为代价；提供 fallback 到 LibreOffice 的开关。

### 风险 3：维护成本失控

OOXML 标准持续演进，子集覆盖容易陷入"修不完的 bug"。

**应对**：严格控制子集边界；用户反馈触发新特性而非主动追求；每 Phase 必须有明确收尾标准。

### 风险 4：人手不足，进度停滞

1 人长期投入 9–14 个月，期间无法兼顾其他工作。

**应对**：启动前必须确认资源；不允许"业余时间慢慢做"；如人手不足直接归档不启动。

## 决策点：什么时候真正启动 Phase 1

### 启动条件（必须**全部**满足）

- [ ] Core 现有特性已稳定（无重大 bug，发布频率稳定）
- [ ] 至少有一个下游商业产品愿意为本能力付费 / 投入资源
- [ ] 团队有 1+ 人专职可投入 9–14 个月
- [ ] 已完成 Phase 0 PDFio writer probe 并通过评审
- [ ] 已确认基础库（PDFio / HarfBuzz / FreeType / Skia）许可证和分发义务可接受

### 不启动的信号

- [ ] 主要下游用户对外部 Office 转换满意
- [ ] 团队工作量已饱和
- [ ] LibreOffice headless 性能改进足以覆盖批量场景
- [ ] 商业 Aspose 授权对下游可接受

## 文档维护

每次本研究方向状态变化（启动 / 推进 Phase / 归档）必须同步更新本文档。

启动 Phase N 时，对应章节要补"实际启动日期"和"实际负责人"。

## 相关文档

- [PDF 渲染策略](pdf-rendering-strategy.md)
- [PDF 渲染架构](pdf-rendering-architecture.md)
- [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)
- [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)
- [Skia PDF 后端](skia-pdf-backend.md)
- [OOXML 子集覆盖](oxml-subset-coverage.md)
