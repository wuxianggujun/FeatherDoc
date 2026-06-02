Styles And Numbering
====================

``Document`` exposes style and numbering APIs for inspection, controlled
mutation, and refactoring.

Numbering
---------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``set_paragraph_list(Paragraph paragraph, list_kind kind, std::uint32_t level = 0U)``
     - ``bool``
     - Attach a generated list to a paragraph.
   * - ``restart_paragraph_list(Paragraph paragraph, list_kind kind, std::uint32_t level = 0U)``
     - ``bool``
     - Start a new list instance.
   * - ``clear_paragraph_list(Paragraph paragraph)``
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
     - Import definitions.
   * - ``set_paragraph_numbering(paragraph, definition_id, level)``
     - ``bool``
     - Attach an existing numbering definition to a paragraph.

Styles
------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``list_styles()``
     - ``std::vector<style_summary>``
     - Enumerate styles.
   * - ``find_style(std::string_view style_id)``
     - ``std::optional<style_summary>``
     - Find a style by id.
   * - ``resolve_style_properties(std::string_view style_id)``
     - ``std::optional<resolved_style_properties_summary>``
     - Resolve inherited style properties.
   * - ``find_style_usage(style_id)``
     - ``std::optional<style_usage_summary>``
     - Inspect one style's usage.
   * - ``list_style_usage()``
     - ``std::optional<style_usage_report>``
     - Inspect all style usage.
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
   :widths: 34 20 46

   * - Method
     - Returns
     - Purpose
   * - ``default_run_font_family()``
     - ``std::optional<std::string>``
     - Read default run font family.
   * - ``set_default_run_font_family(font_family)``
     - ``bool``
     - Set default run font family.
   * - ``set_default_run_language(language)``
     - ``bool``
     - Set default language.
   * - ``set_default_paragraph_bidi(bool enabled = true)``
     - ``bool``
     - Set default paragraph bidi.
   * - ``clear_default_run_font_family()``
     - ``bool``
     - Clear default run font family.

Table Style Quality
-------------------

.. list-table::
   :header-rows: 1
   :widths: 34 20 46

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
     - Apply safe table look repairs.
   * - ``audit_table_style_quality()``
     - ``table_style_quality_audit_report``
     - Run table style quality audit.

