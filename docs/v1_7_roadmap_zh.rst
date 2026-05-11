v1.7.x 路线图（中文）
=====================

这份文档用于把 ``FeatherDoc`` 下一阶段的功能补齐方向，从“零散建议”收敛成
可以执行的版本路线图。

本文基于当前仓库状态整理，目标不是一次性把所有 Word 能力都做全，而是优先补齐
正式文档生成场景里最常用、最缺高层 API 的几块能力。

补充说明：这份路线图成文后，仓库已经继续落地了一批第一版 API 和 CLI
工作流，包括 ``list_styles()`` / ``find_style_usage()`` / ``ensure_*_style()``、
``ensure_numbering_definition()`` / ``set_paragraph_style_numbering()``、numbering
catalog JSON 导入 / 导出 / lint / check / diff、``validate_template()``、
``template_schema_patch``、``scan_template_schema(...)``、``get_section_page_setup()``、
页码字段、PAGEREF / STYLEREF / DOCPROPERTY / DATE / HYPERLINK / SEQ
简单字段、题注 / XE / INDEX 字段、复杂域单层嵌套、``w:updateFields`` 打开时刷新开关、content control 基础枚举 / 纯文本替换、table style definition 以及第一版
floating table positioning。阅读本文时，建议把重点放在这些方向的“深化和补齐”，
而不是把它们理解成仍然完全未开始的能力。


当前判断
--------

结合当前公开 API、README、样例、CLI 和测试，可以把项目现状概括为：

1. ``FeatherDoc`` 已经具备可靠的 ``.docx`` 打开、保存、段落 / Run 编辑、
   表格编辑、图片替换、书签模板、分节与页眉页脚编辑能力。
2. 当前仓库在工程化、MSVC 可用性、发布打包和 Word 截图级验证方面已经投入很多。
3. 下一阶段最值得补的，不是再堆外围流程，而是正式文档高频能力的 typed API：
   样式管理、编号体系、模板校验、页面设置、字段、图片高级布局和更完整 CLI。


路线原则
--------

后续实现建议遵循下面几条原则：

1. 优先补“正式 Word 文档会反复用到”的高频能力，而不是低频边角能力。
2. 优先做 inspection API，再做 mutation API，避免一上来就做难以诊断的写入接口。
3. 新能力必须覆盖 body、header、footer 三种 part，除非该能力天然只属于 body。
4. 任何会影响版式的能力，除了单测和 reopen-save 回归，还应补到现有 Word visual
   validation 流程里。
5. 避免一次性重做整个样式或编号系统，优先补最小可闭环子集。


当前明确不优先的事项
--------------------

至少在 ``v1.7.x`` 阶段，以下能力不建议抢在前面：

1. 带密码或加密 ``.docx`` 的支持。
2. 完整的 OMML 公式构造器。
3. 批注 / 修订 / 审阅痕迹的完整 authoring API。
4. 内容控件复杂表单保护、重复节与数据绑定同步。
5. 纯粹为了“看起来完整”而增加的大而全 CLI。

这些能力不是没价值，而是复杂度高、边界宽，更适合等前面的正式文档主路径补齐后
再单独立项。


建议的版本切分
--------------

建议把 ``v1.7.x`` 分成三个连续里程碑：

1. ``v1.7.0``: 样式目录与编号管理
2. ``v1.7.1``: 模板校验与页面设置 / 字段
3. ``v1.7.2``: 图片高级布局与 CLI 扩展

后面如果节奏允许，再把完整 OMML builder、批注修订 authoring、复杂表单保护 / 数据绑定同步
和更多复杂域 typed builder、主动字段刷新策略等更复杂字段扩展放到后续版本评估。


v1.7.0: 样式目录与编号管理
--------------------------

这一版建议先补“文档骨架能力”。

如果没有样式目录检查、自定义样式定义和更灵活的编号体系，很多正式文档模板虽然
能写出内容，但仍然不好维护，也不利于下游做稳定模板复用。

建议交付内容
^^^^^^^^^^^^

1. 样式目录 inspection API
2. 最小可用的 paragraph / character / table style definition API
3. 自定义编号定义 API，而不再只依赖托管 bullet / decimal 两套内建列表
4. 基于样式挂接编号的最小闭环

其中第 1、2 项已经落地，编号管理也已经从托管列表扩展到自定义编号定义、
样式挂接编号、style numbering 审计 / 修复和 numbering catalog JSON 工作流。
后续重点不再是“有没有编号 API”，而是 exemplar 提取、冲突审计和真实模板治理体验。

