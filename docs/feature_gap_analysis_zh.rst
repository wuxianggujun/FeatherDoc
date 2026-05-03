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

- 真实业务模板接入、迁移和审批的高层工作流
- 样式与编号治理在真实语料上的置信度校准和冲突处理
- 表格样式与复杂版式的更完整属性覆盖
- 公式、字段、批注、修订等正式文档对象
- 面向真实项目模板的接入、迁移和质量门禁体验

其中前三项最应该优先推进。


P0：近期最值得补的能力
-----------------------

P0 不是说立刻全部做完，而是说这些能力最贴合当前项目定位，
投入产出比也最高。


一、模板 schema 迁移与审批工作流
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

当前已经有模板校验、schema 导出、normalize、diff、patch、baseline gate
等能力；C++ 侧也已经补齐第一批内存级 schema mutation helper，覆盖 target
replace/remove/rename、slot upsert/remove/rename/update，以及 patch preview/apply。

当前已进一步支持从文档扫描结果生成内存级 schema patch：
``scan_template_schema(...)`` 可以扫描书签和 content control，
``build_template_schema_patch_from_scan(...)`` 可以把扫描结果与 baseline schema
直接 diff 成 ``template_schema_patch``。schema mutation review summary 也已有稳定
JSON 输出，schema 固定为 ``featherdoc.template_schema_patch_review.v1``；CLI 的
``preview-template-schema-patch`` / ``build-template-schema-patch`` 可通过
``--review-json`` 写出同格式 review 文件。baseline gate / manifest gate 脚本也已经
支持 ``-ReviewJsonOutput`` / ``-ReviewJsonOutputDir``，能在 CI 中直接保留可审计的
schema mutation review 文件。project template smoke 现在也会把 per-entry review JSON
聚合进 ``summary.json`` / ``summary.md``，并为 changed review 生成
``schema_patch_approval_result.json`` 审批记录和 ``schema_patch_approval_items``，
把生成的 schema update candidate 明确交给人工审批；
``sync_project_template_schema_approval.ps1`` 可以把人工编辑或命令行登记的
``decision`` 回灌到 ``summary.json`` / ``summary.md``，并可通过
``-ReleaseCandidateSummaryJson`` 同步 release-candidate summary 与 ``final_review.md``。
审批结果现在会校验非 ``pending`` decision 是否携带 ``reviewer`` 与 ``reviewed_at``，
缺失时汇总为 ``invalid_result``、``schema_patch_approval_compliance_issue_count`` 与
``compliance_issues``，避免误把缺少审计信息的记录当作已完成审批。同步到 release
candidate 时还会写入 ``schema_patch_approval_gate_status`` / ``gate_blocked``，
并在状态为 ``blocked`` 时让 release preflight 和 reviewer checklist 明确阻断发布。
同步后的 release summary 还会维护顶层 ``release_blockers`` 与
``release_blocker_count``，并用 ``project_template_smoke.schema_approval``
稳定标识 schema approval 阻断，让 CI / 发布面板无需解析 Markdown 就能读取
issue keys、审批条目和修复动作。
``write_project_template_schema_approval_history.ps1`` 也已能把多次 smoke / release
summary 汇总为 ``featherdoc.project_template_schema_approval_history.v1`` 趋势报表，
并已接入 release-candidate preflight 自动生成 release report 产物；报表现在会输出
``latest_blocking_summary`` 与 ``entry_histories``，用于观察 blocked / pending / passed
的长期变化和每个模板 entry 的审批走势。onboarding summary 和人工复核文档会同步展示
schema drift 数量、变更摘要、审批 decision 与下一步动作；单模板 onboarding
产物现在还会写出 ``schema_approval_state``、``release_blockers``、
``action_items`` 和 ``manual_review_recommendations``，让调用方不解析 Markdown
也能判断 schema approval 是否阻断发布、下一步应先复核 candidate 还是修复审批记录。

后续建议继续补齐的是“工程化治理层”，而不是基础 patch API：

- 基于业务模板语料校准 rename / update 建议的置信度
- 继续沉淀真实项目里的 schema approval 审计字段和多项目发布门禁策略

价值：

- 让下游可以把模板契约管理嵌入自己的工具链
- 让高级 schema 操作不仅可调用，还能进入审批、审计和发布门禁链路
- 为项目级模板管理、版本迁移、自动修复打基础


二、真实业务模板接入向导
^^^^^^^^^^^^^^^^^^^^^^^^

