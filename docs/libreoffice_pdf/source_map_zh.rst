LibreOffice 源码定位图
======================

这一页记录 LibreOffice 中和 PDF 相关的源码位置。

这一轮已经先做了源码粗扫，结论是：

- PDF 导出、PDF 导入、PDFium 适配不是同一层代码。
- 导出主要落在 ``filter`` 和 ``vcl``。
- 导入主要落在 ``sdext``、``vcl`` 和 ``external/pdfium``。
- PDF 类型注册、UI 注册、服务注册都分散在配置片段里。


导出链路
--------

- ``filter/source/pdf/pdffilter.cxx``：UNO 过滤器入口，读取 ``FilterData`` /
  ``FilterOptions``，必要时回读用户配置，再把数据交给 ``PDFExport``。
- ``filter/source/pdf/pdfexport.cxx`` / ``pdfexport.hxx``：真正执行导出，
  通过 ``XRenderable`` 和 ``vcl::PDFWriter`` 输出页面内容。
- ``filter/source/pdf/pdfdialog.cxx`` / ``pdfdialog.hxx``：PDF 选项对话框，
  把 UI 里选出来的设置写回 ``FilterData``。
- ``filter/source/pdf/pdfinteract.cxx``：处理导出阶段的
  ``PDFExportException``。
- ``filter/source/pdf/pdfdecomposer.cxx``：把 PDF 数据转成单页 bitmap
  primitive，和图形分解有关。
- ``filter/source/config/fragments/filters/*.xcu``：按 Writer、Calc、
  Draw、Impress、Math 注册 PDF 导出过滤器。


导入链路
--------

- ``sdext/source/pdfimport/pdfiadaptor.cxx`` / ``pdfiadaptor.hxx``：
  PDF 导入适配层。这里同时有两条路。
- ``PDFIRawAdaptor``：直接走 ``xpdf_ImportFromStream`` /
  ``xpdf_ImportFromFile``，再把结果发给 ODF emitter。
- ``PDFIHybridAdaptor``：处理 hybrid PDF，必要时取 embedded substream，
  再调用子过滤器。
- ``sdext/source/pdfimport/tree/pdfiprocessor.cxx``：把 PDF 语义转成内部
  element tree，是导入链路的核心转换层。
- ``sdext/source/pdfimport/config/pdf_import_filter.xcu``：
  注册 Draw / Impress / Writer 的 PDF 导入过滤器。
- ``sdext/source/pdfimport/config/pdf_types.xcu``：
  注册 ``pdf_Portable_Document_Format`` 类型和探测器。


VCL / PDFium 核心
---------------

- ``include/vcl/filter/PDFiumLibrary.hxx``：
  PDFium 抽象接口，定义 document / page / bitmap / annotation 等能力。
- ``vcl/source/pdf/PDFiumLibrary.cxx``：
  PDFium 的具体适配实现，封装 FPDF API。
- ``vcl/source/filter/ipdf/pdfread.cxx``：
  用 PDFium 读取 PDF，支持 bitmap 渲染和图形导入。
- ``vcl/source/gdi/pdfwriter.cxx``：
  ``PDFWriter`` 的薄封装，实际工作交给 ``PDFWriterImpl``。
- ``vcl/source/gdi/pdfwriter_impl*.cxx``：
  导出底层实现，后面如果要迁移导出能力，这里是必看点。


第三方依赖
----------

- ``external/pdfium/Module_pdfium.mk``：pdfium 模块入口。
- ``external/pdfium/Library_pdfium.mk``：pdfium 库构建与编译单元列表。
- ``external/tarballs/pdfium-*.tar.bz2``：上游源码包。


测试入口
--------

- ``filter/CppunitTest_filter_pdf.mk``
- ``sd/CppunitTest_sd_pdf_import_test.mk``
- ``sdext/CppunitTest_sdext_pdfimport.mk``
- ``vcl/CppunitTest_vcl_filter_ipdf.mk``
- ``vcl/CppunitTest_vcl_pdfexport.mk``
- ``vcl/CppunitTest_vcl_pdfium_library_test.mk``
- ``xmlsecurity/CppunitTest_xmlsecurity_pdfsigning.mk``


优先级判断
--------

如果你的目标是替换 FeatherDoc 的 PDF 能力，优先级大致是：

1. 先看 ``vcl`` 的 PDF 读写基础。
2. 再看 ``filter`` 的导出封装。
3. 再看 ``sdext`` 的导入转换层。
4. 最后看 UI、配置和测试壳。


记录模板
--------

.. code-block:: text

    模块：
    路径：
    作用：
    属于导出/导入：
    备注：
