# Word ⇄ PDF 转换器架构

> 本文档定义 FeatherDoc Core 作为 C++ 学习项目时，探索 Word ⇄ PDF
> 双向转换能力的整体架构方案。
>
> 阅读前置：[Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)、
> [PDF 渲染策略](pdf-rendering-strategy.md)、[PDFio 字节写出层](pdfio-byte-writer.md)。

## 定位

FeatherDoc 不把"转换器"理解成一个黑盒大库，而是拆成三个职责清晰的部分：

```text
自研 docs 库  +  PDFio  +  PDFium
```

其中，docs 库负责 Word 文档的结构化读写，PDFio 负责 PDF 写出方向，PDFium 负责
PDF 读入方向。

这套方案的核心目标不是一次性实现完整 Office 兼容，也不是提前重写成熟 PDF 库，而是给学习和长期演进留下清晰边界：

- Word → PDF：先做可控子集，逐步补 Layout 和字体能力
- PDF → Word：先做启发式重建，明确接受非 100% 还原
- 两个方向都复用同一套内部 AST / DOM，避免生成和解析各写一套模型
- Phase 1 / Phase 2 优先使用 PDFio / PDFium，Phase 3 再考虑选择性自研

## 三件套架构

### 1. 自研 docs 库

当前 FeatherDoc 已经具备 Word 文档的解析 / 生成能力，后续应继续承担：

- 读取 DOCX 包结构、XML、样式、段落、表格等语义信息
- 生成 DOCX 文件
- 定义内部 AST / DOM
- 为正向和反向转换提供统一的文档模型

docs 库是整个转换器的中心，PDFio 和 PDFium 都不直接成为业务模型。

### 2. PDFio（写出方向）

PDFio 使用 Apache License 2.0，适合作为商业友好的 PDF 写出层。

它在本项目里的职责是：

- 创建 PDF document / page / object / stream
- 写出字体、图片和基础绘制指令
- 承接 `AST → PDFio 适配层` 生成的绘制结果

PDFio 不负责 Word 排版，也不负责 PDF 解析。它只是把已经决定好的页面内容写成
PDF 字节。

### 3. PDFium（读入方向）

PDFium 使用 Apache License 2.0，适合作为商业友好的 PDF 读入层。

它在本项目里的职责是：

- 打开 PDF 文件
- 提取页面、文本、字体、坐标、图片等低层信息
- 承接 `PDFium → AST 适配层` 的输入数据

PDFium 不负责把 PDF 自动变回 Word 语义。PDF 只有绘制结果，没有可靠的段落、
表格、标题语义，这部分必须由 FeatherDoc 自己做启发式重建。

## 选型理由

### 协议友好

PDFio 和 PDFium 均为 Apache License 2.0 路线，闭源商用和二次分发压力较小。

这比 MPL / LGPL / AGPL 类依赖更适合 FeatherDoc Core 的长期目标。

### 职责清晰

```text
PDFio  = PDF 写出
PDFium = PDF 读入
docs   = Word 读写 + AST / DOM
```

写和读分开后，每个依赖只承担自己最擅长的部分，Core 的文档语义和转换策略仍然掌握在
FeatherDoc 自己手里。

### 体积可控

PDFio 本身较轻。PDFium 虽然是完整 PDF 引擎，但预编译版本可以控制在较小体积
（当前方案按约 6 MB 级别评估）。

因此，PDF 能力仍应保持可选编译，不强制进入 Core 主库。

## 当前推进顺序

PDF 能力是未来支持，不能反过来影响当前 docs 库的稳定性。主路线见
[Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)。本架构文档只记录工程边界。

当前推进顺序应按以下优先级执行：

1. **稳定 docs 库主线**

   继续把默认构建、安装导出、CLI、测试和现有 DOCX 编辑能力打磨稳定。

   PDFio / PDFium 不允许成为默认依赖，也不允许影响 `FeatherDoc::FeatherDoc` 的 ABI 和安装体验。