真实业务模板 onboarding 已落地为库级诊断 API 与项目级脚本工作流：

- ``Document::onboard_template(...)`` 可复用 schema scan / validate / patch review，
  输出 issues、schema patch、render data skeleton、mapping lint 和人工复核入口
- ``scripts/onboard_project_template.ps1`` 可为单个业务模板生成一站式 onboarding
  bundle，包含 schema candidate、临时 smoke manifest、可填写 data skeleton、
  smoke summary、人工复核文档和 ``repair/`` 工作区；``onboarding_summary.json``
  会明确暴露 schema approval 状态、release blocker、action item 和人工复核建议
- ``scripts/new_project_template_smoke_onboarding_plan.ps1`` 可在不改 manifest 的前提下，
  从候选发现、schema baseline、render data skeleton 和视觉 smoke 入口生成接入计划，
  并把尚未运行 smoke 的候选标记为 ``schema_approval_state.status=not_evaluated``
- project template smoke 已把 schema patch approval items、审批结果、compliance gate
  和 release blocker 同步进 ``summary.json`` / ``summary.md`` / release summary

后续更值得继续沉淀的是跨项目 onboarding 策略和真实业务语料中的置信度校准。

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

后续建议集中在：

- 从 exemplar 文档自动提取 numbering catalog，并输出冲突审计报告
- 把 ``repair-style-numbering`` 的安全修复建议进一步转成可复用 catalog patch
- 对企业模板里的重复、孤儿、跨样式绑定冲突做更细的置信度分级
- 将 style usage、style numbering audit 与 catalog baseline gate 汇入统一骨架治理报告

价值：

- 标书、制度文件、合同里的多级标题和列表会更可控
- 减少生成后进入 Word 手工修编号的概率
- 与样式治理一起形成“文档骨架治理”能力


四、自定义表格样式定义编辑
^^^^^^^^^^^^^^^^^^^^^^^^^^

当前表格结构和单元格属性已经很强，``word/styles.xml`` 中 table style
definition 的第一版高层编辑也已经开始落地：``ensure_table_style(...)``
现在可以写入整表区域、首末行列、第一 / 第二条带区域的边框、底色、文字颜色、加粗、斜体、字号、字体族、单元格垂直对齐、文本方向、段落对齐、段落间距、行距与 cell margin，
也可以通过 ``find_table_style_definition(...)`` 和 CLI JSON 反查这些属性，通过 ``audit-table-style-regions`` 审计空的已声明区域，通过 ``audit-table-style-inheritance`` 审计缺失、跨类型或循环 ``basedOn`` 继承链，通过 ``audit-table-style-quality`` 聚合这些定义检查与 ``tblLook`` 实例检查作为 CI gate，通过 ``plan-table-style-quality-fixes`` 将失败项拆成自动修复和人工处理清单，通过 ``apply-table-style-quality-fixes --look-only`` 只落盘安全的 ``tblLook`` 修复，并用 ``scripts/run_table_style_quality_visual_regression.ps1`` 固化 before/after Word 渲染、contact sheet 和像素摘要证据包，同时通过 ``check-table-style-look`` 检查表格实例 ``tblLook`` 与条件区域是否一致，或通过 ``repair-table-style-look`` 应用安全的标志修复。
现在也已经补入表格实例层面的第一版浮动定位 API：
``Table::set_position(...)``、``position()`` 和 ``clear_position()`` 会读写
``w:tblpPr`` 的水平 / 垂直参照以及 twips 偏移，适合先覆盖“表格脱离普通
文档流并相对页面 / 边距 / 段落定位”的常见模板场景。后续仍需要继续补齐
更完整的复杂版式属性面。

后续建议集中在：

- 扩展 custom table style 已建模属性覆盖面，特别是条件区域中的复杂属性
- 继续补齐浮动表格的环绕距离、重叠控制等 ``w:tblpPr`` 细节
- 为常见正式文档版式沉淀 table style + ``tblLook`` + floating position preset
- 将 table style 质量计划和真实模板迁移报告联动，减少人工判断成本

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

当前已经有页码相关字段能力，并补齐了第一版通用字段 typed API：

