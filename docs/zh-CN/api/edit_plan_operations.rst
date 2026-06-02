编辑计划操作
============

``scripts/edit_document_from_plan.ps1`` 接收 JSON 编辑计划，用来脚本化改写
``.docx`` 文档。本页补齐计划格式、执行结果和常用 operation 的参数字段，避免
中文读者只能看到 operation 名称列表。

编辑计划格式
------------

.. FDOC_EDIT_PLAN_PLAN_SHAPE

编辑计划可以是 JSON 数组，也可以是带 ``operations`` 数组的对象。每个
operation 都必须提供 ``op``。``op`` 中的短横线会被规范化为下划线，因此
``append-image`` 和 ``append_image`` 会进入同一个 dispatcher 分支。

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_text",
         "find": "草稿",
         "replace": "定稿",
         "match_case": true
       },
       {
         "op": "append_hyperlink",
         "text": "FeatherDoc",
         "target": "https://github.com/wuxianggujun/FeatherDoc"
       }
     ]
   }

输入 DOCX、输出 DOCX、可选 ``summary_json``、构建目录和 ``skip_build`` 是
``scripts/edit_document_from_plan.ps1`` 的命令参数。单个 operation 通常不
设置 ``input_path`` 或 ``output_path``；``apply_table_position_plan`` 会在调用
CLI 前把当前 ``input_path`` 注入表格定位计划。

执行结果
--------

.. FDOC_EDIT_PLAN_EXECUTION_RESULT

脚本会写出 summary 对象，记录整次运行状态和每个 operation 的执行结果。

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - 字段
     - 含义
   * - ``status``
     - 整个编辑计划的状态，通常是 ``succeeded`` 或 ``failed``。
   * - ``input_docx`` / ``output_docx``
     - 解析后的源 DOCX 和目标 DOCX 路径。
   * - ``edit_plan``
     - 解析后的 JSON 编辑计划路径。
   * - ``summary_json``
     - 调用者要求写出的 summary 文件路径。
   * - ``operation_count``
     - 从计划中读取到的 operation 数量。
   * - ``operations``
     - 每个 operation 的记录，包含 ``index``、``op``、``command``、
       ``input_docx``、``output_docx``、``status``、``exit_code``、
       ``output``，以及失败时的 ``error``。
   * - ``error``
     - 只有在整次运行失败时才出现。

操作参考
--------

.. FDOC_EDIT_PLAN_OPERATION_REFERENCE
.. FDOC_EDIT_PLAN_REQUIRED_FIELDS
.. FDOC_EDIT_PLAN_OPTIONAL_FIELDS

下面优先列出 JSON 计划里最常用的 operation 参数。完整 operation 名称索引见
页面下方。

审阅和修订
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``accept_all_revisions``
     - ``op``
     - 无。
     - 对整个文档调用 ``accept-all-revisions``。
   * - ``set_comment_resolved``
     - ``op``。脚本会把字段包装为 review mutation plan。
     - ``comment_index``/``index``、``resolved``、``expected_text``/
       ``expected_anchor_text``、``expected_comment_text``/
       ``expected_comment_body``、``expected_resolved``、
       ``expected_parent_index``。
     - 调用 ``apply-review-mutation-plan`` 更新批注 resolved 状态。
   * - ``append_comment``
     - ``selected_text``/``anchor_text`` 和 ``comment_text``/``text``/``body``。
     - ``author``/``comment_author``、``initials``/``comment_initials``、
       ``date``/``comment_date``。
     - 在匹配文本上追加批注。
   * - ``apply_review_mutation_plan``
     - ``plan_file``/``plan_path``/``review_plan_file``/
       ``review_mutation_plan_file``，或内联 ``review_plan``/``operations``。
     - 文件计划和内联计划不能同时提供。
     - 通过 ``apply-review-mutation-plan`` 回放批注变更。

注释和字段
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``append_page_number_field``
     - ``op``
     - ``part`` 默认 ``body``；``part_index``/``index``、``section``、
       ``kind``、``dirty``、``locked``。
     - 在指定文档 part 中追加页码字段。
   * - ``append_hyperlink_field``
     - ``bookmark_name``/``bookmark``/``name``。
     - ``result_text``/``text``、字段状态和 part 选择字段。
     - 追加指向书签的 Word hyperlink field。
   * - ``set_update_fields_on_open``
     - ``op``
     - 可通过专用 enable/disable operation 设置 ``enabled``。
     - 设置文档打开时更新字段的开关。

模板槽和内容控件
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``replace_content_control_text_by_tag``
     - ``text``，并且只能提供一种 selector：``tag``/``content_control_tag``
       或 ``alias``/``content_control_alias``。
     - ``part`` 默认 ``body``；``part_index``/``index``、``section``、
       ``kind``。
     - 对选中的内容控件调用 ``replace-content-control-text``。
   * - ``replace_content_control_table_rows``
     - selector 字段和 ``rows``。
     - part 选择字段；本 operation 允许空 rows。
     - 替换内容控件表格行。
   * - ``replace_bookmark_text``
     - ``bookmark``/``bookmark_name`` 和 ``text``。
     - 书签 selector 选项。
     - 用文本替换书签内容。

