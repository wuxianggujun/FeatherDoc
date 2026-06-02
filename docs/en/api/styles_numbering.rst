Styles And Numbering
====================

``featherdoc::Document`` exposes style and numbering APIs for inspection,
controlled mutation, refactoring, and default formatting. Use this page to find
the direct entry points for style and numbering work.

Typed Signature Guide
---------------------

.. FDOC_EN_STYLES_NUMBERING_TYPED_SIGNATURE_GUIDE

Style APIs address styles by ``style_id``. Numbering levels are zero-based.
Methods returning ``std::optional<T>`` return an empty value when the style,
numbering definition, or generated plan cannot be resolved.

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``bool set_paragraph_list(Paragraph paragraph, list_kind kind, std::uint32_t level = 0U)``
     - ``paragraph``: target paragraph. ``kind``: bullet/decimal list kind. ``level``: zero-based level.
     - ``true`` when list metadata was attached.
   * - ``numbering_catalog export_numbering_catalog()``
     - None.
     - Current numbering catalog for governance or reuse.
   * - ``numbering_catalog_import_summary import_numbering_catalog(const numbering_catalog &catalog)``
     - ``catalog``: definitions to import.
     - Import counts and conflicts.
   * - ``std::optional<std::uint32_t> ensure_numbering_definition(const numbering_definition &definition)``
     - ``definition``: requested numbering definition.
     - Definition id, or empty when it cannot be ensured.
   * - ``bool set_paragraph_numbering(Paragraph paragraph, std::uint32_t numbering_definition_id, std::uint32_t level = 0U)``
     - ``paragraph``: target paragraph. ``numbering_definition_id`` and ``level``: numbering target.
     - ``true`` when paragraph numbering was set.
   * - ``std::optional<style_summary> find_style(std::string_view style_id)``
     - ``style_id``: style identifier.
     - Summary, or empty when the style is missing.
   * - ``bool rename_style(std::string_view old_style_id, std::string_view new_style_id)``
     - ``old_style_id``: source id. ``new_style_id``: replacement id.
     - ``true`` when style id and references were renamed.
   * - ``bool merge_style(std::string_view source_style_id, std::string_view target_style_id)``
     - ``source_style_id``: style to remove. ``target_style_id``: replacement style.
     - ``true`` when references moved and the source style was removed.
   * - ``bool set_default_run_language(std::string_view language)``
     - ``language``: BCP-47 style language tag.
     - ``true`` when the default run language was written.
   * - ``table_style_region_audit_report audit_table_style_regions(std::optional<std::string_view> style_id = std::nullopt)``
     - ``style_id``: optional table style filter.
     - Audit report for all or one table style.

Numbering
---------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``set_paragraph_list(paragraph, kind, level = 0U)``
     - ``bool``
     - Attach a generated list to a paragraph.
   * - ``restart_paragraph_list(paragraph, kind, level = 0U)``
     - ``bool``
     - Start a new list instance for a paragraph.
   * - ``clear_paragraph_list(paragraph)``
     - ``bool``
     - Remove list metadata from a paragraph.
   * - ``list_numbering_definitions()``
     - ``std::vector<numbering_definition_summary>``
     - Enumerate numbering definitions.
   * - ``find_numbering_definition(definition_id)``
     - ``std::optional<numbering_definition_summary>``
     - Find one numbering definition.
   * - ``export_numbering_catalog()``
     - ``numbering_catalog``
     - Export definitions for governance or reuse.
   * - ``import_numbering_catalog(catalog)``
     - ``numbering_catalog_import_summary``
     - Import a numbering catalog.
   * - ``set_paragraph_numbering(paragraph, definition_id, level)``
     - ``bool``
     - Attach an existing numbering definition to a paragraph.

Styles
------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``list_styles()``
     - ``std::vector<style_summary>``
     - Enumerate styles.
   * - ``find_style(style_id)``
     - ``std::optional<style_summary>``
     - Find a style by id.
   * - ``resolve_style_properties(style_id)``
     - ``std::optional<resolved_style_properties_summary>``
     - Resolve inherited style properties.
   * - ``find_style_usage(style_id)``
     - ``std::optional<style_usage_summary>``
     - Inspect one style's usage.
   * - ``list_style_usage()``
     - ``std::optional<style_usage_report>``
     - Inspect usage across all styles.
   * - ``rename_style(old_style_id, new_style_id)``
     - ``bool``
     - Rename a style id.
   * - ``merge_style(source_style_id, target_style_id)``
     - ``bool``
     - Move usage from one style to another.
   * - ``suggest_style_merges()``
     - ``std::optional<style_refactor_plan>``
     - Generate conservative merge suggestions.
   * - ``plan_prune_unused_styles()`` / ``prune_unused_styles()``
     - ``std::optional<style_prune_*>``
     - Plan or apply unused style cleanup.

Default Formatting
------------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``default_run_font_family()``
     - ``std::optional<std::string>``
     - Read the default run font family.
   * - ``set_default_run_font_family(font_family)``
     - ``bool``
     - Set the default run font family.
   * - ``set_default_run_language(language)``
     - ``bool``
     - Set the default run language.
   * - ``set_default_paragraph_bidi(enabled = true)``
     - ``bool``
     - Set the default paragraph bidi flag.
   * - ``clear_default_run_font_family()``
     - ``bool``
     - Clear the default run font family.

Table Style Quality
-------------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``find_table_style_definition(style_id)``
     - ``std::optional<table_style_definition_summary>``
     - Inspect a table style definition.
   * - ``audit_table_style_regions(style_id)``
     - ``table_style_region_audit_report``
     - Audit table style region consistency.
   * - ``audit_table_style_inheritance(style_id)``
     - ``table_style_inheritance_audit_report``
     - Audit table style inheritance.
   * - ``check_table_style_look_consistency()``
     - ``table_style_look_consistency_report``
     - Inspect table look flags.
   * - ``repair_table_style_look_consistency()``
     - ``table_style_look_repair_report``
     - Apply conservative table look repairs.
   * - ``audit_table_style_quality()``
     - ``table_style_quality_audit_report``
     - Run table style quality checks.

Example
-------

.. code-block:: cpp

   featherdoc::Document doc{"styled.docx"};
   doc.open();

   auto styles = doc.list_styles();
   auto usage = doc.list_style_usage();
   auto suggestions = doc.suggest_style_merges();

   auto &paragraph = doc.paragraphs();
   doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet);
   doc.set_default_run_language("en-US");