- ``list_fields()`` 可枚举 ``w:fldSimple`` 字段、字段类型、指令、结果文本、dirty / locked 标记
- ``append_field(...)`` 可插入任意简单字段指令
- ``append_table_of_contents_field(...)`` 覆盖 TOC / 目录字段常见 ``\o``、``\h``、``\z``、``\u`` 选项
- ``append_reference_field(...)`` 覆盖 REF 书签引用字段
- ``append_page_reference_field(...)`` 覆盖 PAGEREF 页码引用字段，可选择 hyperlink / relative-position / preserve-formatting
- ``append_style_reference_field(...)`` 覆盖 STYLEREF 样式引用字段，可选择 paragraph-number / relative-position / preserve-formatting
- ``append_document_property_field(...)`` 覆盖 DOCPROPERTY 文档属性字段
- ``append_date_field(...)`` 覆盖 DATE 日期字段和 Word 日期格式
- ``append_hyperlink_field(...)`` 覆盖 HYPERLINK 目标、anchor 与 tooltip
- ``append_sequence_field(...)`` 覆盖 SEQ 编号字段和 restart 值
- ``append_caption(...)`` 覆盖题注段落和题注编号字段
- ``append_index_entry_field(...)`` / ``append_index_field(...)`` 覆盖 XE 索引项和 INDEX 索引字段
- ``field_state_options`` / CLI ``--dirty``、``--locked`` 可写入简单字段状态
- ``enable_update_fields_on_open()`` / CLI ``set-update-fields-on-open`` 可写入 ``w:updateFields`` 打开时刷新开关
- ``append_complex_field(...)`` / CLI ``append-complex-field`` 可写入 begin / separate / end 复杂域，并支持一个嵌套字段片段
- ``replace_field(...)`` 可按枚举索引替换字段指令和显示结果
- ``scripts/run_generic_fields_visual_regression.ps1`` 已固化 Word PDF 渲染证据和 ``word/settings.xml`` 证据

注意：库本身不负责“计算 Word 字段结果”。当前目标是结构化插入、检查字段，
并可让 Word 在打开文档时自动刷新字段。PAGEREF、STYLEREF、DOCPROPERTY、DATE、
HYPERLINK、题注、XE 索引项、INDEX 索引字段和打开时刷新开关已经有第一版
typed insert / CLI 一次性命令；后续可继续扩展更多复杂域 builder 和更完整的主动刷新策略。


二、OMML 公式 typed API
^^^^^^^^^^^^^^^^^^^^^^^

轻量 OMML 路线已落地，当前覆盖：

- ``list_omml()`` 枚举正文 / 模板部件中的 ``m:oMath`` 与 ``m:oMathPara``
- ``append_omml(...)`` 插入已有 OMML 片段，并自动补齐 ``xmlns:m``
- ``replace_omml(...)`` 按枚举索引替换公式，display OMML 会保持段落内结构
- ``remove_omml(...)`` 按索引移除公式
- ``make_omml_*()`` 已覆盖文本、分式、上下标、根式、定界符和 n-ary
  运算符等常见轻量片段
- ``make_omml_delimiter(...)`` 覆盖括号、方括号等定界符公式片段
- ``make_omml_nary(...)`` 覆盖求和、积分等带上下限的 n-ary 公式片段
- ``inspect-omml`` / ``append-omml`` / ``replace-omml`` / ``remove-omml``
  已提供 CLI 一次性 inspection / mutation
- ``scripts/run_omml_visual_regression.ps1`` 已固化 Word PDF / PNG 渲染证据

后续仍可继续补更完整公式布局 builder 和更复杂的 OMML 结构构造器。


三、批注、修订与审核对象
^^^^^^^^^^^^^^^^^^^^^^^^

正式合同和制度文件经常需要批注和修订痕迹。当前轻量审阅路线已经落地：

- ``list_footnotes()`` / ``list_endnotes()`` 可枚举脚注和尾注正文
- ``list_comments()`` 可枚举批注作者、时间、锚点原文、resolved 状态、
  回复父批注关系和正文
- ``list_revisions()`` 可枚举插入、删除、移动和属性变更类修订
- ``append_insertion_revision(...)`` / ``append_deletion_revision(...)`` 可生成
  第一版正文插入 / 删除修订痕迹
- ``insert_run_revision_after(...)`` / ``delete_run_revision(...)`` /
  ``replace_run_revision(...)`` 可基于正文段落 / run 索引生成原位插入、
  删除和替换修订痕迹
- ``insert_paragraph_text_revision(...)`` / ``delete_paragraph_text_revision(...)`` /
  ``replace_paragraph_text_revision(...)`` 可基于正文段落纯文本 offset / length
  生成跨 run 区间插入、删除和替换修订痕迹
- ``insert_text_range_revision(...)`` / ``delete_text_range_revision(...)`` /
  ``replace_text_range_revision(...)`` 可基于正文起止段落和纯文本 offset
  生成跨段落半开区间插入、删除和替换修订痕迹
