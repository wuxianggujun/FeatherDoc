功能缺口分析（中文）
====================

这份文档回答一个偏产品和工程判断的问题：

``FeatherDoc`` 当前已经很能做事了，但如果继续把它做成一个面向正式
文档交付场景的 ``.docx`` 引擎，接下来还缺哪些能力。

本文不是承诺清单，也不是版本计划。它更像一个功能雷达：帮助后续
评估需求优先级，避免项目被低价值的 WordprocessingML 表面功能拖散。


判断口径
--------

判断“缺什么”时，不按“Word 里有什么就补什么”来算。

更合适的口径是看它是否强化下面这条闭环：

``打开诊断 → 结构化编辑 → 模板生成 → 保存复用 → 真实渲染验证``

一个功能越能服务这条闭环，越值得优先做。反过来，如果一个能力只是让
覆盖面看起来更完整，但很难进入真实业务模板、正式报告、合同、发票、
标书、制度文件这类场景，就不应该排在前面。


一句话结论
----------

``FeatherDoc`` 现在最不缺的是基础读写能力。

更缺的是：

- 模板契约的高层治理能力
- 样式与编号的批量治理能力
- 表格样式和复杂版式的高层 typed API
- 公式、字段、批注、修订等正式文档对象
- 面向真实项目模板的接入、迁移和质量门禁体验

其中前三项最应该优先推进。


P0：近期最值得补的能力
-----------------------

P0 不是说立刻全部做完，而是说这些能力最贴合当前项目定位，
投入产出比也最高。


一、模板 schema 高层变更 API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

当前已经有模板校验、schema 导出、normalize、diff、patch、baseline gate
等能力，但整体更偏 CLI 和文件工作流。

真正缺的是 C++ 侧更高层的 schema mutation API。

建议补齐：

- 新增、删除、重命名 template target
- 新增、删除、重命名 slot
- 修改 slot kind、required、occurrence 约束
- 从文档扫描结果生成 schema patch
- 把 schema patch 应用到内存对象并输出稳定 JSON
- 对 schema 变更生成 review summary

价值：

- 让下游可以把模板契约管理嵌入自己的工具链
- 避免所有高级 schema 操作都只能绕 CLI 和 JSON 文件
- 为项目级模板管理、版本迁移、自动修复打基础


二、真实业务模板接入向导
^^^^^^^^^^^^^^^^^^^^^^^^

当前已经有 project template smoke 和 baseline 相关脚本，但接入一份新模板
仍然偏“知道内部流程的人才跑得顺”。

建议补齐一个更高层的 onboarding 工作流：

- 扫描模板中的书签、slot、表格候选区
- 生成初始 template schema
- 生成 render data skeleton
- 生成最小 smoke manifest entry
- 输出接入报告，标出缺失、重复、未知、类型不匹配的模板点位
- 给出下一步修复建议，而不是只返回失败

价值：

- 降低把真实合同、发票、报告模板接入项目的成本
- 让 ``FeatherDoc`` 从“有 API 的库”更进一步，变成“能落地模板工程”的工具
- 更适合对外展示项目成熟度


三、编号 catalog 的导入、导出与 override 管理
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

当前列表、编号定义、样式挂接编号、样式编号只读盘点 / 审计 / command_template 修复建议 / plan-apply 安全清理、based-on 对齐、唯一同名 definition relink 与 catalog 导入预修复，以及 numbering catalog 的内存级
与 CLI JSON 导入 / 导出已经有不错基础。catalog JSON 现在也支持
definition level upsert、批量 override upsert/remove、结构 lint、文档对 baseline 的 check、单文件 /
manifest 脚本级 baseline gate 和 JSON 差异对比。后续对既有文档里的
复杂 numbering catalog，仍可继续补 exemplar 提取和冲突审计入口。

建议补齐：

- 导出既有 numbering catalog 为稳定 JSON（已覆盖基础 CLI 工作流）
- 从 JSON 或 exemplar 文档导入 numbering definition（JSON 已覆盖基础 CLI 工作流）
- 检查 paragraph style 与 numbering definition 的绑定关系（已覆盖 inspect-style-numbering 只读盘点、audit-style-numbering issue gate / command_template 修复建议，缺失 level 会指向 upsert_levels patch，以及 repair-style-numbering 安全清理、based-on 对齐、唯一同名 definition relink 与 catalog 导入预修复）
- 管理 definition level、level override、restart、abstractNum / num 实例映射（基础 JSON patch 已覆盖）
- 检测重复、孤儿、冲突的 numbering 定义（基础 lint 已覆盖重复与结构问题）
- 对比两个 numbering catalog 的差异（基础 diff 已覆盖 definition / level / instance / override）
- 检查文档当前 numbering catalog 是否匹配 baseline（基础 check、单文件 wrapper 和 manifest wrapper 已覆盖）

价值：