2. **定义内部 AST / DOM 边界**

   先明确 Word 语义模型中哪些结构会进入转换层，例如：

   - Document
   - Section
   - Paragraph
   - Run
   - Table / Row / Cell
   - Image
   - Style

   这一步比直接写 PDF 更重要。没有稳定 AST，`AST → PDFio` 和 `PDFium → AST` 都会反复返工。

3. **定义 PDF 抽象接口**

   在引入更多 PDF 能力前，先定义 Core 自己的接口：

   - `IPdfGenerator`：负责从 `LayoutResult` 写出 PDF
   - `IPdfParser`：负责从 PDF 提取中间结构

   接口层不暴露 `pdfio_*` 或 `FPDF_*` 类型。PDFio / PDFium 只是默认实现。

4. **定义最小 LayoutResult**

   Word → PDF 不是 XML 字段映射，而是排版。下一步应先定义最小页面布局结果：

   - 页面尺寸和边距
   - 文本框 / 段落框
   - 行盒和 baseline
   - 字体引用
   - 绘制坐标

   初始只覆盖单页、纯文本、基础字号和颜色。

5. **实现 AST → LayoutResult 的合同子集**

   先用 FeatherDoc 自己生成的简单合同 DOCX 做输入，只处理最小可控子集。

   不要一开始追求完整 Word 兼容，也不要一开始接复杂表格、浮动图片、目录、域和修订显示。

6. **实现 LayoutResult → PDFio**

   PDFio 适配层只消费已经排好的 LayoutResult，把它翻译成 PDF content stream。

   这一层不应该反向理解 Word，也不应该自己做复杂排版。

7. **实现 PDFium → AST probe**

   PDF → Word 是启发式版面重建，难度高于 Word → PDF。Phase 1 只要求能提取文本、坐标和调试结构。

   初始目标只做单栏文本和简单表格，不承诺 100% 还原。

8. **6 个月后再选择性自研**

   只有当 PDFio / PDFium 的某个部分成为明确瓶颈时，才考虑自研替换。推荐优先替换 PDFio，
   并保留 PDFio 实现做 A/B 对比。

## 数据流

### 正向：Word → PDF

```text
Word / DOCX
   │
   ▼
docs 库
   │
   ▼
AST / DOM
   │
   ▼
AST → PDFio 适配层
   │
   ▼
PDFio
   │
   ▼
PDF
```

这里的 `AST → PDFio 适配层` 不是简单字段映射。它必须完成最小排版工作：

- 段落断行
- 表格布局
- 分页
- 坐标转换
- 字体选择和字体嵌入策略

现有 [PDF 渲染架构](pdf-rendering-architecture.md) 中的 `LayoutResult` 可作为这一层的
中间结果。

### 反向：PDF → Word

```text
PDF
   │
   ▼
PDFium
   │
   ▼
PDFium → AST 适配层
   │
   ▼
AST / DOM
   │
   ▼
docs 库
   │
   ▼
Word / DOCX
```

这里的 `PDFium → AST 适配层` 本质是版面理解，不是格式解析。

PDFium 能提供文字、坐标、字体、路径和图片等低层信息；FeatherDoc 需要根据这些信息推断：

- 阅读顺序
- 行和段落
- 标题层级
- 表格区域
- 列布局
- 页眉页脚

因此，反向转换只能按 best-effort 设计，不应承诺 100% 还原原始 Word。

## 待实现的核心模块

### 模块 1：AST → PDFio 适配层

职责：

- 接收 docs 库产生的 AST / DOM
- 生成页面级 LayoutResult
- 将 LayoutResult 翻译成 PDFio content stream
- 处理 PDF 坐标系、字体、图片和基础图形

初始目标：

- 支持纯文本段落
- 支持基础样式（字号、粗体、斜体、颜色）
- 支持简单表格
- 支持 A4 页面和基础边距
- 输出文本可选中、可复制的 PDF

### 模块 2：PDFium → AST 适配层

