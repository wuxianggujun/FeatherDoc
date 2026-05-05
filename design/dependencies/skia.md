# Skia PDF 后端（长期评估方向）

> 本文档讨论 FeatherDoc Core 在自研 PDF 渲染场景下，**Skia 该不该用、用在哪、什么时候用**。
>
> ⚠️ 本文不在当前主路线（[../02-current-roadmap.md](../02-current-roadmap.md)）范围内，仅为长期评估。
>
> 阅读前置：[../03-long-term-roadmap.md](../03-long-term-roadmap.md)。

## 结论先行

**Skia 可以保留为未来复杂绘制后端**，但不作为当前第一步。

- ✅ 当前第一步：用 PDFio 验证轻量 PDF 字节写出
- ⚠️ 后续评估：用 Skia + SkPDF 处理复杂绘制、图片预览或更高质量图形输出
- ❌ 不用 Skia 做屏幕显示（下游 Flutter / Qt 等已有自己的渲染栈）
- ❌ 不用 Skia 做排版（Skia 不排版，只画）

## 1. 基本认知（避免踩坑）

### 1.1 Skia 只负责"画"，不负责"排版"

Skia 提供：

- `SkCanvas`：drawTextBlob / drawRect / drawPath / drawImage
- `SkPDF`：把 canvas 指令序列化为 PDF
- `SkTypeface` / `SkFont`：字体加载与度量
- `SkShaper`（基于 HarfBuzz）：字形塑形
- 链接、outline、metadata 等 PDF 特性

Skia **不会**帮你决定：

- 一行塞几个字（断行）
- 一页塞几段（分页）
- 表格跨页怎么切
- 段落首行缩进、两端对齐、CJK 标点挤压
- 页眉页脚、脚注、目录域更新

**排版必须自己写**，Skia 只是"把算好的东西画出来"的那一环。

### 1.2 PDF 导出的真正价值是"矢量 + 可选中文本"

如果用 Skia 导出 PDF，文本是矢量的、可搜索、可复制、字体可子集化嵌入。

这是截图式方案（把每页渲染成位图再塞进 PDF）做不到的，也是 Core 自研 PDF 路线最核心的卖点。

如果只是想要"看起来像 PDF"的输出，调外部 Office 截图拼接更省事，**没必要走自研路线**。

### 1.3 Skia 集成成本不低

Skia 用 **GN + Ninja** 构建，不是 CMake。Core 用 CMake，所以集成需要：

- 用 `ExternalProject_Add` 或 vcpkg 的 skia port
- 链接 HarfBuzz / ICU / FreeType（Skia 自己也用，但版本可能要对齐）
- Windows 上还要处理 MSVC 版本、Runtime 库匹配
- Linux 上要处理静态/动态链接策略

**预算至少留 2–3 周做集成验证**（Phase 0 spike 阶段必须做）。

## 2. 分场景决策表

| 场景 | 用 Skia？ | 替代方案 | 理由 |
|---|---|---|---|
| 导出 PDF（矢量、文本可选中） | ⚠️ 后续评估 | PDFio | 当前先用 PDFio 跑通字节写出 |
| 跨平台 headless 渲染（CLI / 服务端） | ⚠️ 后续评估 | PDFio | Core 本身是 headless，但第一步不需要 Skia |
| 导出 PNG / JPG 预览 | ⚠️ 可选 | 直接 SkSurface→bitmap | 复用同一个 Skia 实例即可 |
| 做排版引擎 | ❌ 不是它的活 | 自研 + HarfBuzz + ICU | Skia 只画不排 |
| 渲染数学公式 / 图表 | ⚠️ 间接 | 先转图片或 SVG | 短期不要自己实现 OMML / chart |
| Core 屏幕显示（如有） | ❌ 不需要 | Core 没有屏幕场景 | Core 是 headless 库 |

## 3. 备选方案对比

如果 Skia 集成太复杂，可考虑兜底方案：