建议的最小 API 草案
^^^^^^^^^^^^^^^^^^^

下面这些名字只是建议方向；其中只读 inspection 部分已经按下面这组接口落地：

.. code-block:: cpp

    enum class style_kind : std::uint8_t {
        paragraph,
        character,
        table,
        numbering,
        unknown,
    };

    struct style_summary {
        std::string style_id;
        std::string name;
        std::optional<std::string> based_on;
        style_kind kind{style_kind::unknown};
        std::string type_name;
        bool is_default{false};
        bool is_custom{false};
        bool is_semi_hidden{false};
        bool is_unhide_when_used{false};
        bool is_quick_format{false};
    };

    std::vector<style_summary> Document::list_styles();
    std::optional<style_summary> Document::find_style(std::string_view style_id);

    struct paragraph_style_definition {
        std::string name;
        std::optional<std::string> based_on;
        std::optional<std::string> next_style;
        bool is_custom{true};
        bool is_semi_hidden{false};
        bool is_unhide_when_used{false};
        bool is_quick_format{false};
        std::optional<std::string> run_font_family;
        std::optional<std::string> run_east_asia_font_family;
        std::optional<std::string> run_language;
        std::optional<std::string> run_east_asia_language;
        std::optional<std::string> run_bidi_language;
        std::optional<bool> run_rtl;
        std::optional<bool> paragraph_bidi;
        std::optional<std::uint32_t> outline_level;
    };

    struct character_style_definition {
        std::string name;
        std::optional<std::string> based_on;
        bool is_custom{true};
        bool is_semi_hidden{false};
        bool is_unhide_when_used{false};
        bool is_quick_format{false};
        std::optional<std::string> run_font_family;
        std::optional<std::string> run_east_asia_font_family;
        std::optional<std::string> run_language;
        std::optional<std::string> run_east_asia_language;
        std::optional<std::string> run_bidi_language;
        std::optional<bool> run_rtl;
    };

    struct table_style_definition {
        std::string name;
        std::optional<std::string> based_on;
        bool is_custom{true};
        bool is_semi_hidden{false};
        bool is_unhide_when_used{false};
        bool is_quick_format{false};
    };

    bool Document::ensure_paragraph_style(std::string_view style_id,
                                          const paragraph_style_definition& definition);
    bool Document::ensure_character_style(std::string_view style_id,
                                          const character_style_definition& definition);
    bool Document::ensure_table_style(std::string_view style_id,
                                      const table_style_definition& definition);

    struct numbering_level_definition {
        featherdoc::list_kind kind{};
        std::uint32_t start{1};
        std::uint32_t level{0};
        std::string text_pattern;
    };

    struct numbering_definition {
        std::string name;
        std::vector<numbering_level_definition> levels;
    };

    std::optional<std::uint32_t> Document::ensure_numbering_definition(
        const numbering_definition& definition);
    bool Document::set_paragraph_numbering(Paragraph paragraph,
                                           std::uint32_t numbering_id,
                                           std::uint32_t level = 0U);

建议的实现落点
^^^^^^^^^^^^^^

1. ``src/document_styles.cpp``: 样式目录读取、样式定义最小写入
2. ``src/document_numbering.cpp``: abstract numbering / num instance 管理
3. ``include/featherdoc.hpp``: 新 struct 与 inspection API
4. ``samples/``: 增加 style / numbering 相关样例

建议的测试门槛
^^^^^^^^^^^^^^

1. 样式 inspection 必须能 round-trip 现有 ``word/styles.xml``。
2. 创建或更新样式时，不能破坏不相关 style 节点。
3. reopen-save-reopen 后，style id、based-on、语言和字体元数据仍可读。
4. 新编号定义必须覆盖多级列表，而不仅是一级 bullet。
5. 基于样式的编号至少要验证：样式挂接后，新段落套样式即可稳定继承编号外观。

退出条件
^^^^^^^^

满足下面几点后，才建议收 ``v1.7.0``：

1. 下游可以不手写 XML，直接列出样式目录并检查关键 style 是否存在。
2. 下游可以新增一组最小 paragraph / character / table style。
3. 下游可以创建一组自定义多级编号并稳定套用到段落。
4. 文档、样例、测试和 README 都已同步更新。


v1.7.1: 模板校验与页面设置 / 字段
---------------------------------

这一版建议补“正式模板可控性”。

