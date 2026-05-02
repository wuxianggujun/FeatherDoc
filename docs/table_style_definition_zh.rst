自定义表格样式定义（中文）
==========================

这份文档记录 ``FeatherDoc`` 的自定义表格样式定义编辑入口。它面向正式
文档里的报告表、发票明细、制度附表和标书评分表，目标是让表格版式尽量由
``word/styles.xml`` 中的 table style 统一管理，而不是每张表逐格写局部属性。


当前能力边界
------------

``Document::ensure_table_style(...)`` 现在除了能创建或更新 table style 的
名称、``basedOn`` 和显示元数据外，还可以写入：

- ``whole_table``：整张表的默认样式区域
- ``first_row`` / ``last_row``：首行和末行条件区域
- ``first_column`` / ``last_column``：首列和末列条件区域
- ``banded_rows`` / ``banded_columns``：第一组横向 / 纵向条带区域
- ``second_banded_rows`` / ``second_banded_columns``：第二组横向 / 纵向条带区域

每个区域当前支持十三类高频属性：

- ``fill_color``：单元格底色，写入 ``w:tcPr/w:shd``
- ``text_color``：区域默认文字颜色，写入 ``w:rPr/w:color``
- ``bold``：区域默认加粗状态，写入 ``w:rPr/w:b``
- ``italic``：区域默认斜体状态，写入 ``w:rPr/w:i``
- ``font_size_points``：区域默认字号，按 Word 半磅值写入 ``w:rPr/w:sz``
  和 ``w:rPr/w:szCs``
- ``font_family``：区域默认西文字体，写入 ``w:rPr/w:rFonts`` 的
  ``w:ascii``、``w:hAnsi`` 和 ``w:cs``
- ``east_asia_font_family``：区域默认东亚字体，写入
  ``w:rPr/w:rFonts`` 的 ``w:eastAsia``
- ``cell_vertical_alignment``：区域单元格垂直对齐，写入 ``w:tcPr/w:vAlign``
- ``cell_text_direction``：区域单元格文本方向，写入
  ``w:tcPr/w:textDirection``
- ``paragraph_alignment``：区域段落水平对齐，写入 ``w:pPr/w:jc``
- ``paragraph_spacing``：区域段前、段后和行距，写入 ``w:pPr/w:spacing``
- ``cell_margins``：单元格边距，整表区域写入 ``w:tblPr/w:tblCellMar``，
  条件区域写入 ``w:tcPr/w:tcMar``
- ``borders``：边框，整表区域写入 ``w:tblPr/w:tblBorders``，条件区域写入
  ``w:tcPr/w:tcBorders``

没有显式提供的属性不会被清除。这样可以安全更新既有模板中的部分 table
style 元数据，同时保留未被 FeatherDoc 建模的 Word XML。


最小示例
--------

下面示例创建一个 ``InvoiceGrid`` 表格样式：整表使用浅蓝底色、固定边距和
边框；首行使用深蓝底色和双线底边框。

