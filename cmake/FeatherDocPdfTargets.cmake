# Optional PDF writer and PDF import targets.
# Keeping these targets in one module prevents experimental PDF dependencies
# from obscuring the DOCX core build graph in the top-level project file.
include_guard(GLOBAL)

if(FEATHERDOC_BUILD_PDF)
    include("${CMAKE_CURRENT_LIST_DIR}/FeatherDocPdfio.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FeatherDocPdfThirdParty.cmake")
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
        set_target_properties(FeatherDocPdf PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON)
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
            FeatherDoc::Core
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
    include("${CMAKE_CURRENT_LIST_DIR}/FeatherDocPdfium.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FeatherDocPdfThirdParty.cmake")

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
        set_target_properties(FeatherDocPdfImport PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()

    target_include_directories(FeatherDocPdfImport
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_libraries(FeatherDocPdfImport
        PUBLIC
            FeatherDoc::Core
        PRIVATE
            ${FEATHERDOC_PDFIUM_TARGET}
    )

    if(MSVC)
        target_compile_options(FeatherDocPdfImport PRIVATE /W4 /permissive-)
    else()
        target_compile_options(FeatherDocPdfImport PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
