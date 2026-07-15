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

打开校验与资源限制
------------------

无参数 ``open()`` 等价于传入默认 ``document_open_options``。默认使用
``package_validation_mode::strict``，并要求 ``[Content_Types].xml``、根关系、
``word/document.xml`` 和 ``w:document/w:body`` 结构有效。需要读取历史损坏包做
修复时，调用方必须显式选择 ``package_validation_mode::tolerant``；宽容模式不会
关闭 ZIP 资源限制，也不会静默修改文档。调用方应读取 ``package_diagnostics()``，
再显式调用 ``repair_package()``，并使用 ``save_as()`` 写入新文件。

默认 ``archive_limits`` 为 10,000 个条目、单个 XML 64 MiB、单个二进制部件
256 MiB、总解压量 512 MiB、最大压缩比 200。限制在解压前依据 ZIP metadata
检查。业务确有需要时可以调高，但应保留与输入来源相匹配的上限。

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
* 成功且实际发生修改的 ``repair_package(...)`` 会替换被修复的 DOM，因此也必须
  重新获取此前保存的 XML-backed 句柄。
* 删除节点会使该节点及其后代句柄失效。
* 表格、段落、内容控件或模板部件的结构重建，会使被替换子树上的句柄失效。

``Paragraph``、``Run``、``Table``、``TableRow`` 和 ``TableCell`` 可通过
``valid()`` 检查；``TemplatePart`` 使用显式 ``bool`` 转换检查。失效句柄的读取
返回空结果，修改返回 ``false`` 或空句柄，不会访问已释放内存。未被删除的兄弟
子树保持有效。完成上述操作后，应从 ``Document`` 或仍有效的父级入口重新获取
需要继续使用的句柄。

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
     - ``inherit_header_footer``：为 ``true`` 时复制上一分节的页眉页脚引用。
     - ``bool``
     - 在文档末尾追加分节。
   * - ``insert_section(std::size_t section_index, bool inherit_header_footer = true)``
     - ``section_index``：插入位置。``inherit_header_footer``：为 ``true`` 时
       继承相邻分节的页眉页脚引用。
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
   * - ``compare_semantic(const Document &other, document_semantic_diff_options options = {}) const``
     - ``other``：对比目标文档。``options``：语义差异开关。
     - ``std::optional<document_semantic_diff_result>``
     - 比较两个文档的语义内容差异。

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