- ``preview_text_range(...)`` 可在写入批注或修订前预览正文区间的选中文本、
  分段文本和 plain-text run 支持状态
- ``find_text_ranges(...)`` / ``find-text-ranges`` 可按精确正文文本查找匹配
  位置，返回可直接复用的段落索引、纯文本 offset、分段 preview 和
  plain-text run 支持状态，为后续自动生成审阅修订计划提供定位基础
- ``build-review-mutation-plan`` 可读取 ``find_text`` / ``occurrence`` 请求，
  将自然文本定位解析成带 ``expected_text`` guard 的 JSON 修订计划；生成的
  plan 可继续交给 ``preview-review-mutation-plan`` 预检或
  ``apply-review-mutation-plan`` 批量写入，减少调用方手写 offset 的风险；
  请求还可带 ``before_text`` / ``after_text`` 上下文过滤，在重复正文文本中
  通过邻近文本稳定选择目标匹配，并在 JSON 输出中回显 raw / filtered match
  计数和选中的原始匹配序号；若调用方设置 ``require_unique``，过滤后仍然
  不是唯一匹配时会在写出 plan 前失败，避免批量修订误选目标；插入型请求
  可通过 ``insert_after_match`` 选择写入匹配文本前或匹配文本后，并生成
  不携带 ``expected_text`` 的零长度插入 plan；批注型请求可生成
  ``append_paragraph_text_comment`` / ``append_text_range_comment`` plan，并
  携带 ``comment_text``、``expected_text``、author / initials / date 元数据，
  让批量审阅意见和批量修订共用同一套定位、预检与应用入口
- 段落 / 跨段落 text range 修订 API 的删除和替换 options 支持
  ``expected_text``，CLI 对应命令也支持 ``--expected-text``；写入前会通过
  ``preview_text_range(...)`` 校验目标文本，避免 offset 漂移造成误改；
  mismatch 诊断会携带起止 offset 和分段 preview，便于自动化定位漂移来源
- ``preview-review-mutation-plan`` 可读取 JSON 批量计划，对段落文本区间和
  跨段落文本区间批注、既有批注 resolved 状态变更、批注回复追加、
  批注 / 回复正文改写、删除、元数据更新，以及修订插入 / 删除 / 替换操作
  逐项执行只读预检，并在 JSON 输出中返回 ``expected_text``、实际选中文本、
  批注正文、父批注索引、批注当前 resolved 状态和分段 preview，供批量
  审阅编辑在写入前拦截漂移
- ``apply-review-mutation-plan`` 可复用同一 JSON 计划批量写入修订和批注；
  执行前会对全部操作先做 preview / ``expected_text`` guard；仍会拒绝涉及
  正文变更的重叠范围，但允许多个只读批注锚点重叠或嵌套，并保留
  ``inspect-review`` 可审计的完整外层锚点文本；``set_comment_resolved``
  plan 还可通过 ``comment_index``、``expected_text`` 与
  ``expected_resolved`` 批量关闭或重开既有批注；``append_comment_reply``
  plan 可用相同 guard 批量追加线程化回复；``set_comment_metadata`` plan
  可结合 ``expected_comment_text`` 与 ``expected_parent_index`` 批量更新
  批注 / 回复 author、initials 和 date；``replace_comment`` /
  ``remove_comment`` plan 可用相同 guard 批量改写或删除批注正文，删除父批注时
  会同步清理其回复
- ``accept_revision(...)`` / ``reject_revision(...)`` 以及批量接受 / 拒绝入口
  已覆盖第一版修订清理工作流
- ``set_revision_metadata(...)`` 可设置或清除既有修订的 author 和 date
- ``append_footnote(...)`` / ``replace_footnote(...)`` / ``remove_footnote(...)``
  和对应 endnote API 已覆盖脚注 / 尾注创建、改写和删除
- ``append_comment(...)`` / ``replace_comment(...)`` / ``remove_comment(...)``
  已覆盖批注创建、改写和删除；创建时可写入 author / initials / date
- ``append_comment_reply(...)`` 可通过 ``commentsExtended.xml`` 为既有批注
  追加线程化回复，并可写入 author / initials / date；删除父批注时会同步
  清理子回复
- ``append_paragraph_text_comment(...)`` / ``append_text_range_comment(...)``
  可把批注原位绑定到正文段落纯文本区间或跨段落半开区间