| 后端 | 优点 | 缺点 | 推荐度 |
|---|---|---|---|
| **PDFio** | 轻量、Apache 2.0、适合直接写 PDF 字节 | 不负责复杂绘制和排版 | ✅ 当前首选 |
| **Skia + SkPDF** | 渲染质量高，文本可选中，矢量输出 | 集成复杂，体积大 | ⚠️ 后续评估 |
| **libharu** | 体积小（~30k LOC），Zlib 许可证宽松 | API 偏底层，CJK 支持需自己包 | ⚠️ 兜底 |
| **PoDoFo** | 能读能写 PDF | LGPL，许可证复杂 | ❌ 商业不友好 |
| **PDFium** | Google 出品，质量高 | 主要面向"读 PDF"，写功能弱 | ❌ 不匹配场景 |

**推荐策略**：Phase 0 先用 PDFio 跑通最小 PDF writer；Skia 只在 PDFio 无法满足复杂绘制或图形输出质量时再启动 spike。

## 4. 集成路线

### 阶段 A：Phase 0 Spike

- 先完成 PDFio writer probe
- 明确 PDFio 在字体、图片、路径绘制上的能力边界
- 只有发现 PDFio 无法覆盖目标场景时，再写 Skia 最小程序验证成本

### 阶段 B：Phase 1 集成

- 默认继续使用 PDFio 输出合同子集
- 如确需 Skia，再通过 `ExternalProject_Add` 或 vcpkg 引入 Skia
- 实现 `IDocumentRenderer` 的 Skia 实现：`SkiaRenderer`
- 接到 Layout 输出
- 集成测试：合同子集 sample 全部通过

### 阶段 C：Phase 2+ 持续优化

- 字体子集化（减小 PDF 体积）
- PDF/A 兼容性
- 加密 / 数字签名（若下游需要）
- 大文档分块写入（避免一次性占用大内存）

## 5. 与基础库的关系

```text
Core 自研 PDF 渲染
  │
  ├─► Layout 引擎
  │     ├─► HarfBuzz   (文字塑形)
  │     ├─► ICU        (Unicode、断行规则)
  │     └─► FreeType   (字体度量)
  │
  └─► Renderer
        ├─► PDFio         (当前：PDF 字节写出)
        └─► Skia + SkPDF  (未来：复杂绘制后端评估)
              └─► HarfBuzz、FreeType (Skia 内部也用)
```

**注意**：HarfBuzz / FreeType 在 Layout 和 Skia 内部各被使用一次。最好让两侧用同一份实例（避免版本冲突），或者明确知道版本兼容范围。

## 6. 常见误区

### 误区 1："用 Skia 能解决 Word 兼容性问题"

不能。Word 兼容性 99% 是排版算法的问题（断行规则、表格列宽、浮动环绕），渲染后端换什么都一样。

### 误区 2："Skia 集成很简单"

不简单。理由见 §1.3。

### 误区 3："导出 PDF 可以先截图拼接"

可以，但那不叫"PDF 导出"，叫"图片打包"。文本不可选、不可搜、文件巨大、打印模糊。**做短期 demo 可以，产品里别用**。

### 误区 4："Skia 比 libharu 一定好"

不一定。Skia 渲染质量高、API 现代，但集成复杂；libharu 简单粗暴但够用。**根据团队工程能力选**。

## 7. 许可证

- **Skia**：BSD 3-Clause（商业友好）
- **HarfBuzz**：MIT（商业友好）
- **FreeType**：FreeType License（双授权 BSD/GPL，商业友好）
- **ICU**：Unicode 许可证（商业友好）
- **libharu**：Zlib（最宽松）

全部许可证均**与 Core 的 Apache 2.0 兼容**，可以放心嵌入。

## 8. 一句话总结

> **Skia 是未来复杂绘制后端候选，不是当前第一步。**
> **排版永远是 Core 自己的事，PDFio / Skia 都只是把算好的结果变成 PDF 字节流。**

## 附录：参考资料

- Skia 官方：https://skia.org
- SkPDF 文档：https://skia.org/docs/user/sample/pdf/
- SkShaper + HarfBuzz：https://skia.org/docs/user/modules/shaper/
- HarfBuzz：https://harfbuzz.github.io
- ICU 断行：https://unicode-org.github.io/icu/userguide/boundaryanalysis/
- libharu：http://libharu.org/

## 相关文档

- [../03-long-term-roadmap.md](../03-long-term-roadmap.md) — 长期渲染愿景（策略/架构/阶段）
- [pdfio.md](pdfio.md) — PDFio 字节写出层（当前默认 backend）
