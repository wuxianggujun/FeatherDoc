Document
========

``featherdoc::Document`` 是 ``.docx`` 文档包的根对象。通常先用它打开或创建
文档，再通过正文、页眉、页脚、分节和检查 API 完成编辑与验证。

常用任务入口
------------

.. FDOC_ZH_CN_DOCUMENT_COMMON_TASKS

已经知道目标工作流时，可以从这里开始：

* 打开和保存文档：使用 ``Document(path)``、``open()``、``save()`` 和
  ``save_as(path)``。
* 填充正文模板：使用 ``body_template()``，或直接调用文档级
  ``replace_content_control_text_by_tag(...)`` 快捷入口。
* 修改页眉或页脚：使用 ``header_template(...)``、``footer_template(...)``、
  ``section_header_template(...)`` 或 ``section_footer_template(...)``。
* 直接编辑正文内容：使用 ``paragraphs()``、``tables()``、``append_table(...)``
  和图片追加方法。
* 修改前检查结构：使用 ``inspect_sections()``、``inspect_body_blocks()``、
  ``list_bookmarks()``、``list_content_controls()`` 和 ``last_error()``。

成功/失败语义
-------------

.. FDOC_ZH_CN_DOCUMENT_SUCCESS_FAILURE_SEMANTICS

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - 返回形态
     - 成功含义
     - 失败或无变化含义
   * - ``std::error_code``
     - 空错误码表示包操作完成。
     - 非空错误码表示操作失败；用 ``last_error()`` 查看细节和包内条目。
   * - ``bool``
     - ``true`` 表示目标文档 XML 或包元数据已变更。
     - ``false`` 表示目标不可用、参数无效，或没有可应用的变化。
   * - ``std::size_t``
     - 非零值表示匹配并修改或追加的项目数量。
     - ``0`` 表示没有匹配的书签、内容控件、批注目标、超链接或脚注目标。
   * - ``std::optional<T>``
     - 包含请求的检查结果或设置值。
     - 空值表示分节、设置或语义对比结果无法解析。
   * - 句柄对象
     - 返回的 ``TemplatePart``、``Paragraph``、``Table`` 等句柄是下一步编辑入口。
     - 使用前应按对应对象页的有效性规则确认目标存在。

``bool`` 修改方法的 ``false`` 是“未发生修改”的合并信号，不是结构化错误码；
它可能表示目标无效、参数不适用，或目标本来已经处于请求状态。只有某个方法的
文档明确承诺设置 ``last_error()`` 时，调用方才能用它区分失败原因，也不能把上一次
操作遗留的错误当作当前 ``false`` 的原因。需要可靠区分这些状态时，应先检查
``is_open()``、句柄 ``valid()`` 和目标当前值，再比较修改后的状态。``1.13.x`` 为保持
源码兼容，不批量改变现有 ``bool`` 签名；下一次破坏性 API 版本再统一引入结构化
mutation result。

短 C++ 示例
------------

.. FDOC_ZH_CN_DOCUMENT_SHORT_EXAMPLE

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) return 1;

   doc.replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

类型化签名导读
--------------

.. FDOC_ZH_CN_DOCUMENT_TYPED_SIGNATURE_GUIDE