- ``set_paragraph_text_comment_range(...)`` /
  ``set_text_range_comment_range(...)`` 可移动或扩缩已有批注锚点范围
- ``set_comment_resolved(...)`` 可通过 ``commentsExtended.xml`` 设置或清除
  既有批注的 resolved / done 状态
- ``set_comment_metadata(...)`` 可设置或清除既有批注 / 回复的 author、
  initials 和 date
- ``inspect-review`` 以及脚注、尾注、批注、append 修订、run 原位修订、
  段落文本区间修订和跨段落文本区间修订 mutation CLI 已可用于一次性
  自动化处理
- ``compare_semantic(...)`` 与 ``semantic-diff`` 已将脚注、尾注、批注和修订
  纳入语义 diff，JSON 会输出 ``footnote_changes``、``endnote_changes``、
  ``comment_changes`` 与 ``revision_changes``，CLI 可用 ``--no-footnotes`` /
  ``--no-endnotes`` / ``--no-comments`` / ``--no-revisions`` 分别关闭
- ``scripts/run_review_inspection_visual_regression.ps1`` 与
  ``scripts/run_review_mutation_visual_regression.ps1`` 已保留 Word 渲染证据

建议中期继续补：

- comment 回复、批注多范围 / 线程化元数据 API
- 复杂 run 区间诊断、段落标记级修订和更细粒度冲突诊断
- 审阅对象与复杂模板合并时的冲突解决策略

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
并通过 ``replace-content-control-text``、``replace-content-control-paragraphs``、
``replace-content-control-table``、``replace-content-control-table-rows`` 和
``replace-content-control-image`` 提供 CLI 一次性替换命令。
C++ 层也已经补齐 ``replace_content_control_with_paragraphs_by_*()``、
``replace_content_control_with_table_rows_by_*()``、
``replace_content_control_with_table_by_*()`` 和
``replace_content_control_with_image_by_*()``，可按 tag / alias 将块级内容控件
替换为多段落、将行级内容控件替换为多行、将块级内容控件替换为整表，
或替换为内联图片，并用
``scripts/run_content_control_rich_replacement_visual_regression.ps1`` 与
``scripts/run_content_control_image_replacement_visual_regression.ps1``
保留 Word PDF 渲染证据。
现在内容控件也已经可以通过 ``content_control_tag`` /
``content_control_alias`` 进入 template schema slot 导出、校验、normalize、
patch 与 diff 工作流。

当前还新增了轻量表单状态 inspection / mutation：``content_control_summary`` 会返回 ``form_kind``、``lock``、``w:dataBinding`` 的 store item / XPath / prefix mappings、复选框 ``checked``、日期格式 / locale、下拉 / 组合框列表项与当前选中项，``inspect-content-controls`` 的 JSON / 文本输出也会暴露这些字段；``set_content_control_form_state_by_tag(...)`` / ``set_content_control_form_state_by_alias(...)`` 与 CLI ``set-content-control-form-state`` 已支持 checkbox checked、下拉 / 组合框选中项、日期文本 / 格式 / locale、lock 设置 / 清除，以及 dataBinding 设置 / 清除。``sync_content_controls_from_custom_xml()`` 与 ``sync-content-controls-from-custom-xml`` 也已支持读取匹配 ``customXml/item*.xml``，按 ``w:dataBinding`` XPath 单向刷新内容控件显示文本，并通过 Word PDF / PNG 可视化回归固化。

下一步继续补齐：

- 表单保护、重复节和模板数据模型之间的同步策略
- 更复杂表单状态与外部数据源之间的双向同步（Custom XML → 内容控件单向刷新已完成）

价值：

- 与 Word 原生模板制作方式更兼容
- 可以弥补书签在可视化编辑和稳定定位上的不足


五、文档语义 diff 与变更报告
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

第一版已经落地。``document_semantic_diff_options``、
``document_semantic_diff_result`` 与 ``Document::compare_semantic(...)``
现在可以比较两个已打开文档的段落、表格摘要、drawing 图片、content
control 摘要、通用字段对象（包含 TOC / REF / SEQ 等字段的 kind、instruction、
result_text、dirty / locked、complex 与 depth 元数据）、样式摘要、编号定义、
脚注、尾注、批注、修订、section/page setup 摘要，以及 typed 的 ``body`` /
``header`` / ``footer`` 模板 part 结果；默认忽略图片
relationship id 与 content control id
这类噪音，并默认启用内容感知序列对齐以减少插入项导致的级联误报；必要时
可通过 options 或 CLI 开关回退。CLI ``semantic-diff <left.docx> <right.docx>``
会输出人类可读摘要或稳定 JSON，并支持 ``--fail-on-diff`` 作为 CI / release
gate。

