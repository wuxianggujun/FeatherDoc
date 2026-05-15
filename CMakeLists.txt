cmake_minimum_required(VERSION 3.20)

project(
    FeatherDoc
    VERSION 1.11.0
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_numbering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_styles.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/document_template.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image_helpers.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paragraph.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/run.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/table.cpp"
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
set(THIRD_PARTY_SRC
    ${PUGIXML_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zip/zip.c"
)

add_library(FeatherDoc ${SOURCES} ${THIRD_PARTY_SRC})
add_library(FeatherDoc::FeatherDoc ALIAS FeatherDoc)

file(TO_CMAKE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FeatherDocCopyRuntimeDlls.cmake"
    FEATHERDOC_COPY_RUNTIME_DLLS_SCRIPT)

function(featherdoc_copy_runtime_dlls target_name)
    if(WIN32)
        # Copy transitive runtime DLLs next to executables so CTest can run
        # without depending on an ad hoc PATH setup.
        set(copy_runtime_dlls_extra)
        foreach(runtime_target IN ITEMS
            PNG::PNG
            Freetype::Freetype
            harfbuzz::harfbuzz
            harfbuzz::harfbuzz-subset
        )
            featherdoc_get_imported_runtime_file(runtime_dll "${runtime_target}")
            if(NOT runtime_dll STREQUAL "")
                file(TO_CMAKE_PATH "${runtime_dll}" runtime_dll)
                list(APPEND copy_runtime_dlls_extra "${runtime_dll}")
            endif()
        endforeach()
        set(copy_runtime_companion_names
            zlib1.dll
            bz2.dll
            brotlicommon.dll
            brotlidec.dll
            brotlienc.dll
        )
        foreach(runtime_dll IN LISTS copy_runtime_dlls_extra)
            get_filename_component(runtime_dir "${runtime_dll}" DIRECTORY)
            foreach(companion_name IN LISTS copy_runtime_companion_names)
                set(companion_dll "${runtime_dir}/${companion_name}")
                if(EXISTS "${companion_dll}")
                    list(APPEND copy_runtime_dlls_extra "${companion_dll}")
                endif()
            endforeach()
        endforeach()
        if(copy_runtime_dlls_extra)
            list(REMOVE_DUPLICATES copy_runtime_dlls_extra)
        endif()

        set(copy_runtime_dlls_extra_lines "")
        foreach(runtime_dll IN LISTS copy_runtime_dlls_extra)
            string(APPEND copy_runtime_dlls_extra_lines
                "list(APPEND FEATHERDOC_COPY_RUNTIME_DLLS_DLLS \"${runtime_dll}\")\n")
        endforeach()

        set(copy_runtime_dlls_script
            "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_copy_runtime_dlls.cmake")
        set(copy_runtime_dlls_content [=[
set(FEATHERDOC_COPY_RUNTIME_DLLS_DEST "$<TARGET_FILE_DIR:@TARGET_NAME@>")
set(FEATHERDOC_COPY_RUNTIME_DLLS_DLLS "$<TARGET_RUNTIME_DLLS:@TARGET_NAME@>")
@EXTRA_RUNTIME_DLLS@
include("@COMMON_SCRIPT@")
]=])
        string(REPLACE "@TARGET_NAME@" "${target_name}"
            copy_runtime_dlls_content "${copy_runtime_dlls_content}")
        string(REPLACE "@EXTRA_RUNTIME_DLLS@" "${copy_runtime_dlls_extra_lines}"
            copy_runtime_dlls_content "${copy_runtime_dlls_content}")
        string(REPLACE "@COMMON_SCRIPT@" "${FEATHERDOC_COPY_RUNTIME_DLLS_SCRIPT}"
            copy_runtime_dlls_content "${copy_runtime_dlls_content}")
        file(GENERATE
            OUTPUT "${copy_runtime_dlls_script}"
            CONTENT "${copy_runtime_dlls_content}"
        )
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -P "${copy_runtime_dlls_script}"
        )
    endif()
endfunction()

function(featherdoc_get_imported_runtime_file out_var target_name)
    if(NOT TARGET ${target_name})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(runtime_location "")
    foreach(property_name IN ITEMS
        IMPORTED_LOCATION_RELEASE
        IMPORTED_LOCATION_RELWITHDEBINFO
        IMPORTED_LOCATION_MINSIZEREL
        IMPORTED_LOCATION_DEBUG
        IMPORTED_LOCATION_NOCONFIG
        IMPORTED_LOCATION
        IMPORTED_IMPLIB_RELEASE
        IMPORTED_IMPLIB_RELWITHDEBINFO
        IMPORTED_IMPLIB_MINSIZEREL
        IMPORTED_IMPLIB_DEBUG
        IMPORTED_IMPLIB_NOCONFIG
        IMPORTED_IMPLIB
    )
        if(runtime_location STREQUAL "")
            get_target_property(runtime_location ${target_name} ${property_name})
            if(runtime_location STREQUAL "runtime_location-NOTFOUND")
                set(runtime_location "")
            endif()
        endif()
    endforeach()

    if(runtime_location STREQUAL "")
        if(target_name STREQUAL "PNG::PNG")
            foreach(variable_name IN ITEMS PNG_LIBRARY_RELEASE PNG_LIBRARY_DEBUG PNG_LIBRARY)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "Freetype::Freetype")
            foreach(variable_name IN ITEMS
                FREETYPE_LIBRARY_RELEASE
                FREETYPE_LIBRARY_DEBUG
                FEATHERDOC_FREETYPE_LIBRARY_RELEASE
                FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                FREETYPE_LIBRARY
            )
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz-subset")
            foreach(variable_name IN ITEMS
                HARFBUZZ_SUBSET_LIBRARY_RELEASE
                HARFBUZZ_SUBSET_LIBRARY_DEBUG
                HARFBUZZ_SUBSET_LIBRARY
            )
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz")
            foreach(variable_name IN ITEMS
                HARFBUZZ_LIBRARY_RELEASE
                HARFBUZZ_LIBRARY_DEBUG
                HARFBUZZ_LIBRARY
            )
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        endif()
    endif()

    if(runtime_location STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(runtime_ext "${runtime_location}" EXT)
    if(runtime_ext STREQUAL ".lib")
        string(REPLACE "/lib/" "/bin/" runtime_location "${runtime_location}")
        string(REPLACE "\\lib\\" "\\bin\\" runtime_location "${runtime_location}")
        string(REGEX REPLACE "\\.lib$" ".dll" runtime_location
            "${runtime_location}")
    endif()

    if(NOT EXISTS "${runtime_location}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(runtime_ext "${runtime_location}" EXT)
    if(NOT runtime_ext STREQUAL ".dll")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "${runtime_location}" PARENT_SCOPE)
endfunction()

function(featherdoc_get_imported_runtime_dir out_var target_name)
    featherdoc_get_imported_runtime_file(runtime_location ${target_name})
    if(runtime_location STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(runtime_dir "${runtime_location}" DIRECTORY)
    set(${out_var} "${runtime_dir}" PARENT_SCOPE)
endfunction()

function(featherdoc_collect_pdf_runtime_environment out_var include_pdfium_dir)
    set(runtime_dirs)

    featherdoc_get_imported_runtime_dir(png_runtime_dir PNG::PNG)
    if(NOT png_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${png_runtime_dir}")
    endif()

    featherdoc_get_imported_runtime_dir(freetype_runtime_dir Freetype::Freetype)
    if(NOT freetype_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${freetype_runtime_dir}")
    endif()

    featherdoc_get_imported_runtime_dir(harfbuzz_runtime_dir harfbuzz::harfbuzz)
    if(NOT harfbuzz_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_runtime_dir}")
    endif()

    featherdoc_get_imported_runtime_dir(harfbuzz_subset_runtime_dir
        harfbuzz::harfbuzz-subset)
    if(NOT harfbuzz_subset_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_subset_runtime_dir}")
    endif()

    if(include_pdfium_dir AND DEFINED FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR AND
       NOT "${FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR}" STREQUAL "")
        list(APPEND runtime_dirs "${FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR}")
    endif()

    list(REMOVE_DUPLICATES runtime_dirs)
    if(runtime_dirs)
        string(JOIN ";" runtime_path ${runtime_dirs})
        set(${out_var} "PATH=${runtime_path};$ENV{PATH}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

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
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/pugixml>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty/zip>
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

    if(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "source")
        featherdoc_add_pdfium_source_target(featherdoc_pdfium_source)
        set(FEATHERDOC_PDFIUM_TARGET featherdoc_pdfium_source)
    elseif(FEATHERDOC_PDFIUM_PROVIDER STREQUAL "package")
        featherdoc_find_pdfium_package(FEATHERDOC_PDFIUM_TARGET)
    else()
        message(FATAL_ERROR
            "Unsupported FEATHERDOC_PDFIUM_PROVIDER: "
            "${FEATHERDOC_PDFIUM_PROVIDER}. Use source or package.")
    endif()

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

if(BUILD_SAMPLES)
    add_executable(featherdoc_sample1 samples/sample1.cpp)
    target_link_libraries(featherdoc_sample1 PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample1)

    add_executable(featherdoc_sample2 samples/sample2.cpp)
    target_link_libraries(featherdoc_sample2 PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample2)

    add_executable(featherdoc_sample_chinese samples/sample_chinese.cpp)
    target_link_libraries(featherdoc_sample_chinese PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_chinese)

    add_executable(featherdoc_sample_chinese_template samples/sample_chinese_template.cpp)
    target_link_libraries(featherdoc_sample_chinese_template PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_chinese_template)

    add_executable(featherdoc_visual_smoke_tables samples/visual_smoke_tables.cpp)
    target_link_libraries(featherdoc_visual_smoke_tables PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_visual_smoke_tables)

    add_executable(featherdoc_section_inspection_visual
        samples/section_inspection_visual.cpp)
    target_link_libraries(featherdoc_section_inspection_visual PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_section_inspection_visual)

    add_executable(featherdoc_sample_edit_existing samples/sample_edit_existing.cpp)
    target_link_libraries(featherdoc_sample_edit_existing PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing)

    add_executable(featherdoc_sample_insert_paragraph_before
        samples/sample_insert_paragraph_before.cpp)
    target_link_libraries(featherdoc_sample_insert_paragraph_before
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_paragraph_before)

    add_executable(featherdoc_sample_insert_paragraph_like_existing
        samples/sample_insert_paragraph_like_existing.cpp)
    target_link_libraries(featherdoc_sample_insert_paragraph_like_existing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_paragraph_like_existing)

    add_executable(featherdoc_sample_insert_run_around_existing
        samples/sample_insert_run_around_existing.cpp)
    target_link_libraries(featherdoc_sample_insert_run_around_existing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_run_around_existing)

    add_executable(featherdoc_sample_insert_run_like_existing
        samples/sample_insert_run_like_existing.cpp)
    target_link_libraries(featherdoc_sample_insert_run_like_existing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_run_like_existing)

    add_executable(featherdoc_sample_edit_existing_part_tables
        samples/sample_edit_existing_part_tables.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_part_tables
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_part_tables)

    add_executable(featherdoc_sample_template_table_cli_visual
        samples/sample_template_table_cli_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_visual)

    add_executable(featherdoc_sample_template_table_cli_bookmark_visual
        samples/sample_template_table_cli_bookmark_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_bookmark_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_bookmark_visual)

    add_executable(featherdoc_sample_template_bookmark_multiline_visual
        samples/sample_template_bookmark_multiline_visual.cpp)
    target_link_libraries(featherdoc_sample_template_bookmark_multiline_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_bookmark_multiline_visual)

    add_executable(featherdoc_sample_bookmark_block_visibility_visual
        samples/sample_bookmark_block_visibility_visual.cpp)
    target_link_libraries(featherdoc_sample_bookmark_block_visibility_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_bookmark_block_visibility_visual)

    add_executable(featherdoc_sample_bookmark_table_replacement_visual
        samples/sample_bookmark_table_replacement_visual.cpp)
    target_link_libraries(featherdoc_sample_bookmark_table_replacement_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_bookmark_table_replacement_visual)

    add_executable(featherdoc_sample_content_control_rich_replacement_visual
        samples/sample_content_control_rich_replacement_visual.cpp)
    target_link_libraries(featherdoc_sample_content_control_rich_replacement_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_content_control_rich_replacement_visual)

    add_executable(featherdoc_sample_content_control_image_replacement_visual
        samples/sample_content_control_image_replacement_visual.cpp)
    target_link_libraries(featherdoc_sample_content_control_image_replacement_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_content_control_image_replacement_visual)

    add_executable(featherdoc_sample_semantic_diff_visual
        samples/sample_semantic_diff_visual.cpp)
    target_link_libraries(featherdoc_sample_semantic_diff_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_semantic_diff_visual)

    add_executable(featherdoc_sample_bookmark_image_visual
        samples/sample_bookmark_image_visual.cpp)
    target_link_libraries(featherdoc_sample_bookmark_image_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_bookmark_image_visual)

    add_executable(featherdoc_sample_bookmark_floating_image_visual
        samples/sample_bookmark_floating_image_visual.cpp)
    target_link_libraries(featherdoc_sample_bookmark_floating_image_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_bookmark_floating_image_visual)

    add_executable(featherdoc_sample_fill_bookmarks_visual
        samples/sample_fill_bookmarks_visual.cpp)
    target_link_libraries(featherdoc_sample_fill_bookmarks_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_fill_bookmarks_visual)

    add_executable(featherdoc_sample_append_image_visual
        samples/sample_append_image_visual.cpp)
    target_link_libraries(featherdoc_sample_append_image_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_append_image_visual)

    add_executable(featherdoc_sample_table_row_visual
        samples/sample_table_row_visual.cpp)
    target_link_libraries(featherdoc_sample_table_row_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_row_visual)

    add_executable(featherdoc_sample_paragraph_run_style_visual
        samples/sample_paragraph_run_style_visual.cpp)
    target_link_libraries(featherdoc_sample_paragraph_run_style_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_paragraph_run_style_visual)

    add_executable(featherdoc_sample_run_font_language_visual
        samples/sample_run_font_language_visual.cpp)
    target_link_libraries(featherdoc_sample_run_font_language_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_run_font_language_visual)

    add_executable(featherdoc_sample_paragraph_list_visual
        samples/sample_paragraph_list_visual.cpp)
    target_link_libraries(featherdoc_sample_paragraph_list_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_paragraph_list_visual)

    add_executable(featherdoc_sample_paragraph_numbering_visual
        samples/sample_paragraph_numbering_visual.cpp)
    target_link_libraries(featherdoc_sample_paragraph_numbering_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_paragraph_numbering_visual)

    add_executable(featherdoc_sample_paragraph_style_numbering_visual
        samples/sample_paragraph_style_numbering_visual.cpp)
    target_link_libraries(featherdoc_sample_paragraph_style_numbering_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_paragraph_style_numbering_visual)

    add_executable(featherdoc_sample_ensure_style_visual
        samples/sample_ensure_style_visual.cpp)
    target_link_libraries(featherdoc_sample_ensure_style_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_ensure_style_visual)

    add_executable(featherdoc_sample_table_cell_merge_visual
        samples/sample_table_cell_merge_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_merge_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_merge_visual)

    add_executable(featherdoc_sample_table_cell_fill_visual
        samples/sample_table_cell_fill_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_fill_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_fill_visual)

    add_executable(featherdoc_sample_table_cell_border_visual
        samples/sample_table_cell_border_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_border_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_border_visual)

    add_executable(featherdoc_sample_table_cell_text_direction_visual
        samples/sample_table_cell_text_direction_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_text_direction_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_text_direction_visual)

    add_executable(featherdoc_sample_table_cell_vertical_alignment_visual
        samples/sample_table_cell_vertical_alignment_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_vertical_alignment_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_vertical_alignment_visual)

    add_executable(featherdoc_sample_table_cell_width_visual
        samples/sample_table_cell_width_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_width_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_width_visual)

    add_executable(featherdoc_sample_table_cell_margin_visual
        samples/sample_table_cell_margin_visual.cpp)
    target_link_libraries(featherdoc_sample_table_cell_margin_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_cell_margin_visual)

    add_executable(featherdoc_sample_table_row_height_visual
        samples/sample_table_row_height_visual.cpp)
    target_link_libraries(featherdoc_sample_table_row_height_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_row_height_visual)

    add_executable(featherdoc_sample_table_row_repeat_header_visual
        samples/sample_table_row_repeat_header_visual.cpp)
    target_link_libraries(featherdoc_sample_table_row_repeat_header_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_row_repeat_header_visual)

    add_executable(featherdoc_sample_table_row_cant_split_visual
        samples/sample_table_row_cant_split_visual.cpp)
    target_link_libraries(featherdoc_sample_table_row_cant_split_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_table_row_cant_split_visual)

    add_executable(featherdoc_sample_template_bookmark_paragraphs_pagination_visual
        samples/sample_template_bookmark_paragraphs_pagination_visual.cpp)
    target_link_libraries(featherdoc_sample_template_bookmark_paragraphs_pagination_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_bookmark_paragraphs_pagination_visual)

    add_executable(featherdoc_sample_section_text_multiline_visual
        samples/sample_section_text_multiline_visual.cpp)
    target_link_libraries(featherdoc_sample_section_text_multiline_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_section_text_multiline_visual)

    add_executable(featherdoc_sample_section_part_refs_visual
        samples/sample_section_part_refs_visual.cpp)
    target_link_libraries(featherdoc_sample_section_part_refs_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_section_part_refs_visual)

    add_executable(featherdoc_sample_section_order_visual
        samples/sample_section_order_visual.cpp)
    target_link_libraries(featherdoc_sample_section_order_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_section_order_visual)

    add_executable(featherdoc_sample_template_table_cli_section_kind_visual
        samples/sample_template_table_cli_section_kind_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_section_kind_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_visual)

    add_executable(featherdoc_sample_template_table_cli_section_kind_row_visual
        samples/sample_template_table_cli_section_kind_row_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_section_kind_row_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_row_visual)

    add_executable(featherdoc_sample_template_table_cli_section_kind_column_visual
        samples/sample_template_table_cli_section_kind_column_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_section_kind_column_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_column_visual)

    add_executable(featherdoc_sample_template_table_cli_section_kind_merge_unmerge_visual
        samples/sample_template_table_cli_section_kind_merge_unmerge_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_section_kind_merge_unmerge_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_section_kind_merge_unmerge_visual)

    add_executable(featherdoc_sample_template_table_cli_merge_unmerge_visual
        samples/sample_template_table_cli_merge_unmerge_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_merge_unmerge_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_merge_unmerge_visual)

    add_executable(featherdoc_sample_template_table_cli_column_visual
        samples/sample_template_table_cli_column_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_column_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_column_visual)

    add_executable(featherdoc_sample_template_table_cli_direct_column_visual
        samples/sample_template_table_cli_direct_column_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_direct_column_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_direct_column_visual)

    add_executable(featherdoc_sample_template_table_cli_direct_visual
        samples/sample_template_table_cli_direct_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_direct_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_direct_visual)

    add_executable(featherdoc_sample_template_table_cli_direct_merge_unmerge_visual
        samples/sample_template_table_cli_direct_merge_unmerge_visual.cpp)
    target_link_libraries(featherdoc_sample_template_table_cli_direct_merge_unmerge_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_table_cli_direct_merge_unmerge_visual)

    add_executable(featherdoc_sample_edit_existing_part_paragraphs
        samples/sample_edit_existing_part_paragraphs.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_part_paragraphs
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_part_paragraphs)

    add_executable(featherdoc_sample_edit_existing_part_images
        samples/sample_edit_existing_part_images.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_part_images
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_part_images)

    add_executable(featherdoc_sample_edit_existing_part_append_images
        samples/sample_edit_existing_part_append_images.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_part_append_images
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_part_append_images)

    add_executable(featherdoc_sample_floating_images
        samples/sample_floating_images.cpp)
    target_link_libraries(featherdoc_sample_floating_images
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_floating_images)

    add_executable(featherdoc_sample_floating_image_z_order_visual
        samples/sample_floating_image_z_order_visual.cpp)
    target_link_libraries(featherdoc_sample_floating_image_z_order_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_floating_image_z_order_visual)

    add_executable(featherdoc_sample_remove_images
        samples/sample_remove_images.cpp)
    target_link_libraries(featherdoc_sample_remove_images
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_remove_images)

    add_executable(featherdoc_sample_remove_bookmark_block
        samples/sample_remove_bookmark_block.cpp)
    target_link_libraries(featherdoc_sample_remove_bookmark_block
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_remove_bookmark_block)

    add_executable(featherdoc_sample_remove_table
        samples/sample_remove_table.cpp)
    target_link_libraries(featherdoc_sample_remove_table
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_remove_table)

    add_executable(featherdoc_sample_remove_table_column
        samples/sample_remove_table_column.cpp)
    target_link_libraries(featherdoc_sample_remove_table_column
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_remove_table_column)

    add_executable(featherdoc_sample_insert_table_column
        samples/sample_insert_table_column.cpp)
    target_link_libraries(featherdoc_sample_insert_table_column
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_table_column)

    add_executable(featherdoc_sample_unmerge_table_cells
        samples/sample_unmerge_table_cells.cpp)
    target_link_libraries(featherdoc_sample_unmerge_table_cells
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_unmerge_table_cells)

    add_executable(featherdoc_sample_merge_right_fixed_grid
        samples/sample_merge_right_fixed_grid.cpp)
    target_link_libraries(featherdoc_sample_merge_right_fixed_grid
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_merge_right_fixed_grid)

    add_executable(featherdoc_sample_merge_down_fixed_grid
        samples/sample_merge_down_fixed_grid.cpp)
    target_link_libraries(featherdoc_sample_merge_down_fixed_grid
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_merge_down_fixed_grid)

    add_executable(featherdoc_sample_unmerge_right_fixed_grid
        samples/sample_unmerge_right_fixed_grid.cpp)
    target_link_libraries(featherdoc_sample_unmerge_right_fixed_grid
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_unmerge_right_fixed_grid)

    add_executable(featherdoc_sample_unmerge_down_fixed_grid
        samples/sample_unmerge_down_fixed_grid.cpp)
    target_link_libraries(featherdoc_sample_unmerge_down_fixed_grid
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_unmerge_down_fixed_grid)

    add_executable(featherdoc_sample_insert_table_around_existing
        samples/sample_insert_table_around_existing.cpp)
    target_link_libraries(featherdoc_sample_insert_table_around_existing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_table_around_existing)

    add_executable(featherdoc_sample_insert_table_like_existing
        samples/sample_insert_table_like_existing.cpp)
    target_link_libraries(featherdoc_sample_insert_table_like_existing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_table_like_existing)

    add_executable(featherdoc_sample_edit_existing_table_spacing
        samples/sample_edit_existing_table_spacing.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_table_spacing
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_table_spacing)

    add_executable(featherdoc_sample_edit_existing_table_column_widths
        samples/sample_edit_existing_table_column_widths.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_table_column_widths
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_table_column_widths)

    add_executable(featherdoc_sample_edit_existing_table_style_look
        samples/sample_edit_existing_table_style_look.cpp)
    target_link_libraries(featherdoc_sample_edit_existing_table_style_look
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_edit_existing_table_style_look)

    add_executable(featherdoc_sample_restart_paragraph_list
        samples/sample_restart_paragraph_list.cpp)
    target_link_libraries(featherdoc_sample_restart_paragraph_list
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_restart_paragraph_list)

    add_executable(featherdoc_sample_style_linked_numbering
        samples/sample_style_linked_numbering.cpp)
    target_link_libraries(featherdoc_sample_style_linked_numbering
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_style_linked_numbering)

    add_executable(featherdoc_sample_section_page_setup
        samples/sample_section_page_setup.cpp)
    target_link_libraries(featherdoc_sample_section_page_setup
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_section_page_setup)

    add_executable(featherdoc_sample_page_number_fields
        samples/sample_page_number_fields.cpp)
    target_link_libraries(featherdoc_sample_page_number_fields
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_page_number_fields)

    add_executable(featherdoc_sample_generic_fields_visual
        samples/sample_generic_fields_visual.cpp)
    target_link_libraries(featherdoc_sample_generic_fields_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_generic_fields_visual)

    add_executable(featherdoc_sample_hyperlinks_visual
        samples/sample_hyperlinks_visual.cpp)
    target_link_libraries(featherdoc_sample_hyperlinks_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_hyperlinks_visual)

    add_executable(featherdoc_sample_omml_visual
        samples/sample_omml_visual.cpp)
    target_link_libraries(featherdoc_sample_omml_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_omml_visual)

    add_executable(featherdoc_sample_extended_image_formats_visual
        samples/sample_extended_image_formats_visual.cpp)
    target_link_libraries(featherdoc_sample_extended_image_formats_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_extended_image_formats_visual)

    add_executable(featherdoc_sample_review_inspection_visual
        samples/sample_review_inspection_visual.cpp)
    target_link_libraries(featherdoc_sample_review_inspection_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_review_inspection_visual)

    add_executable(featherdoc_sample_review_mutation_visual
        samples/sample_review_mutation_visual.cpp)
    target_link_libraries(featherdoc_sample_review_mutation_visual
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_review_mutation_visual)

    add_executable(featherdoc_sample_template_validation
        samples/sample_template_validation.cpp)
    target_link_libraries(featherdoc_sample_template_validation
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_validation)

    add_executable(featherdoc_sample_part_template_validation
        samples/sample_part_template_validation.cpp)
    target_link_libraries(featherdoc_sample_part_template_validation
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_part_template_validation)

    add_executable(featherdoc_sample_template_schema_validation
        samples/sample_template_schema_validation.cpp)
    target_link_libraries(featherdoc_sample_template_schema_validation
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_schema_validation)

    add_executable(featherdoc_sample_template_schema_manifest_fixture
        samples/sample_template_schema_manifest_fixture.cpp)
    target_link_libraries(featherdoc_sample_template_schema_manifest_fixture
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_template_schema_manifest_fixture)

    add_executable(featherdoc_sample_insert_table_row
        samples/sample_insert_table_row.cpp)
    target_link_libraries(featherdoc_sample_insert_table_row
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_table_row)

    add_executable(featherdoc_sample_insert_table_row_before
        samples/sample_insert_table_row_before.cpp)
    target_link_libraries(featherdoc_sample_insert_table_row_before
        PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_sample_insert_table_row_before)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/samples/my_test.docx
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/samples/chinese_invoice_template.docx
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(BUILD_SAMPLES AND FEATHERDOC_BUILD_PDF)
    featherdoc_collect_pdf_runtime_environment(
        FEATHERDOC_PDF_RUNTIME_ENVIRONMENT FALSE)

    add_executable(featherdoc_pdfio_probe samples/pdfio_probe.cpp)
    target_link_libraries(featherdoc_pdfio_probe PRIVATE FeatherDoc::Pdf)
    featherdoc_copy_runtime_dlls(featherdoc_pdfio_probe)

    add_executable(featherdoc_pdf_document_probe samples/pdf_document_probe.cpp)
    target_link_libraries(featherdoc_pdf_document_probe PRIVATE FeatherDoc::Pdf)
    featherdoc_copy_runtime_dlls(featherdoc_pdf_document_probe)

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
        add_test(
            NAME pdfio_generator_probe
            COMMAND featherdoc_pdfio_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-pdfio-probe.pdf"
        )
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfio_generator_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        add_test(
            NAME pdf_document_generator_probe
            COMMAND featherdoc_pdf_document_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-document-probe.pdf"
        )
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdf_document_generator_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
    endif()
endif()

if(BUILD_SAMPLES AND FEATHERDOC_BUILD_PDF_IMPORT)
    add_executable(featherdoc_pdfium_probe samples/pdfium_probe.cpp)
    target_link_libraries(featherdoc_pdfium_probe PRIVATE FeatherDoc::PdfImport)
    featherdoc_copy_runtime_dlls(featherdoc_pdfium_probe)

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND TARGET featherdoc_pdfio_probe)
        add_test(
            NAME pdfium_parser_probe
            COMMAND featherdoc_pdfium_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-pdfio-probe.pdf"
        )
        featherdoc_collect_pdf_runtime_environment(
            FEATHERDOC_PDF_RUNTIME_ENVIRONMENT TRUE)
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfium_parser_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        set_tests_properties(pdfium_parser_probe
            PROPERTIES DEPENDS pdfio_generator_probe)
    endif()

    if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING AND TARGET featherdoc_pdf_document_probe)
        add_test(
            NAME pdfium_document_parser_probe
            COMMAND featherdoc_pdfium_probe
                "${CMAKE_CURRENT_BINARY_DIR}/featherdoc-document-probe.pdf"
        )
        featherdoc_collect_pdf_runtime_environment(
            FEATHERDOC_PDF_RUNTIME_ENVIRONMENT TRUE)
        if(NOT FEATHERDOC_PDF_RUNTIME_ENVIRONMENT STREQUAL "")
            set_tests_properties(pdfium_document_parser_probe PROPERTIES
                ENVIRONMENT "${FEATHERDOC_PDF_RUNTIME_ENVIRONMENT}")
        endif()
        set_tests_properties(pdfium_document_parser_probe
            PROPERTIES DEPENDS pdf_document_generator_probe)
    endif()
endif()

if(BUILD_CLI)
    add_executable(featherdoc_cli
        cli/featherdoc_cli_main.cpp
        cli/featherdoc_cli_json.cpp
        cli/featherdoc_cli_parse.cpp
        cli/featherdoc_cli_usage.cpp
        cli/featherdoc_cli.cpp)
    target_link_libraries(featherdoc_cli PRIVATE FeatherDoc::FeatherDoc)
    if(FEATHERDOC_BUILD_PDF AND TARGET FeatherDocPdf)
        target_compile_definitions(featherdoc_cli PRIVATE FEATHERDOC_CLI_ENABLE_PDF=1)
        target_link_libraries(featherdoc_cli PRIVATE FeatherDoc::Pdf)
    endif()
    if(FEATHERDOC_BUILD_PDF_IMPORT AND TARGET FeatherDocPdfImport)
        target_compile_definitions(
            featherdoc_cli PRIVATE FEATHERDOC_CLI_ENABLE_PDF_IMPORT=1)
        target_link_libraries(featherdoc_cli PRIVATE FeatherDoc::PdfImport)
    endif()
    featherdoc_copy_runtime_dlls(featherdoc_cli)

    install(
        TARGETS featherdoc_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# PDF writer builds can use bundled PDFio/libpng helper targets that are not
# exported as part of the public package yet. Keep install exports free of the
# experimental writer until the dependency story is package-safe.
set(FEATHERDOC_INSTALLS_PDF OFF)
set(FEATHERDOC_INSTALLS_PDF_IMPORT OFF)
if(FEATHERDOC_BUILD_PDF_IMPORT AND FEATHERDOC_PDFIUM_PROVIDER STREQUAL "package")
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
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/pdf_import.rst"
    DESTINATION "${FEATHERDOC_INSTALL_DATADIR}/docs"
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
