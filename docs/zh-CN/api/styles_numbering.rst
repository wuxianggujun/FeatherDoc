样式和编号 API
==============

``featherdoc::Document`` 提供样式和编号 API，用于检查、受控修改、重构和默认格式
设置。这个页面先给出直接入口，完整旧版分组页面仍可继续查阅。

类型化签名导读
--------------

.. FDOC_ZH_CN_STYLES_NUMBERING_TYPED_SIGNATURE_GUIDE

样式 API 通过 ``style_id`` 定位样式。编号层级从 0 开始。返回
``std::optional<T>`` 的方法在样式、编号定义或生成计划无法解析时返回空值。

run 字号 setter 只接受有限、正数且可表示为无符号 32 位 half-point 的值，最大为
``2147483647.5`` point。非法、NaN、无穷、零、负数和溢出输入都会在修改 styles
DOM 前失败。编号修改也会在挂接包部件或修改段落/样式 XML 前预留全部所需
``abstractNumId`` 和 ``numId``；空间耗尽返回 ``identifier_space_exhausted``，不会
遗留半个编号定义。

``import_numbering_catalog(...)`` 采用 all-or-nothing 事务：所有 definition、instance
和返回摘要都会先在隔离的 checked clone 中完整构造，全部可失败步骤成功后才挂接
singleton 元数据并发布 numbering DOM。pugixml 或标准库分配失败时，返回摘要中的已导入
数量为零；已有编号部件保持可原样保存，缺失部件仍不挂接，同一个 ``Document`` 可安全重试。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``bool set_paragraph_list(Paragraph paragraph, list_kind kind, std::uint32_t level = 0U)``
     - ``paragraph``：目标段落。``kind``：项目符号或数字列表类型。``level``：从 0 开始的层级。
     - 列表元数据绑定成功时返回 ``true``。
   * - ``numbering_catalog export_numbering_catalog()``
     - 无。
     - 返回当前编号目录，供治理或复用。
   * - ``numbering_catalog_import_summary import_numbering_catalog(const numbering_catalog &catalog)``
     - ``catalog``：要导入的编号定义。
     - 返回导入数量和冲突摘要。
   * - ``std::optional<std::uint32_t> ensure_numbering_definition(const numbering_definition &definition)``
     - ``definition``：期望存在的编号定义。
     - 返回定义 id；无法确保时为空。
   * - ``bool set_paragraph_numbering(Paragraph paragraph, std::uint32_t numbering_definition_id, std::uint32_t level = 0U)``
     - ``paragraph``：目标段落。``numbering_definition_id`` 和 ``level``：编号目标。
     - 段落编号设置成功时返回 ``true``。
   * - ``std::optional<style_summary> find_style(std::string_view style_id)``
     - ``style_id``：样式标识。
     - 返回样式摘要；样式缺失时为空。
   * - ``bool rename_style(std::string_view old_style_id, std::string_view new_style_id)``
     - ``old_style_id``：源样式 id。``new_style_id``：替换 id。
     - 样式 id 及引用重命名成功时返回 ``true``。
   * - ``bool merge_style(std::string_view source_style_id, std::string_view target_style_id)``
     - ``source_style_id``：要移除的样式。``target_style_id``：替代样式。
     - 引用迁移且源样式移除成功时返回 ``true``。
   * - ``bool set_default_run_language(std::string_view language)``
     - ``language``：类似 BCP-47 的语言标签。
     - 默认 run 语言写入成功时返回 ``true``。
   * - ``table_style_region_audit_report audit_table_style_regions(std::optional<std::string_view> style_id = std::nullopt)``
     - ``style_id``：可选表格样式过滤。
     - 返回全部或单个表格样式的区域审计报告。

编号
----

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``set_paragraph_list(paragraph, kind, level = 0U)``
     - ``bool``
     - 给段落绑定生成的列表。
   * - ``restart_paragraph_list(paragraph, kind, level = 0U)``
     - ``bool``
     - 为段落启动新的列表实例。
   * - ``clear_paragraph_list(paragraph)``
     - ``bool``
     - 清除段落列表元数据。
   * - ``list_numbering_definitions()``
     - ``std::vector<numbering_definition_summary>``
     - 枚举编号定义。
   * - ``find_numbering_definition(definition_id)``
     - ``std::optional<numbering_definition_summary>``
     - 查找一个编号定义。
   * - ``export_numbering_catalog()``
     - ``numbering_catalog``
     - 导出编号定义，便于治理或复用。
   * - ``import_numbering_catalog(catalog)``
     - ``numbering_catalog_import_summary``
     - 导入编号目录。
   * - ``set_paragraph_numbering(paragraph, definition_id, level)``
     - ``bool``
     - 将已有编号定义绑定到段落。

编号修改事务
~~~~~~~~~~~~

``set_paragraph_numbering(...)``、``set_paragraph_list(...)`` 和
``restart_paragraph_list(...)`` 会先完整准备 numbering DOM、document
relationships、Content Types 以及替换用的段落直接 ``w:pPr``，之后才发布任一状态。
``set_paragraph_style_numbering(...)`` 和 ``ensure_style_linked_numbering(...)``
则把 styles DOM、numbering DOM、document relationships 与 Content Types 作为一个
事务准备。校验、ID 规划、部件挂接、clone 或分配任一环节失败时，全部已发布 DOM、
part-presence 标记、dirty 标记和现有句柄都保持不变。

传入的 ``Paragraph`` 必须是由当前接收调用的 ``Document`` 返回且仍然有效的受跟踪
句柄，并且属于该文档的正文或已加载页眉/页脚 story。来自其他文档或已经失效的句柄
会在修改任一文档前以 ``std::errc::invalid_argument`` 拒绝。

