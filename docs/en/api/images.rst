Images
======

FeatherDoc does not expose a single ``featherdoc::Image`` object. Image APIs
are available on ``featherdoc::Document`` and ``featherdoc::TemplatePart``.
Use them to append inline or floating images, inspect packaged image metadata,
and extract, replace, or remove existing image parts.

Common Tasks
------------

Use these entry points for image workflows:

* Append an inline image with detected source dimensions: call
  ``Document::append_image(image_path)``.
* Append an inline image with explicit display size: call
  ``Document::append_image(image_path, width_px, height_px)``.
* Append an image to a header, footer, or section part: call
  ``TemplatePart::append_image(...)``.
* Place a floating image: configure ``floating_image_options`` and call
  ``append_floating_image(...)``.
* Inspect packaged images before editing: call ``drawing_images()`` or
  ``inline_images()``.
* Preserve references while changing media: use ``replace_drawing_image(...)``
  or ``replace_inline_image(...)``.
* Export or delete media: use the matching ``extract_*`` or ``remove_*`` method
  with an index from the corresponding list method.

Success And Failure Semantics
-----------------------------

.. list-table::
   :header-rows: 1
   :widths: 24 36 40

   * - Return shape
     - Success
     - Failure or no-op
   * - ``std::vector<..._image_info>``
     - Contains image metadata in the order used by image-index APIs.
     - Empty means no matching image placement exists in the document.
   * - ``bool`` append
     - ``true`` means the image bytes, relationship, and drawing XML were added.
     - ``false`` means the image file, target part, dimensions, or format probe
       were invalid.
   * - ``bool`` extract
     - ``true`` means image bytes were copied to the output path.
     - ``false`` means the index was invalid or the output file could not be
       written.
   * - ``bool`` replace
     - ``true`` means the image part changed while existing references stayed in
       place.
     - ``false`` means the index, replacement file, dimensions, or format probe
       could not be resolved.
   * - ``bool`` remove
     - ``true`` means the drawing and related package part were removed.
     - ``false`` means the index was unavailable or the package could not be
       updated.

Short Example
-------------

.. code-block:: cpp

   // PNG/JPEG/GIF/BMP dimensions are detected through the stb_image path.
   doc.append_image("logo.png", 240, 80);

   auto images = doc.inline_images();
   if (!images.empty()) {
       doc.replace_inline_image(images.front().index, "logo-new.png");
   }

Dimension Detection
-------------------

When an append or replace method needs source dimensions, FeatherDoc uses
``stb_image`` as the authoritative byte probe for ``PNG``, ``JPEG``, ``GIF``,
and ``BMP`` files. ``SVG``, ``WebP``, and ``TIFF`` keep FeatherDoc-specific
dimension readers because they are outside the supported ``stb_image`` decode
set used by this project.

Explicit ``width_px`` and ``height_px`` values bypass source-size inference for
layout size, but the image file still has to be readable and recognized so the
package media part and content type can be written correctly.

The package content type and media part extension are selected from the source
file extension. Keep the extension aligned with the actual image bytes; for
example, do not store JPEG bytes in a ``.png`` file when the generated DOCX
should advertise ``image/png``.

Typed Signature Guide
---------------------

.. FDOC_EN_IMAGES_TYPED_SIGNATURE_GUIDE

Image APIs use zero-based ``image_index`` values from ``drawing_images()`` or
``inline_images()``. ``width_px`` and ``height_px`` are explicit pixel sizes.
Methods returning ``bool`` report whether the package image part and its
relationships were updated. Methods that infer size may return ``false`` when
dimension detection cannot identify a supported image format.

.. list-table::
   :header-rows: 1
   :widths: 38 34 28

   * - Signature
     - Parameters
     - Return semantics
   * - ``std::vector<drawing_image_info> drawing_images() const``
     - None.
     - All drawing images, including anchored floating images.
   * - ``std::vector<inline_image_info> inline_images() const``
     - None.
     - All inline images with package metadata and optional body location.
   * - ``bool append_image(const std::filesystem::path &image_path)``
     - ``image_path``: source image path.
     - ``true`` when an inline image was appended with detected source
       dimensions.
   * - ``bool append_image(const std::filesystem::path &image_path, std::uint32_t width_px, std::uint32_t height_px)``
     - ``image_path``: source image. ``width_px`` / ``height_px``: explicit size.
     - ``true`` when a sized inline image was appended.
   * - ``bool append_floating_image(const std::filesystem::path &image_path, floating_image_options options = {})``
     - ``image_path``: source image. ``options``: anchor, wrap, offset, crop, and layer options.
     - ``true`` when a floating image was appended.
   * - ``bool extract_inline_image(std::size_t image_index, const std::filesystem::path &output_path) const``
     - ``image_index``: item from ``inline_images()``. ``output_path``: destination file.
     - ``true`` when the image bytes were copied.
   * - ``bool replace_drawing_image(std::size_t image_index, const std::filesystem::path &image_path)``
     - ``image_index``: item from ``drawing_images()``. ``image_path``: replacement image.
     - ``true`` when the image part was replaced while references and placement
       metadata were preserved.
   * - ``bool remove_inline_image(std::size_t image_index)``
     - ``image_index``: item from ``inline_images()``.
     - ``true`` when the inline drawing and related package part were removed.

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
     - Append an inline image to the document body using detected source
       dimensions.
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
     - Append an inline image in a body, header, footer, or section part using
       detected source dimensions.
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

   // Uses detected PNG dimensions when no explicit size is supplied.
   doc.append_image("logo.png");

   // Uses explicit layout dimensions.
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
