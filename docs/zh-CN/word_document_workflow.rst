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

``open()`` 默认执行严格 OPC 结构校验。公开 ``archive_limits`` 保持原有 5 个字段，
默认值为 10,000 个条目、单个 XML 部件 64 MiB、单个二进制部件 256 MiB、总解压量
512 MiB、最大压缩比 200。中央目录 64 MiB、单个物理 entry name 511 bytes、全部
entry name 合计 8 MiB 是内部固定 metadata 限制，不是调用方可配置的
``archive_limits`` 字段；entry name 包含嵌入 NUL 时会被拒绝。只有在修复已知损坏
的历史文档时，才显式使用
``document_open_options`` 的 ``tolerant`` 模式。调用 ``set_path()``、再次
``open()`` 或 ``create_empty()`` 后，必须重新获取段落、Run、表格和模板部件句柄。
懒加载、Custom XML 同步、图片提取和保存都会复验当前完整源包，因此在 ``open()``
后替换文件也不能绕过资源限制或 UTF-8/规范路径检查。它们还会把 entry 名、逻辑身份、
CRC32、大小和目录标记组成的有序 metadata 指纹与 ``open()`` 快照比较；不一致时返回
``source_archive_changed``，而不是混用两代包内容。覆盖源路径的 ``save()`` 会在替换前
做最后一次比较，并用打开时的原始限制校验输出。该机制是协作式一致性检测，不是密码学
哈希或跨进程文件锁。
strict 模式要求 ZIP entry 使用 ASCII 规范物理名称和大写 ``%HH`` 转义；tolerant
模式可以读取历史包中的 raw Unicode 物理名称，但仍会拒绝存在歧义的逻辑身份。
``save()``/``save_as()`` 会把复制的 entry 统一写成 percent-encoded 规范名称。
在 tolerant 历史拼写完成规范化后，两种模式都会拒绝逻辑名称中超出 OPC ``pchar``
集合的字符、空段、全点段、以点结尾的段，以及一个 PartName 通过追加路径段即可得到
另一个 PartName 的歧义组合。
``[Content_Types].xml`` 还会按 ASCII 大小写无关身份检查重复的
``Override/@PartName`` 与 ``Default/@Extension``。Unicode 扩展名必须写成 UTF-8
``%HH`` 字节，因为 raw 非 ASCII 文本不符合 ``ST_Extension``。
核心 WordprocessingML 部件允许 transitional namespace 使用默认、别名或合法 UTF-8
前缀，包括中文前缀。FeatherDoc 会把内存 DOM 的元素名和属性名规范化为 ``w:``。
常规保存始终重新序列化主文档及已加载的页眉/页脚；clean 的懒加载 singleton 部件仍可
按原始字节复制，dirty 后保存为规范形式。错误的显式 ``xmlns:w`` 绑定会 fail closed。
主文档根错误在 strict 模式被拒绝、在 tolerant 模式记录结构诊断；页眉/页脚根错误在
两种模式下都会使打开失败，懒加载 singleton 根错误则在首次访问时失败。
``commentsExtended`` 不会进入该 WML 规范化器。
每个 ``Default`` 和 ``Override`` 还必须提供语法合法的 ``ContentType`` media
type；缺失属性、非法参数、控制字符、非法 UTF-8 与不支持的非 ASCII token 在两种校验
模式下都会被拒绝。
内部 OPC relationship target 以来源部件为基准，直接按 UTF-8 package path 解析，
不依赖主机文件系统 code page；嵌套页眉、页脚部件及其图片关系同样遵循这一规则。
移动 ``Document`` 也会使源对象返回的旧句柄失效；移动赋值还会使目标对象旧状态的
句柄失效。移动后的源对象处于关闭状态，可以重新初始化。移动操作继续保持
``noexcept``，``Document`` 维持 1.13 对象布局；新增内部 bookkeeping 存放在 opaque
sidecar 中。

修复损坏 DOCX 时，不要把 tolerant 打开成功当成文档已经合法。先读取诊断，再显式
修复并另存；修复成功后也必须重新获取 XML-backed 句柄。

不要因为 tolerant 打开成功就直接进入常规编辑流程；该模式只用于检查和显式修复。
真正缺失的 relationship part 可以由负责它的 API 创建，但已经存在且根节点或 package
namespace、Relationship 子节点 namespace 或结构非法的 document、页眉或页脚
``.rels`` 会受到保护，不会被隐式重建；缺失必需属性、重复 Id 和非法 ``TargetMode``
同样会被拒绝。
超链接、图片、singleton part 和分节 relationship 修改会返回
``invalid_package_structure``。内容控件图片替换会在修改控件、添加 drawing XML 或媒体
部件、更新 Content Types 之前失败；随后保存仍保留原有非法关系结构，不会静默覆盖。
必须先显式修复诊断指出的包问题，再继续编辑。

合法 relationship 元素按 expanded QName 匹配，因此默认 namespace 与带前缀的
``Relationships``/``Relationship`` 都受支持。已有前缀会保留，新建 relationship 子节点
复用根节点前缀；OPC relationship 属性本身仍必须不带前缀。

.. code-block:: cpp

   featherdoc::Document damaged{"legacy.docx"};
   featherdoc::document_open_options options;
   options.validation = featherdoc::package_validation_mode::tolerant;
   if (damaged.open(options)) {
       return 1;
   }

   for (const auto &diagnostic : damaged.package_diagnostics()) {
       if (!diagnostic.repairable) {
           return 1;
       }
   }
   if (!damaged.repair_package()) {
       return 1;
   }
   return damaged.save_as("legacy-repaired.docx") ? 1 : 0;

对应 CLI 流程为 ``featherdoc_cli inspect-package legacy.docx --json``，确认所有问题
可修复后执行 ``featherdoc_cli repair-package legacy.docx --output
legacy-repaired.docx --json``。修复命令强制要求输出路径，不会覆盖输入文件。

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

CLI 在 POSIX 上要求命令行参数是合法 UTF-8；Windows 上直接读取原生 UTF-16 命令行，
中文、日文、emoji、空格和长路径不会经过当前控制台 code page。所有平台上的 JSON、
文本输入、stdout 和 stderr 仍统一使用 UTF-8。

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