.. code-block:: cpp

    auto whole_margins = featherdoc::table_style_margins_definition{};
    whole_margins.top_twips = 80U;
    whole_margins.left_twips = 120U;
    whole_margins.bottom_twips = 80U;
    whole_margins.right_twips = 120U;

    auto whole_borders = featherdoc::table_style_borders_definition{};
    whole_borders.top = featherdoc::border_definition{
        featherdoc::border_style::single, 12U, "4472C4", 0U};
    whole_borders.inside_horizontal = featherdoc::border_definition{
        featherdoc::border_style::dotted, 4U, "A5A5A5", 0U};

    auto whole_table = featherdoc::table_style_region_definition{};
    whole_table.fill_color = std::string{"DDEEFF"};
    whole_table.text_color = std::string{"1F1F1F"};
    whole_table.bold = false;
    whole_table.italic = false;
    whole_table.font_size_points = 11U;
    whole_table.font_family = std::string{"Aptos"};
    whole_table.east_asia_font_family = std::string{"Microsoft YaHei"};
    whole_table.cell_vertical_alignment = featherdoc::cell_vertical_alignment::center;
    whole_table.cell_text_direction =
        featherdoc::cell_text_direction::left_to_right_top_to_bottom;
    whole_table.paragraph_alignment = featherdoc::paragraph_alignment::center;
    auto whole_paragraph_spacing = featherdoc::table_style_paragraph_spacing_definition{};
    whole_paragraph_spacing.before_twips = 120U;
    whole_paragraph_spacing.after_twips = 80U;
    whole_paragraph_spacing.line_twips = 360U;
    whole_paragraph_spacing.line_rule = featherdoc::paragraph_line_spacing_rule::exact;
    whole_table.paragraph_spacing = whole_paragraph_spacing;
    whole_table.cell_margins = whole_margins;
    whole_table.borders = whole_borders;

    auto first_row_borders = featherdoc::table_style_borders_definition{};
    first_row_borders.bottom = featherdoc::border_definition{
        featherdoc::border_style::double_line, 8U, "1F4E79", 0U};

    auto first_row = featherdoc::table_style_region_definition{};
    first_row.fill_color = std::string{"1F4E79"};
    first_row.text_color = std::string{"FFFFFF"};
    first_row.bold = true;
    first_row.italic = true;
    first_row.font_size_points = 14U;
    first_row.font_family = std::string{"Aptos Display"};
    first_row.east_asia_font_family = std::string{"SimHei"};
    first_row.cell_vertical_alignment = featherdoc::cell_vertical_alignment::bottom;
    first_row.cell_text_direction =
        featherdoc::cell_text_direction::top_to_bottom_right_to_left;
    first_row.paragraph_alignment = featherdoc::paragraph_alignment::right;
    auto first_row_paragraph_spacing =
        featherdoc::table_style_paragraph_spacing_definition{};
    first_row_paragraph_spacing.after_twips = 120U;
    first_row_paragraph_spacing.line_twips = 240U;
    first_row_paragraph_spacing.line_rule =
        featherdoc::paragraph_line_spacing_rule::at_least;
    first_row.paragraph_spacing = first_row_paragraph_spacing;
    first_row.borders = first_row_borders;

    auto style = featherdoc::table_style_definition{};
    style.name = "Invoice Grid";
    style.based_on = std::string{"TableGrid"};
    style.is_quick_format = true;
    style.whole_table = whole_table;
    style.first_row = first_row;

    if (!doc.ensure_table_style("InvoiceGrid", style)) {
        throw std::runtime_error(doc.last_error().detail);
    }

    auto table = doc.append_table(3, 4);
    table.set_style_id("InvoiceGrid");


CLI 写入示例
------------

``ensure-table-style`` 也可以直接写入已建模的 table style 区域属性，适合在
模板迁移脚本里批量生成样式：

.. code-block:: sh

    featherdoc_cli ensure-table-style input.docx InvoiceGrid \
      --name "Invoice Grid" --based-on TableGrid \
      --style-fill whole_table:DDEEFF \
      --style-text-color whole_table:1F1F1F \
      --style-bold whole_table:false \
      --style-italic whole_table:false \
      --style-font-size whole_table:11 \
      --style-font-family whole_table:Aptos \
      --style-east-asia-font-family "whole_table:Microsoft YaHei" \
      --style-cell-vertical-alignment whole_table:center \
      --style-cell-text-direction whole_table:left_to_right_top_to_bottom \
      --style-paragraph-alignment whole_table:center \
      --style-paragraph-spacing-before whole_table:120 \
      --style-paragraph-spacing-after whole_table:80 \
      --style-paragraph-line-spacing whole_table:360:exact \
      --style-margin whole_table:left:120 \
      --style-margin whole_table:right:160 \
      --style-border whole_table:top:single:12:4472C4 \
      --style-fill first_row:1F4E79 \
      --style-text-color first_row:FFFFFF \
      --style-bold first_row:true \
      --style-italic first_row:true \
      --style-font-size first_row:14 \
      --style-font-family "first_row:Aptos Display" \
      --style-east-asia-font-family first_row:SimHei \
      --style-cell-vertical-alignment first_row:bottom \
      --style-cell-text-direction first_row:top_to_bottom_right_to_left \
      --style-paragraph-alignment first_row:right \
      --style-paragraph-spacing-after first_row:120 \
      --style-paragraph-line-spacing first_row:240:at_least \
      --style-border first_row:bottom:double_line:8:1F4E79 \
      --style-fill second_banded_rows:F2F2F2 \
      --style-text-color second_banded_rows:666666 \
      --style-fill second_banded_columns:E2F0D9 \
      --output styled.docx --json

参数规则保持稳定、可脚本化：

