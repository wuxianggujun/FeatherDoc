项目评分评估
============

评估结论
--------

FeatherDoc 当前综合评分为 **84 / 100**。

这是一次基于仓库内容的静态评估。评估范围包括项目结构、CMake
工程化、公开 API、核心源码体量、CLI 组织、测试组织、文档与发布
资料。按照本次评估要求，未执行构建、测试或任何耗时验证命令。

总体判断：FeatherDoc 已经不是玩具项目，而是一个具备明确产品方向、
较完整工程链路和丰富验证资产的中高级 C++ 开源库项目。当前最主要
的改进空间不在功能数量，而在 release governance 证据链、一致性的
脚本契约治理，以及大规模代码后的长期维护成本。


分项评分
--------

- **产品定位：8.5 / 10**

  项目定位清晰，聚焦 Microsoft Word ``.docx`` 的创建、读取、编辑、
  模板填充和可视化验证。目标用户和核心场景明确。

- **工程化：9 / 10**

  CMake 项目结构、安装导出、``find_package`` 消费、CI 和发布脚本
  都比较完整。Linux / macOS / Windows 三条 GitHub Actions 路径已经
  具备稳定的 configure、build、test 和 smoke 入口；Windows 还承担
  安装验证、release metadata bundle 和 public release asset preview。

- **功能完整度：8.5 / 10**

  已覆盖段落、Run、表格、图片、样式、编号、页眉页脚、模板部件、
  内容控件基础链路、schema patch 和重新打开后继续编辑等核心能力。
  Word 渲染级视觉验证是明显亮点。

- **文档质量：9 / 10**

  中英文 README、视觉验证说明、发布策略、路线说明和自动化流程文档
  较完整，对新用户和维护者都有帮助。

- **测试与验证：8.5 / 10**

  测试、脚本和视觉回归资产数量充足，说明项目重视质量。Linux 和
  macOS 已经固定运行 ``cli_smoke`` / ``release_smoke``，Windows 还会
  继续跑样例文档、安装 smoke 和发布材料预览。当前压力主要转移到
  大型 PowerShell fixture、release governance 契约测试和 reviewer-facing
  材料的一致性维护。

- **代码可维护性：7 / 10**

  主要短板已经不再是 CLI 入口过度集中，而是 CLI、测试和脚本都进入
  “文件很多、契约很多”的阶段。CLI 主入口已很轻，命令实现也拆成了
  大量 parse / command / output 单元；当前更需要治理的是跨文件
  discoverability、命令注册索引和脚本契约的重复定义。

- **公开 API 设计：7.5 / 10**

  ``std::error_code``、``last_error()`` 和结构化错误信息是优点。
  但部分返回 ``bool`` 的接口语义还不够自解释，未来可以考虑更统一的
  ``result`` / ``expected`` 风格。

- **跨平台与发布成熟度：8 / 10**

  Linux GCC/Clang、macOS 和 Windows MSVC 都已经进入稳定 GitHub
  Actions。当前短板已经转为覆盖深度不对称：
  Word visual validation、部分 release material preview 和 reviewer-facing
  bundle 仍主要依赖 Windows 路线。


功能评分
--------

如果只看功能能力，不把代码结构、测试组织、文档质量和发布工程计入
主要权重，FeatherDoc 的功能评分为 **86 / 100**。

这个分数对应的是“实用型现代 C++ DOCX 编辑库”标准。FeatherDoc 的
功能面已经明显超过普通示例项目，尤其在表格、模板、样式、编号和
CLI 自动化方面比较突出。但如果按商业级完整 Word SDK 的标准衡量，
仍需要继续补齐完整域引擎、复杂表单、批注回复 / 修订 authoring、
复杂引用对象、图表、完整公式构造器和更复杂的 Word 对象能力。

功能分项如下：

- **文档创建、打开与保存：9 / 10**

  支持新建、打开、保存、另存和 reopen-after-save 持续编辑，基础
  文档生命周期能力比较扎实。

- **段落与 Run 编辑：8.5 / 10**

  支持文本、Run、字体、语言、RTL、插入前后内容等常见文本编辑场景。

- **表格能力：9 / 10**

  支持表格、行、列、单元格、合并、拆分、宽度、边框、填充、边距、
  表格样式定义和第一版浮动表格定位等操作，是当前功能体系中的强项。

- **图片能力：8 / 10**

  支持追加图片、书签图片、浮动图片和层级顺序等，不只是简单插图。

- **样式系统：8.5 / 10**

  覆盖段落样式、字符样式、表格样式、默认 Run 属性和样式重构等场景。

- **编号与列表能力：8.5 / 10**

  支持段落编号、样式绑定编号、重启列表、编号目录导入导出等能力。

- **模板与书签能力：9 / 10**

  支持书签文本替换、content control 纯文本替换、块显示隐藏、表格行填充、
  schema patch / baseline gate 和 JSON 数据渲染，适合报表、合同、发票等
  业务文档生成场景。