图片、链接和 OMML
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``append_image``
     - ``image_path``/``image``/``path``/``file``。
     - part 选择字段；``floating``；``width``/``width_px``；``height``/
       ``height_px``；浮动图片还支持 ``horizontal_reference``、
       ``horizontal_offset``、``vertical_reference``、``vertical_offset``、
       ``behind_text``、``allow_overlap``、``z_order``、``wrap_mode``、
       wrap distance 和 crop 字段。
     - 调用 ``append-image``。默认追加行内图片，``floating`` 为 true 时追加浮动图片。
   * - ``append_hyperlink``
     - ``text`` 和 ``target``/``url``/``href``。
     - 无。
     - 调用 ``append-hyperlink`` 追加超链接。
   * - ``append_omml``
     - ``xml``/``omml``/``omml_xml``。
     - 无。
     - 追加 OMML 公式。

文本和段落变更
~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``replace_text``
     - ``find``/``search``/``old_text`` 和 ``replace``/``replacement``/
       ``new_text``/``text``。
     - ``match_case`` 默认 ``true``。
     - 替换 ``word/document.xml`` 中的段落文本，并返回替换数量。
   * - ``set_text_style``
     - 一个目标：``paragraph_index``/``index``、``text_contains``/
       ``contains``，或 ``table_index`` + ``row_index`` + ``cell_index``；
       同时至少提供一个样式字段。
     - ``bold``、``font_family``/``font``/``latin_font_family``、
       ``east_asia_font_family`` 别名、``language`` 别名、
       ``east_asia_language`` 别名、``color``/``text_color``/
       ``font_color``、``font_size_points`` 别名、``font_size_half_points`` 别名。
     - 对正文段落或表格单元格内段落的 runs 设置样式。
   * - ``set_paragraph_alignment``
     - 目标 ``paragraph_index``/``index`` 或 ``text_contains``/
       ``paragraph_text_contains``；``alignment``/``horizontal_alignment``。
     - 对齐值别名：``start`` = ``left``，``centre`` = ``center``，
       ``end`` = ``right``，``justify`` = ``both``。
     - 设置正文段落水平对齐。

表格和编号
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 24 28 30 18

   * - Operation
     - 必填字段
     - 可选字段和别名
     - 作用
   * - ``set_table_col_width``
     - ``table_index``、``column_index``/``col_index``/``column``，以及大于
       0 的 ``width_twips``/``column_width_twips``/``width``。
     - 除上述别名外无额外字段。
     - 设置正文表格 grid column 和单元格宽度。
   * - ``merge_table_cell``
     - ``table_index``、``row_index``、``cell_index``、``direction``/
       ``merge_direction``。
     - ``count``/``merge_count``。
     - 从指定单元格向指定方向调用 ``merge-table-cells``。
   * - ``unmerge_table_cell``
     - ``table_index``、``row_index``、``cell_index``、``direction``/
       ``merge_direction``。
     - 无。
     - 从指定单元格向指定方向调用 ``unmerge-table-cells``。
   * - ``apply_table_position_plan``
     - ``plan_file``/``plan_path``/``table_position_plan_file``/
       ``table_position_plan_path``，或内联 ``table_position_plan``/``plan``。
     - ``dry_run`` 必须省略或为 ``false``；脚本会在调用
       ``apply-table-position-plan`` 前注入 ``input_path``。
     - 应用表格定位计划并写出编辑计划的目标 DOCX。
   * - ``import_numbering_catalog``
     - ``catalog_file``/``catalog_path``/``numbering_catalog_file``/
       ``numbering_catalog_path``。
     - 无。
     - 调用 ``import-numbering-catalog`` 导入编号目录。

JSON 示例
---------

.. FDOC_EDIT_PLAN_JSON_EXAMPLES

替换内容控件、追加图片并设置文本样式：

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_content_control_text_by_tag",
         "tag": "customer_name",
         "text": "Ada Lovelace"
       },
       {
         "op": "append_image",
         "image_path": "assets/logo.png",
         "part": "body",
         "width": 320
       },
       {
         "op": "set_text_style",
         "text_contains": "Ada Lovelace",
         "bold": true,
         "font_family": "Aptos",
         "east_asia_font_family": "Microsoft YaHei",
         "color": "1F4E79",
         "font_size_points": 12
       }
     ]
   }

更新表格布局和编号：

