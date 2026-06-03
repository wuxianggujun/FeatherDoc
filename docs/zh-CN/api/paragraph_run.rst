Paragraph 和 Run
================

``featherdoc::Paragraph`` 是 Word 段落的编辑句柄。
``featherdoc::Run`` 是段落内部文本片段的编辑句柄，用于处理 run 级格式、
语言和文字方向元数据。

常用任务入口
------------

.. FDOC_ZH_CN_PARAGRAPH_RUN_COMMON_TASKS

编辑文本 run 或段落布局时，可以从这里开始：

* 替换整个段落：调用 ``Paragraph::set_text(...)``。
* 在段落里追加带样式文本：调用 ``add_run(...)``，再设置 run 字体、颜色、
  语言或文字方向元数据。
* 插入相邻内容：使用 ``insert_paragraph_before(...)``、
  ``insert_paragraph_after(...)``、``insert_run_before(...)`` 或
  ``insert_run_after(...)``。
* 保留附近格式：先使用 ``insert_*_like_*`` 辅助方法，再改文本。
* 调整版式：使用段落对齐、双向布局和缩进 API。

成功/失败语义
-------------

.. FDOC_ZH_CN_PARAGRAPH_RUN_SUCCESS_FAILURE_SEMANTICS

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - 返回形态
     - 成功含义
     - 失败或无变化含义
   * - ``bool``
     - ``true`` 表示当前段落或 run 的 XML 节点已修改。
     - ``false`` 表示当前节点缺失、不支持，或没有发生变化。
   * - ``std::optional<T>``
     - 包含请求的格式、布局、语言或方向值。
     - 空值表示该属性未显式设置或无法解析。
   * - ``Paragraph`` / ``Run``
     - 返回句柄指向插入或追加的段落/run 节点。
     - 后续格式设置应使用该返回句柄。
   * - 迭代器引用
     - ``Paragraph &`` 和 ``Run &`` 表示当前迭代位置。
     - 遍历已有内容时，推进前先调用 ``has_next()``。

短示例
------

.. FDOC_ZH_CN_PARAGRAPH_RUN_SHORT_EXAMPLE

.. code-block:: cpp

   auto paragraph = doc.body_template().append_paragraph("Status:");
   auto value = paragraph.add_run(" approved", featherdoc::formatting_flag::bold);
   value.set_font_family("Aptos");
   paragraph.set_alignment(featherdoc::paragraph_alignment::center);

类型化签名导读
--------------

.. FDOC_ZH_CN_PARAGRAPH_RUN_TYPED_SIGNATURE_GUIDE

写代码前可以先看这一组完整签名。涉及索引的 API 都使用从 0 开始的索引。
返回 ``bool`` 的方法表示底层 XML 节点是否成功修改。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``bool set_text(const std::string &text) const``
     - ``text``：替换后的段落或 run 文本。
     - 当前节点成功重写时返回 ``true``。
   * - ``Run add_run(const std::string &text, formatting_flag formatting = formatting_flag::none)``
     - ``text``：追加文本。``formatting``：可选基础格式标记。
     - 返回追加后的新 ``Run`` 句柄。
   * - ``Paragraph insert_paragraph_before(const std::string &text, formatting_flag formatting = formatting_flag::none)``
     - ``text``：插入段落文本。``formatting``：可选首个 run 格式。
     - 返回插入在当前段落前的新段落句柄。
   * - ``Paragraph insert_paragraph_after(const std::string &text, formatting_flag formatting = formatting_flag::none)``
     - ``text``：插入段落文本。``formatting``：可选首个 run 格式。
     - 返回插入在当前段落后的新段落句柄。
   * - ``bool set_alignment(paragraph_alignment alignment) const``
     - ``alignment``：文档中列出的段落对齐枚举值。
     - 段落属性成功更新时返回 ``true``。
   * - ``bool set_indent_left_twips(std::uint32_t indent_twips) const``
     - ``indent_twips``：左缩进，单位是 twips。
     - 段落缩进成功更新时返回 ``true``。
   * - ``Run insert_run_before(const std::string &text, formatting_flag formatting = formatting_flag::none)``
     - ``text``：插入 run 文本。``formatting``：可选基础格式标记。
     - 返回插入在当前 run 前的新 ``Run`` 句柄。
   * - ``Run insert_run_after(const std::string &text, formatting_flag formatting = formatting_flag::none)``
     - ``text``：插入 run 文本。``formatting``：可选基础格式标记。
     - 返回插入在当前 run 后的新 ``Run`` 句柄。
   * - ``bool set_font_family(std::string_view font_family) const``
     - ``font_family``：主要西文字体名称。
     - run 属性成功更新时返回 ``true``。
   * - ``bool set_language(std::string_view language) const``
     - ``language``：类似 ``zh-CN`` 或 ``en-US`` 的语言标签。
     - 语言元数据成功更新时返回 ``true``。
   * - ``bool set_rtl(bool enabled = true) const``
     - ``enabled``：为 ``true`` 时标记 run 为从右到左。
     - run 方向元数据成功更新时返回 ``true``。

