图片 API
========

FeatherDoc 没有单独的 ``featherdoc::Image`` 对象。图片相关能力分布在
``featherdoc::Document`` 和 ``featherdoc::TemplatePart`` 上，用于追加行内图片
或浮动图片，检查包内图片元数据，并提取、替换或删除已有图片部件。

图片信息类型
------------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - 类型
     - 关键字段
     - 用途
   * - ``featherdoc::inline_image_info``
     - ``index``、``entry_name``、``width_px``、``height_px``
     - 描述一张行内图片以及它在正文内容中的位置。
   * - ``featherdoc::drawing_image_info``
     - ``placement``、``floating_options``、``entry_name``
     - 描述行内 drawing 对象或锚定浮动图片。
   * - ``featherdoc::floating_image_options``
     - 引用系、偏移、环绕距离、``crop``
     - 配置浮动图片的位置、环绕、层级和可选裁剪。
   * - ``featherdoc::floating_image_crop``
     - ``left_per_mille``、``top_per_mille``、``right_per_mille``
     - 用千分比保存裁剪比例。

追加图片
--------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``Document::append_image(image_path)``
     - ``bool``
     - 按源图片尺寸向正文追加行内图片。
   * - ``Document::append_image(image_path, width_px, height_px)``
     - ``bool``
     - 按显式像素尺寸向正文追加行内图片。
   * - ``Document::append_floating_image(image_path, options)``
     - ``bool``
     - 向正文追加带定位元数据的浮动图片。
   * - ``Document::append_floating_image(image_path, width_px, height_px, options)``
     - ``bool``
     - 向正文追加显式尺寸的浮动图片。
   * - ``TemplatePart::append_image(image_path)``
     - ``bool``
     - 在正文、页眉、页脚或分节部件中追加行内图片。
   * - ``TemplatePart::append_floating_image(image_path, options)``
     - ``bool``
     - 在选中的模板部件中追加浮动图片。

检查和修改图片
--------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - 方法
     - 返回值
     - 用途
   * - ``drawing_images() const``
     - ``std::vector<drawing_image_info>``
     - 枚举 drawing 图片，包括锚定浮动图片。
   * - ``inline_images() const``
     - ``std::vector<inline_image_info>``
     - 枚举行内图片。
   * - ``extract_drawing_image(image_index, output_path) const``
     - ``bool``
     - 将一张 drawing 图片部件复制到文件系统路径。
   * - ``replace_drawing_image(image_index, image_path)``
     - ``bool``
     - 替换一张 drawing 图片，同时保留文档引用。
   * - ``remove_drawing_image(image_index)``
     - ``bool``
     - 删除一张 drawing 图片及其关联包部件。
   * - ``extract_inline_image(image_index, output_path) const``
     - ``bool``
     - 将一张行内图片部件复制到文件系统路径。
   * - ``replace_inline_image(image_index, image_path)``
     - ``bool``
     - 替换一张行内图片，同时保留文档引用。
   * - ``remove_inline_image(image_index)``
     - ``bool``
     - 删除一张行内图片及其关联包部件。

示例
----

.. code-block:: cpp

   featherdoc::Document doc{"template.docx"};
   doc.open();

   doc.append_image("logo.png", 240, 80);

   featherdoc::floating_image_options options;
   options.horizontal_offset_px = 32;
   options.vertical_offset_px = 12;
   options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
   doc.body_template().append_floating_image("badge.png", options);

   auto inline_images = doc.inline_images();
   if (!inline_images.empty()) {
       doc.replace_inline_image(0, "logo-updated.png");
   }

旧版完整页面仍保留在 :doc:`../../api/images_sections`。