.. code-block:: json

   {
     "operations": [
       {
         "op": "set_table_col_width",
         "table_index": 0,
         "column_index": 1,
         "width_twips": 2880
       },
       {
         "op": "apply_table_position_plan",
         "table_position_plan": {
           "tables": [
             {
               "table_index": 0,
               "alignment": "center"
             }
           ]
         }
       },
       {
         "op": "import_numbering_catalog",
         "catalog_file": "numbering.catalog.json"
       }
     ]
   }

完整操作索引
------------

审阅与修订操作
~~~~~~~~~~~~~~

* ``accept_all_revisions``
* ``reject_all_revisions``
* ``set_comment_resolved``
* ``set_comment_metadata``
* ``append_comment``
* ``append_comment_reply``
* ``replace_comment``
* ``remove_comment``
* ``apply_review_mutation_plan`` (CLI route: ``apply-review-mutation-plan``)
* ``append_insertion_revision``
* ``append_deletion_revision``
* ``insert_run_revision_after``
* ``delete_run_revision``
* ``replace_run_revision``
* ``accept_revision``
* ``reject_revision``
* ``set_revision_metadata``

注释、脚注尾注与字段
~~~~~~~~~~~~~~~~~~~~

* ``append_footnote``
* ``replace_footnote``
* ``remove_footnote``
* ``append_endnote``
* ``replace_endnote``
* ``remove_endnote``
* ``append_page_number_field`` (CLI route: ``append-page-number-field``)
* ``append_total_pages_field``
* ``append_table_of_contents_field``
* ``append_document_property_field``
* ``append_field``
* ``append_date_field``
* ``append_reference_field``
* ``append_page_reference_field``
* ``append_style_reference_field``
* ``append_hyperlink_field``
* ``append_caption``
* ``append_index_entry_field``
* ``append_index_field``
* ``append_sequence_field``
* ``append_complex_field``
* ``replace_field``
* ``set_update_fields_on_open``
* ``enable_update_fields_on_open``
* ``disable_update_fields_on_open``
* ``clear_update_fields_on_open``

模板槽与内容控件
~~~~~~~~~~~~~~~~

* ``replace_content_control_text``
* ``replace_content_control_text_by_tag``
* ``replace_content_control_text_by_alias``
* ``replace_content_control_paragraphs``
* ``replace_content_control_table``
* ``replace_content_control_table_rows``
* ``replace_content_control_image``
* ``set_content_control_form_state``
* ``sync_content_controls_from_custom_xml``
* ``sync_content_control_from_custom_xml``
* ``replace_bookmark_text``
* ``replace_bookmark_paragraphs``
* ``replace_bookmark_table_rows``
* ``replace_bookmark_table``
* ``remove_bookmark_block``
* ``delete_bookmark_block``
* ``replace_bookmark_image``
* ``replace_bookmark_floating_image``

图片、链接与 OMML
~~~~~~~~~~~~~~~~~

* ``append_image`` (CLI route: ``append-image``)
* ``append_floating_image``
* ``replace_image``
* ``remove_image``
* ``append_hyperlink`` (CLI route: ``append-hyperlink``)
* ``replace_hyperlink``
* ``remove_hyperlink``
* ``delete_hyperlink``
* ``append_omml`` (CLI route: ``append-omml``)
* ``replace_omml``
* ``remove_omml``
* ``delete_omml``

文本与段落变更
~~~~~~~~~~~~~~

* ``insert_paragraph_text_revision``
* ``delete_paragraph_text_revision``
* ``replace_paragraph_text_revision``
* ``insert_text_range_revision``
* ``delete_text_range_revision``
* ``replace_text_range_revision``
* ``append_paragraph_text_comment``
* ``append_text_range_comment``
* ``set_paragraph_text_comment_range``
* ``set_text_range_comment_range``
* ``replace_text``
* ``replace_document_text``
* ``set_text_style``
* ``set_text_format``
* ``set_paragraph_text_style``
* ``delete_paragraph``
* ``remove_paragraph``
* ``set_paragraph_horizontal_alignment``
* ``set_paragraph_alignment``
* ``clear_paragraph_horizontal_alignment``
* ``clear_paragraph_alignment``
* ``set_paragraph_line_spacing``
* ``clear_paragraph_spacing``

表格与编号
~~~~~~~~~~

* ``apply_table_position_plan`` (CLI route: ``apply-table-position-plan``)
* ``import_numbering_catalog``
* ``set_table_col_width``
* ``clear_table_col_width``
* ``clear_table_width``
* ``clear_table_alignment``
* ``clear_table_indent``
* ``clear_table_layout_mode``
* ``clear_table_style_id``
* ``clear_table_style_look``
* ``clear_table_cell_spacing``
* ``clear_table_default_cell_margin``
* ``clear_table_border``
* ``delete_table_row``
* ``delete_table_column``
* ``delete_table``
* ``insert_table_before``
* ``insert_table_like_before``
* ``merge_table_cell``
* ``unmerge_table_cells``
* ``unmerge_table_cell``
