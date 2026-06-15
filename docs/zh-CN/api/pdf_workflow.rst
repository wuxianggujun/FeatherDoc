PDF 工作流
==========

PDF 支持仍是实验性、显式开启的能力。本页是 ``export-pdf`` 和
``import-pdf`` CLI 工作流、JSON 诊断、支持范围和发布验证引用的中文公开契约。

构建选项
--------

.. list-table::
   :header-rows: 1
   :widths: 24 32 46

   * - 工作流
     - CMake 选项
     - 用途
   * - PDF 导出
     - ``-DFEATHERDOC_BUILD_PDF=ON``
     - 启用 ``featherdoc_cli export-pdf`` 使用的进程内 PDF writer。
   * - PDF 导入
     - ``-DFEATHERDOC_BUILD_PDF_IMPORT=ON``
     - 启用 ``featherdoc_cli import-pdf`` 使用的 PDFium 文本导入器。

使用 CLI 入口前，需要先构建 PDF 能力：

.. code-block:: sh

   cmake -S . -B build-pdf -DFEATHERDOC_BUILD_PDF=ON -DFEATHERDOC_BUILD_PDF_IMPORT=ON -DBUILD_CLI=ON

PDF 导出
--------

``export-pdf`` 通过进程内 PDF writer 把 ``.docx`` 文档写成 PDF：

.. code-block:: sh

   featherdoc_cli export-pdf input.docx --output output.pdf --json
   featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --summary-json output.summary.json --json
   featherdoc_cli export-pdf input.docx --output output.pdf --render-headers-and-footers --expand-header-footer-page-placeholders --summary-json output.summary.json --json

常用选项：

* ``--output <pdf>`` 选择输出 PDF 路径。
* ``--render-headers-and-footers`` 渲染分节页眉和页脚。
* ``--expand-header-footer-page-placeholders`` 在导出时展开 ``{{page}}`` 和
  ``{{total_pages}}`` 占位符。
* ``--render-inline-images`` 启用受支持的行内图片渲染。
* ``--font-file <path>`` 和 ``--cjk-font-file <path>`` 提供显式字体。
* ``--font-map <family>=<path>`` 把文档字体族映射到字体文件。
* ``--no-font-subset`` 关闭 Unicode 字体子集。
* ``--no-system-font-fallbacks`` 关闭系统字体回退查找。
* ``--summary-json <path>`` 写出机器可读导出摘要。
* ``--json`` 输出机器可读命令结果。

成功 JSON 包含 ``command``、``ok``、``output``、``bytes_written`` 和实际选项：

.. code-block:: json

   {
     "command": "export-pdf",
     "ok": true,
     "output": "output.pdf",
     "bytes_written": 12345,
     "options": {
       "render_headers_and_footers": true,
       "render_inline_images": false,
       "expand_header_footer_page_placeholders": true,
       "subset_unicode_fonts": true,
       "use_system_font_fallbacks": true
     }
   }

失败 JSON
~~~~~~~~~

带 ``--json`` 时，失败输出保持 ``ok`` 为 ``false``，并报告 ``command``、
``stage`` 和 ``message``。文档或 writer 失败还可能携带 ``detail``、``entry`` 和
``xml_offset``：

.. code-block:: json

   {
     "command": "export-pdf",
     "ok": false,
     "stage": "export",
     "message": "Operation not supported",
     "detail": "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON"
   }

当前导出失败阶段包括 ``"stage": "parse"``、``"stage": "open"``、
``"stage": "export"`` 和 ``"stage": "summary"``。

导出支持范围包括段落文本、基础表格、基础 run 样式、部分 CJK 字体回退流程、分节页面设置、
可选页眉页脚、可选行内图片，以及 PDF 视觉验证流程使用的回归样本。它不是生产级 Word
兼容排版引擎，复杂分页、完整 Word 字段求值、任意绘图重建和视觉级精确还原仍在稳定契约外。

开发构建和发布闸门证据仍记录在 ``BUILDING_PDF.md`` 和
``docs/pdf_release_readiness_checklist_zh.rst``。

PDF 导入
--------

``import-pdf`` 面向可提取文本和字符几何信息的 text-first PDF。它不是通用的
PDF-to-Word converter，也不承诺 arbitrary visual fidelity。

.. code-block:: sh

   featherdoc_cli import-pdf input.pdf --output imported.docx --json
   featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --json
   featherdoc_cli import-pdf input.pdf --output imported.docx --import-table-candidates-as-tables --min-table-continuation-confidence 90 --json

``--import-table-candidates-as-tables`` 显式启用表格候选提升。
``--min-table-continuation-confidence`` 调整跨页表格延续阈值。不要在正式页面写占位形式；
示例应使用具体数字阈值。

PDF 导入 JSON 诊断
------------------