- **页眉页脚与 Section：8 / 10**

  支持页眉、页脚、section scoped 操作、页面设置和页码字段，已经覆盖
  一批复杂文档需求。

- **CLI 功能面：9 / 10**

  CLI 命令非常丰富，适合自动化、脚本集成和批处理工作流。

- **高级 Word 特性覆盖：7 / 10**

  当前还不是完整 Word SDK。批注、修订、脚注、尾注、目录 / REF / SEQ、
  内容控件富内容和图片替换已经有轻量 API 或第一版闭环，但复杂表单、
  完整域引擎、图表和完整公式构造器仍是未来提升空间。

按不同定位换算，功能成熟度可以这样理解：

- 作为轻量 DOCX 生成库：**92 / 100**。
- 作为实用 DOCX 编辑库：**86 / 100**。
- 作为商业级完整 Word SDK：**68-72 / 100**。

功能层面的一句话评价：

    FeatherDoc 的功能已经很能打，尤其是表格、模板、样式、编号和
    CLI 自动化；下一阶段如果补齐完整域引擎、审阅 authoring、复杂引用对象
    和复杂表单等高级 Word 特性，功能分有机会提升到 90 分以上。


功能缺失问题
------------

从当前项目定位看，FeatherDoc 已经能覆盖大量实用文档生成和编辑场景，
但距离“完整 Word SDK”仍有一些明显功能缺口。下面这些缺口不代表都要
立刻实现，而是后续规划功能边界和优先级时需要明确取舍。

1. **批注与审阅修订能力仍需深化**

   当前已经有脚注、尾注、批注、修订的轻量 inspection，脚注 / 尾注 / 批注
   创建修改删除入口，以及修订接受 / 拒绝入口。但批注回复、已解决状态、
   修订插入 / 删除 authoring 和复杂审阅冲突处理还没有形成完整工作流。
   若面向合同、法务、协同审阅场景，
   这是优先级较高的缺口。

2. **脚注、尾注与引用类内容还不完整**

   学术文档、法律文档和长报告常需要脚注、尾注、题注、交叉引用和索引。
   当前项目已经可以枚举、创建、修改、删除脚注 / 尾注，并能插入
   TOC / REF / PAGEREF / STYLEREF / DOCPROPERTY / DATE / HYPERLINK / SEQ
   等简单字段，且支持题注、XE / INDEX 索引字段以及 dirty / locked
   状态写入、复杂域 begin / separate / end 写入、单层嵌套字段片段，并支持 ``w:updateFields`` 打开时刷新开关；更多复杂域 builder 和主动字段刷新策略仍需补齐。

3. **目录与通用域能力不完整**

   项目已覆盖页码、目录、REF、PAGEREF、STYLEREF、DOCPROPERTY、DATE、
   HYPERLINK 和 SEQ 等简单字段结构写入，但还不是完整域引擎。图表目录、
   更高阶复杂交叉引用、多层复杂域 builder 和主动域更新仍可能依赖 Word 本身刷新；打开时刷新可通过 ``w:updateFields`` 开关启用。

4. **内容控件富内容与表单能力不足**

   当前已经具备 content control 枚举、tag / alias 过滤、纯文本替换、段落 / 表格
   / 图片替换，并已纳入 template schema slot 导出和校验；轻量 inspection
   也能报告复选框、下拉框、日期、锁定状态和 ``w:dataBinding`` 元数据，
   mutation 已覆盖 checkbox、下拉 / 组合框、日期文本 / 格式 / locale、lock
   设置 / 清除以及 dataBinding 设置 / 清除。
   企业模板常用的复杂表单保护、重复节和外部 Custom XML 同步仍需要独立设计 API。

5. **图表、SmartArt、公式和嵌入对象缺失**

   当前优势集中在文档结构、文本、表格、图片、样式和模板。图表、
   SmartArt、Office 数学公式、OLE 嵌入对象等复杂部件还不是主力能力。

6. **完整主题与高级排版控制仍有限**

   Word 的主题字体、主题颜色、兼容性设置、复杂分页规则、分栏、文本框、
   浮动对象环绕和复杂版式组合仍有大量边界。当前能力更适合可控模板和
   有限结构编辑。

7. **复杂历史文档导入鲁棒性仍需加强**

   真实企业文档可能来自 WPS、LibreOffice、旧版 Word、在线 Office 或
   第三方系统，内部 XML 结构差异很大。项目若要处理任意历史文档，需要
   更多损坏样本、兼容样本和修复策略。

8. **受保护、加密、宏和外部关系策略需要明确**

   加密文档、密码保护、宏、外部链接、嵌入文件和远程关系会涉及安全与
   兼容策略。即使不支持，也建议在文档中明确“检测、拒绝、保留或清理”
   的行为边界。

9. **高级模板语义还可以继续扩展**

   当前书签和 JSON 渲染已经实用，但条件渲染、循环块、嵌套表格、数据
   格式化、模板校验诊断和模板版本迁移仍可继续增强。

