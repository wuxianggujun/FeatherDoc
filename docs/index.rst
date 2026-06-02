FeatherDoc Documentation
========================

Release v\ |version|

Choose a language entry point for the documentation site. The API reference is
now maintained under the language-local trees so readers do not have to browse
one long mixed page.

* :doc:`en/index`
* :doc:`zh-CN/index`

Quick Links
-----------

* :doc:`en/api/index`
* :doc:`zh-CN/api/index`

Older root-level pages are still built for external links and automation, but
the visible documentation flow now starts from the language-local trees.

.. toctree::
   :maxdepth: 2
   :caption: Languages

   en/index
   zh-CN/index

.. toctree::
   :maxdepth: 1
   :caption: Compatibility
   :hidden:

   api/index
   getting_started
   pdf_export
   pdf_import
   pdf_import_json_diagnostics
   pdf_import_scope
