图片 API
========

FeatherDoc 没有单独的 ``featherdoc::Image`` 对象。图片相关能力分布在
``featherdoc::Document`` 和 ``featherdoc::TemplatePart`` 上，用于追加行内图片
或浮动图片，检查包内图片元数据，并提取、替换或删除已有图片部件。

常用任务入口
------------

.. FDOC_ZH_CN_IMAGES_COMMON_TASKS

图片工作流可以从这些入口开始：

* 按源图片探测尺寸向正文追加行内图片：调用 ``Document::append_image(image_path)``。
* 按显式显示尺寸向正文追加行内图片：调用
  ``Document::append_image(image_path, width_px, height_px)``。
* 向页眉、页脚或分节部件追加图片：调用 ``TemplatePart::append_image(...)``。
* 放置浮动图片：配置 ``floating_image_options``，再调用
  ``append_floating_image(...)``。
* 修改前检查包内图片：调用 ``drawing_images()`` 或 ``inline_images()``。
* 保留引用并替换媒体：使用 ``replace_drawing_image(...)`` 或
  ``replace_inline_image(...)``。
* 导出或删除媒体：先从对应列表方法取得索引，再调用匹配的 ``extract_*`` 或
  ``remove_*`` 方法。

成功/失败语义
-------------

.. FDOC_ZH_CN_IMAGES_SUCCESS_FAILURE_SEMANTICS

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - 返回形态
     - 成功含义
     - 失败或无变化含义
   * - ``std::vector<..._image_info>``
     - 按图片索引 API 使用的顺序返回图片元数据。
     - 空列表表示文档中没有对应类型的图片位置。
   * - ``bool`` 追加
     - ``true`` 表示图片字节、关系和 drawing XML 已追加。
     - ``false`` 表示图片文件、目标部件、尺寸或格式探测无效。
   * - ``bool`` 提取
     - ``true`` 表示图片字节已经复制到输出路径。
     - ``false`` 表示索引无效或输出文件无法写入。
   * - ``bool`` 替换
     - ``true`` 表示图片部件已替换，既有引用保持不变。
     - ``false`` 表示索引、替换文件、尺寸或格式探测无法解析。
   * - ``bool`` 删除
     - ``true`` 表示 drawing 和关联包部件已删除。
     - ``false`` 表示索引不可用，或文档包无法更新。

短示例
------

.. FDOC_ZH_CN_IMAGES_SHORT_EXAMPLE

.. code-block:: cpp

   // PNG/JPEG/GIF/BMP 尺寸通过 stb_image 路径探测。
   doc.append_image("logo.png", 240, 80);

   auto images = doc.inline_images();
   if (!images.empty()) {
       doc.replace_inline_image(images.front().index, "logo-new.png");
   }

尺寸探测
--------

当追加或替换方法需要源图片尺寸时，FeatherDoc 会优先使用基于 stb_image 的
raster 探测路径处理 ``PNG``、``JPEG``、``GIF`` 和 ``BMP`` 文件。``SVG``、
``WebP`` 和 ``TIFF`` 继续使用 FeatherDoc 既有的格式探测和 fallback 路径。

显式传入 ``width_px`` 和 ``height_px`` 时，布局尺寸不再依赖源尺寸推断；但图片
文件仍必须可读取且格式可识别，这样才能正确写入包内媒体部件和内容类型。

类型化签名导读
--------------

.. FDOC_ZH_CN_IMAGES_TYPED_SIGNATURE_GUIDE

图片 API 使用从 0 开始的 ``image_index``，索引来源是 ``drawing_images()`` 或
``inline_images()``。``width_px`` 和 ``height_px`` 是显式像素尺寸。返回
``bool`` 的方法表示图片包部件和关系是否成功更新。需要推断尺寸的方法在无法
识别受支持图片格式时会返回 ``false``。

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - 签名
     - 参数
     - 返回语义
   * - ``std::vector<drawing_image_info> drawing_images() const``
     - 无。
     - 返回全部 drawing 图片，包括锚定浮动图片。
   * - ``std::vector<inline_image_info> inline_images() const``
     - 无。
     - 返回全部行内图片及包元数据和可选正文位置。
   * - ``bool append_image(const std::filesystem::path &image_path)``
     - ``image_path``：源图片路径。
     - 按探测到的源图片尺寸追加行内图片成功时返回 ``true``。
   * - ``bool append_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``image_path``：源图片。``width_px`` / ``height_px``：显式尺寸。
     - 追加指定尺寸的行内图片成功时返回 ``true``。
   * - ``bool append_floating_image(const std::filesystem::path &image_path, floating_image_options options = {})``
     - ``image_path``：源图片。``options``：锚点、环绕、偏移、裁剪和层级选项。
     - 追加浮动图片成功时返回 ``true``。
   * - ``bool extract_inline_image(std::size_t image_index, const std::filesystem::path &output_path) const``
     - ``image_index``：来自 ``inline_images()`` 的索引。``output_path``：目标文件。
     - 图片字节复制成功时返回 ``true``。
   * - ``bool replace_drawing_image(std::size_t image_index, const std::filesystem::path &image_path)``
     - ``image_index``：来自 ``drawing_images()`` 的索引。``image_path``：替换图片。
     - 保留引用和放置元数据，并替换图片部件成功时返回 ``true``。
   * - ``bool remove_inline_image(std::size_t image_index)``
     - ``image_index``：来自 ``inline_images()`` 的索引。
     - 行内 drawing 和关联包部件删除成功时返回 ``true``。

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
     - 按探测到的源图片尺寸向正文追加行内图片。
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
     - 在正文、页眉、页脚或分节部件中按探测到的源图片尺寸追加行内图片。
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

   // 未传入显式尺寸时，使用探测到的 PNG 尺寸。
   doc.append_image("logo.png");

   // 使用显式布局尺寸。
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