- ``--style-fill <region>:<color>``：设置区域底色。
- ``--style-text-color <region>:<color>``：设置区域默认文字颜色。
- ``--style-bold <region>:true|false``：设置区域默认加粗状态。
- ``--style-italic <region>:true|false``：设置区域默认斜体状态。
- ``--style-font-size <region>:<points>``：设置区域默认字号，单位为磅。
- ``--style-font-family <region>:<name>``：设置区域默认西文字体。
- ``--style-east-asia-font-family <region>:<name>``：设置区域默认东亚字体。
- ``--style-cell-vertical-alignment <region>:<top|center|bottom|both>``：
  设置区域单元格垂直对齐。
- ``--style-cell-text-direction <region>:<direction>``：设置区域单元格
  文本方向。``direction`` 支持
  ``left_to_right_top_to_bottom``、``top_to_bottom_right_to_left``、
  ``bottom_to_top_left_to_right``、``left_to_right_top_to_bottom_rotated``、
  ``top_to_bottom_right_to_left_rotated`` 和
  ``top_to_bottom_left_to_right_rotated``。
- ``--style-paragraph-alignment <region>:<left|center|right|justified|distribute>``：
  设置区域段落水平对齐。
- ``--style-paragraph-spacing-before <region>:<twips>``：设置区域段前间距。
- ``--style-paragraph-spacing-after <region>:<twips>``：设置区域段后间距。
- ``--style-paragraph-line-spacing <region>:<twips>:<auto|at_least|exact>``：
  设置区域行距和行距规则。
- ``--style-margin <region>:<edge>:<twips>``：设置区域单元格边距。
- ``--style-border <region>:<edge>:<style>:<size>:<color>[:space]``：设置
  区域边框，可选 ``space`` 默认为 ``0``。

``region`` 支持 ``whole_table``、``first_row``、``last_row``、
``first_column``、``last_column``、``banded_rows``、``banded_columns``、
``second_banded_rows`` 和 ``second_banded_columns``，也接受对应的短横线
写法，例如 ``whole-table``；第二条带还兼容 ``banded_rows_2`` 和
``banded_columns_2`` 这类脚本友好别名。


反查和 CLI JSON
---------------

如果需要从既有模板中反查已建模的 table style 属性，可以使用
``Document::find_table_style_definition(...)``。它会返回基础 style 元数据，
以及 ``whole_table``、``first_row``、``second_banded_rows`` 等区域中
已经支持的底色、文字颜色、加粗状态、斜体状态、字号、字体族、
单元格垂直对齐、文本方向、段落对齐、段落间距、行距、cell margin
和边框信息。

命令行可以直接输出 JSON，便于在自动化里审阅或生成后续迁移草稿：

.. code-block:: sh

    featherdoc_cli inspect-table-style input.docx InvoiceGrid --json

当前 inspection 只反查 FeatherDoc 已建模的属性；未建模的 Word XML 会继续保留
在文档中，但不会进入 typed summary。


style look 一致性检查
----------------------

条件区域是否真正生效，还取决于表格实例上的 ``w:tblLook``。例如 table
style 里定义了 ``first_row`` 或 ``second_banded_rows``，但表格实例关闭了
``first_row`` 或 ``banded_rows`` 标志，Word 就不会按预期路由这些条件样式。

可以用下面命令检查文档中表格实例和 table style 条件区域是否一致：

.. code-block:: sh

    featherdoc_cli check-table-style-look input.docx --json
    featherdoc_cli check-table-style-look input.docx --fail-on-issue --json
    featherdoc_cli repair-table-style-look input.docx --plan-only --json
    featherdoc_cli repair-table-style-look input.docx --apply --output repaired.docx --json

JSON 输出包含 ``table_count``、``issue_count`` 和逐条 ``issues``。每条 issue
会给出 ``table_index``、``style_id``、受影响 ``region``、需要打开的
``required_style_look_flag``、实际值和修复建议。``--fail-on-issue`` 适合
放进模板 CI gate：存在不一致时返回非零退出码。

``repair-table-style-look`` 会先复用同一套诊断模型：``--plan-only`` 只输出
可修复项，``--apply`` 会打开缺失或关闭的 ``tblLook`` 标志并写到
``--output`` 指定的新文档。它只自动处理 ``style_look_missing`` 和
``style_look_disabled``；缺失 style 或非 table style 仍会保留为人工处理 issue。


区域覆盖率审计
--------------