职责：

- 使用 PDFium 提取页面文本和几何信息
- 将字符 / 文本块聚合为行、段落、表格候选
- 重建 FeatherDoc AST / DOM
- 交给 docs 库生成 DOCX

初始目标：

- 支持单栏文本 PDF
- 支持基础段落识别
- 支持简单表格线或坐标对齐表格识别
- 保留页级布局信息，便于后续调试

## 关键技术点

### PDF 坐标系

PDF 坐标系通常以左下角为原点，Y 轴向上。

Word / UI / 图片处理里常见坐标系则以左上角为原点，Y 轴向下。适配层必须集中处理：

- 页面高度换算
- baseline 与文本框坐标换算
- pt、twip、px 等单位换算
- 浮点误差和像素对齐

坐标转换逻辑不应散落在各个 Renderer 里。

### 字体嵌入

Word → PDF 的可用性高度依赖字体处理，尤其是 CJK 字体。

至少要解决：

- 字体发现和 fallback
- CJK 字体子集嵌入
- ToUnicode 映射
- 文本可选中、可搜索、可复制
- 字体缺失时的降级策略

如果字体策略做不好，生成的 PDF 即使看起来正确，也可能无法搜索或复制。

### 段落 / 版式启发式识别

PDF → Word 的核心难点是从坐标还原语义。

适配层需要用启发式规则逐步恢复：

- 同一行字符聚合
- 行距和缩进判断段落
- 字号和粗细判断标题
- 坐标对齐判断表格
- 页眉、页脚和页码识别

这类规则必须可测试、可调参，并且允许输出 warning，避免给用户造成"完全还原"的误解。

## 编译与依赖原则

- docs 库保持默认轻量，不强制链接 PDFio / PDFium
- `FeatherDoc::Pdf` 负责 Word → PDF 写出方向，默认关闭
- `FeatherDoc::PdfImport` 负责 PDF → Word 读入方向，默认关闭
- PDFium 默认从源码 checkout 构建，依赖 Chromium `depot_tools` 中的 `gn` / `ninja`
- 预编译 PDFium 包只作为可选 provider，不作为默认路线
- 公开 API 不暴露 `pdfio_*` 或 `FPDF_*` 类型
- 第三方许可证和 NOTICE 必须随启用模块一起维护

默认构建必须满足：

- 不拉取 PDFio
- 不拉取 PDFium
- 不构建 `FeatherDoc::Pdf`
- 不安装 PDF 实验模块头文件
- 不改变 `FeatherDoc::FeatherDoc` 的依赖集合

## 与现有设计文档的关系

- 本文是 Word ⇄ PDF 转换器的总览。
- [Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md) 是当前优先执行的阶段计划。
- [PDFium 源码构建接入](pdfium-source-build.md) 记录 PDFium 源码 provider 的构建流程。
- [PDF 渲染策略](pdf-rendering-strategy.md) 只覆盖 Word → PDF 正向路线。
- [PDFio 字节写出层](pdfio-byte-writer.md) 只覆盖 PDFio 写出层。
- [自研还是外购](why-not-buy.md) 解释为什么转换不是一个单一黑盒库。

## 启动条件

进入真实双向转换实施前，至少满足：

- [ ] docs 库的 AST / DOM 边界稳定
- [ ] PDFio probe 可稳定生成 PDF
- [ ] PDFium 的预编译分发方式和许可证义务确认
- [ ] Word → PDF 的最小 LayoutResult 已定义
- [ ] PDF → Word 的 best-effort 目标样例已定义

## 放弃条件

满足以下任一时，应停止双向转换器路线或降级为只做单向能力：

- [ ] PDFium 分发体积或构建复杂度不可接受
- [ ] PDF → Word 的启发式质量长期无法满足目标场景
- [ ] 字体嵌入和 ToUnicode 成本超过项目学习收益
- [ ] 三件套依赖导致 Core 主库无法保持轻量

## Owner

本方向负责人：wuxianggujun