段落文本与迭代
--------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``next()``
     - ``Paragraph &``
     - 将段落迭代器推进到下一个段落。
   * - ``has_next() const``
     - ``bool``
     - 检查段落迭代器是否还能继续推进。
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - 替换当前段落的全部文本。
   * - ``remove()``
     - ``bool``
     - 删除当前段落。
   * - ``runs()``
     - ``Run &``
     - 访问当前段落中的 run 迭代器。
   * - ``add_run(text, formatting_flag formatting = none)``
     - ``Run``
     - 在段落末尾追加一个 run，并可附带基础格式。
   * - ``insert_paragraph_before(text, formatting)``
     - ``Paragraph``
     - 在当前段落前插入新段落。
   * - ``insert_paragraph_after(text, formatting)``
     - ``Paragraph``
     - 在当前段落后插入新段落。
   * - ``insert_paragraph_like_before()``
     - ``Paragraph``
     - 在当前段落前插入段落，并复制段落属性。
   * - ``insert_paragraph_like_after()``
     - ``Paragraph``
     - 在当前段落后插入段落，并复制段落属性。

段落布局
--------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``bidi() const``
     - ``std::optional<bool>``
     - 读取段落的从右到左布局元数据。
   * - ``set_bidi(bool enabled = true) const``
     - ``bool``
     - 启用或关闭段落从右到左布局。
   * - ``clear_bidi() const``
     - ``bool``
     - 清除显式段落双向布局设置。
   * - ``alignment() const``
     - ``std::optional<paragraph_alignment>``
     - 读取段落对齐方式。
   * - ``set_alignment(paragraph_alignment alignment) const``
     - ``bool``
     - 设置段落对齐方式。
   * - ``clear_alignment() const``
     - ``bool``
     - 清除显式段落对齐方式。
   * - ``indent_left_twips() const`` / ``indent_right_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取左右缩进，单位是 twips。
   * - ``first_line_indent_twips() const`` / ``hanging_indent_twips() const``
     - ``std::optional<std::uint32_t>``
     - 读取首行缩进或悬挂缩进。
   * - ``set_indent_left_twips(indent_twips) const``
     - ``bool``
     - 设置左缩进。
   * - ``set_indent_right_twips(indent_twips) const``
     - ``bool``
     - 设置右缩进。
   * - ``set_first_line_indent_twips(indent_twips) const``
     - ``bool``
     - 设置首行缩进。
   * - ``set_hanging_indent_twips(indent_twips) const``
     - ``bool``
     - 设置悬挂缩进。

Run 文本与样式
--------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``get_text() const``
     - ``std::string``
     - 读取当前 run 的文本。
   * - ``set_text(const std::string &text) const``
     - ``bool``
     - 替换当前 run 的文本。
   * - ``remove()``
     - ``bool``
     - 删除当前 run。
   * - ``insert_run_before(text, formatting)``
     - ``Run``
     - 在当前 run 前插入新 run。
   * - ``insert_run_after(text, formatting)``
     - ``Run``
     - 在当前 run 后插入新 run。
   * - ``insert_run_like_before()``
     - ``Run``
     - 在当前 run 前插入 run，并复制 run 属性。
   * - ``insert_run_like_after()``
     - ``Run``
     - 在当前 run 后插入 run，并复制 run 属性。
   * - ``style_id() const``
     - ``std::optional<std::string>``
     - 读取 run 样式 ID。
   * - ``font_family() const``
     - ``std::optional<std::string>``
     - 读取主要字体。
   * - ``set_font_family(font_family) const``
     - ``bool``
     - 设置主要字体。
   * - ``text_color() const``
     - ``std::optional<std::string>``
     - 读取文本颜色。
   * - ``bold() const`` / ``italic() const`` / ``underline() const``
     - ``std::optional<bool>``
     - 读取基础文字样式。
   * - ``strikethrough() const``
     - ``std::optional<bool>``
     - 读取删除线样式。
   * - ``superscript() const`` / ``subscript() const``
     - ``std::optional<bool>``
     - 读取上标或下标样式。
   * - ``font_size_points() const``
     - ``std::optional<double>``
     - 读取字号，单位是 point。

Run 语言与方向
--------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - 方法
     - 返回值
     - 用途
   * - ``language() const``
     - ``std::optional<std::string>``
     - 读取主要语言元数据。
   * - ``east_asia_language() const``
     - ``std::optional<std::string>``
     - 读取东亚语言元数据。
   * - ``bidi_language() const``
     - ``std::optional<std::string>``
     - 读取双向文本语言元数据。
   * - ``set_language(language) const``
     - ``bool``
     - 设置主要语言元数据。
   * - ``set_east_asia_language(language) const``
     - ``bool``
     - 设置东亚语言元数据。
   * - ``set_bidi_language(language) const``
     - ``bool``
     - 设置双向文本语言元数据。
   * - ``rtl() const``
     - ``std::optional<bool>``
     - 读取 run 的从右到左方向元数据。
   * - ``set_rtl(bool enabled = true) const``
     - ``bool``
     - 启用或关闭 run 的从右到左方向元数据。

示例
----

.. code-block:: cpp

   auto paragraph = doc.body_template().append_paragraph("Title");
   paragraph.set_alignment(featherdoc::paragraph_alignment::center);

   auto emphasized = paragraph.add_run(
       " approved",
       featherdoc::formatting_flag::bold);
   emphasized.set_language("en-US");

   auto &run = paragraph.runs();
   if (run.has_next()) {
       run.next().set_font_family("Aptos");
   }