样式迁移时有时会留下空的 ``w:tblPr`` 或 ``w:tblStylePr`` 节点。它们表示
“声明了 whole-table / first-row / banded-row 等区域”，但没有任何 FeatherDoc
已建模的 typed 视觉属性，容易造成“以为样式已配置，实际没有效果”的误判。

可以用下面命令做只读审计：

.. code-block:: sh

    featherdoc_cli audit-table-style-regions input.docx --json
    featherdoc_cli audit-table-style-regions input.docx --style InvoiceGrid --fail-on-issue --json

JSON 输出包含 ``table_style_count``、``region_count``、``issue_count`` 和
``issues``。当前 issue 类型为 ``empty_region``，表示该 table style 区域节点
存在，但 typed 属性数量为 0。``--style`` 可限定单个 table style，
``--fail-on-issue`` 适合放进模板质量 gate。


继承链审计
----------

表格样式的 ``basedOn`` 继承链如果指向缺失 style、非 table style，或形成
循环，后续属性解析和模板迁移都会变得不可靠。可以用下面命令做只读审计：

.. code-block:: sh

    featherdoc_cli audit-table-style-inheritance input.docx --json
    featherdoc_cli audit-table-style-inheritance input.docx --style InvoiceGrid --fail-on-issue --json

JSON 输出包含 ``table_style_count``、``issue_count``、``fail_on_issue`` 和
逐条 ``issues``。当前 issue 类型包括：

- ``missing_based_on``：``basedOn`` 指向的 style id 不存在。
- ``based_on_not_table``：table style 继承到了 paragraph / character 等非表格样式。
- ``inheritance_cycle``：table style 继承链中出现循环。

每条 issue 会给出 ``style_id``、``based_on_style_id``、``based_on_style_kind``、
``inheritance_chain`` 和修复建议。``--style`` 可限定单个 table style，
``--fail-on-issue`` 适合和区域覆盖率、style look 检查一起作为模板质量 gate。


质量总览 gate
-------------

如果模板 CI 只希望跑一个入口，可以使用 ``audit-table-style-quality`` 聚合
区域覆盖率、继承链和 style look 三类只读检查：

.. code-block:: sh

    featherdoc_cli audit-table-style-quality input.docx --json
    featherdoc_cli audit-table-style-quality input.docx --fail-on-issue --json

JSON 顶层会输出总 ``issue_count``，以及 ``region_issue_count``、
``inheritance_issue_count``、``style_look_issue_count`` 三个分项计数；同时保留
``region_audit``、``inheritance_audit`` 和 ``style_look`` 的完整明细，方便 CI
失败后直接定位是样式定义区域、``basedOn`` 继承链，还是表格实例 ``tblLook``
路由问题。

如果希望把失败项进一步拆成自动修复和人工处理清单，可以先生成只读修复计划：

.. code-block:: sh

    featherdoc_cli plan-table-style-quality-fixes input.docx --json
    featherdoc_cli plan-table-style-quality-fixes input.docx --fail-on-issue --json
    featherdoc_cli apply-table-style-quality-fixes input.docx --look-only --output repaired.docx --json

计划输出包含 ``plan_item_count``、``automatic_fix_count`` 和 ``manual_fix_count``。
其中 ``style_look_missing`` / ``style_look_disabled`` 会标记为可自动修复，并给出
``repair-table-style-look --plan-only`` 作为下一步命令；空区域、缺失 ``basedOn``、
跨类型继承和循环继承会保留为人工处理项，避免误删或误改模板样式定义。

``apply-table-style-quality-fixes`` 当前必须显式传入 ``--look-only``，只会应用
安全的 ``tblLook`` 标志修复并写入 ``--output``；样式定义层面的人工项会继续
留在 ``after_plan`` 中。涉及视觉输出时，建议对修复前后文档跑 Word 渲染
可视化验证，确认首行 / 条带等条件样式确实按预期生效。


专用视觉回归脚本
----------------

为了避免每次手工生成 before / after 文档、调用 Word 渲染和整理证据，可以使用
专用脚本一键跑完 table style quality look-only 修复验证：

.. code-block:: powershell

    powershell -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1 -BuildDir build-codex-clang-compat -OutputDir output/table-style-quality-visual-regression -SkipBuild

脚本会生成一个最小复现 DOCX：table style 定义了 ``first_row`` 深蓝表头，
但表格实例关闭了 ``tblLook firstRow``。随后脚本会依次执行
``audit-table-style-quality``、``plan-table-style-quality-fixes``、
``apply-table-style-quality-fixes --look-only`` 和修复后的审计，并在启用视觉验证时
调用 ``run_word_visual_smoke.ps1`` 渲染 before / after 页面。

