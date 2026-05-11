LibreOffice 到 FeatherDoc 的迁移差距
=====================================

这一页用于记录 LibreOffice 能力和 FeatherDoc 现状之间的差距。

这一轮的初步判断是：

- LibreOffice 的 PDF 能力被拆得很细。
- 导出和导入是两条不同世界线。
- PDFium 在导入和图形读取里是核心依赖，不只是一个附件库。
- 直接“搬一坨源码”进 FeatherDoc 的性价比不高，更适合先拆能力边界。


建议先比对的能力项
----------------

- PDF 导出参数和默认值
- PDF 选项 UI
- PDF 导入的语义还原能力
- hybrid PDF 支持
- PDF bitmap 渲染能力
- 签名 / 注释 / 表单相关处理


初步差距判断
------------

- 如果 FeatherDoc 只需要导出，优先研究 ``filter`` + ``vcl``。
- 如果 FeatherDoc 需要导入，必须额外评估 ``sdext`` 和 PDFium。
- 如果只是要“看图式 PDF 预览”，核心是 ``vcl::RenderPDFBitmaps``。
- 如果要完整替换当前 PDF 子系统，最好先定义目标范围，不然会
  膨胀成整套 Office 级能力迁移。


建议输出格式
------------

.. code-block:: text

    能力项：
    FeatherDoc 现状：
    LibreOffice 做法：
    差距：
    迁移难度：
    结论：
