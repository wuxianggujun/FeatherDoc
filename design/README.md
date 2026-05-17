# FeatherDoc Core 设计文档

> 本目录存放 FeatherDoc Core 的**前瞻性设计文档**，记录尚未实施或处于研究阶段的方向。
>
> 已实施特性的开发者文档在 `docs/`（Sphinx 系统）。本目录与 `docs/` 解耦，避免把"还在想"和"已经做"混在一起。

## 阅读路径（按时间窗口分层）

不要一上来就读全部文档。按以下顺序看：

### 第 1 步：理解长期决策依据（5 分钟）

[**00-vision.md**](00-vision.md) — 自研还是外购，为什么不重写 LibreOffice，PDFio / PDFium
为什么是长期稳定依赖。

如果你想问"能不能把 LibreOffice 的 XX 挖出来"或者"能不能用 AI 整体重写 LO 的 PDF 核心"，
**先读这一篇**。

### 第 2 步：看当前主路线（10 分钟）

| 文档 | 用途 |
|---|---|
| [**02-current-roadmap.md**](02-current-roadmap.md) | **当前 0–6 个月在做什么**（依赖优先：用好 PDFio / PDFium） |
| [**04-pdf-execution-plan.md**](04-pdf-execution-plan.md) | **下一步具体怎么做、怎么验收**（任务拆分、测试命令、阻塞条件） |
| [05-pdf-rtl-table-cell-complex-task-plan.md](05-pdf-rtl-table-cell-complex-task-plan.md) | RTL table cell complex task 的执行源与收口记录 |
| [01-architecture.md](01-architecture.md) | 三件套架构总览（docs + PDFio + PDFium） |

`02-current-roadmap.md` 负责说明主路线和阶段边界；`04-pdf-execution-plan.md` 负责说明后续
实际执行顺序、每阶段验收标准和本地测试命令。**这条线是在已落地的基础段落 / 表格 / 基础样式 /
读写链路之上继续收口，不是从零起步。**

### 第 3 步：长期愿景（按需读，长期不阻塞）

[**03-long-term-roadmap.md**](03-long-term-roadmap.md) — 自研排版引擎的渲染愿景（策略 +
架构 + 阶段 LR-1 / 2 / 3 / 4 / 5）。**6 个月之后才考虑启动**，启动需要满足明确前置条件。

### 第 4 步：依赖和参考资料（用到再查）

| 文档 | 用途 |
|---|---|
| [dependencies/pdfio.md](dependencies/pdfio.md) | PDFio 字节写出层接入 + 能力评估 vs LibreOffice |
| [dependencies/pdfium.md](dependencies/pdfium.md) | PDFium 源码 checkout、GN/Ninja 和 CMake provider |
| [dependencies/skia.md](dependencies/skia.md) | Skia + SkPDF（长期评估方向，非当前任务） |
| [references/oxml-subset.md](references/oxml-subset.md) | 各 Profile 阶段覆盖的 OOXML 特性矩阵 |
| [../BUILDING_PDF.md](../BUILDING_PDF.md) | PDFio / PDFium 本地编译实操手册 |

## 文档分层结构

```
design/
├── 00-vision.md          ← 决策依据（"做不做" 和 "怎么不做"）
├── 01-architecture.md    ← 三件套架构（不分时间窗口）
├── 02-current-roadmap.md ← 当前主路线（0–6 个月）
├── 03-long-term-roadmap.md ← 长期愿景（6 个月后才启动）
├── 04-pdf-execution-plan.md ← PDF 后续执行路线与验收清单
├── 05-pdf-rtl-table-cell-complex-task-plan.md ← RTL table cell complex task 收口记录
├── dependencies/         ← 第三方依赖接入细节
│   ├── pdfio.md
│   ├── pdfium.md
│   └── skia.md           ← 长期评估
└── references/           ← 规范/矩阵等参考资料
    └── oxml-subset.md
```

## 本目录的核心约束

放进本目录的文档必须满足：

1. **前瞻性**：尚未实施或在研究阶段（实施完毕的迁到 `docs/`）
2. **Core 视角**：从 Core 自身的角度描述，不依赖任何下游消费者
3. **可演进**：每个研究方向的主控文档必须写明「启动条件」和「放弃条件」；支撑文档必须能回链到主控文档
4. **小步可验证**：阶段切分能让阶段产出独立可用，不要"做完才能用"
5. **明确时间窗口**：每篇文档头部用 banner 标明它属于「当前主路线」还是「长期愿景」，避免读者把两条线混在一起读

## 不在本目录的内容

- ✗ Core 的 API 文档（在 `docs/`）
- ✗ Core 的发布说明（CHANGELOG.md）
- ✗ Core 的构建说明（README.md / 各种 BUILDING_*）
- ✗ 第三方系统的设计文档（如 FeatherDoc Studio 自己的 PDF 策略，那在 Studio 仓库）

## 维护者

本目录文档由 Core 长期方向负责人维护：**wuxianggujun**。

每个研究方向至少要有一个具名 owner，避免文档变成"无主的愿景"。

## 重整记录

**2026/05** — 目录从 11 篇平铺重整为按工作流分层：

- `why-not-buy.md` → `00-vision.md`，新增「学习 LibreOffice ≠ 替换开源依赖」一节
- `word-pdf-converter-architecture.md` → `01-architecture.md`
- `word-pdf-converter-roadmap.md` → `02-current-roadmap.md`，新增「当前下一步任务清单」（FreeType / CJK / 样式 / HarfBuzz）
- `pdf-rendering-{strategy,architecture,roadmap}.md` 三篇合并为 `03-long-term-roadmap.md`，
  阶段编号从 `Phase 1/2/3/4/5` 改为 `LR-1/2/3/4/5` 避免与主路线撞车
- `pdfio-byte-writer.md` → `dependencies/pdfio.md`，新增「PDFio 与 LibreOffice 写出器能力对比」
- `pdfium-source-build.md` → `dependencies/pdfium.md`
- `skia-pdf-backend.md` → `dependencies/skia.md`（标注为长期评估方向）
- `oxml-subset-coverage.md` → `references/oxml-subset.md`
