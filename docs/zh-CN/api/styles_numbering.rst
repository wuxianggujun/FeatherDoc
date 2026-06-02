样式和编号 API
==============

``featherdoc::Document`` 提供样式和编号 API，用于检查、受控修改、重构和默认格式
设置。这个页面先给出直接入口，完整旧版分组页面仍可继续查阅。

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

旧版完整页面仍保留在 :doc:`../../api/styles_numbering`。
