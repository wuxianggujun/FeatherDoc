# OOXML 子集覆盖

> 本文档定义 FeatherDoc Core 自研 PDF 渲染**各阶段覆盖的 OOXML 特性子集**。
>
> 阅读前置：[../03-long-term-roadmap.md](../03-long-term-roadmap.md)。

## 设计原则

完整覆盖 OOXML 是不可能的（LibreOffice 用了 22 年仍未做到 100%）。Core 的策略是**按业务场景分级覆盖**，每个场景对应一个 Profile。

```text
合同子集 → 论文子集 → 项目文档子集 → 通用子集
   ↓           ↓            ↓             ↓
Phase 1    Phase 2      Phase 3       Phase 4+
```

每个 Profile 独立可发布。下游可以"用到 Phase N 就停在 Phase N"。

## Profile 矩阵

### 段落级特性

| 特性 | Phase 1 (合同) | Phase 2 (论文) | Phase 3 (项目) | Phase 4+ |
|---|---|---|---|---|
| 段落对齐（左/中/右/两端） | ✅ | ✅ | ✅ | ✅ |
| 首行缩进 | ✅ | ✅ | ✅ | ✅ |
| 行距（固定/倍数/最小） | ✅ | ✅ | ✅ | ✅ |
| 段前 / 段后间距 | ✅ | ✅ | ✅ | ✅ |
| 段落边框 | ❌ | ✅ | ✅ | ✅ |
| 段落底纹 | ❌ | ✅ | ✅ | ✅ |
| keep-with-next | ❌ | ✅ | ✅ | ✅ |
| keep-together | ❌ | ✅ | ✅ | ✅ |
| 孤行寡行控制（widow/orphan） | ❌ | ✅ | ✅ | ✅ |

### 字符级特性

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 字体（family / size） | ✅ | ✅ | ✅ | ✅ |
| 加粗 / 斜体 / 下划线 | ✅ | ✅ | ✅ | ✅ |
| 字体颜色 | ✅ | ✅ | ✅ | ✅ |
| 删除线 | ❌ | ✅ | ✅ | ✅ |
| 上标 / 下标 | ❌ | ✅ | ✅ | ✅ |
| 字符间距（spacing） | ❌ | ✅ | ✅ | ✅ |
| 字符高亮（highlight） | ❌ | ❌ | ✅ | ✅ |
| 字符底纹 | ❌ | ❌ | ✅ | ✅ |
| 文字效果（阴影/轮廓） | ❌ | ❌ | ❌ | ✅ |

### 标题与样式

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 字符样式（rPr） | ✅ | ✅ | ✅ | ✅ |
| 段落样式（pPr） | ✅ | ✅ | ✅ | ✅ |
| 标题样式（heading 1–6） | ❌ | ✅ | ✅ | ✅ |
| 自定义样式继承 | ⚠️ 简化 | ✅ | ✅ | ✅ |
| 主题（theme）颜色 / 字体 | ⚠️ 简化 | ✅ | ✅ | ✅ |

### 列表与编号

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 简单项目符号（• / ○） | ✅ | ✅ | ✅ | ✅ |
| 简单数字编号（1. 2. 3.） | ✅ | ✅ | ✅ | ✅ |
| 多级编号（1.1.1） | ❌ | ✅ | ✅ | ✅ |
| 自定义编号格式 | ❌ | ❌ | ✅ | ✅ |
| 续编 / 重置 | ❌ | ❌ | ✅ | ✅ |

### 表格

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 简单表格（无嵌套、无合并、无跨页） | ✅ | ✅ | ✅ | ✅ |
| 表格边框 | ✅ | ✅ | ✅ | ✅ |
| 单元格底纹 | ✅ | ✅ | ✅ | ✅ |
| 列宽（固定 / autofit） | ⚠️ 仅固定 | ✅ | ✅ | ✅ |
| 合并单元格 | ❌ | ❌ | ✅ | ✅ |
| 嵌套表格 | ❌ | ❌ | ✅ | ✅ |
| 跨页拆分 | ❌ | ❌ | ✅ | ✅ |
| 表格样式（tblStyle） | ❌ | ⚠️ 简化 | ✅ | ✅ |
| 表头重复 | ❌ | ❌ | ✅ | ✅ |

### 页眉、页脚、分节

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 静态页眉 / 页脚 | ✅ | ✅ | ✅ | ✅ |
| 页码字段 | ❌ | ✅ | ✅ | ✅ |
| 日期字段 | ❌ | ✅ | ✅ | ✅ |
| 首页不同 | ❌ | ✅ | ✅ | ✅ |
| 奇偶页不同 | ❌ | ❌ | ✅ | ✅ |
| 多分节 | ❌ | ❌ | ✅ | ✅ |
| 分栏 | ❌ | ❌ | ⚠️ 简单 2 栏 | ✅ |

