include_guard(GLOBAL)

include(ExternalProject)

set(FEATHERDOC_PDFIUM_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(FEATHERDOC_PDFIUM_PROVIDER "source" CACHE STRING
    "PDFium provider for the experimental PDF import module: source or package")
set_property(CACHE FEATHERDOC_PDFIUM_PROVIDER PROPERTY STRINGS source package)

set(FEATHERDOC_PDFIUM_SOURCE_DIR "" CACHE PATH
    "Existing PDFium source checkout root. It must contain public/fpdfview.h.")
set(FEATHERDOC_PDFIUM_OUT_DIR "" CACHE PATH
    "PDFium GN output directory. Defaults to <PDFium source>/out/FeatherDoc.")
set(FEATHERDOC_PDFIUM_GN_ARGS
    "is_debug=false is_component_build=false pdf_is_complete_lib=true pdf_is_standalone=true pdf_enable_v8=false pdf_enable_xfa=false pdf_use_skia=false pdf_enable_fontations=false clang_use_chrome_plugins=false use_custom_libcxx=false"
    CACHE STRING
    "GN args used when building PDFium from source")
set(FEATHERDOC_PDFIUM_NINJA_TARGET "pdfium" CACHE STRING
    "Ninja target used when building PDFium from source")
set(FEATHERDOC_PDFIUM_EXTRA_LIBS "" CACHE STRING
    "Extra libraries to link with a source-built PDFium target")
set(FEATHERDOC_DEPOT_TOOLS_DIR "" CACHE PATH
    "Optional Chromium depot_tools directory used while building PDFium")
set(FEATHERDOC_PDFIUM_VS_YEAR "" CACHE STRING
    "Optional Visual Studio year hint for PDFium source builds, such as 2026 or 2022")
set_property(CACHE FEATHERDOC_PDFIUM_VS_YEAR PROPERTY STRINGS "" 2026 2022 2019 2017)
set(FEATHERDOC_PDFIUM_VS_INSTALL "" CACHE PATH
    "Optional Visual Studio installation path passed to PDFium as vs<year>_install")

function(featherdoc_find_pdfium_package out_target)
    find_package(PDFium REQUIRED)

    if(TARGET pdfium)
        set(${out_target} pdfium PARENT_SCOPE)
    elseif(TARGET PDFium::PDFium)
        set(${out_target} PDFium::PDFium PARENT_SCOPE)
    elseif(TARGET PDFium::pdfium)
        set(${out_target} PDFium::pdfium PARENT_SCOPE)
    else()
        message(FATAL_ERROR
            "PDFium package was found, but no supported CMake target exists. "
            "Expected pdfium, PDFium::PDFium, or PDFium::pdfium.")
    endif()
endfunction()

function(featherdoc_add_pdfium_source_target target_name)
    if(FEATHERDOC_PDFIUM_SOURCE_DIR STREQUAL "")
        message(FATAL_ERROR
            "FEATHERDOC_BUILD_PDF_IMPORT uses PDFium source by default. "
            "Set FEATHERDOC_PDFIUM_SOURCE_DIR to a PDFium checkout created with:\n"
            "  gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git\n"
            "  gclient sync\n"
            "Or set FEATHERDOC_PDFIUM_PROVIDER=package and provide PDFium_DIR.")
    endif()

    get_filename_component(pdfium_source_dir
        "${FEATHERDOC_PDFIUM_SOURCE_DIR}" ABSOLUTE)
    if(NOT EXISTS "${pdfium_source_dir}/public/fpdfview.h")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_SOURCE_DIR must point to a PDFium checkout "
            "containing public/fpdfview.h: ${pdfium_source_dir}")
    endif()

    set(pdfium_gn_search_paths)
    if(WIN32)
        list(APPEND pdfium_gn_search_paths "${pdfium_source_dir}/buildtools/win")
    elseif(APPLE)
        list(APPEND pdfium_gn_search_paths "${pdfium_source_dir}/buildtools/mac")
    else()
        list(APPEND pdfium_gn_search_paths "${pdfium_source_dir}/buildtools/linux64")
    endif()

    find_program(FEATHERDOC_GN_EXECUTABLE
        NAMES gn gn.exe
        PATHS ${pdfium_gn_search_paths}
        NO_DEFAULT_PATH
        DOC "GN executable used to generate the PDFium source build")
    if(NOT FEATHERDOC_GN_EXECUTABLE)
        find_program(FEATHERDOC_GN_EXECUTABLE
            NAMES gn gn.exe
            DOC "GN executable used to generate the PDFium source build")
    endif()
    if(NOT FEATHERDOC_GN_EXECUTABLE)
        message(FATAL_ERROR
            "PDFium source builds require gn. Use a synced PDFium checkout "
            "or set FEATHERDOC_GN_EXECUTABLE explicitly.")
    endif()

    find_program(FEATHERDOC_NINJA_EXECUTABLE
        NAMES ninja ninja.exe
        PATHS "${pdfium_source_dir}/third_party/ninja"
        NO_DEFAULT_PATH
        DOC "Ninja executable used to build PDFium from source")
    if(NOT FEATHERDOC_NINJA_EXECUTABLE)
        find_program(FEATHERDOC_NINJA_EXECUTABLE
            NAMES ninja ninja.exe
            DOC "Ninja executable used to build PDFium from source")
    endif()
    if(NOT FEATHERDOC_NINJA_EXECUTABLE)
        message(FATAL_ERROR
            "PDFium source builds require ninja. Use a synced PDFium checkout "
            "or set FEATHERDOC_NINJA_EXECUTABLE explicitly.")
    endif()

    if(FEATHERDOC_PDFIUM_OUT_DIR STREQUAL "")
        set(pdfium_out_dir "${pdfium_source_dir}/out/FeatherDoc")
    else()
        get_filename_component(pdfium_out_dir
            "${FEATHERDOC_PDFIUM_OUT_DIR}" ABSOLUTE)
    endif()

    if(WIN32)
        set(pdfium_library "${pdfium_out_dir}/obj/pdfium.lib")
    else()
        set(pdfium_library "${pdfium_out_dir}/obj/libpdfium.a")
    endif()

    if(WIN32 AND NOT FEATHERDOC_PDFIUM_VS_INSTALL STREQUAL "")
        if(FEATHERDOC_PDFIUM_VS_YEAR STREQUAL "")
            message(FATAL_ERROR
                "Set FEATHERDOC_PDFIUM_VS_YEAR when "
                "FEATHERDOC_PDFIUM_VS_INSTALL is provided.")
        endif()
    endif()

    set(pdfium_runner "${FEATHERDOC_PDFIUM_CMAKE_DIR}/FeatherDocRunPdfiumBuild.cmake")

    ExternalProject_Add(featherdoc_pdfium_source_build
        SOURCE_DIR "${pdfium_source_dir}"
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND
            ${CMAKE_COMMAND}
            "-DFEATHERDOC_PDFIUM_RUN_STEP=configure"
            "-DFEATHERDOC_PDFIUM_SOURCE_DIR=${pdfium_source_dir}"
            "-DFEATHERDOC_PDFIUM_OUT_DIR=${pdfium_out_dir}"
            "-DFEATHERDOC_GN_EXECUTABLE=${FEATHERDOC_GN_EXECUTABLE}"
            "-DFEATHERDOC_PDFIUM_GN_ARGS=${FEATHERDOC_PDFIUM_GN_ARGS}"
            "-DFEATHERDOC_DEPOT_TOOLS_DIR=${FEATHERDOC_DEPOT_TOOLS_DIR}"
            "-DFEATHERDOC_PDFIUM_VS_YEAR=${FEATHERDOC_PDFIUM_VS_YEAR}"
            "-DFEATHERDOC_PDFIUM_VS_INSTALL=${FEATHERDOC_PDFIUM_VS_INSTALL}"
            -P "${pdfium_runner}"
        BUILD_COMMAND
            ${CMAKE_COMMAND}
            "-DFEATHERDOC_PDFIUM_RUN_STEP=build"
            "-DFEATHERDOC_PDFIUM_SOURCE_DIR=${pdfium_source_dir}"
            "-DFEATHERDOC_PDFIUM_OUT_DIR=${pdfium_out_dir}"
            "-DFEATHERDOC_NINJA_EXECUTABLE=${FEATHERDOC_NINJA_EXECUTABLE}"
            "-DFEATHERDOC_PDFIUM_NINJA_TARGET=${FEATHERDOC_PDFIUM_NINJA_TARGET}"
            "-DFEATHERDOC_DEPOT_TOOLS_DIR=${FEATHERDOC_DEPOT_TOOLS_DIR}"
            "-DFEATHERDOC_PDFIUM_VS_YEAR=${FEATHERDOC_PDFIUM_VS_YEAR}"
            "-DFEATHERDOC_PDFIUM_VS_INSTALL=${FEATHERDOC_PDFIUM_VS_INSTALL}"
            -P "${pdfium_runner}"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${pdfium_library}"
    )

    add_library(${target_name} STATIC IMPORTED GLOBAL)
    add_dependencies(${target_name} featherdoc_pdfium_source_build)
    set_target_properties(${target_name} PROPERTIES
        IMPORTED_LOCATION "${pdfium_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${pdfium_source_dir}/public"
    )

    set(pdfium_link_libraries)
    if(WIN32)
        list(APPEND pdfium_link_libraries winmm)
    endif()
    if(FEATHERDOC_PDFIUM_EXTRA_LIBS)
        list(APPEND pdfium_link_libraries ${FEATHERDOC_PDFIUM_EXTRA_LIBS})
    endif()
    if(pdfium_link_libraries)
        set_property(TARGET ${target_name} PROPERTY
            INTERFACE_LINK_LIBRARIES "${pdfium_link_libraries}")
    endif()
endfunction()
