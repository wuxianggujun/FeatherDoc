cmake_minimum_required(VERSION 3.20)

project(
    FeatherDoc
    VERSION 1.12.4
    DESCRIPTION "Modern C++ .docx library focused on explicit diagnostics, MSVC support, and clean API behavior"
    LANGUAGES C CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(FEATHERDOC_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/FeatherDoc")
set(FEATHERDOC_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/FeatherDoc")

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

option(BUILD_SHARED_LIBS "Build shared instead of static library" OFF)
option(BUILD_SAMPLES "Build provided samples" OFF)
option(BUILD_CLI "Build featherdoc_cli command line utility" ${PROJECT_IS_TOP_LEVEL})
# PDF support is experimental future work and must remain opt-in.
option(FEATHERDOC_BUILD_PDF
    "Build experimental PDF byte writer module backed by PDFio" OFF)
option(FEATHERDOC_BUILD_PDF_IMPORT
    "Build experimental PDF parser module backed by PDFium" OFF)

if(MSVC)
    # 统一按 UTF-8 解析源码，避免中文字符串字面量在样例和测试里被误解码。
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
    )
endif()

set(HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/constants.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc_iterator.hpp"
)
set(FEATHERDOC_MODULE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/core.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_body_content_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_core.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_image_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_image_private_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_reviews_fields_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_sections_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_sections_private_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_styles_numbering_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/document_templates_members.inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/fwd.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/numbering.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/reviews_fields.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/styles.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/styles_numbering.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/table_styles.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/tables.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/template_part.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/templates.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/text.hpp"
)
set(PDF_COMMON_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_font_subset.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_font_resolver.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_glyph_run.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_interfaces.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_layout.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_text_metrics.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_text_shaper.hpp"
)
set(PDF_WRITER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_document_adapter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_writer.hpp"
)
set(PDF_IMPORT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_document_importer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc/pdf/pdf_parser.hpp"
)
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constants.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_image.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_image_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_numbering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_section_inspection_methods.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_section_xml_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_semantic_diff.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_semantic_diff_values.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_styles.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_table_style_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_table_style_read_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_template.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_template_content_control_replacement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paragraph.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/run.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table_cell_xml_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table_column_edit_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table_grid_row_xml_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table_xml_conversion_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table_xml_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/xml_helpers.cpp"
)

set(PUGIXML_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pugixml/pugixml.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pugixml/pugiconfig.hpp"
)
set(PUGIXML_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pugixml/pugixml.cpp"
)
set(ZIP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zip/miniz.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zip/zip.h"
)
set(STB_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb/stb_image.h"
)
set(THIRD_PARTY_SRC
    ${PUGIXML_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zip/zip.c"
    ${STB_HEADERS}
)

add_library(FeatherDoc ${SOURCES} ${THIRD_PARTY_SRC})
add_library(FeatherDoc::FeatherDoc ALIAS FeatherDoc)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FeatherDocRuntimeDlls.cmake")


target_compile_features(FeatherDoc PUBLIC cxx_std_20)
set_target_properties(FeatherDoc PROPERTIES
    CXX_EXTENSIONS NO
    CXX_STANDARD_REQUIRED YES
    EXPORT_NAME FeatherDoc
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(WIN32)
    set_target_properties(FeatherDoc PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(FeatherDoc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pugixml>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zip>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/pugixml>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/zip>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/stb>
)

if(WIN32)
    target_compile_definitions(FeatherDoc PRIVATE ZIP_ENABLE_SHARABLE_FILE_OPEN)
endif()

if(MSVC)
    target_compile_options(FeatherDoc PRIVATE /W4 /permissive-)
    target_compile_definitions(FeatherDoc PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(FeatherDoc PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(FEATHERDOC_BUILD_PDF)
    include(cmake/FeatherDocPdfio.cmake)
    include(cmake/FeatherDocPdfThirdParty.cmake)
    featherdoc_add_pdfio_object_library(featherdoc_pdfio_objects)
    featherdoc_add_freetype_target()
    featherdoc_add_png_target(featherdoc_png)
    featherdoc_add_harfbuzz_targets()

    target_compile_definitions(featherdoc_pdfio_objects PRIVATE HAVE_LIBPNG=1)
    target_link_libraries(featherdoc_pdfio_objects PRIVATE PNG::PNG zlibstatic)

    add_library(FeatherDocPdf
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_headers.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_images.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_paragraphs.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_render.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_table_borders.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_table_layout.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_adapter_text.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_font_resolver.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_font_subset.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_text_metrics.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_text_shaper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_writer.cpp"
        $<TARGET_OBJECTS:featherdoc_pdfio_objects>
    )
    add_library(FeatherDoc::Pdf ALIAS FeatherDocPdf)

    target_compile_features(FeatherDocPdf PUBLIC cxx_std_20)
    set_target_properties(FeatherDocPdf PROPERTIES
        CXX_EXTENSIONS NO
        CXX_STANDARD_REQUIRED YES
        EXPORT_NAME Pdf
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    if(WIN32)
        set_target_properties(FeatherDocPdf PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()

    target_include_directories(FeatherDocPdf
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            $<BUILD_INTERFACE:${FEATHERDOC_RESOLVED_PDFIO_SOURCE_DIR}>
            $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_link_libraries(FeatherDocPdf
        PUBLIC
            FeatherDoc::FeatherDoc
            Freetype::Freetype
            PNG::PNG
            zlibstatic
    )
    if(FEATHERDOC_HARFBUZZ_SUBSET_AVAILABLE)
        target_compile_definitions(FeatherDocPdf
            PRIVATE
                FEATHERDOC_ENABLE_PDF_FONT_SUBSET=1
                FEATHERDOC_ENABLE_PDF_TEXT_SHAPER=1)
        target_link_libraries(FeatherDocPdf
            PRIVATE
                harfbuzz::harfbuzz
                harfbuzz::harfbuzz-subset)
    endif()

    if(MSVC)
        target_compile_options(FeatherDocPdf PRIVATE /W4 /permissive-)
        target_compile_definitions(FeatherDocPdf PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(FeatherDocPdf PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(FEATHERDOC_BUILD_PDF_IMPORT)
    include(cmake/FeatherDocPdfium.cmake)
    include(cmake/FeatherDocPdfThirdParty.cmake)

    if(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "auto")
        featherdoc_try_find_pdfium_package(pdfium_package_target)
        if(NOT pdfium_package_target STREQUAL "")
            set(FEATHERDOC_PDFIUM_TARGET "${pdfium_package_target}")
            set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "package")
        else()
            featherdoc_has_pdfium_prebuilt_inputs(pdfium_has_prebuilt_inputs)
            if(pdfium_has_prebuilt_inputs)
                featherdoc_add_pdfium_prebuilt_target(featherdoc_pdfium_prebuilt)
                set(FEATHERDOC_PDFIUM_TARGET featherdoc_pdfium_prebuilt)
                set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "prebuilt")
            elseif(NOT FEATHERDOC_PDFIUM_SOURCE_DIR STREQUAL "")
                featherdoc_add_pdfium_source_target(featherdoc_pdfium_source)
                set(FEATHERDOC_PDFIUM_TARGET featherdoc_pdfium_source)
                set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "source")
            else()
                message(FATAL_ERROR
                    "FEATHERDOC_PDFIUM_PROVIDER=auto could not resolve PDFium. "
                    "Tried package discovery via find_package(PDFium), then "
                    "prebuilt inputs via FEATHERDOC_PDFIUM_LIBRARY + "
                    "FEATHERDOC_PDFIUM_INCLUDE_DIR, then source via "
                    "FEATHERDOC_PDFIUM_SOURCE_DIR.")
            endif()
        endif()
    elseif(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "source")
        featherdoc_add_pdfium_source_target(featherdoc_pdfium_source)
        set(FEATHERDOC_PDFIUM_TARGET featherdoc_pdfium_source)
        set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "source")
    elseif(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "package")
        featherdoc_find_pdfium_package(FEATHERDOC_PDFIUM_TARGET)
        set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "package")
    elseif(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "prebuilt")
        featherdoc_add_pdfium_prebuilt_target(featherdoc_pdfium_prebuilt)
        set(FEATHERDOC_PDFIUM_TARGET featherdoc_pdfium_prebuilt)
        set(FEATHERDOC_RESOLVED_PDFIUM_PROVIDER "prebuilt")
    else()
        message(FATAL_ERROR
            "Unsupported FEATHERDOC_PDFIUM_PROVIDER: "
            "${FEATHERDOC_PDFIUM_PROVIDER}. Use auto, source, package, or prebuilt.")
    endif()
    message(STATUS "Using PDFium provider: ${FEATHERDOC_RESOLVED_PDFIUM_PROVIDER}")

    add_library(FeatherDocPdfImport
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdf_document_importer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/pdf/pdfium_parser.cpp"
    )
    add_library(FeatherDoc::PdfImport ALIAS FeatherDocPdfImport)

    target_compile_features(FeatherDocPdfImport PUBLIC cxx_std_20)
    set_target_properties(FeatherDocPdfImport PROPERTIES
        CXX_EXTENSIONS NO
        CXX_STANDARD_REQUIRED YES
        EXPORT_NAME PdfImport
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    if(WIN32)
        set_target_properties(FeatherDocPdfImport PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()

    target_include_directories(FeatherDocPdfImport
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_libraries(FeatherDocPdfImport
        PUBLIC
            FeatherDoc::FeatherDoc
        PRIVATE
            ${FEATHERDOC_PDFIUM_TARGET}
    )

    if(MSVC)
        target_compile_options(FeatherDocPdfImport PRIVATE /W4 /permissive-)
    else()
        target_compile_options(FeatherDocPdfImport PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FeatherDocSamples.cmake")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FeatherDocCli.cmake")

# PDF writer builds can use bundled PDFio/libpng helper targets that are not
# exported as part of the public package yet. Keep install exports free of the
# experimental writer until the dependency story is package-safe.
set(FEATHERDOC_INSTALLS_PDF OFF)
set(FEATHERDOC_INSTALLS_PDF_IMPORT OFF)
if(FEATHERDOC_BUILD_PDF_IMPORT AND
   DEFINED FEATHERDOC_RESOLVED_PDFIUM_PROVIDER AND
   FEATHERDOC_RESOLVED_PDFIUM_PROVIDER STREQUAL "package")
    set(FEATHERDOC_INSTALLS_PDF_IMPORT ON)
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FeatherDocConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FeatherDocConfig.cmake"
    INSTALL_DESTINATION "${FEATHERDOC_INSTALL_CMAKEDIR}"
    PATH_VARS FEATHERDOC_INSTALL_DATADIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FeatherDocConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    TARGETS FeatherDoc
    EXPORT FeatherDocTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(FEATHERDOC_INSTALLS_PDF)
    install(
        TARGETS FeatherDocPdf
        EXPORT FeatherDocTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

if(FEATHERDOC_INSTALLS_PDF_IMPORT)
    install(
        TARGETS FeatherDocPdfImport
        EXPORT FeatherDocTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(
    EXPORT FeatherDocTargets
    NAMESPACE FeatherDoc::
    DESTINATION "${FEATHERDOC_INSTALL_CMAKEDIR}"
)

install(
    FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES ${FEATHERDOC_MODULE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/featherdoc
)

if(FEATHERDOC_BUILD_PDF OR FEATHERDOC_INSTALLS_PDF_IMPORT)
    install(
        FILES ${PDF_COMMON_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/featherdoc/pdf
    )
endif()

if(FEATHERDOC_BUILD_PDF)
    install(
        FILES ${PDF_WRITER_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/featherdoc/pdf
    )

    install(
        FILES
            "${FEATHERDOC_RESOLVED_PDFIO_SOURCE_DIR}/LICENSE"
            "${FEATHERDOC_RESOLVED_PDFIO_SOURCE_DIR}/NOTICE"
        DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/thirdparty/pdfio"
            OPTIONAL
    )
endif()

if(FEATHERDOC_INSTALLS_PDF_IMPORT)
    install(
        FILES ${PDF_IMPORT_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/featherdoc/pdf
    )
endif()

install(
    FILES ${PUGIXML_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/pugixml
)

install(
    FILES ${ZIP_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/zip
)

install(
    FILES ${STB_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/stb
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/FeatherDocConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/FeatherDocConfigVersion.cmake"
    DESTINATION "${FEATHERDOC_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/README.zh-CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/RELEASE_ARTIFACT_TEMPLATE.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/RELEASE_ARTIFACT_TEMPLATE.zh-CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/VISUAL_VALIDATION_QUICKSTART.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/VISUAL_VALIDATION_QUICKSTART.zh-CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/VISUAL_VALIDATION.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/VISUAL_VALIDATION.zh-CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.upstream-mit"
        "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE"
        "${CMAKE_CURRENT_SOURCE_DIR}/LEGAL.md"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/changelog_archive_1.9.0.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/changelog_archive_1.8.0_and_earlier.md"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/en/api/pdf_workflow.rst"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs/en/api"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/zh-CN/api/pdf_workflow.rst"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs/zh-CN/api"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/en/word_document_workflow.rst"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs/en"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/zh-CN/word_document_workflow.rst"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs/zh-CN"
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/visual-smoke-contact-sheet.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/visual-smoke-page-05.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/visual-smoke-page-06.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/reopened-fixed-layout-column-widths-page-01.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/fixed-grid-merge-right-page-01.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/fixed-grid-merge-down-page-01.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/fixed-grid-aggregate-contact-sheet.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/assets/readme/sample-chinese-template-page-01.png"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/visual-validation"
)

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    add_subdirectory(test)
endif()