``Document`` 是文档包级句柄。处理已有文件时先调用 ``open()``，创建新包时
先调用 ``create_empty()``，再使用编辑方法。大多数下标都从 0 开始。路径参数
表示文件系统路径；文本参数会写入解析后的 WordprocessingML 目标。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``explicit Document(std::filesystem::path path)``
     - ``path``：源文件或目标 ``.docx`` 路径。
     - 创建句柄；调用 ``open()`` 前不会加载文档包。
   * - ``Document(Document &&other) noexcept`` / ``operator=(Document &&other) noexcept``
     - ``other``：源文档对象。
     - 转移文档包状态，使源、目标对象此前返回的句柄全部失效；源对象变为关闭状态，
       但仍可复用。
   * - ``std::error_code create_empty()``
     - 无。
     - 空错误码表示新文档包已初始化。
   * - ``std::error_code open()``
     - 无。
     - 空错误码表示当前路径已加载。
   * - ``std::error_code save_as(std::filesystem::path path) const``
     - ``path``：非空输出 ``.docx`` 路径。
     - 空错误码表示文档包已写入新路径。
   * - ``TemplatePart body_template()``
     - 无。
     - 返回正文模板部件句柄；编辑前应检查句柄有效性。
   * - ``TemplatePart header_template(std::size_t index = 0U)``
     - ``index``：物理页眉部件下标。
     - 返回该物理部件的页眉模板部件句柄。
   * - ``TemplatePart section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``reference_kind``：默认页、首页或
       偶数页页眉引用。
     - 返回解析后的分节页眉模板部件句柄。
   * - ``bookmark_fill_result fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``：书签名和文本绑定。
     - 返回匹配、替换、请求和缺失书签统计。
   * - ``std::size_t replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``：内容控件 tag。``replacement``：插入文本。
     - 返回被替换的匹配控件数量；``0`` 表示未命中。
   * - ``Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` 和 ``column_count``：初始正文表格形状。
     - 返回新建的正文表格句柄。
   * - ``bool set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``：目标分节。``setup``：页面尺寸、边距和方向数据。
     - 分节页面设置写入成功时返回 ``true``。
   * - ``std::optional<section_page_setup> get_section_page_setup(std::size_t section_index) const``
     - ``section_index``：目标分节。
     - 分节或页面设置无法解析时返回空值。

生命周期
--------

用于打开、创建、保存文档，并检查当前包状态。

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``Document()``
     - 无。
     - ``Document``
     - 创建空句柄；调用 ``open()`` 前需要先设置路径。
   * - ``explicit Document(std::filesystem::path path)``
     - ``path``：源文件或目标 ``.docx`` 路径。
     - ``Document``
     - 创建绑定到指定路径的文档句柄。
   * - ``Document(Document &&other) noexcept`` / ``operator=(Document &&other) noexcept``
     - ``other``：源文档对象。
     - ``Document`` / ``Document &``
     - 转移文档包状态；源对象变为关闭状态并可继续复用。
   * - ``create_empty()``
     - 无。
     - ``std::error_code``
     - 初始化一个新的空 ``.docx`` 文档包。
   * - ``set_path(std::filesystem::path path)``
     - ``path``：替换 ``open()`` 和 ``save()`` 使用的路径。
     - ``void``
     - 替换当前路径，并重置已经加载的包状态。
   * - ``path() const``
     - 无。
     - ``const std::filesystem::path &``
     - 返回当前绑定路径。
   * - ``open()``
     - 无。
     - ``std::error_code``
     - 以严格 OPC 校验加载当前路径对应的 ``.docx`` 文档包。
   * - ``open(const document_open_options &options)``
     - ``options``：校验模式和 ZIP 资源限制。
     - ``std::error_code``
     - 按指定策略加载文档；仅在修复历史损坏包时选择 ``tolerant``。
   * - ``package_diagnostics() const noexcept``
     - 无。
     - ``const std::vector<package_diagnostic> &``
     - 返回宽容打开期间发现的包结构问题、严重度、部件名和可修复标记。
   * - ``repair_package(const document_repair_options &options = {})``
     - ``options``：允许修复的确定性问题类别。
     - ``std::optional<package_repair_report>``
     - 事务式修复内存中的包结构；遇到不安全或被选项禁用的问题时不做部分修改。
   * - ``save() const``
     - 无。
     - ``std::error_code``
     - 将修改保存回当前路径。
   * - ``save_as(std::filesystem::path path) const``
     - ``path``：输出 ``.docx`` 路径，不能为空。
     - ``std::error_code``
     - 将修改另存为新路径。
   * - ``is_open() const``
     - 无。
     - ``bool``
     - 判断 ``open()`` 或 ``create_empty()`` 是否已经加载文档包。
   * - ``last_error() const noexcept``
     - 无。
     - ``const document_error_info &``
     - 查看最近一次失败的 ``code``、``detail``、``entry_name`` 和可选
       XML 偏移。
   * - ``enable_update_fields_on_open()``
     - 无。
     - ``bool``
     - 写入 ``w:updateFields``，让 Word 打开文件时自动更新域。
   * - ``clear_update_fields_on_open()``
     - 无。
     - ``bool``
     - 清除“打开时更新域”的设置。
   * - ``update_fields_on_open_enabled()``
     - 无。
     - ``std::optional<bool>``
     - 检查“打开时更新域”设置；为空表示当前无法读取该设置。

路径绑定与 UTF-8
-----------------

文件系统路径由 ``std::filesystem::path`` 传入；Windows 内部使用宽字符系统调用，传给
ZIP 层、错误详情和 CLI JSON 的路径统一转换为 UTF-8，因此中文、日文、emoji、空格和
启用长路径后的路径不会经过系统 ANSI code page。源码、文档、CLI stdout/stderr 和
JSON 也统一使用 UTF-8；MSVC 构建保持 ``/utf-8``。

成功 ``open()`` 时，FeatherDoc 会把源路径解析为当时工作目录下的绝对 I/O 路径并冻结。
后续懒加载图片、样式、编号、页眉页脚等部件以及 ``save()`` 重开源归档时都使用该冻结
路径，即使进程之后改变 current working directory，也不会误读同名文件。为保持调用方
兼容，``path()`` 仍原样返回传入的路径拼写。``save_as(relative_path)`` 则在每次调用时
按当时的 current working directory 解析输出路径；它不会改变 ``path()`` 或已冻结的源
归档路径。

保存事务与持久化语义
----------------------

``save()`` 和 ``save_as()`` 在目标同目录排他创建唯一临时文件，不使用固定
``.tmp``/``.bak`` 名称。ZIP 完整 finalize 后，临时文件会先执行用户态 flush 和文件
同步，再通过同文件系统原子替换发布为目标；POSIX 随后同步父目录，Windows 使用
write-through 的替换 API。POSIX 写入期间临时文件保持 ``0600``；替换已有目标时
保留其 mode bits，新目标使用进程 ``umask`` 计算后的权限。

错误边界如下：

* ``output_archive_open_failed``、条目写入错误、
  ``output_archive_finalize_failed``、``output_file_sync_failed``、
  ``package_repair_validation_failed`` 和 ``output_replace_failed`` 都发生在替换完成
  之前；原目标保持不变。
* ``output_directory_sync_failed_after_replace`` 只在原子替换已经成功、但 POSIX
  父目录持久化无法确认时返回。此时新目标已经可见，``last_error().detail`` 会明确
  说明该状态；调用方应重新打开目标核对内容，不能假设旧文件仍然存在。
* 成功返回表示 ZIP finalize、文件同步和平台替换步骤均成功；在支持父目录同步的
  平台上也表示目录项同步成功。

保存期裁剪与包修复不再使用 pugixml 无法报告失败的 ``reset(source)`` 复制。FeatherDoc
会在隔离 DOM 中逐项检查 node、name、value 和 attribute 分配，完整成功后才发布副本；
clone 或修复 mutation 分配失败会在部分 DOM 替换内存包状态或目标文件前返回
``std::errc::not_enough_memory``。

所有归档 reader、条目缓冲区、临时输出流和 ZIP writer 均由 RAII guard 持有。原子替换
前发生标准分配失败时，FeatherDoc 会关闭全部归档资源、删除唯一临时文件、保持已有目标
逐字节不变，并允许同一个 ``Document`` 实例直接重试。替换后的持久化路径不再构造新的
路径或诊断字符串，因此已经完成的替换不会在随后被误报为分配失败。

打开校验与资源限制
------------------

无参数 ``open()`` 等价于传入默认 ``document_open_options``。默认使用
``package_validation_mode::strict``，并要求 ``[Content_Types].xml``、根关系、
``word/document.xml`` 和 ``w:document/w:body`` 结构有效。需要读取历史损坏包做
修复时，调用方必须显式选择 ``package_validation_mode::tolerant``；宽容模式不会
关闭 ZIP 资源限制，也不会静默修改文档。
XML parser 分配失败属于运行故障，而不是历史包损坏：所有急切加载和懒加载 XML 部件
在两种模式下都必须以 ``std::errc::not_enough_memory`` fail closed，不能降级为 tolerant
诊断，也不能静默跳过 Custom XML item。``Default/@ContentType`` 或
``Override/@ContentType`` 缺失、空值或 MIME 语法损坏时，两种模式都会拒绝，因为库
无法安全推断其真实含义。调用方应读取 ``package_diagnostics()``，再显式调用
``repair_package()``，并使用 ``save_as()`` 写入新文件。

tolerant 模式是“检查并显式修复”的安全边界，不是通用的兼容编辑模式。真正缺失的
relationship part 可以由负责该关系的 API 创建；但如果已经存在的
``word/_rels/document.xml.rels`` 或页眉/页脚 ``.rels`` 的根节点名称或 package
namespace、Relationship 子节点 namespace 或结构非法，库绝不会隐式重建该关系部件。
校验会拒绝 namespace reset、未知或嵌套元素、缺失的 ``Id``/``Type``/``Target``、重复
标识符和非法 ``TargetMode``。元素名称按 expanded QName 匹配，因此
``Relationships``/``Relationship`` 既支持默认 namespace，也支持绑定到 OPC
relationship namespace 的合法前缀。已有子节点前缀保持不变，新建 relationship 使用根
节点前缀；``Id``/``Type``/``Target``/``TargetMode`` 仍必须是不带前缀的 OPC 属性。

所有已加载的 ``.rels`` 部件都会先完成 Markup Compatibility and Extensibility
（MCE）预处理，再执行 OPC schema 校验。该预处理配置只理解 OPC Relationships
namespace，不额外声明任何 markup configuration namespace。``mc:AlternateContent``
按文档顺序检查 ``Choice/@Requires``，直到找到第一个受支持的 Choice。完整 XML 树都会
接受 namespace well-formedness 与 MCE 语法校验，包括所有 Choice/Fallback wrapper、
指令、``Requires`` 列表，以及随后会被忽略或未选中的内容。只有最终选中的 Choice 或
Fallback 才进入内容语义预处理；被忽略和未选中的内容不会执行其 ``ProcessContent``
语义，也不会强制其 ``MustUnderstand``。外来元素自身的
``Ignorable``/``ProcessContent`` 声明会在语义阶段参与判断该元素应被丢弃还是解包。被
``ProcessContent`` 选中并解包的外来 wrapper 并非被完整忽略：移除 wrapper 前仍会
执行它自己的 ``MustUnderstand``。未知
``mc:MustUnderstand``、非法 MCE 语法、旧版
``Preserve*`` 指令和非 ignorable 扩展标记，在 strict 与 tolerant 两种模式中都会以
``mce_mismatch`` 或 ``invalid_mce_markup`` fail closed。

未发生修改的来源 relationship entry 仍可按原始字节复制；一旦该关系部件变为 dirty，
FeatherDoc 写出的是已清洗的选中视图，ignorable 内容、未选分支和 MCE 控制属性会被
移除。XML declaration、comment、processing instruction，以及 OPC ``Relationship``
允许的简单文本/CDATA 都会保留，包括合法 UTF-8 中文和其他 Unicode 文本。

任何依赖 relationship 的修改都会
fail closed，并返回 ``document_errc::invalid_package_structure``，包括超链接和图片
关系、singleton part 挂接、分节页眉页脚关系编辑，以及内容控件图片替换。失败发生在
提交相关内容 XML、媒体 entry、Content Types 修改或 dirty 状态之前；后续保存不会用
新关系树覆盖原来的非法 ``.rels``；保存边界还会再次拒绝 dirty 且结构非法的关系 DOM，
内容控件图片操作也不会留下只完成一半的替换结果。
调用方应先检查诊断并执行明确支持的显式修复，再恢复常规编辑流程。

``[Content_Types].xml`` 同样按 expanded QName 匹配。``Types``、``Default`` 和
``Override`` 可以使用默认 namespace，也可以使用任意正确绑定到 OPC Content Types
namespace 的前缀；修改时会沿用现有根前缀。namespace shadow/reset、未知或嵌套元素、
任何文本或 CDATA（包括纯空白）以及未声明属性都属于结构错误。tolerant 打开会记录
``invalid_content_types_root``，仍允许检查或不改变该部件的原样另存，但所有依赖
Content Types 的普通修改都会以 ``invalid_package_structure`` fail closed，不会扩展
存在歧义的 XML 树。

挂接 settings、numbering 或 styles 单例部件时，会把该 DOM 与
``word/_rels/document.xml.rels`` 的 checked clone 作为同一事务准备。只有 relationship
与对应 ``Override`` 都完整创建后，才会发布两个副本、dirty 状态和新部件；准备挂接时
发生分配失败，文档包保持不变，同一个 ``Document`` 可以直接重试。现有源包首次挂接
该部件后会立即标记为 dirty，因此随后内容修改即使失败，保存也不会只写 relationship
与 ``Override`` 而漏掉新 XML 部件。

主文档、页眉、页脚、样式、编号、设置、脚注、尾注和批注也统一按 expanded QName
解析。transitional WordprocessingML namespace 可以使用默认 namespace，或任意合法的
UTF-8 NCName 前缀，包括中文前缀。加载时，FeatherDoc 会以原子方式把
WordprocessingML 元素名和属性名规范化为内部 ``w:`` 拼写，同时保留 namespace 声明、
外来标记，以及 MCE/data-binding QName 等依赖前缀的属性值。常规保存始终会重新序列化
主文档及已加载的页眉/页脚，并写出规范 ``w:`` 名称。未修改的懒加载 singleton 部件
（样式、编号、设置、脚注、尾注和批注）仍可按原始字节复制；一旦 dirty，保存会写出
规范形式。显式 ``xmlns:w`` 如果绑定到其他
namespace，或出现未绑定/非法 QName、重复 expanded attribute，都会被拒绝，绝不会
冒充 WordprocessingML。主文档根错误时，strict 模式拒绝，tolerant 模式只记录结构
诊断；页眉/页脚在打开时急切加载，因此错误根在两种模式下都会使 ``open()`` 失败。
懒加载 singleton 的根会在首次访问时校验，并在两种模式下返回
``invalid_package_structure``；此时包本身可能已经打开成功。``commentsExtended`` 属于
另一套 schema，不会经过该规范化器。遍历采用迭代实现，并限制为最多 65,536 层、
1,000,000 个元素和 1,000,000 个属性。

公开 ``archive_limits`` 聚合体保持原有 5 个可配置字段，默认值分别为：10,000 个
条目、单个 XML 部件 64 MiB、单个二进制部件 256 MiB、总解压量 512 MiB、最大
压缩比 200。业务确有需要时可以调高这些限制，但应保留与输入来源相匹配的上限。
中央目录 64 MiB、单个物理 entry name 511 bytes、全部物理 entry name 合计
8 MiB 是另外三项内部固定安全默认值，会在分配或读取完整中央目录之前生效；它们
不是 ``archive_limits`` 的新增字段。

Relationships MCE 清洗还会把临时输出 DOM 限制为最多 1,000,000 个元素、262,144 个
属性，并限制每个元素最多 hoist 4,096 个有效 namespace binding。namespace token
解析、复制、比较、context push/pop、排序和 binding hoist 共用 64 MiB 累计工作预算。
这些内部固定边界
用于阻止 ``ProcessContent`` 移除 wrapper 后，把继承的 namespace 声明无限复制到
大量子元素。超过限制时原始 DOM 保持不变，并返回 ``archive_limit_exceeded``；分配失败
统一返回 ``std::errc::not_enough_memory``，tolerant 模式也不得把它降级为包诊断继续读取。

物理 entry 名必须是合法 UTF-8 的相对包路径，且不得包含嵌入 NUL。strict 模式还
要求物理名称采用 OPC 规范 ASCII 形式：非 ASCII UTF-8 字节和空格必须使用大写
``%HH`` 转义。tolerant 模式可以读取历史包中的 raw Unicode 或其他非规范拼写，但
两种模式都会拒绝路径穿越、错误转义、重复项、ASCII 大小写等价项，以及 raw/percent
表示不同但逻辑身份相同的部件。保存时统一写出 percent-encoded 规范物理名称，因此
中文、日文、emoji 等 Unicode 逻辑部件名不依赖具体 ZIP 实现的 raw-name 行为。
内部 OPC relationship target 以来源部件为基准，直接按 UTF-8 package path 解析，
不经过主机文件系统 code page 转换。

Package PartName 使用 RFC 3986 ``pchar`` 字符集合，并额外遵守 OPC 段规则：不得有
空段或全点段，段不能以点结尾，也不能同时存在“给一个名称追加路径段即可得到另一个
名称”的两个部件。编码后的 ``#``、``?``（``%23``、``%3F``）以及字面量冒号仍是
合法名称。Content Types 中的 ``Override`` PartName 和 ``Default`` 扩展名按 ASCII
大小写无关身份比较，逻辑重复声明会 fail closed。``Default/@Extension`` 遵守 OPC
``ST_Extension`` 词法；Unicode 扩展名必须使用 UTF-8 ``%HH`` 字节，不能直接写 raw
非 ASCII XML 文本。

内部 metadata 限制不会扩展 ``archive_limits`` 或 ``Document``。新增的包校验与
relationship bookkeeping 通过现有 opaque handle-lifetime state 持有的 sidecar 保存，
因此 ``Document`` 继续维持 1.13 对象布局，移动构造和移动赋值也继续保持
``noexcept``。原有五参数 ``archive_limits`` 聚合初始化及依赖 1.13 ``Document`` 布局
的代码，仍保持原来的源码与二进制字段映射。

每次懒加载重新打开源包时，都会先复验当前整个包的 entry 数、单项/总量、压缩比、
UTF-8 路径、规范形式和唯一性；随后再依据关系与内容语义对选中的 XML 部件应用
XML 限制，而不是只看扩展名。``save()``/``save_as()`` 在复制保留 entry 前也会执行
同样的全源包复验。reader 关闭失败会返回 ``archive_close_failed``，且发生在提交
懒加载结果或替换保存目标之前。

``open()`` 还会为每个 ZIP entry 保存有顺序、只含 metadata 的源包指纹：物理名称、
规范名称、逻辑身份、CRC32、压缩/解压大小和目录标记。懒加载和保存时复制来源 entry
都会把重新打开的包与该快照精确比较。entry 修改、新增、删除或调序通常会改变一个或
多个已记录的指纹字段；只要这些字段存在差异，就会在混用旧 DOM 与新包内容之前返回
``source_archive_changed``。``save()`` 覆盖自身源路径
时，会在原子替换前立即再比较一次，并用 ``open()`` 当时的原始限制重新打开已 finalize
的输出；只有替换成功后才刷新快照。输出一旦超过原始限制，会在 FeatherDoc 替换源路径
之前拒绝替换。
源路径身份遵循目录项语义：Windows 会解析大小写、8.3 与 Win32 别名，同时尊重启用
大小写敏感的目录；POSIX 会解析带符号链接的父目录。两个平台都不会把不同 hard-link
名称合并，因为原子替换发布的是单个目录项，而不是同一文件对象的全部链接。

该指纹用于一致性保护，不是身份认证或安全哈希：ZIP CRC32 和中央目录 metadata 都不
提供密码学完整性证明。最终检查基于路径，并不会锁住“关闭检查句柄到执行原子替换”间
的极短窗口；需要多写入者协同时，调用方必须提供外部文件锁或更高层版本协议。

显式包修复
----------

默认修复策略只处理能够确定恢复且不会覆盖未知元数据的问题：在合法
``w:document`` 下补建 ``w:body``，补建缺失的根关系或主文档关系，以及补建缺失的
``[Content_Types].xml`` 或修正主文档 MIME。已有 XML 损坏、根节点/namespace
错误、重复主文档声明和 external 主文档关系会返回
``package_repair_not_possible``，不会先修改其他部件。

修复成功后，``save()``/``save_as()`` 会先把结果写入同目录临时文件，再用 strict
模式重新打开该临时包；复验失败返回 ``package_repair_validation_failed``，原目标
保持不变。CLI 可使用 ``inspect-package <input.docx> --json`` 查看诊断，使用
``repair-package <input.docx> --output <repaired.docx> --json`` 安全地另存修复结果。

句柄失效规则
------------

``Paragraph``、``Run``、``Table``、``TableRow``、``TableCell`` 和
``TemplatePart`` 等对象是指向当前 DOM 的受跟踪非拥有句柄。句柄同时记录包级
generation 和节点级 epoch；因此它不会延长 ``Document`` 生命周期，也不会在
DOM 已重置或节点已删除后继续解引用悬空的 pugixml 节点。

* ``set_path(...)``、``open(...)`` 和 ``create_empty()`` 会重置整个包，因此使
  该 ``Document`` 先前返回的全部 XML 句柄失效。
* 移动构造和移动赋值会使源对象此前返回的句柄失效；移动赋值还会使目标对象旧状态
  返回的句柄失效。移动后的源 ``Document`` 处于关闭状态，可以重新设置路径、打开
  文档或调用 ``create_empty()``。
* 成功且实际发生修改的 ``repair_package(...)`` 会替换被修复的 DOM，因此也必须
  重新获取此前保存的 XML-backed 句柄。
* 删除节点会使该节点及其后代句柄失效。
* ``Paragraph::set_text(...)`` 和 ``TableCell::set_text(...)`` 会先构建完整替换内容。
  失败时原 XML 和全部句柄保持不变；成功时段落或单元格自身及其祖先句柄保持有效，
  但被替换的旧段落/run 子句柄失效。``Run::set_text(...)`` 成功后该 ``Run`` 句柄仍
  有效。段落/run/表格追加与插入失败时不会留下空占位节点。
* 表格插入、行列修改、合并与取消合并会统一准备 ``tblGrid``、单元格属性和待删除
  子树。失败时整个表格及句柄保持不变；成功时仅被删除或被替换子树上的句柄失效，
  未删除的表格、行、单元格和兄弟子树句柄继续有效。
* ``fill_bookmarks(...)`` 和 ``apply_bookmark_block_visibility(...)`` 以整个 bindings
  列表为一个事务。任一绑定重复、非法、结构损坏或分配失败时，早先绑定不会被发布，
  原 XML、错误详情和句柄保持不变；成功时仅被替换或删除范围内的句柄失效。
* 脚注、尾注、批注及修订 mutation 会把正文/相关 story、relationship、Content Types、
  review sidecar 和 dirty 状态作为一个事务准备。分配失败返回
  ``std::errc::not_enough_memory``，不会发布部分包状态或推进句柄 epoch。成功时，已发布
  正文、页眉或页脚 story 中的旧句柄失效，调用方必须从 ``Document`` 重新获取；未发布
  story 的句柄继续有效。
* 成功执行 ``remove_header_part(...)`` 或 ``remove_footer_part(...)`` 会统一发布
  分节引用清理并销毁一个物理 XML 部件。由于 ``TemplatePart`` 在文档 generation
  范围内跟踪该部件，本操作会使该文档此前返回的全部 XML-backed 句柄失效；被拒绝
  或失败的删除不会推进 generation，现有句柄保持有效。
* ``move_section(...)`` 会先在隔离副本中准备新的正文顺序。失败时正文、settings
  元数据及全部现有句柄保持不变；成功时正文句柄失效，不相关的页眉、页脚句柄仍有效。
* ``append_section(...)``、``insert_section(...)`` 和 ``remove_section(...)``
  同样先在隔离副本中准备正文结构修改。失败时已发布正文和全部句柄保持不变；成功时
  正文句柄失效，已加载的页眉、页脚句柄仍有效。
* ``set_section_page_setup(...)`` 以及非结构性的分节引用创建、绑定、复制或移除，
  只发布完整准备好的 ``w:sectPr``。失败时全部状态和句柄保持不变；对结构正常的包，
  成功后正文和相关部件句柄仍有效。若 tolerant 包中已有错误的 ``xmlns:r`` 绑定，且
  操作必须修复该绑定，则可能回退为整份 document 发布，此时正文句柄会失效。
* ``move_header_part(...)`` 和 ``move_footer_part(...)`` 会在隔离副本中准备 relationship
  顺序，并在发布阶段以不分配的方式调整所有权顺序；无论失败还是成功，现有正文及
  相关部件句柄都保持有效。
* 替换一个已解析且已存在的页眉/页脚部件文本采用事务式发布。分配失败时该部件及全部
  现有句柄保持不变；成功时只有目标部件子树中的句柄失效，正文和其他 story 的句柄
  仍有效。若目标物理部件尚不存在，操作所需的 relationship 和 Content Types 也会
  作为一个事务准备；对结构正常的包，先前返回的句柄仍保持有效。
* ``sync_content_controls_from_custom_xml()`` 会把正文及全部已加载页眉、页脚作为一次
  事务处理。分配失败时，已发布 DOM、未保存编辑和现有句柄均保持不变；成功同步至少
  一个内容控件时才统一发布隔离副本，并使全部 XML-backed 句柄失效；同步数量为零时
  不会使句柄失效。
* ``rename_style(...)``、``merge_style(...)``、干净且非空的
  ``apply_style_refactor(...)`` 批处理，以及成功执行且实际修改文档的
  ``restore_style_refactor(...)``，都会把样式、正文、页眉页脚及样式挂接所需的包部件
  作为一个事务统一发布。分配失败时，完整的已发布 DOM、dirty 状态和现有句柄均保持
  不变；``apply_style_refactor(...)`` 的后续操作失败时不会留下此前操作的修改。被拒绝
  的 restore 条目自身不发生修改，但同一 restore 结果中的其他有效条目仍可成功应用。
  任一成功修改都会使全部 XML-backed 句柄失效，必须从 ``Document`` 重新获取。
* 表格、段落、内容控件或模板部件的结构重建，会使被替换子树上的句柄失效。

``Paragraph``、``Run``、``Table``、``TableRow`` 和 ``TableCell`` 可通过
``valid()`` 检查；``TemplatePart`` 使用显式 ``bool`` 转换检查。失效句柄的读取
返回空结果，修改返回 ``false`` 或空句柄，不会访问已释放内存。除非上文明确说明
某个操作会推进整个文档 generation，否则未被删除的兄弟子树保持有效。完成上述
操作后，应从 ``Document`` 或仍有效的父级入口重新获取需要继续使用的句柄。

破坏性 API 迁移说明
~~~~~~~~~~~~~~~~~~~~

旧版允许调用方用两个 ``pugi::xml_node`` 直接构造 ``Paragraph``、``Run``、
``Table``、``TableRow`` 或 ``TableCell``，也公开了 ``set_parent`` /
``set_current``。这些入口无法携带生命周期信息，现已彻底删除。调用方应只从
``Document``、``TemplatePart`` 或父级句柄获取子句柄；公开 API 不再要求业务
代码直接依赖 pugixml DOM。

模板部件入口
------------

当同一类操作需要作用在正文、页眉或页脚上时，优先使用模板部件入口。

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``body_template()``
     - 无。
     - ``TemplatePart``
     - 通过模板部件 API 访问正文。
   * - ``header_template(std::size_t index = 0U)``
     - ``index``：物理页眉部件下标。
     - ``TemplatePart``
     - 按物理页眉部件下标访问页眉。
   * - ``footer_template(std::size_t index = 0U)``
     - ``index``：物理页脚部件下标。
     - ``TemplatePart``
     - 按物理页脚部件下标访问页脚。
   * - ``section_header_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：从 0 开始的分节下标。``reference_kind``：
       默认页、首页或偶数页页眉引用类型。
     - ``TemplatePart``
     - 访问某个分节和引用类型解析后的页眉。
   * - ``section_footer_template(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：从 0 开始的分节下标。``reference_kind``：
       默认页、首页或偶数页页脚引用类型。
     - ``TemplatePart``
     - 访问某个分节和引用类型解析后的页脚。

分节与检查
----------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``append_section(bool inherit_header_footer = true)``
     - ``inherit_header_footer``：为 ``true`` 时复制上一分节的页眉页脚引用；为
       ``false`` 时不复制显式引用。按照 WordprocessingML 规则，缺少本地引用表示
       linked-to-previous，并不表示强制使用空白页眉页脚。
     - ``bool``
     - 在文档末尾追加分节。
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``：插入位置。``inherit_header_footer``：为 ``true`` 时
       复制相邻分节的页眉页脚引用；为 ``false`` 时通过缺少本地引用保持
       linked-to-previous。
     - ``bool``
     - 在指定分节前插入新分节。
   * - ``remove_section(std::size_t section_index)``
     - ``section_index``：要删除的分节。
     - ``bool``
     - 删除指定分节。
   * - ``move_section(std::size_t source_section_index, std::size_t target_section_index)``
     - ``source_section_index``：要移动的分节。``target_section_index``：
       移除源分节后的目标位置。
     - ``bool``
     - 在不重建整篇文档的情况下调整分节顺序。
   * - ``section_count() const noexcept``
     - 无。
     - ``std::size_t``
     - 返回分节数量。
   * - ``header_count() const noexcept`` / ``footer_count() const noexcept``
     - 无。
     - ``std::size_t``
     - 返回物理页眉/页脚部件数量。
   * - ``inspect_sections()``
     - 无。
     - ``sections_inspection_summary``
     - 返回分节、页眉和页脚检查结果。
   * - ``inspect_header_parts()`` / ``inspect_footer_parts()``
     - 无。
     - ``std::vector<related_part_inspection_summary>``
     - 返回每个物理关联部件，以及引用它的分节和引用类型。
   * - ``inspect_section(std::size_t section_index)``
     - ``section_index``：从 0 开始的分节下标。
     - ``std::optional<section_inspection_summary>``
     - 检查单个分节；为空表示目标分节不可用。
   * - ``get_section_page_setup(std::size_t section_index) const``
     - ``section_index``：要读取页面设置的分节。
     - ``std::optional<section_page_setup>``
     - 读取页面尺寸、页边距、方向等页面设置。
   * - ``set_section_page_setup(std::size_t section_index, const section_page_setup &setup)``
     - ``section_index``：目标分节。``setup``：页面设置值。
     - ``bool``
     - 将页面设置应用到指定分节。
   * - ``replace_section_header_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``replacement_text``：新的页眉文本。
       ``reference_kind``：要解析的页眉引用类型。
     - ``bool``
     - 替换指定分节解析后的页眉文本。
   * - ``replace_section_footer_text(std::size_t section_index, std::string_view replacement_text, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``replacement_text``：新的页脚文本。
       ``reference_kind``：要解析的页脚引用类型。
     - ``bool``
     - 替换指定分节解析后的页脚文本。
   * - ``remove_section_header_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``reference_kind``：要移除的页眉引用类型。
     - ``bool``
     - 只移除分节引用，不删除物理页眉部件。
   * - ``remove_section_footer_reference(std::size_t section_index, section_reference_kind reference_kind = default_reference)``
     - ``section_index``：目标分节。``reference_kind``：要移除的页脚引用类型。
     - ``bool``
     - 只移除页脚引用，不删除物理页脚部件。
   * - ``inspect_body_blocks()``
     - 无。
     - ``std::vector<body_block_inspection_summary>``
     - 检查正文中段落和表格的顺序。
   * - ``inspect_paragraphs()`` / ``inspect_paragraphs_with_options(const paragraph_inspection_options &options)``
     - ``options``：控制可选元数据解析。
     - ``std::vector<paragraph_inspection_summary>``
     - 检查正文段落，返回原始编号 ID，并可选择解析编号定义元数据。
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``other``：对比目标文档。``options``：语义差异开关。
     - ``std::optional<document_semantic_diff_result>``
     - 比较两个文档的语义内容差异。

``paragraph_inspection_options::resolve_numbering_metadata`` 默认为 ``true``。
启用时，段落检查会把 ``numId`` 解析到编号定义；源包重新打开或编号查找失败时，通过
``last_error()`` 原样返回错误，不会交付只填充了一部分的结果。只需要原始 ``num_id``
和 ``level``，且不希望触发编号部件解析时，可将其设为 ``false``。``TemplatePart``
也通过 ``inspect_paragraphs_with_options(options)`` 和
``inspect_paragraph_with_options(index, options)`` 为正文、页眉和页脚部件提供相同
选项。独立的 ``*_with_options`` 名称可避免形成重载，从而保证已有代码对原
``inspect_paragraphs`` 或 ``inspect_paragraph`` 成员函数取址时仍保持源码兼容。

模板填充快捷入口
----------------

这些方法直接从 ``Document`` 操作。常见模板填充场景不必先取得
``TemplatePart``。

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``replace_bookmark_text(const std::string &bookmark_name, const std::string &replacement)``
     - ``bookmark_name``：要填充的书签。``replacement``：插入文本。
     - ``std::size_t``
     - 替换匹配书签文本，并返回替换数量。
   * - ``fill_bookmarks(std::span<const bookmark_text_binding> bindings)``
     - ``bindings``：一次填充的书签/文本绑定列表。
     - ``bookmark_fill_result``
     - 批量填充书签，并报告 requested、matched、replaced 和缺失书签。
   * - ``fill_bookmarks(std::initializer_list<bookmark_text_binding> bindings)``
     - ``bindings``：内联书签/文本绑定列表。
     - ``bookmark_fill_result``
     - 不额外构造容器时填充少量书签。
   * - ``list_bookmarks() const`` / ``find_bookmark(std::string_view bookmark_name) const``
     - ``bookmark_name``：要查找的书签名。
     - ``std::vector<bookmark_summary>`` / ``std::optional<bookmark_summary>``
     - 填充前检查可用书签槽位。
   * - ``replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)``
     - ``tag``：内容控件 tag。``replacement``：插入文本。
     - ``std::size_t``
     - 替换匹配内容控件，并返回替换数量。
   * - ``replace_content_control_text_by_alias(std::string_view alias, std::string_view replacement)``
     - ``alias``：内容控件 alias/title。``replacement``：插入文本。
     - ``std::size_t``
     - 按 alias/title 填充内容控件。
   * - ``list_content_controls() const``
     - 无。
     - ``std::vector<content_control_summary>``
     - 填充前检查内容控件 tag、alias、锁定状态和数据绑定信息。
   * - ``find_content_controls_by_tag(std::string_view tag) const`` / ``find_content_controls_by_alias(std::string_view alias) const``
     - ``tag`` 或 ``alias``：结构化文档标签查找键。
     - ``std::vector<content_control_summary>``
     - 修改文档前定位要填充的内容控件。
   * - ``replace_content_control_with_paragraphs_by_tag(std::string_view tag, const std::vector<std::string> &paragraphs)``
     - ``tag``：内容控件 tag。``paragraphs``：替换段落文本。
     - ``std::size_t``
     - 将匹配内容控件替换为生成段落。
   * - ``replace_content_control_with_table_rows_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``：内容控件 tag。``rows``：作为表格行追加的行/单元格文本矩阵。
     - ``std::size_t``
     - 在周围表格上下文中将匹配内容控件替换为表格行。
   * - ``replace_content_control_with_table_by_tag(std::string_view tag, const std::vector<std::vector<std::string>> &rows)``
     - ``tag``：内容控件 tag。``rows``：生成表格矩阵。
     - ``std::size_t``
     - 将匹配内容控件替换为生成表格。
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path)``
     - ``tag``：内容控件 tag。``image_path``：替换图片路径。
     - ``std::size_t``
     - 将匹配内容控件替换为图片。
   * - ``replace_content_control_with_image_by_tag(std::string_view tag, const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``tag``：内容控件 tag。``image_path``：替换图片路径。
       ``width_px`` / ``height_px``：显式图片尺寸。
     - ``std::size_t``
     - 将匹配内容控件替换为指定尺寸图片。

正文、表格和图片
----------------

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``paragraphs()``
     - 无。
     - ``Paragraph &``
     - 返回正文段落迭代/编辑入口。
   * - ``tables()``
     - 无。
     - ``Table &``
     - 返回正文表格迭代/编辑入口。
   * - ``append_table(std::size_t row_count = 1U, std::size_t column_count = 1U)``
     - ``row_count`` 和 ``column_count``：初始表格行列数。
     - ``Table``
     - 追加表格并返回新建表格句柄。
   * - ``append_image(const std::filesystem::path &image_path)``
     - ``image_path``：要追加的行内图片。
     - ``bool``
     - 按图片自然尺寸追加行内图片。
   * - ``append_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``image_path``：要追加的图片。``width_px`` / ``height_px``：
       显式尺寸。
     - ``bool``
     - 按指定尺寸追加行内图片。
   * - ``append_floating_image(const std::filesystem::path &image_path, floating_image_options options = {})``
     - ``image_path``：要追加的图片。``options``：浮动位置和环绕选项。
     - ``bool``
     - 追加浮动图片。
   * - ``append_floating_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px, floating_image_options options = {})``
     - ``image_path``：要追加的图片。``width_px`` / ``height_px``：
       显式尺寸。``options``：浮动位置和环绕选项。
     - ``bool``
     - 追加指定尺寸的浮动图片。

批注、修订和链接
----------------

详细能力在对应对象页中展开；这里列出 ``Document`` 级常用入口。

.. list-table::
   :header-rows: 1
   :widths: 31 29 18 42

   * - 方法
     - 参数
     - 返回值
     - 用途
   * - ``append_comment(std::string_view selected_text, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - ``selected_text``：被批注文本。``comment_text``：批注正文。
       ``author`` / ``initials`` / ``date``：可选元数据。
     - ``std::size_t``
     - 添加批注；成功返回 ``1``，找不到可批注文本时返回 ``0``。
   * - ``append_paragraph_text_comment(std::size_t paragraph_index, std::size_t text_offset, std::size_t text_length, std::string_view comment_text, ...)``
     - ``paragraph_index``：目标段落。``text_offset`` 和 ``text_length``：
       被批注文本范围。
     - ``std::size_t``
     - 给段落内精确文本范围添加批注。
   * - ``append_text_range_comment(std::size_t start_paragraph_index, std::size_t start_text_offset, std::size_t end_paragraph_index, std::size_t end_text_offset, std::string_view comment_text, author = {}, initials = {}, date = {})``
     - 起止段落下标、文本偏移、批注正文和可选元数据。
     - ``std::size_t``
     - 给跨段落文本范围添加批注。
   * - ``list_revisions() const``
     - 无。
     - ``std::vector<revision_summary>``
     - 检查修订记录。
   * - ``accept_revision(std::size_t revision_index)`` / ``reject_revision(std::size_t revision_index)``
     - ``revision_index``：``list_revisions()`` 返回的修订下标。
     - ``bool``
     - 接受或拒绝单条修订。
   * - ``append_footnote(std::string_view reference_text, std::string_view note_text)``
     - ``reference_text``：脚注标记文本。``note_text``：脚注正文。
     - ``std::size_t``
     - 追加脚注。
   * - ``list_hyperlinks() const``
     - 无。
     - ``std::vector<hyperlink_summary>``
     - 编辑前检查超链接文本和目标。
   * - ``append_hyperlink(std::string_view text, std::string_view target)``
     - ``text``：显示文本。``target``：URL 或关系目标。
     - ``std::size_t``
     - 在正文追加超链接。
   * - ``replace_hyperlink(std::size_t hyperlink_index, std::string_view text, std::string_view target)``
     - ``hyperlink_index``：来自 ``list_hyperlinks()`` 的条目。
       ``text`` 和 ``target``：替换后的显示文本和 URL/关系目标。
     - ``bool``
     - 替换已有超链接。

相关 API 家族
-------------

``Document`` 根对象还暴露以下公开 API 家族。完整方法集和示例请进入对应专题页：

* :doc:`sections`：分节段落、页眉页脚分配、物理页眉页脚部件删除和页面设置。
* :doc:`fields_links_reviews`：批注、批注回复、已解决状态、脚注、尾注、超链接和修订操作。
* :doc:`template_part`：模板校验、schema 扫描、模板上架、自定义 XML 同步和内容控件表单状态。
* :doc:`images`：行内/浮动图片列表、提取、替换和删除。
* :doc:`paragraph_run` 与 :doc:`table`：从 ``Document`` 返回的段落、run、表格、行和单元格编辑入口。
* :doc:`styles_numbering` 与 :doc:`enums`：样式/编号目录、共享选项和 inspection summary 类型。

示例
----

打开模板、填充内容控件并保存：

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (doc.open()) {
       return 1;
   }

   doc.body_template().replace_content_control_text_by_tag("customer", "Ada");
   return doc.save_as("filled.docx") ? 1 : 0;

直接从 ``Document`` 填充，并读取失败诊断：

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   if (auto error = doc.open()) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   const auto replaced =
       doc.replace_content_control_text_by_tag("customer", "Ada");
   if (replaced == 0) {
       return 1;
   }

   if (auto error = doc.save_as("filled.docx")) {
       std::cerr << doc.last_error().detail << '\n';
       return 1;
   }

   return 0;

创建新文档、追加表格并设置打开时更新域：

.. code-block:: cpp

   featherdoc::Document doc;
   if (doc.create_empty()) {
       return 1;
   }

   doc.enable_update_fields_on_open();
   auto table = doc.append_table(2, 3);
   if (auto customer = table.find_cell(0, 0)) {
       customer->set_text("Customer");
   }
   if (auto status = table.find_cell(0, 1)) {
       status->set_text("Status");
   }

   return doc.save_as("report.docx") ? 1 : 0;