输出目录中会包含：

- ``quality-look-before.docx`` / ``quality-look-after.docx``：修复前后文档。
- ``before-audit-table-style-quality.json`` / ``after-audit-table-style-quality.json``：
  修复前后质量审计结果。
- ``apply-table-style-quality-fixes.json``：自动修复结果，包含 ``changed_table_count``。
- ``before_after_contact_sheet.png``：Word 渲染前后对比图。
- ``pixel-summary.json`` / ``pixel-summary.txt``：深蓝表头像素差异摘要。
- ``summary.json``、``review_manifest.json``、``review_checklist.md`` 和
  ``final_review.md``：适合归档或发布前人工复核的证据包。

如果只想在 CI 中验证脚本结构和 CLI 修复链路，不启动 Word，可以加
``-SkipVisual``。这种模式仍会生成 before / after DOCX 和所有 JSON 审计结果，
但不会生成渲染图片和像素摘要。

这条视觉回归也可以显式纳入 Word visual release gate。为了避免默认发布检查
变慢，它是 opt-in bundle，需要额外传 ``-IncludeTableStyleQuality``：

.. code-block:: powershell

    powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 -SkipBuild -IncludeTableStyleQuality
    powershell -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -SkipConfigure -SkipBuild -IncludeTableStyleQuality

纳入后，gate summary 的 ``curated_visual_regressions`` 会出现
``table-style-quality``，review task key 为
``table-style-quality-visual-regression-bundle``。


使用建议
--------

1. 优先把正式文档里的通用表格视觉规则沉淀为 table style。
2. 表格实例上只保留必要的局部 override，例如列宽、fixed layout 和特殊单元格。
3. 如果依赖首行、末行或条带区域，记得同时设置表格实例的 ``style_look``，
   并用 ``check-table-style-look`` 做自动化检查；迁移旧模板时可先跑
   ``repair-table-style-look --plan-only``，确认后再 ``--apply``。
4. 迁移旧模板后先跑 ``audit-table-style-regions``，清理空区域，避免保留
   “声明了区域但没有 typed 属性”的无效样式片段。
5. 如果 table style 大量复用 ``basedOn``，再跑
   ``audit-table-style-inheritance``，先清理缺失、跨类型或循环继承。
6. 模板 CI 推荐直接跑 ``audit-table-style-quality --fail-on-issue --json``，
   用一个入口覆盖区域、继承和 ``tblLook`` 路由三类风险。
7. CI 失败后跑 ``plan-table-style-quality-fixes --json``，先区分可自动修复的
   ``tblLook`` 项和需要人工确认的样式定义项。
8. 确认可自动项后再跑 ``apply-table-style-quality-fixes --look-only``，并对
   修复前后文档做 Word visual validation。
9. 影响视觉输出的样式变更，建议补充 Word visual validation 或至少做
   reopen-save 回归。


后续扩展方向
------------

当前版本先覆盖边框、底色、文字颜色、加粗、斜体、字号、字体族、单元格垂直对齐、文本方向、段落对齐、段落间距、行距、cell margin 和第二条带区域。后续可以继续补：

- 更完整的 table style property inspection，例如尚未建模的 Word 专有属性
- 将真实业务模板中的表格样式提取为可复用 style definition 草稿

表格实例浮动定位
----------------

表格样式定义本身不负责表格实例在页面上的浮动位置；这类属性存放在
``w:tblPr/w:tblpPr``。当前已经提供第一版 typed API：
``Table::set_position(...)``、``position()`` 和 ``clear_position()``，可读写
水平参照（margin / page / column）、垂直参照（margin / page / paragraph）
以及 ``tblpX`` / ``tblpY`` twips 偏移。

CLI 侧也已打通同一能力，便于模板迁移脚本直接批量处理：