如果项目目标是轻量、可控、可维护的 DOCX 编辑库，可以优先补“完整域引擎、
内容控件复杂表单、复杂引用对象、批注修订 authoring”这四类高价值能力；如果目标
升级为完整 Word SDK，则还需要继续投入图表、公式、SmartArt、嵌入对象和复杂布局。


主要优势
--------

1. **项目方向清楚**

   FeatherDoc 不是泛泛而谈的文件库，而是围绕 ``.docx`` 真实编辑、
   模板渲染和 Word 视觉结果验证展开。

2. **工程链路比较完整**

   项目已经具备现代 CMake 集成、安装导出、CLI 构建、样例程序、测试
   入口、发布素材和自动化脚本。

3. **文档投入明显**

   文档不是事后补丁，而是项目资产的一部分。README、视觉验证、发布
   指南和路线说明都能支撑项目继续演进。

4. **质量意识强**

   项目不仅检查 XML 结构，还引入 Word 渲染截图、视觉回归和发布门禁，
   这对 ``.docx`` 这种强布局格式非常重要。

5. **真实用户场景丰富**

   中文发票模板、表格编辑、书签替换、图片插入、页眉页脚、编号目录等
   场景说明项目已经经历了不少真实需求驱动。


主要风险
--------

1. **CLI 与命令索引的可发现性压力仍然偏高**

   CLI 主入口已经收敛成轻量壳，命令实现也从单体文件拆到了大量
   parse / command / output 单元。但复杂度并没有消失，而是转移到
   dispatch、usage、CMake target 列表和跨领域命令家族的组织方式上。

2. **测试与脚本夹具的复杂度正在取代旧的大文件风险**

   基础 C++ 测试文件已经明显拆分，当前维护压力更多来自大型
   PowerShell fixture、release governance / material safety 契约测试和
   多平台 smoke 标签矩阵。

3. **公开头文件职责偏重**

   ``include/featherdoc.hpp`` 暴露了大量类型和能力。随着功能继续扩展，
   编译依赖、API 稳定性和用户理解成本都会提高。

4. **跨平台验证仍有覆盖深度不对称**

   Linux、Clang、GCC 和 macOS 已经进入常规 CI，但 Word visual
   validation、release metadata bundle 预览和部分 reviewer-facing 材料
   仍然主要依赖 Windows。后续更需要守的是“平台间证据一致性”，而不
   是单纯再补一条 CI。

5. **大脚本与发布治理资产仍需持续收口**

   ``scripts`` 目录能力非常多，reviewer-facing bundle、handoff、
   manifest signoff 和 release material safety 之间还存在大量同字段、
   同语义的重复透传逻辑。后续需要继续统一 helper、错误输出和入口说明，
   并让 ``docs/script_task_index_zh.rst`` 保持对齐。


优先改进建议
------------

短期建议优先做三件事：

1. **继续收口 release governance / schema calibration 证据链**

   优先继续做 ``P1-SCHEMA-01``，把 ``business_document_type`` /
   ``corpus_role`` 的 missing / mismatch 计数、warning、action 和
   reviewer runbook 继续稳定透传到 handoff、final review、bundle 和
   material safety 负例。

2. **治理大脚本资产和 reviewer-facing bundle 契约**

   当前最值得做的不是再做一轮“拆文件”，而是把 release metadata、
   reviewer checklist、artifact guide、final review、material safety
   之间重复的字段 contract 和 helper 继续收拢。

3. **继续提升多平台 smoke 与 Windows 专属 gate 的一致性**

   Linux / macOS / Windows 的 build、test、``cli_smoke``、
   ``release_smoke`` 已经跑起来了；下一步更该确保 workflow、CTest label、
   发布材料契约和 Windows visual / release preview 的分层边界继续清晰。

中期可以继续推进：

- 统一 ``bool`` 返回接口的错误语义，逐步引入更明确的结果类型。
- 将模板、样式、编号、图片、表格等领域能力拆成更清晰的内部模块。
- 为 DOCX ZIP/XML 输入增加恶意样本、损坏样本和 fuzz 风格回归。
- 继续维护 ``docs/script_task_index_zh.rst`` 脚本任务索引，并用
  ``scripts/check_script_task_index.ps1`` 固定索引路径、文档入口和轻量 CI
  注册的一致性。


评分总结
--------

如果按个人或独立开发项目标准看，FeatherDoc 已经非常优秀。它有明确
定位、真实功能、工程链路、文档资产和质量验证意识。

如果按工业级长期维护库标准看，当前主要差距已经从“CLI 入口集中”和
“平台 CI 基线”转移到 release governance 契约复杂度、脚本资产治理和
跨平台覆盖深度一致性。只要继续收拢 reviewer-facing bundle 字段
contract、减少重复脚本逻辑，并守住多平台 smoke 与 Windows visual
gate 的分层验证，项目有机会从 **84 分** 提升到 **90 分以上**。

一句话评价：

    功能和工程化已经很强，下一阶段的关键是让治理证据链和维护复杂度一起收口。