设置段落直接编号或段落样式编号时，重复的直接 ``w:numPr`` 会规范化为一个按
``CT_PPr`` schema 顺序放置的元素。clear 方法会删除全部直接 ``w:numPr``，包括重复
直接 ``w:pPr`` 中的副本；由此变为空且无属性的 ``w:pPr`` 也会被删除。目标本来没有
直接编号时，clear 是成功的纯 no-op。尤其是包内没有 styles 部件时，清除隐式
``Normal`` 样式不会创建 ``word/styles.xml``、relationship、Content Types Override
或 dirty 状态；其他缺失样式仍返回错误，但同样保证不修改状态。

成功的段落直接编号操作只在现有 story DOM 内交换该段落的直接 ``w:pPr``，不会淘汰
文档句柄 generation；因此调用前已经取得的目标 ``Paragraph`` 及其 ``Run`` 子句柄在
成功后仍然有效。失败时这些句柄及全部包状态也保持不变。这个保证不包括指向被替换
旧 ``w:pPr`` 子树的原始 XML 句柄。

样式
----

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_styles()``
     - ``std::vector<style_summary>``
     - 枚举样式。
   * - ``find_style(style_id)``
     - ``std::optional<style_summary>``
     - 按样式 ID 查找样式。
   * - ``resolve_style_properties(style_id)``
     - ``std::optional<resolved_style_properties_summary>``
     - 解析继承后的样式属性。
   * - ``find_style_usage(style_id)``
     - ``std::optional<style_usage_summary>``
     - 检查一个样式的使用情况。
   * - ``list_style_usage()``
     - ``std::optional<style_usage_report>``
     - 检查全部样式使用情况。
   * - ``rename_style(old_style_id, new_style_id)``
     - ``bool``
     - 重命名样式 ID。
   * - ``merge_style(source_style_id, target_style_id)``
     - ``bool``
     - 将一个样式的使用迁移到另一个样式。
   * - ``suggest_style_merges()``
     - ``std::optional<style_refactor_plan>``
     - 生成保守的样式合并建议。
   * - ``plan_prune_unused_styles()`` / ``prune_unused_styles()``
     - ``std::optional<style_prune_*>``
     - 规划或执行未使用样式清理。
   * - ``set_paragraph_style(paragraph, style_id)`` / ``clear_paragraph_style(paragraph)``
     - ``bool``
     - 设置或清除段落直接应用的样式。
   * - ``set_run_style(run, style_id)`` / ``clear_run_style(run)``
     - ``bool``
     - 设置或清除 run 直接应用的字符样式。

直接应用样式 API 只接受属于当前接收调用的 ``Document`` 且仍然有效的受跟踪句柄。
外部或陈旧的 ``Paragraph`` / ``Run`` 句柄会在任一包发生变化前以
``std::errc::invalid_argument`` 拒绝。setter 会先完整准备 checked
``w:pPr`` / ``w:rPr`` 替换树以及可能缺失的 styles 部件元数据，全部成功后才统一发布。
因此校验、clone、挂接或分配失败时，完整包状态和已有句柄都保持不变，并可用同一句柄
安全重试。

setter 成功后，直接属性容器内只保留一个位于首位的 ``w:pStyle`` 或 ``w:rStyle``，
并删除该容器内的重复 style 子节点。clear 会删除全部这类重复节点；属性容器由此变空
且没有属性时也会一并删除。这些直接修改不会淘汰文档句柄 generation。

``rename_style(...)`` 和 ``merge_style(...)`` 会把样式、正文、已加载页眉页脚、
document relationships 与 Content Types 作为一次原子事务处理；
``apply_style_refactor(...)`` 以 all-or-nothing 方式应用干净的多操作计划，
``restore_style_refactor(...)`` 则统一发布全部被接受的恢复条目。分配失败返回
``std::errc::not_enough_memory``，且不改变已发布 DOM、dirty 状态或现有句柄。
成功执行非空修改后，旧 DOM generation 被淘汰，应从 ``Document`` 重新获取
XML-backed 句柄。

默认格式
--------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``default_run_font_family()``
     - ``std::optional<std::string>``
     - 读取默认 run 字体。
   * - ``set_default_run_font_family(font_family)``
     - ``bool``
     - 设置默认 run 字体。
   * - ``set_default_run_language(language)``
     - ``bool``
     - 设置默认 run 语言。
   * - ``set_default_paragraph_bidi(enabled = true)``
     - ``bool``
     - 设置默认段落 bidi 标记。
   * - ``clear_default_run_font_family()``
     - ``bool``
     - 清除默认 run 字体。

表格样式质量
------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``find_table_style_definition(style_id)``
     - ``std::optional<table_style_definition_summary>``
     - 检查表格样式定义。
   * - ``audit_table_style_regions(style_id)``
     - ``table_style_region_audit_report``
     - 审计表格样式区域一致性。
   * - ``audit_table_style_inheritance(style_id)``
     - ``table_style_inheritance_audit_report``
     - 审计表格样式继承。
   * - ``check_table_style_look_consistency()``
     - ``table_style_look_consistency_report``
     - 检查表格 look 标记。
   * - ``repair_table_style_look_consistency()``
     - ``table_style_look_repair_report``
     - 执行保守的表格 look 修复。
   * - ``audit_table_style_quality()``
     - ``table_style_quality_audit_report``
     - 运行表格样式质量检查。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"styled.docx"};
   doc.open();

   auto styles = doc.list_styles();
   auto usage = doc.list_style_usage();
   auto suggestions = doc.suggest_style_merges();

   auto &paragraph = doc.paragraphs();
   doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet);
   doc.set_default_run_language("zh-CN");