.. code-block:: bash

   featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --json
   featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --fail-on-change --json
   featherdoc_cli plan-table-position-presets input.docx --preset paragraph-callout --output positioned.docx --output-plan table-position-plan.json --json
   featherdoc_cli apply-table-position-plan table-position-plan.json --dry-run --json
   featherdoc_cli apply-table-position-plan table-position-plan.json --json
   featherdoc_cli set-table-position input.docx 0 --preset page-corner --horizontal-offset 360 --bottom-from-text 288 --output table-position-preset.docx --json
   featherdoc_cli set-table-position input.docx all --preset paragraph-callout --output table-position-all.docx --json
   featherdoc_cli set-table-position input.docx 0 --table 1 --preset margin-anchor --output table-position-list.docx --json
   featherdoc_cli set-table-position input.docx 0 --horizontal-reference page --horizontal-offset 720 --vertical-reference paragraph --vertical-offset -120 --left-from-text 144 --right-from-text 288 --top-from-text 72 --bottom-from-text 216 --overlap never --output table-position.docx --json
   featherdoc_cli clear-table-position table-position-all.docx all --output table-position-all-cleared.docx --json
   featherdoc_cli clear-table-position table-position-list.docx 0 --table 1 --output table-position-list-cleared.docx --json
   featherdoc_cli clear-table-position table-position.docx 0 --output table-position-cleared.docx --json

``plan-table-position-presets`` 是只读计划命令，会根据目标 preset 扫描正文表格，输出待套用 preset 的未定位表格、已匹配表格数量，以及需要人工复核的既有定位表格；计划结果同时包含 ``input_path``、``automatic_table_indices``、``already_matching_table_indices``、``review_table_indices``、``table_fingerprints``、占位模板命令 ``recommended_batch_command`` 和可直接复制的 ``resolved_recommended_batch_command``，便于先按索引审阅再执行批量命令；当存在自动可执行项时，``resolved_output_path`` 会默认派生为 ``<原文件名>-table-position-<preset>.docx``，也可通过 ``--output <docx>`` 显式覆盖，并自动追加到 ``resolved_recommended_*`` 的 ``--output`` 参数，避免复制命令后原地覆盖源 DOCX；加上 ``--output-plan`` 可把同一份 JSON 计划落盘留档，加上 ``--fail-on-change`` 后可用于 CI 门禁。审阅通过后可用 ``apply-table-position-plan <plan.json>`` 回放执行计划：该命令只读取 ``input_path``、``preset``、``table_count``、``automatic_table_indices``、``resolved_output_path`` 和 ``table_fingerprints`` 等结构化字段，不执行 JSON 中的命令字符串，并会在写入前校验当前表格数量，以及表格样式、宽度、行列、列宽和文本摘要是否仍与计划一致；计划指纹不匹配时会拒绝写入，并在错误详情中列出变化字段，避免把过期计划套到已变化文档。加上 ``--dry-run`` 时只执行计划解析、表格数量和指纹校验，并输出 ``table_count``、``fingerprint_checked_count``、``would_apply_count`` / ``table_indices``，不会保存 DOCX，适合在 CI 或人工复核阶段提前确认计划仍可安全回放；计划缺少 ``table_fingerprints`` 时会直接被拒绝，避免 CI 意外使用无法做指纹校验的计划。正式回放成功时也会在 JSON 中返回同样的校验统计，便于流水线记录本次实际校验覆盖。

``--preset`` 目前提供三类可复用默认值：

- ``paragraph-callout``：相对当前文本列 / 段落锚定，适合正文旁的小型说明表。
- ``page-corner``：相对页面左上区域锚定，适合页内固定角标或摘要表。
- ``margin-anchor``：相对页边距 / 段落锚定，适合随段落移动的边距浮动表。

显式传入的 ``--horizontal-offset``、``--bottom-from-text``、``--overlap`` 等细粒度参数会覆盖预设默认值，便于先套模板再微调。

批量场景可将第二个位置参数写为 ``all``，一次处理所有正文表格；也可保留第一个表格索引，并重复传入 ``--table <index>`` 追加目标表格。``set-table-position`` 与 ``clear-table-position`` 使用同一套目标语义。单表 JSON 输出继续返回 ``table_index`` / ``position``；多表输出返回 ``table_indices`` / ``positions`` 数组，清除时 ``positions`` 中对应项为 ``null``。

``inspect-tables --table <index> --json`` 会同步返回 ``position`` 字段；未设置时
为 ``null``，设置后包含水平 / 垂直参照、signed twips 偏移、文本环绕距离
（``left/right/top/bottom_from_text_twips``）以及 ``overlap`` 策略。

这能覆盖模板中常见的“表格相对页面、边距或段落固定摆放”的迁移场景。
后续可以继续把真实业务模板中沉淀出的定位组合加入 preset，并在真实 Word
渲染下补充视觉回归。
