字段、链接和审阅 API
====================

字段、超链接、脚注、尾注、批注和修订 API 位于 ``featherdoc::Document`` 上。
其中可以限定到正文、页眉、页脚或分节部件的字段和超链接操作，也可通过
``featherdoc::TemplatePart`` 使用。

摘要类型
--------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - 类型
     - 关键字段
     - 用途
   * - ``featherdoc::field_summary``
     - ``kind``、``instruction``、``result_text``、``dirty``、``locked``
     - 描述简单或复杂 Word 字段。
   * - ``featherdoc::hyperlink_summary``
     - ``text``、``target``、``anchor``、``external``
     - 描述内部或外部超链接。
   * - ``featherdoc::review_note_summary``
     - ``kind``、``author``、``date``、``resolved``、``text``
     - 描述脚注、尾注、批注和批注回复。
   * - ``featherdoc::revision_summary``
     - ``kind``、``author``、``part_entry_name``、``text``
     - 描述跟踪插入、删除以及相关修订记录。

字段
----

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_fields() const``
     - ``std::vector<field_summary>``
     - 枚举选中文档部件中的字段。
   * - ``append_field(instruction, result_text, state)``
     - ``bool``
     - 追加简单字段指令和可选显示结果。
   * - ``append_complex_field(instruction, result_text, options)``
     - ``bool``
     - 追加复杂 Word 字段。
   * - ``append_page_number_field(state = {})``
     - ``bool``
     - 追加 PAGE 字段。
   * - ``append_total_pages_field(state = {})``
     - ``bool``
     - 追加 NUMPAGES 字段。
   * - ``append_table_of_contents_field(options, result_text)``
     - ``bool``
     - 追加 TOC 字段。
   * - ``append_reference_field(bookmark_name, options, result_text)``
     - ``bool``
     - 追加指向书签的 REF 字段。
   * - ``replace_field(field_index, instruction, result_text)``
     - ``bool``
     - 按索引替换字段指令和结果。

超链接
------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_hyperlinks() const``
     - ``std::vector<hyperlink_summary>``
     - 枚举超链接。
   * - ``append_hyperlink(text, target)``
     - ``std::size_t``
     - 追加超链接并返回创建的索引。
   * - ``replace_hyperlink(hyperlink_index, text, target)``
     - ``bool``
     - 替换超链接文本和目标。
   * - ``remove_hyperlink(hyperlink_index)``
     - ``bool``
     - 删除一个超链接。

脚注、批注和修订
----------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``list_footnotes() const`` / ``list_endnotes() const``
     - ``std::vector<review_note_summary>``
     - 枚举脚注或尾注。
   * - ``append_footnote(reference_text, note_text)``
     - ``std::size_t``
     - 追加脚注。
   * - ``append_endnote(reference_text, note_text)``
     - ``std::size_t``
     - 追加尾注。
   * - ``replace_footnote(note_index, note_text)``
     - ``bool``
     - 替换一个脚注。
   * - ``remove_endnote(note_index)``
     - ``bool``
     - 删除一个尾注。
   * - ``list_comments() const``
     - ``std::vector<review_note_summary>``
     - 枚举批注。
   * - ``append_comment(selected_text, comment_text, author, initials, date)``
     - ``std::size_t``
     - 追加锚定到选中文本的批注。
   * - ``append_text_range_comment(start_paragraph_index, start_text_offset, end_paragraph_index, end_text_offset, comment_text, author, initials, date)``
     - ``std::size_t``
     - 为文本范围添加批注。
   * - ``set_comment_resolved(comment_index, resolved)``
     - ``bool``
     - 标记批注已解决或未解决。
   * - ``list_revisions() const``
     - ``std::vector<revision_summary>``
     - 枚举跟踪修订。
   * - ``accept_revision(revision_index)`` / ``reject_revision(revision_index)``
     - ``bool``
     - 接受或拒绝一个修订。
   * - ``accept_all_revisions()`` / ``reject_all_revisions()``
     - ``std::size_t``
     - 接受或拒绝全部修订。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"review.docx"};
   doc.open();

   auto body = doc.body_template();
   body.append_page_number_field();
   body.append_hyperlink("FeatherDoc", "https://github.com/wuxianggujun/FeatherDoc");

   for (const auto &comment : doc.list_comments()) {
       if (!comment.resolved) {
           doc.set_comment_resolved(comment.index, true);
       }
   }

旧版完整页面仍保留在 :doc:`../../api/fields_links_reviews`。