bookmark 模板替换、schema 事前校验和页面级版式控制已经有第一版闭环。
对于合同、报告、标书、发票、制度文件，后续更值得继续打磨的是 schema
迁移、审批、真实模板 onboarding 和质量门禁体验。

建议交付内容
^^^^^^^^^^^^

1. 模板 schema validation API
2. section page setup API
3. 最小字段 API，优先覆盖页码和总页数，而不是一开始就做完整 field system

当前状态补记
^^^^^^^^^^^^

- ``validate_template(...)`` 已经覆盖 slot 声明、缺失、重复、意外书签、
  kind 不匹配和 occurrence 约束。
- 文档级 ``validate_template_schema(...)`` 也已经落地，并支持 body /
  header / footer / section-header / section-footer 多目标聚合校验。
- CLI 侧已经补上 ``validate-template-schema`` 和
  ``export-template-schema``，样例可参考
  ``samples/sample_template_schema_validation.cpp``；其中
  ``--section-targets`` 表示 direct section reference 视角，
  ``--resolved-section-targets`` 表示 linked-to-previous 解析后的实际生效视角。
- CLI schema 工具链已经包括 ``normalize-template-schema``、``diff-template-schema``
  和 ``check-template-schema``，可以稳定 schema 文件顺序、比较版本差异，并作为
  CI gate 输出非零退出码。
- 仓库级 ``baselines/template-schema/manifest.json`` 现在也已经落地，并支持
  静态模板与 generated fixture 混合登记、``prepare_sample_target`` 自动准备、
  ``register_template_schema_manifest_entry.ps1`` 一步登记、以及
  ``describe_template_schema_manifest.ps1`` 的维护视图；同一份 manifest gate
  也已经接入 release preflight 和生成的 release metadata bundle。

建议的最小 API 草案
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    enum class template_slot_kind {
        text,
        table_rows,
        table,
        image,
        floating_image,
        block,
    };

    struct template_slot_requirement {
        std::string bookmark_name;
        template_slot_kind kind{};
        bool required{true};
    };

    struct template_validation_result {
        std::vector<std::string> missing_required;
        std::vector<std::string> duplicate_bookmarks;
        std::vector<std::string> malformed_placeholders;
    };

    template_validation_result Document::validate_template(
        std::span<const template_slot_requirement> requirements) const;

    enum class page_orientation {
        portrait,
        landscape,
    };

    struct page_margins {
        std::uint32_t top_twips{};
        std::uint32_t bottom_twips{};
        std::uint32_t left_twips{};
        std::uint32_t right_twips{};
        std::uint32_t header_twips{};
        std::uint32_t footer_twips{};
    };

    struct section_page_setup {
        page_orientation orientation{page_orientation::portrait};
        std::uint32_t width_twips{};
        std::uint32_t height_twips{};
        page_margins margins{};
        std::optional<std::uint32_t> page_number_start;
    };

    std::optional<section_page_setup> Document::get_section_page_setup(
        std::size_t section_index) const;
    bool Document::set_section_page_setup(std::size_t section_index,
                                          const section_page_setup& setup);

    bool TemplatePart::append_page_number_field();
    bool TemplatePart::append_total_pages_field();
    bool TemplatePart::append_page_reference_field(...);
    bool TemplatePart::append_style_reference_field(...);
    bool TemplatePart::append_document_property_field(...);
    bool TemplatePart::append_date_field(...);
    bool TemplatePart::append_hyperlink_field(...);

建议的实现落点
^^^^^^^^^^^^^^

1. ``src/document_template.cpp``: 书签占位符预检与 schema 校验
2. ``src/document.cpp``: ``w:sectPr`` 下的纸张方向、尺寸、页边距、起始页码
3. ``src/paragraph.cpp`` / ``src/run.cpp``: 字段插入的最小公共助手
4. ``samples/``: 增加合同 / 报告类 page setup 与页码样例

建议的测试门槛
^^^^^^^^^^^^^^

1. 模板校验必须能给出“缺失、重复、结构不合法”三类明确结果。
2. 页面设置读写要覆盖 body-level final section 和 paragraph-level section break。
3. 横向页面和自定义页边距必须在 reopen 后保持稳定。
4. 页码字段至少要覆盖 header 和 footer 两种常见 part。
5. 字段插入属于排版变化，建议补到 Word visual smoke 或新增独立 visual sample。

退出条件
^^^^^^^^

满足下面几点后，才建议收 ``v1.7.1``：