- 标书、制度文件、合同里的多级标题和列表会更可控
- 减少生成后进入 Word 手工修编号的概率
- 与样式治理一起形成“文档骨架治理”能力


四、自定义表格样式定义编辑
^^^^^^^^^^^^^^^^^^^^^^^^^^

当前表格结构和单元格属性已经很强，但 ``word/styles.xml`` 中 table style
definition 的高层编辑仍然不足。

建议补齐：

- 创建或更新 custom table style
- 设置 wholeTable、firstRow、lastRow、bandedRows 等区域属性
- 管理边框、底色、字体、段落间距、cell margin
- 将表格实例绑定到样式，并保留必要的局部 override
- 提供 style look 与 table style property 的一致性检查

价值：

- 正式报告和发票类文档会更依赖表格样式，而不是每张表逐格写属性
- 有利于减少生成文档体积和 XML 噪音
- 更符合 Word 原生文档模型


五、样式重构工作流
^^^^^^^^^^^^^^^^^^

当前已经有样式检查、单样式 / 全量 usage report、继承解析、局部 rebase、materialize、基础 style id rename、同类型 style merge、批量 rename / merge 非破坏性计划、重复样式保守 merge 建议、顶层 ``suggestion_confidence_summary``、可用 ``--source-style`` / ``--target-style`` 收敛具体样式对，再用 ``--confidence-profile recommended|strict|review|exploratory`` 或 ``--min-confidence <0-100>`` 过滤、并可用 ``--fail-on-suggestion`` 作为 CI gate 的持久化 JSON plan、受控 apply 与 rollback 记录（含 merge source style XML 与原 source usage hits 快照）、基于 rollback JSON 的 merge restore dry-run（也可用 ``--plan-only``）审计 / 正式恢复，并支持重复 ``--entry`` 或 ``--source-style`` / ``--target-style`` 选择 rollback 项，restore issue 会输出可操作 ``suggestion`` 与顶层 ``issue_count`` / ``issue_summary``，以及保守的未使用 custom style prune plan / apply 能力。
下一步缺的是基于真实语料的建议置信度校准、merge restore 更完整冲突处理与更完整的样式治理闭环。

建议补齐：

- 为 merge restore 补更完整的冲突处理与交互式选择体验
- 为重复样式建议补基于真实语料的置信度校准与阈值推荐
- 将未使用样式清理与 refactor plan 汇入统一的批量 workflow
- 检查 basedOn 链路循环、断链和隐藏样式
- 基于 style usage report 生成可审阅的批量变更计划
- 对 heading、caption、table、list style 提供专门治理 helper

价值：

- 解决长期维护模板时最容易出现的样式漂移问题
- 让模板升级、品牌样式替换、旧模板清理更自动化
- 与 numbering catalog 治理形成组合能力


P1：中期值得补的能力
--------------------

P1 能力也重要，但要么实现复杂度更高，要么需要先等 P0 能力沉淀后再做，
否则容易做成孤立 API。


一、通用字段 API
^^^^^^^^^^^^^^^^

当前已经有页码相关字段能力，但正式文档常见的不只是页码。

建议中期补：

- TOC / 目录字段
- REF / PAGEREF / SEQ / STYLEREF
- DATE / DOCPROPERTY / HYPERLINK
- 字段插入、枚举、替换、锁定、刷新提示

注意：库本身不一定负责“计算 Word 字段结果”。更稳妥的目标是结构化插入
和检查字段，让 Word 在打开或刷新时计算显示值。


二、OMML 公式 typed API
^^^^^^^^^^^^^^^^^^^^^^^

README 已经明确当前没有高层公式 API。这个缺口是真实存在的。

建议先做轻量路线：

- 枚举文档中的 OMML 对象
- 插入已有 OMML 片段
- 替换、删除、提取公式 XML
- 后续再考虑公式 builder

不要一开始就尝试完整表达所有数学结构，否则复杂度会快速失控。


三、批注、修订与审核对象
^^^^^^^^^^^^^^^^^^^^^^^^

正式合同和制度文件经常需要批注和修订痕迹。

建议中期补：

- comment 插入、枚举、删除
- comment range 与正文定位
- tracked changes 的基础 inspection
- 接受/拒绝修订可以后置，先做可诊断和可保留

价值：

- 让库更适合合同审阅、制度流转、人工审核协同场景
- 也能支持生成“带审核意见”的交付文档


四、内容控件 Content Controls
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

当前模板能力主要围绕书签。Word 真实业务模板中也常见 content control。
现在已经先补了 ``list_content_controls()`` / ``inspect-content-controls``
这条基础 inspection 链路，可以枚举 ``w:sdt`` 并按 tag / alias 过滤；
同时也提供了 ``replace_content_control_text_by_tag(...)`` /
``replace_content_control_text_by_alias(...)`` 这组 C++ 层纯文本替换入口，
以及 ``replace-content-control-text`` 这条 CLI 一次性纯文本替换命令。