本节说明 ``featherdoc_cli import-pdf --json`` 输出的 PDF import JSON diagnostics。

成功 JSON 包含 ``command``、``ok``、``input``、``output`` 和导入计数：

.. code-block:: json

   {
     "command": "import-pdf",
     "ok": true,
     "input": "input.pdf",
     "output": "imported.docx",
     "paragraphs_imported": 2,
     "tables_imported": 1,
     "table_continuation_diagnostics_count": 2,
     "table_continuation_diagnostics": [
       {
         "page_index": 0,
         "block_index": 1,
         "source_row_offset": 0,
         "continuation_confidence": 0,
         "minimum_continuation_confidence": 0,
         "has_previous_table": false,
         "is_first_block_on_page": false,
         "is_near_page_top": true,
         "source_rows_consistent": true,
         "column_count_matches": false,
         "column_anchors_match": false,
         "previous_has_repeating_header": false,
         "source_has_repeating_header": false,
         "header_matches_previous": true,
         "header_match_kind": "not_required",
         "skipped_repeating_header": false,
         "disposition": "created_new_table",
         "blocker": "no_previous_table"
       },
       {
         "page_index": 1,
         "block_index": 0,
         "source_row_offset": 0,
         "continuation_confidence": 85,
         "minimum_continuation_confidence": 0,
         "has_previous_table": true,
         "is_first_block_on_page": true,
         "is_near_page_top": true,
         "source_rows_consistent": true,
         "column_count_matches": true,
         "column_anchors_match": true,
         "previous_has_repeating_header": false,
         "source_has_repeating_header": false,
         "header_matches_previous": true,
         "header_match_kind": "not_required",
         "skipped_repeating_header": false,
         "disposition": "merged_with_previous_table",
         "blocker": "none"
       }
     ],
     "import_table_candidates_as_tables": true
   }

命令行设置 ``--min-table-continuation-confidence`` 时会额外输出
``min_table_continuation_confidence``。
``table_continuation_diagnostics`` 按表格候选发现顺序排列，用于解释候选表格为什么新建表格、
为什么与上一页表格合并，或者为什么被阻止跨页合并。

稳定字段包括：

* ``page_index``、``block_index``、``source_row_offset``。
* ``continuation_confidence``、``minimum_continuation_confidence``。
* ``has_previous_table``、``is_first_block_on_page``、``is_near_page_top``、
  ``source_rows_consistent``、``column_count_matches``、``column_anchors_match``。
* ``previous_has_repeating_header``、``source_has_repeating_header``、
  ``header_matches_previous``、``header_match_kind``、``skipped_repeating_header``。
* ``disposition``：``none``、``created_new_table`` 或
  ``merged_with_previous_table``。
* ``blocker``：拒绝跨页合并的第一个原因，成功合并时为 ``none``。

诊断对象字段按 CLI JSON 输出顺序展示，用户可见示例应与 CLI 实现保持一致：
``page_index``、``block_index``、``source_row_offset``、
``continuation_confidence``、``minimum_continuation_confidence``、
``has_previous_table``、``is_first_block_on_page``、``is_near_page_top``、
``source_rows_consistent``、``column_count_matches``、
``column_anchors_match``、``previous_has_repeating_header``、
``source_has_repeating_header``、``header_matches_previous``、
``header_match_kind``、``skipped_repeating_header``、``disposition`` 和
``blocker``。

``header_match_kind`` 可以是 ``none``、``not_required``、``exact``、
``normalized_text``、``plural_variant``、``canonical_text`` 或 ``token_set``。
``blocker`` 可以是 ``none``、``no_previous_table``、
``not_first_block_on_page``、``not_near_page_top``、
``inconsistent_source_rows``、``column_count_mismatch``、
``column_anchors_mismatch``、``repeated_header_mismatch`` 或
``continuation_confidence_below_threshold``。

重复表头续页候选合并时，诊断会显式记录规则决策：
``source_row_offset = 1`` 表示跳过候选表第一行重复表头，
``skipped_repeating_header = true``，``continuation_confidence = 95``，
``header_match_kind = exact``，``blocker = none``。该 confidence 是确定性的启发式分数，
不是概率。

.. code-block:: json

   {
     "source_row_offset": 1,
     "continuation_confidence": 95,
     "header_match_kind": "exact",
     "skipped_repeating_header": true,
     "disposition": "merged_with_previous_table",
     "blocker": "none"
   }

``inconsistent_source_rows`` 是内部一致性保护；
当前 parser 会按检测到的列锚点补齐候选表的每一行，因此正常导入不应把它当作稳定的用户可触发
blocker。

常见 continuation blockers：

* ``column_count_mismatch``：候选表格检测到的列数与上一张表不一致。
* ``repeated_header_mismatch``：重复表头语义不匹配。
* ``column_anchors_mismatch``：列锚点漂移超过容忍范围。
* ``continuation_confidence_below_threshold``：未达到
  ``--min-table-continuation-confidence``。
