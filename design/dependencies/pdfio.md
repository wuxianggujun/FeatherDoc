# PDFio 字节写出层

> 本文档定义 FeatherDoc Core 自研 PDF 路线中的**第一层落地目标**：
> 先拥有轻量、可替换、可商业分发的 PDF 字节生成能力。
>
> 阅读前置：[../03-long-term-roadmap.md](../03-long-term-roadmap.md)、
> [../01-architecture.md](../01-architecture.md)。

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

## PDFio 与 LibreOffice PDF 写出器的能力对比

会反复出现的疑问：**"PDFio 的能力是不是不够，要不要把 LibreOffice 的 PDF 核心挖过来？"**

这个比较在层级上是错位的。

### 层级对位

LibreOffice 的 PDF 输出能力**不是一个模块**，是分层的：

```text
Writer / Calc / Impress 排版引擎
        ↓ 已经决定坐标 / 字体 / 字形
vcl/source/gdi/pdfwriter_impl.cxx   ← 这一层才和 PDFio 对位
        ↓
PDF 字节
```

也就是说，LO 的 `pdfwriter_impl` 在职责范围上和 PDFio **几乎相同**——都是 PDF 对象 / 页 /
流 / xref / 字体嵌入 / 图像写出。LO 的 PDF 输出"看起来强"主要来自上游排版引擎，**不是**
来自 pdfwriter 本身比 PDFio 神。

### 功能盘点

PDFio 已具备的能力：

- PDF 1.x 对象、xref、压缩流
- TrueType / CFF 字体嵌入 + 子集化
- 图像：JPEG / JPEG2000 / CCITT / 索引色
- AES-128 / AES-256 加密
- /ToUnicode CMap、CID 字体（CJK 可用）
- 读 + 写双向能力

PDFio 当前不具备但 LO 有的能力，以及它们对 FeatherDoc 路线的相关性：

| 功能 | PDFio | LO | 相关性 |
|---|---|---|---|
| PDF/A-1/2/3 严格合规 | 无 | 完整流水线 | 归档场景需要，可在 PDFio 之上加合规层 |
| Tagged PDF / PDF/UA | 弱 | 完整结构树 | 无障碍场景需要，独立模块挂载 |
| 数字签名 PAdES | 无 | 完整链 | 合同电子签需要，OpenSSL 直接接 |
| AcroForm 创建 | 弱 | 有 | 表单生成才需要 |
| Linearization | 无 | 有 | 在线预览才需要 |
| OCG / 图层 | 无 | 有 | CAD/工程图才需要 |
| ICC 输出意向 | 弱 | 有 | 印刷才需要 |
| 复杂文字整形（双向 / 连字 / 组合字符） | 不管 | 接 HarfBuzz | **属于 Layout 层，不是 PDF 写出层** |

注意最后一行：**复杂文字整形归 Layout 引擎**，HarfBuzz 由 FeatherDoc 自己直接接，不需要从
LO 里挖。PDFio / pdfwriter 这一层拿到的都是已经整形好的字形 ID + 坐标。

### 正确的能力补齐方式：分层挂载，不是替换

PDFio 是**写出基座**。当业务确实需要 PDF/A / 签名 / Tagged PDF 等高级特性时，正确做法是
**在 PDFio 之上挂载孤立、有界的功能模块**，而不是把 PDFio 整体替换成"AI 重写的 LO 子集"。

```text
你的 Layout Engine
       ↓
   PDFio （字节写出基座，~1.5 万行，可读可改可 fork）
       ↓ 按需挂载：
       ├── PDF/A 合规模块（自写，~1–2 千行 + veraPDF 校验）
       ├── 数字签名（OpenSSL 算 CMS，~500 行胶水）
       ├── Tagged PDF 结构树（自写，~1 千行）
       └── HarfBuzz 整形（接库，~300 行胶水）
```

这个方案与"挖 LO" 的工程量级对比：

| 维度 | 挖 LO 子系统 | PDFio + 按需补差 |
|---|---|---|
| 一次性工程量 | 几十万行 + VCL/UNO/SAL 解耦 | 每块 ≤ 2 千行，独立 |
| 测试托底 | 没有 LO 测试集 = 没用 | veraPDF / pdfium / 真实 PDF 阅读器 |
| 维护负担 | 长期跟 LO 上游 + 自己 patch | 维护 PDFio 上游 + 局部模块 |
| 许可证 | MPL / LGPL（详见 [../00-vision.md](../00-vision.md)） | Apache 2.0 + 自有代码 |
| 可发布性 | 受 copyleft 约束 | 自由 |

### 关于 PDFio 的真实风险

公平起见，PDFio 也有风险，但都不是"功能不够"：

- **Bus factor = 1**：Michael Sweet 一人维护（同一作者维护 CUPS）。
  缓解：Apache 2.0 允许 fork，1.5 万行 C 在 FeatherDoc 团队内可控。
- **生态小**：第三方扩展少，遇到边角问题需要自己读代码定位。
  缓解：用 PDFium 做反向校验（PDFio 写出 → PDFium 解析 → 字段 diff）。

这些风险**显著小于**"挖 LO 几十万行进项目"的风险。具体见
[../00-vision.md](../00-vision.md) 的"学习 LibreOffice ≠ 替换开源依赖"一节。

### 结论

PDFio **不是"功能弱"，是"职责窄"**——窄是优点。FeatherDoc PDF 路线长期保留 PDFio 作为
写出基座；高级功能通过孤立模块在其上挂载，不通过整体替换。

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