### 图片与图形

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 行内图片（PNG/JPEG） | ✅ | ✅ | ✅ | ✅ |
| 固定大小图片 | ✅ | ✅ | ✅ | ✅ |
| 浮动图片 | ❌ | ❌ | ⚠️ 简化 | ✅ |
| 图文环绕 | ❌ | ❌ | ❌ | ✅ |
| EMF / WMF 矢量图 | ❌ | ❌ | ⚠️ 基础回放 | ✅ |
| SVG 图片 | ❌ | ❌ | ✅ | ✅ |
| 文本框 / 形状 | ❌ | ❌ | ❌ | ⚠️ 部分 |

### 高级特性

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| 脚注 | ❌ | ✅ | ✅ | ✅ |
| 尾注 | ❌ | ❌ | ✅ | ✅ |
| 目录占位符 | ❌ | ⚠️ 不更新 | ✅ | ✅ |
| 交叉引用 | ❌ | ❌ | ⚠️ 简化 | ✅ |
| 修订标记可视化 | ❌ | ❌ | ❌ | ✅ |
| 批注 | ❌ | ❌ | ❌ | ✅ |
| 书签（保留位置但不渲染） | ✅ | ✅ | ✅ | ✅ |
| 超链接 | ❌ | ✅ | ✅ | ✅ |
| 数学公式（OMML） | ❌ | ❌ | ⚠️ 简化 | ✅ |
| Chart（图表） | ❌ | ❌ | ❌ | 未规划 |
| SmartArt | ❌ | ❌ | ❌ | 未规划 |

### CJK / 国际化

| 特性 | Phase 1 | Phase 2 | Phase 3 | Phase 4+ |
|---|---|---|---|---|
| CJK 字体 | ✅ | ✅ | ✅ | ✅ |
| CJK 标点禁则 | ✅ | ✅ | ✅ | ✅ |
| CJK 字符压缩 | ❌ | ✅ | ✅ | ✅ |
| Bidi（阿拉伯/希伯来） | ❌ | ❌ | ⚠️ 基础 | ✅ |
| 复杂文字（Indic 等） | ❌ | ❌ | ❌ | ⚠️ 部分 |
| 字体回退（fallback） | ✅ | ✅ | ✅ | ✅ |

## 不支持特性的处理

文档遇到当前 Profile 不支持的特性时，按 [../03-long-term-roadmap.md](../03-long-term-roadmap.md) §13「错误处理」中"不支持特性的处理策略"段落处理：

| 策略 | 行为 |
|---|---|
| `Strict` | 立即报错 |
| `BestEffort`（默认） | 跳过该特性，记 warning |
| `Fallback` | 整文档降级，建议下游用 LibreOffice |

## 测试 sample

每个 Profile 维护一份**最小 sample 集**，必须能正确渲染：

### Phase 1 sample（合同）

- `samples/render/phase1/simple-contract.docx` — 标准合同模板
- `samples/render/phase1/contract-with-bookmarks.docx` — 含书签
- `samples/render/phase1/contract-with-table.docx` — 含简单表格
- `samples/render/phase1/contract-with-image.docx` — 含 logo 图片

### Phase 2 sample（论文）

- `samples/render/phase2/thesis-skeleton.docx` — 标题层级 + 多级列表
- `samples/render/phase2/thesis-with-footnotes.docx` — 含脚注
- `samples/render/phase2/thesis-with-toc.docx` — 含目录占位符

### Phase 3+ sample

按 Phase 增量补齐。

## 不在覆盖范围（永久）

以下特性 Core 渲染**永远不打算自研**，遇到时直接降级到外部 Office：

- ❌ 完整的 SmartArt 渲染
- ❌ 完整的 Chart（图表）渲染
- ❌ ActiveX 控件
- ❌ 嵌入 OLE 对象
- ❌ 表单控件（content control 仅保留位置）
- ❌ 数字签名验证
- ❌ DRM 保护

## 维护

启动新 Phase 时，本文档对应行从 ❌/⚠️ 改为 ✅，并补对应 sample。

## 相关文档

- [../03-long-term-roadmap.md](../03-long-term-roadmap.md) — 长期渲染愿景（策略/架构/阶段）
- [../02-current-roadmap.md](../02-current-roadmap.md) — 当前主路线