* ``not_first_block_on_page``：候选表格不是页面第一个内容块。
* ``not_near_page_top``：候选表格不在页面顶部附近。

代表性诊断片段：

下面的完整 blocker diagnostic object 按 CLI JSON 字段顺序展示常见拆表场景：

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 70,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": true,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": false,
     "header_match_kind": "none",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "repeated_header_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 30,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": false,
     "column_anchors_match": false,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": false,
     "header_match_kind": "none",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "column_count_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 55,
     "minimum_continuation_confidence": 0,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": false,
     "previous_has_repeating_header": true,
     "source_has_repeating_header": true,
     "header_matches_previous": true,
     "header_match_kind": "exact",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "column_anchors_mismatch"
   }

.. code-block:: json

   {
     "page_index": 1,
     "block_index": 0,
     "source_row_offset": 0,
     "continuation_confidence": 85,
     "minimum_continuation_confidence": 90,
     "has_previous_table": true,
     "is_first_block_on_page": true,
     "is_near_page_top": true,
     "source_rows_consistent": true,
     "column_count_matches": true,
     "column_anchors_match": true,
     "previous_has_repeating_header": false,
     "source_has_repeating_header": false,
     "header_matches_previous": true,
     "header_match_kind": "not_required",
     "skipped_repeating_header": false,
     "disposition": "created_new_table",
     "blocker": "continuation_confidence_below_threshold"
   }

.. code-block:: json

   {
     "is_first_block_on_page": false,
     "disposition": "created_new_table",
     "blocker": "not_first_block_on_page"
   }

.. code-block:: json

   {
     "is_first_block_on_page": true,
     "is_near_page_top": false,
     "disposition": "created_new_table",
     "blocker": "not_near_page_top"
   }

失败 JSON 保持 ``ok`` 为 ``false``，并报告 ``import`` 阶段：

.. code-block:: json

   {
     "command": "import-pdf",
     "ok": false,
     "stage": "import",
     "failure_kind": "table_candidates_detected",
     "message": "PDF text import detected table-like structure candidates; enable table-candidate import to import them as DOCX tables",
     "input": "input.pdf",
     "output": "imported.docx"
   }

``failure_kind`` 可以是 ``parse_failed``、``document_create_failed``、
``document_population_failed``、``extract_text_disabled``、
``extract_geometry_disabled``、``table_candidates_detected`` 或
``no_text_paragraphs``。未启用 ``--import-table-candidates-as-tables`` 且检测到表格候选时，
会报告 ``table_candidates_detected``，并且不会写出目标 DOCX。没有可提取文本段落的扫描件
或 image-only PDF 会报告 ``no_text_paragraphs``，同样不会写出目标 DOCX。

命令行解析错误仍使用 ``stage`` 为 ``parse`` 的通用 JSON 形状。当前消息包括
``missing value after --min-table-continuation-confidence``、
``invalid value after --min-table-continuation-confidence`` 和
``duplicate --min-table-continuation-confidence option``。Parse errors do not write the target DOCX。

PDF 导入支持范围和限制
----------------------

PDF import supported scope and limits 刻意保持保守。导入器是 text-first：从
extractable PDF text 和 character geometry 重建保守 ``Document``，再按需提升表格候选。

可靠范围：

* Paragraph import from extractable PDF text。
* Conservative table-candidate detection，覆盖简单对齐网格、key-value tables 和
  selected borderless aligned tables。
* Table candidates are rejected by default。
* Opt-in table promotion through ``--import-table-candidates-as-tables``。
* Cross-page table continuation：下一页顶部附近出现兼容表格候选时延续。
* Repeated-header detection：支持 exact text、whitespace/case/punctuation 归一化、
  conservative plural variants、small abbreviation whitelist，以及 token-set word order normalization。
* Conservative subtotal / total summary-row handling。
* Diagnostics through ``table_continuation_diagnostics``。

保守边界：

* Column-count mismatches、incompatible column anchors、semantic repeated-header mismatches、
  intervening paragraphs 或 low continuation confidence 会保持表格分离。
* Ordinary two-column prose、numbered lists、short-label prose 和 free-form forms 应保持段落。
* scanned PDFs、OCR、image-only tables、arbitrary nested table semantics、
  complex vector reconstruction、rotated or floating content recovery、
  exact visual reconstruction、arbitrary local column drift 都不属于稳定契约。
* Unsupported cases must fail or remain paragraphs，而不是静默生成误导性的 Word 结构。

发布验证
--------

开发环境和本地验证请看 ``BUILDING_PDF.md``。PDF 发布准入证据仍由发布治理脚本和有界
CTest 检查维护。
