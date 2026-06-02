枚举和通用选项类型
==================

这些枚举和选项类型会在文档、段落、表格、图片、模板、字段和审阅 API
之间复用。这个中文页用于快速查找参数可选值，完整旧版参考仍保留直达链接。

格式
----

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 类型
     - 可选值
   * - ``formatting_flag``
     - ``none``、``bold``、``italic``、``underline``、``strikethrough``、
       ``superscript``、``subscript``、``smallcaps``、``shadow``。

文档布局
--------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 类型
     - 可选值
   * - ``section_reference_kind``
     - ``default_reference``、``first_page``、``even_page``。
   * - ``page_orientation``
     - ``portrait``、``landscape``。
   * - ``paragraph_alignment``
     - ``left``、``center``、``right``、``justified``、``distribute``。
   * - ``paragraph_line_spacing_rule``
     - ``automatic``、``at_least``、``exact``。

表格
----

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 类型
     - 可选值
   * - ``table_layout_mode``
     - ``autofit``、``fixed``。
   * - ``table_alignment``
     - ``left``、``center``、``right``。
   * - ``row_height_rule``
     - ``automatic``、``at_least``、``exact``。
   * - ``cell_vertical_alignment``
     - ``top``、``center``、``bottom``、``both``。
   * - ``cell_vertical_merge``
     - ``none``、``restart``、``continue_merge``。
   * - ``cell_border_edge``
     - ``top``、``left``、``bottom``、``right``。
   * - ``table_border_edge``
     - ``top``、``left``、``bottom``、``right``、``inside_horizontal``、
       ``inside_vertical``。
   * - ``border_style``
     - ``none``、``single``、``double_line``、``dashed``、``dotted``、
       ``thick``。

模板与审阅
----------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 类型
     - 用途
   * - ``bookmark_kind``
     - 用于检查摘要的书签分类。
   * - ``content_control_kind``
     - 用于检查摘要的内容控件分类。
   * - ``content_control_form_kind``
     - 表单字段分类，例如复选框、日期、下拉框和组合框。
   * - ``field_kind``
     - 字段分类，例如通用字段、TOC、REF、PAGE、NUMPAGES、SEQ 以及相关字段族。
   * - ``review_note_kind``
     - 脚注、尾注或批注的说明类型。
   * - ``revision_kind``
     - 插入、删除和相关跟踪修订类型。

旧版完整页面仍保留在 :doc:`../../api/enums`。

.. FDOC_API_ENUMS_ZH_CN_MARKER
