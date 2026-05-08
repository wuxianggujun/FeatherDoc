include_guard(GLOBAL)

include(ExternalProject)

set(FEATHERDOC_PDFIUM_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(FEATHERDOC_PDFIUM_PROVIDER "auto" CACHE STRING
    "PDFium provider for the experimental PDF import module: auto, source, package, or prebuilt")
set_property(CACHE FEATHERDOC_PDFIUM_PROVIDER PROPERTY STRINGS auto source package prebuilt)

set(FEATHERDOC_PDFIUM_SOURCE_DIR "" CACHE PATH
    "Existing PDFium source checkout root. It must contain public/fpdfview.h.")
set(FEATHERDOC_PDFIUM_OUT_DIR "" CACHE PATH
    "PDFium GN output directory. Defaults to <PDFium source>/out/FeatherDoc.")
set(FEATHERDOC_PDFIUM_LIBRARY "" CACHE FILEPATH
    "Prebuilt PDFium library or import library path used by the prebuilt provider")
set(FEATHERDOC_PDFIUM_INCLUDE_DIR "" CACHE PATH
    "Directory containing fpdfview.h used by the prebuilt provider")
set(FEATHERDOC_PDFIUM_RUNTIME_DLL "" CACHE FILEPATH
    "Optional PDFium runtime DLL used by the prebuilt provider on Windows")
set(FEATHERDOC_PDFIUM_RUNTIME_DIR "" CACHE PATH
    "Optional directory containing PDFium runtime binaries used by the prebuilt provider")
set(FEATHERDOC_PDFIUM_GN_ARGS
    "is_debug=false is_component_build=false pdf_is_complete_lib=true pdf_is_standalone=true pdf_enable_v8=false pdf_enable_xfa=false pdf_use_skia=false pdf_enable_fontations=false pdf_use_system_freetype=true clang_use_chrome_plugins=false use_custom_libcxx=false"
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

function(featherdoc_try_find_pdfium_package out_target)
    find_package(PDFium QUIET)
    if(NOT PDFium_FOUND)
        set(${out_target} "" PARENT_SCOPE)
        return()
    endif()

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

function(featherdoc_has_pdfium_prebuilt_inputs out_var)
    if(NOT FEATHERDOC_PDFIUM_LIBRARY STREQUAL "" AND
       NOT FEATHERDOC_PDFIUM_INCLUDE_DIR STREQUAL "")
        set(${out_var} ON PARENT_SCOPE)
    else()
        set(${out_var} OFF PARENT_SCOPE)
    endif()
endfunction()

function(featherdoc_add_pdfium_source_target target_name)
    if(FEATHERDOC_PDFIUM_SOURCE_DIR STREQUAL "")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_PROVIDER=source requires FEATHERDOC_PDFIUM_SOURCE_DIR. "
            "Point it to a PDFium checkout created with:\n"
            "  gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git\n"
            "  gclient sync\n"
            "Or set FEATHERDOC_PDFIUM_PROVIDER to package/prebuilt and provide "
            "the matching PDFium inputs.")
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

    set(FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR "${pdfium_out_dir}" PARENT_SCOPE)
    set(FEATHERDOC_RESOLVED_PDFIUM_RUNTIME_DIR "${pdfium_out_dir}" PARENT_SCOPE)
endfunction()

function(featherdoc_add_pdfium_prebuilt_target target_name)
    if(FEATHERDOC_PDFIUM_LIBRARY STREQUAL "")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_PROVIDER=prebuilt requires FEATHERDOC_PDFIUM_LIBRARY "
            "to point to pdfium.lib, libpdfium.a, or another linkable PDFium library.")
    endif()

    if(FEATHERDOC_PDFIUM_INCLUDE_DIR STREQUAL "")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_PROVIDER=prebuilt requires FEATHERDOC_PDFIUM_INCLUDE_DIR "
            "to point to the directory containing fpdfview.h.")
    endif()

    if(NOT WIN32 AND NOT FEATHERDOC_PDFIUM_RUNTIME_DLL STREQUAL "")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_RUNTIME_DLL is only supported on Windows. "
            "Use FEATHERDOC_PDFIUM_LIBRARY and optionally FEATHERDOC_PDFIUM_RUNTIME_DIR.")
    endif()

    get_filename_component(pdfium_library
        "${FEATHERDOC_PDFIUM_LIBRARY}" ABSOLUTE)
    if(NOT EXISTS "${pdfium_library}")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_LIBRARY does not exist: ${pdfium_library}")
    endif()

    get_filename_component(pdfium_include_dir
        "${FEATHERDOC_PDFIUM_INCLUDE_DIR}" ABSOLUTE)
    if(NOT EXISTS "${pdfium_include_dir}/fpdfview.h")
        message(FATAL_ERROR
            "FEATHERDOC_PDFIUM_INCLUDE_DIR must contain fpdfview.h: "
            "${pdfium_include_dir}")
    endif()

    set(pdfium_runtime_dll "")
    set(pdfium_runtime_dir "")
    if(NOT FEATHERDOC_PDFIUM_RUNTIME_DLL STREQUAL "")
        get_filename_component(pdfium_runtime_dll
            "${FEATHERDOC_PDFIUM_RUNTIME_DLL}" ABSOLUTE)
        if(NOT EXISTS "${pdfium_runtime_dll}")
            message(FATAL_ERROR
                "FEATHERDOC_PDFIUM_RUNTIME_DLL does not exist: "
                "${pdfium_runtime_dll}")
        endif()
        get_filename_component(pdfium_runtime_dir
            "${pdfium_runtime_dll}" DIRECTORY)
    elseif(NOT FEATHERDOC_PDFIUM_RUNTIME_DIR STREQUAL "")
        get_filename_component(pdfium_runtime_dir
            "${FEATHERDOC_PDFIUM_RUNTIME_DIR}" ABSOLUTE)
        if(NOT IS_DIRECTORY "${pdfium_runtime_dir}")
            message(FATAL_ERROR
                "FEATHERDOC_PDFIUM_RUNTIME_DIR must be an existing directory: "
                "${pdfium_runtime_dir}")
        endif()
    endif()

    get_filename_component(pdfium_library_dir "${pdfium_library}" DIRECTORY)
    if(pdfium_runtime_dir STREQUAL "")
        set(pdfium_runtime_dir "${pdfium_library_dir}")
    endif()

    if(WIN32 AND NOT pdfium_runtime_dll STREQUAL "")
        add_library(${target_name} SHARED IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_IMPLIB "${pdfium_library}"
            IMPORTED_LOCATION "${pdfium_runtime_dll}"
            INTERFACE_INCLUDE_DIRECTORIES "${pdfium_include_dir}"
        )
    else()
        add_library(${target_name} UNKNOWN IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION "${pdfium_library}"
            INTERFACE_INCLUDE_DIRECTORIES "${pdfium_include_dir}"
        )
    endif()

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

    set(FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR "${pdfium_library_dir}" PARENT_SCOPE)
    set(FEATHERDOC_RESOLVED_PDFIUM_RUNTIME_DIR "${pdfium_runtime_dir}" PARENT_SCOPE)
endfunction()