已经完成：

- 比较段落、表格摘要、图片摘要、content control 摘要、通用字段对象、样式摘要、编号定义、脚注、尾注、批注、修订、section/page setup 摘要
- 输出 JSON 和人类可读摘要，changed 项同时包含字段级 ``field_changes``
- JSON 顶层包含 ``fields`` / ``field_changes``，字段对象会输出 kind、instruction、
  result_text、dirty / locked、complex、depth 等可审查元数据；CLI 可用
  ``--no-fields`` 跳过字段对象比较
- JSON 顶层包含 ``styles`` / ``style_changes`` 与 ``numbering`` /
  ``numbering_changes``，样式按 ``style_id``、编号按 ``definition_id`` 匹配，
  可审计 style name / based_on / numbering link 和 numbering level kind / start /
  pattern 等变化；CLI 可用 ``--no-styles`` / ``--no-numbering`` 分别关闭
- JSON 顶层包含 ``footnotes`` / ``footnote_changes``、``endnotes`` /
  ``endnote_changes``、``comments`` / ``comment_changes`` 和 ``revisions`` /
  ``revision_changes``，可审计审阅对象的 kind、id、author、date、part 与 text
  变化；CLI 可用 ``--no-footnotes`` / ``--no-endnotes`` / ``--no-comments`` /
  ``--no-revisions`` 分别关闭
- JSON 顶层包含 ``template_parts``，并通过 ``template_part_results`` 暴露
  ``body`` / ``header`` / ``footer`` 的 typed part 明细；同时输出 resolved
  ``section-header`` / ``section-footer`` 视图，带 ``section_index``、
  ``reference_kind`` 和左右 ``*_resolved_from_section_index``，用于审计继承页眉页脚
  而不重复计入物理 header/footer 总数；CLI 可用 ``--no-template-parts`` 关闭该层，
  或用 ``--no-resolved-section-template-parts`` 只保留物理 part 明细
- 默认忽略 transient relationship id / content control id，并提供开关
- 默认启用内容感知序列对齐，插入段落 / 表格等不再造成整段级联误报；CLI 可用
  ``--index-alignment`` 回退到位置对齐，或用 ``--alignment-cell-limit <count>``
  限制对齐成本
- 用 ``samples/sample_semantic_diff_visual.cpp`` 与
  ``scripts/run_semantic_diff_visual_regression.ps1`` 生成 before/after DOCX、
  CLI JSON、Word PDF 和 PNG contact sheet 证据，并覆盖 style-linked numbering
  的样式 / 编号 diff；字段专项还通过
  ``scripts/run_generic_fields_visual_regression.ps1`` 生成 TOC / REF / SEQ
  semantic diff JSON 和左右 Word 渲染证据

后续可继续扩展：

- 更复杂跨 part 匹配策略，例如 section-header / section-footer 解析视图
- 审阅对象与模板升级场景之间的冲突诊断报告
- 针对模板升级场景输出更细的审查报告


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

- content control 复杂表单保护、重复节和模板数据模型双向同步策略（Custom XML 单向刷新已完成）
- 模板 schema approval / release blocker 摘要继续接入发布 gate
- 真实项目模板 smoke manifest 的样例、审批历史和回归证据补强

阶段目标：让一份真实业务模板可以更快接入、校验、生成和回归。


第二阶段：文档骨架治理
^^^^^^^^^^^^^^^^^^^^^^

优先做：

- exemplar 文档到 numbering catalog JSON 的自动提取与冲突审计
- ``repair-style-numbering`` 建议到 catalog patch 的衔接
- style usage report 驱动的 batch audit 与自动建议
- style refactor plan 的真实语料置信度校准与 merge restore 批量选择增强
- heading / list / theme 的组合治理 helper

阶段目标：让标题、列表、样式继承、编号实例不再依赖人工修 Word。


第三阶段：版式与正式文档对象补齐
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

优先做：

- 更完整 OMML builder（轻量 text/fraction/script/radical/delimiter/n-ary 构造器与 CLI list / append / replace / remove 已完成）
- 批注 / 修订 authoring API，例如更复杂的多范围批注与修订插入 / 删除
- 更多复杂域 typed builder 和更完整主动字段刷新策略（复杂域写入、单层嵌套和打开时刷新开关已完成）

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
