Paragraph 和 Run
================

``featherdoc::Paragraph`` 是 Word 段落的编辑句柄。
``featherdoc::Run`` 是段落内部文本片段的编辑句柄，用于处理 run 级格式、
语言和文字方向元数据。

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

旧版完整页面仍保留在 :doc:`../../api/content_blocks`。