建议继续补齐：

- 将内容控件纳入 template schema slot 导出 / 校验
- 继续扩展段落、表格、图片等富内容替换
- 提供与书签模板同级的可视化回归 fixture

价值：

- 与 Word 原生模板制作方式更兼容
- 可以弥补书签在可视化编辑和稳定定位上的不足


五、文档语义 diff 与变更报告
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

当前已有很多 inspect 命令，但还缺一个更面向审查的语义 diff。

建议中期补：

- 比较两个文档的段落、表格、图片、样式、编号、section 差异
- 输出 JSON 和人类可读摘要
- 支持忽略 transient id、关系 id、时间戳等噪音
- 可用于模板升级和回归审查

价值：

- 让自动化生成结果更容易 review
- 也能服务 release gate 和 visual validation 之外的结构化验证


P2：可以后置的能力
------------------

这些能力不是没有价值，而是当前阶段不应该抢占前三条主线资源。


一、密码保护或加密文档
^^^^^^^^^^^^^^^^^^^^^^

这是 README 中已经标出的限制。

建议后置，原因是：

- 实现复杂度高
- 安全边界和依赖选择需要非常谨慎
- 很多正式生成场景可以通过外部流程做加密分发

更合理的短期处理是：继续明确报错，并提供可诊断的错误信息。


二、完整 WordprocessingML 覆盖
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

不建议把“覆盖所有 Word XML 标签”当目标。

原因：

- 范围过大，容易稀释项目主线
- 很多标签很少进入真实业务模板
- 做成低层 XML wrapper 会削弱 typed API 的价值

更好的策略是：只围绕正式文档高频对象逐步补 typed API。


三、宏、VBA、嵌入对象深度编辑
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

这类能力风险高、边界复杂，而且和当前“安全、清晰、可诊断”的项目定位
不完全一致。

短期建议只做检测和保留，不做主动编辑。


四、旧 API 长期兼容层
^^^^^^^^^^^^^^^^^^^^

项目定位已经明确不是被动维护旧 DuckX 形态。

因此不建议为了兼容历史写法长期保留低价值包装层。新增能力应优先服务
清晰的现代 C++ API。


建议的落地顺序
--------------

如果按实际收益排序，建议采用下面的推进节奏。

第一阶段：模板契约继续做实
^^^^^^^^^^^^^^^^^^^^^^^^^^

优先做：

- template schema mutation API
- 真实模板 onboarding 报告
- schema patch 的 C++ API 化
- project template smoke 的接入体验优化

阶段目标：让一份真实业务模板可以更快接入、校验、生成和回归。


第二阶段：文档骨架治理
^^^^^^^^^^^^^^^^^^^^^^

优先做：

- numbering catalog JSON / CLI import-export
- style numbering repair mutation（repair-style-numbering 已补 plan/apply 安全清理、based-on 对齐、唯一同名 definition relink 与 catalog 导入预修复；catalog patch 已支持 definition level upsert，后续扩展 repair 生成 patch）
- style usage report 驱动的 batch audit 与自动建议
- style refactor plan 的真实语料置信度校准与 merge restore 批量选择增强
- heading + numbering 的组合治理 helper

阶段目标：让标题、列表、样式继承、编号实例不再依赖人工修 Word。


第三阶段：版式与正式文档对象补齐
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

优先做：

- custom table style definition editing
- 通用字段 API
- content controls
- 轻量 OMML inspection / insert / replace
- 批注和修订 inspection

阶段目标：扩大正式文档覆盖面，同时不破坏项目 typed API 的清晰边界。


每个功能的验收标准
------------------

后续新增功能建议至少满足这些标准：

1. 有公开 C++ typed API，而不是只新增 CLI 命令。
2. CLI 只是脚本化入口，输出必须支持稳定 JSON。
3. 能说明支持 body、header、footer、section part 中的哪些范围。
4. 修改类能力必须覆盖 reopen-save 场景。
5. 影响版式的能力必须补 Word visual validation。
6. 有最小 sample，能让用户快速复制使用方式。
7. 错误路径要返回结构化诊断，而不是只返回 ``false``。


暂不建议推进的方向
------------------

下面这些方向短期应该克制：

- 为了看起来完整而追逐冷门 Word XML 标签
- 先做大而全 CLI，再反推 C++ API
- 为旧接口兼容牺牲新 API 清晰度
- 把 visual validation 替换成纯 XML 断言
- 把库做成通用 Office 套件处理器
- 把宏、嵌入对象、加密作为近期主线


总结
----

我对这个库的判断是：

``FeatherDoc`` 已经越过了“能不能读写 docx”的阶段。

下一阶段最有价值的不是继续堆零散编辑能力，而是把模板契约、样式编号、
表格版式这三条线做成稳定工作流。这样它会更像一个真正可用于正式文档
生产的 C++ 引擎，而不是一个功能很多但使用路径分散的 XML 操作库。