1. 下游可以在写入前先验证模板是否满足约束。
2. 下游可以按 section 控制纸张方向、尺寸、页边距和起始页码。
3. 下游可以在页眉 / 页脚中稳定生成页码和总页数字段。


v1.7.2: 图片高级布局与 CLI 扩展
------------------------------

这一版建议补“可交付文档的细节能力”和“批处理入口”。

当前 floating image 已有基本锚点和偏移，wrap / crop / z-order 这些正式文档
常见布局参数也已经补上。这个阶段更适合继续把这些能力沉淀成稳定的 CLI 入口和
可视化回归，而不是停留在仅有底层 API。

建议交付内容
^^^^^^^^^^^^

1. 巩固 ``floating_image_options`` 的可交付布局语义
2. 让 wrap / crop / distance / z-order 在 CLI 与 visual regression 中可稳定复用
3. 把前两版补出的 inspection 与 mutation 能力映射到 CLI

建议的最小 API 草案
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

    enum class floating_image_wrap_mode {
        none,
        square,
        top_bottom,
    };

    struct floating_image_crop {
        std::uint32_t left_per_mille{0};
        std::uint32_t top_per_mille{0};
        std::uint32_t right_per_mille{0};
        std::uint32_t bottom_per_mille{0};
    };

    struct floating_image_options {
        featherdoc::floating_image_horizontal_reference horizontal_reference;
        std::int32_t horizontal_offset_px;
        featherdoc::floating_image_vertical_reference vertical_reference;
        std::int32_t vertical_offset_px;
        bool behind_text;
        bool allow_overlap;
        floating_image_wrap_mode wrap_mode;
        std::uint32_t wrap_distance_left_px;
        std::uint32_t wrap_distance_right_px;
        std::uint32_t wrap_distance_top_px;
        std::uint32_t wrap_distance_bottom_px;
        std::optional<floating_image_crop> crop;
    };

建议新增的 CLI 方向
^^^^^^^^^^^^^^^^^^^

1. ``inspect-styles``
2. ``inspect-page-setup``
3. ``set-section-page-setup``
4. ``inspect-bookmarks``
5. ``validate-template``

CLI 不需要一步到位覆盖所有 mutation。

优先做 inspection，再补最稳定、最不易误用的写操作。

建议的测试门槛
^^^^^^^^^^^^^^

1. 新增图片布局字段后，必须验证 reopen-save 不丢失关系和尺寸信息。
2. 新 CLI 至少要有纯文本输出和 ``--json`` 两条回归路径。
3. 会明显影响版式的图片包围或裁剪能力，必须进入 Word visual validation。

退出条件
^^^^^^^^

满足下面几点后，才建议收 ``v1.7.2``：

1. 浮动图片不再只有“能放上去”，而是能做最基本的正式文档布局控制。
2. styles / template / page setup 的 inspection 能力都可从 CLI 调用。
3. 文档用户和脚本用户都能通过统一入口复用这些能力。


建议的实现顺序
--------------

如果按风险和收益排序，建议按下面顺序推进，而不是按模块随意穿插：

1. 先做 ``v1.7.0`` 的 style inspection。
2. 然后补最小 style definition mutation。
3. 再做 ``v1.7.0`` 的 custom numbering definition。
4. 接着做模板 schema validation。
5. 再补 section page setup。
6. 然后做 header/footer 页码字段。
7. 最后再扩展 floating image layout 和 CLI。

这个顺序的好处是：

1. 先把 inspection 补上，后面的 mutation 更容易诊断。
2. 先补文本骨架能力，再补布局细节，返工更少。
3. 最后再做 CLI，可以直接复用前面已经稳定的库 API。


建议保留到 v1.8.x 之后再评估的能力
----------------------------------

下面这些能力值得做，但不建议提前打乱 ``v1.7.x``：

1. 完整 OMML 公式 builder
2. 更多复杂域 typed builder 和主动字段刷新策略（复杂域单层嵌套与打开时刷新开关已落地）
3. 超链接与交叉引用的完整 typed API
4. 批注 / 修订 / track changes authoring
5. 内容控件复杂表单保护与数据绑定同步
6. 加密 ``.docx`` 支持


一句话总结
----------

``v1.7.x`` 的主线不应继续偏向外围发布自动化，而应把 ``FeatherDoc`` 往
“正式 Word 文档高层 API” 方向再推进一大步：先补样式和编号，再补模板校验与
页面设置，最后补图片高级布局和 CLI。
