Images
======

FeatherDoc does not expose a single ``featherdoc::Image`` object. Image APIs
are available on ``featherdoc::Document`` and ``featherdoc::TemplatePart``.
Use them to append inline or floating images, inspect packaged image metadata,
and extract, replace, or remove existing image parts.

Image Info Types
----------------

.. list-table::
   :header-rows: 1
   :widths: 34 26 40

   * - Type
     - Key fields
     - Purpose
   * - ``featherdoc::inline_image_info``
     - ``index``, ``entry_name``, ``width_px``, ``height_px``
     - Describes one inline image and where it appears in body content.
   * - ``featherdoc::drawing_image_info``
     - ``placement``, ``floating_options``, ``entry_name``
     - Describes either an inline drawing object or an anchored floating image.
   * - ``featherdoc::floating_image_options``
     - references, offsets, wrap distances, ``crop``
     - Configures floating image placement, wrapping, layering, and optional crop.
   * - ``featherdoc::floating_image_crop``
     - ``left_per_mille``, ``top_per_mille``, ``right_per_mille``
     - Stores crop percentages in per-mille units.

Append Images
-------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``Document::append_image(image_path)``
     - ``bool``
     - Append an inline image to the document body using source dimensions.
   * - ``Document::append_image(image_path, width_px, height_px)``
     - ``bool``
     - Append an inline image to the body with explicit pixel dimensions.
   * - ``Document::append_floating_image(image_path, options)``
     - ``bool``
     - Append a floating image to the body with placement metadata.
   * - ``Document::append_floating_image(image_path, width_px, height_px, options)``
     - ``bool``
     - Append a floating image to the body with explicit size.
   * - ``TemplatePart::append_image(image_path)``
     - ``bool``
     - Append an inline image in a body, header, footer, or section part.
   * - ``TemplatePart::append_floating_image(image_path, options)``
     - ``bool``
     - Append a floating image in the selected template part.

Inspect And Mutate Images
-------------------------

.. list-table::
   :header-rows: 1
   :widths: 36 18 46

   * - Method
     - Returns
     - Purpose
   * - ``drawing_images() const``
     - ``std::vector<drawing_image_info>``
     - Enumerate drawing images, including anchored floating images.
   * - ``inline_images() const``
     - ``std::vector<inline_image_info>``
     - Enumerate inline images.
   * - ``extract_drawing_image(image_index, output_path) const``
     - ``bool``
     - Copy one drawing image part to a filesystem path.
   * - ``replace_drawing_image(image_index, image_path)``
     - ``bool``
     - Replace one drawing image while preserving document references.
   * - ``remove_drawing_image(image_index)``
     - ``bool``
     - Remove one drawing image and its related package part.
   * - ``extract_inline_image(image_index, output_path) const``
     - ``bool``
     - Copy one inline image part to a filesystem path.
   * - ``replace_inline_image(image_index, image_path)``
     - ``bool``
     - Replace one inline image while preserving document references.
   * - ``remove_inline_image(image_index)``
     - ``bool``
     - Remove one inline image and its related package part.

Example
-------

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

The complete legacy page remains available at :doc:`../../api/images_sections`.
