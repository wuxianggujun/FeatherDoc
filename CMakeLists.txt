cmake_minimum_required(VERSION 3.20)

project(
    FeatherDoc
    VERSION 1.7.0
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

set(HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/constants.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/featherdoc_iterator.hpp"
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

function(featherdoc_copy_runtime_dlls target_name)
    if(WIN32 AND BUILD_SHARED_LIBS)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target_name}>
                $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS
        )
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

if(BUILD_CLI)
    add_executable(featherdoc_cli cli/featherdoc_cli.cpp)
    target_link_libraries(featherdoc_cli PRIVATE FeatherDoc::FeatherDoc)
    featherdoc_copy_runtime_dlls(featherdoc_cli)

    install(
        TARGETS featherdoc_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
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

