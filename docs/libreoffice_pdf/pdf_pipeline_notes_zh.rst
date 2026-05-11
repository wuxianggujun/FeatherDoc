LibreOffice PDF 流程笔记
========================

这一页记录 PDF 数据在 LibreOffice 内部如何流动。

这一轮的粗结论是：LibreOffice 的 PDF 不是单一流水线，而是几条
用途不同的链路拼在一起。


导出流程
--------

1. UNO 过滤器入口进入 ``PDFFilter``。
2. ``PDFFilter::implExport()`` 读取 ``FilterData``、
   ``FilterOptions`` 和用户配置。
3. ``PDFExport`` 接管页面导出，按页调用 ``XRenderable``。
4. ``vcl::PDFWriter`` 和 ``PDFWriterImpl`` 负责真正写 PDF。
5. 若遇到错误，交给 ``PDFInteractionHandler`` 显示。


导入流程
--------

1. 通过类型检测命中 ``pdf_Portable_Document_Format``。
2. 进入 ``sdext`` 的 PDF import adaptor。
3. 原始 PDF 走 ``xpdf_ImportFromStream`` 或 ``xpdf_ImportFromFile``。
4. hybrid PDF 先取 embedded substream，再走子过滤器。
5. ``PDFIProcessor`` 把 PDF 语义转换成 ODF / 内部元素树。


图形流程
--------

1. ``vcl::RenderPDFBitmaps`` 调用 ``PDFiumLibrary``。
2. ``pdfread.cxx`` 把 PDF 页渲染成 bitmap。
3. ``pdfdecomposer.cxx`` 可以把 PDF 作为图形对象交给更高层。


当前判断
--------

- 导出侧偏 UNO / Filter / VCL。
- 导入侧偏 PDFium / ODF 转换 / 模型适配。
- 如果只想“替换 PDF 功能”，不要默认它们是同一层实现。
- 这意味着迁移时要先选方向：导出、导入、图形预览，三者成本不同。


笔记模板
--------

.. code-block:: text

    流程阶段：
    输入：
    中间状态：
    输出：
    依赖：
    可替换性：
