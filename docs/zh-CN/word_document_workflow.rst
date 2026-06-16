Word 文档处理工作流
===================

本页是 FeatherDoc 处理 Microsoft Word ``.docx`` 文件的任务型入口。已经知道要
完成“生成、填充、检查或批量改写文档”时，先从这里选工作流，再进入具体 API
页面查看完整签名和返回语义。

工作流地图
----------

.. list-table::
   :header-rows: 1
   :widths: 24 42 34

   * - 任务
     - 推荐入口
     - 详细说明
   * - 打开、创建和保存文档
     - ``featherdoc::Document``
     - :doc:`api/document`
   * - 填充 Word 模板
     - ``Document::body_template()`` 和 ``TemplatePart``
     - :doc:`api/template_part`
   * - 编辑正文文本
     - ``Paragraph`` 和 ``Run``
     - :doc:`api/paragraph_run`
   * - 编辑表格
     - ``Table``、``TableRow`` 和 ``TableCell``
     - :doc:`api/table`
   * - 添加图片
     - ``Document::append_image(...)`` 或 ``TemplatePart::append_image(...)``
     - :doc:`api/images`
   * - 管理分节、页眉和页脚
     - ``section_header_template(...)``、``section_footer_template(...)`` 和页面设置 API
     - :doc:`api/sections`
   * - 管理字段、超链接、批注和修订
     - 文档级或部件级字段与审阅 API
     - :doc:`api/fields_links_reviews`
   * - 自动化批量编辑
     - ``scripts/edit_document_from_plan.ps1``
     - :doc:`api/edit_plan_operations`

打开与保存
----------

最小安全流程是打开文档、修改内容、另存为新路径。返回 ``std::error_code`` 的
操作失败时，用 ``last_error()`` 查看包内条目和详细原因。

.. code-block:: cpp

   #include <featherdoc.hpp>

   int main() {
       featherdoc::Document doc{"template.docx"};
       if (auto error = doc.open()) {
           return 1;
       }

       const auto replaced =
           doc.replace_content_control_text_by_tag("customer", "Ada Lovelace");
       if (replaced == 0) {
           return 1;
       }

       return doc.save_as("filled.docx") ? 1 : 0;
   }

模板填充
--------

正式业务模板优先使用 Word 书签或内容控件，不要优先依赖全文替换。tag 和 alias
比段落下标更稳定，更适合合同、报价单、发票、通知书这类会反复调整版式的模板。

.. code-block:: cpp

   auto body = doc.body_template();
   if (!body) {
       return 1;
   }

   body.replace_bookmark_text("invoice_no", "INV-2026-001");
   body.replace_content_control_text_by_tag("customer_name", "Ada Lovelace");
   body.replace_content_control_with_table_by_tag("line_items", {
       {"Item", "Qty", "Amount"},
       {"Support", "1", "100.00"}
   });

正文编辑
--------

目标是文本内容或 run 级格式时，使用 ``Paragraph`` 和 ``Run``。目标是表格结构、
单元格文本、宽度、边框、合并单元格或重复表头时，使用 ``Table`` 系列 API。

.. code-block:: cpp

   auto summary = doc.body_template().append_paragraph("Status:");
   auto value = summary.add_run(" approved", featherdoc::formatting_flag::bold);
   value.set_font_family("Aptos");
   summary.set_alignment(featherdoc::paragraph_alignment::center);

   auto table = doc.append_table(2, 2);
   table.set_row_texts(0, {"Name", "Status"});
   table.set_row_texts(1, {"Ada", "Approved"});

页眉、页脚与分节
----------------

模板存在分节专属页眉或页脚时，先解析目标分节，再用和正文一致的 ``TemplatePart``
操作处理页眉页脚内容。

.. code-block:: cpp

   auto footer = doc.section_footer_template(
       0, featherdoc::section_reference_kind::default_reference);
   if (footer) {
       footer.append_page_number_field();
   }

批量编辑
--------

``scripts/edit_document_from_plan.ps1`` 是更适合自动化的改写入口。编辑计划使用
UTF-8 JSON，执行时写出 summary，方便 CI 或发布脚本检查每个 operation 的结果。

.. code-block:: json

   {
     "operations": [
       {
         "op": "replace_content_control_text_by_tag",
         "tag": "customer_name",
         "text": "Ada Lovelace"
       },
       {
         "op": "append_page_number_field",
         "part": "section-footer",
         "section": 0
       }
     ]
   }

.. code-block:: powershell

   powershell -ExecutionPolicy Bypass -File .\scripts\edit_document_from_plan.ps1 `
     -InputDocx .\template.docx `
     -EditPlan .\plan.json `
     -OutputDocx .\output\filled.docx `
     -SummaryJson .\output\filled.summary.json `
     -SkipBuild

验证方式
--------

建议按层验证：

* 对改动到的 API 或脚本补 C++ / PowerShell 聚焦测试。
* 自动化批处理检查 ``summary_json`` 中的 operation 状态。
* 涉及最终可见版式时，运行 ``scripts/run_word_visual_smoke.ps1`` 做视觉验证。
* 源文件、JSON 编辑计划和中文文本统一使用 UTF-8。Windows 上排查中文输出时，
  用 ``Get-Content -Encoding UTF8`` 读取文件，并显式设置控制台输出编码。

相关页面
--------

* :doc:`getting_started`
* :doc:`api/index`
* :doc:`api/document`
* :doc:`api/template_part`
* :doc:`api/edit_plan_operations`
