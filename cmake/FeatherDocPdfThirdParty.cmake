include_guard(GLOBAL)

get_filename_component(FEATHERDOC_PDF_THIRD_PARTY_HELPER_DIR
    "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(FEATHERDOC_PDF_THIRD_PARTY_REPO_ROOT
    "${FEATHERDOC_PDF_THIRD_PARTY_HELPER_DIR}/.." ABSOLUTE)

set(FEATHERDOC_PDF_THIRD_PARTY_FREETYPE_DIR
    "${FEATHERDOC_PDF_THIRD_PARTY_REPO_ROOT}/tmp/pdfium-workspace/pdfium/third_party/freetype/src")
set(FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR
    "${FEATHERDOC_PDF_THIRD_PARTY_REPO_ROOT}/tmp/pdfium-workspace/pdfium/third_party/libpng")
set(FEATHERDOC_PDF_THIRD_PARTY_ZLIB_DIR
    "${FEATHERDOC_PDF_THIRD_PARTY_REPO_ROOT}/tmp/pdfium-workspace/pdfium/third_party/zlib")

function(featherdoc_add_harfbuzz_targets)
    set(harfbuzz_hint_roots)
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET AND
       NOT "${VCPKG_INSTALLED_DIR}" STREQUAL "" AND
       NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
        list(APPEND harfbuzz_hint_roots
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug")
    endif()
    if(DEFINED ENV{VCPKG_ROOT} AND DEFINED VCPKG_TARGET_TRIPLET AND
       NOT "$ENV{VCPKG_ROOT}" STREQUAL "" AND
       NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
        list(APPEND harfbuzz_hint_roots
            "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}"
            "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug")
    endif()
    list(APPEND harfbuzz_hint_roots
        "D:/Programs/vcpkg/installed/x64-windows"
        "D:/Programs/vcpkg/installed/x64-windows/debug")
    list(REMOVE_DUPLICATES harfbuzz_hint_roots)

    foreach(cache_name IN ITEMS
        HARFBUZZ_INCLUDE_DIR
        HARFBUZZ_LIBRARY_RELEASE
        HARFBUZZ_LIBRARY_DEBUG
        HARFBUZZ_SUBSET_LIBRARY_RELEASE
        HARFBUZZ_SUBSET_LIBRARY_DEBUG
    )
        if(DEFINED ${cache_name} AND
           ("${${cache_name}}" MATCHES "-NOTFOUND$" OR
            NOT EXISTS "${${cache_name}}"))
            unset(${cache_name} CACHE)
            unset(${cache_name})
        endif()
    endforeach()

    find_path(HARFBUZZ_INCLUDE_DIR
        NAMES hb-subset.h
        HINTS ${harfbuzz_hint_roots}
        PATH_SUFFIXES include include/harfbuzz harfbuzz
        DOC "Harfbuzz include directory")

    find_library(HARFBUZZ_LIBRARY_RELEASE
        NAMES harfbuzz
        HINTS ${harfbuzz_hint_roots}
        PATH_SUFFIXES lib
        DOC "Harfbuzz release library")
    find_library(HARFBUZZ_LIBRARY_DEBUG
        NAMES harfbuzz
        HINTS ${harfbuzz_hint_roots}
        PATH_SUFFIXES debug/lib lib/debug
        DOC "Harfbuzz debug library")

    find_library(HARFBUZZ_SUBSET_LIBRARY_RELEASE
        NAMES harfbuzz-subset
        HINTS ${harfbuzz_hint_roots}
        PATH_SUFFIXES lib
        DOC "Harfbuzz subset release library")
    find_library(HARFBUZZ_SUBSET_LIBRARY_DEBUG
        NAMES harfbuzz-subset
        HINTS ${harfbuzz_hint_roots}
        PATH_SUFFIXES debug/lib lib/debug
        DOC "Harfbuzz subset debug library")

    if(NOT HARFBUZZ_INCLUDE_DIR OR NOT HARFBUZZ_SUBSET_LIBRARY_RELEASE)
        set(FEATHERDOC_HARFBUZZ_SUBSET_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    if(NOT TARGET harfbuzz)
        add_library(harfbuzz UNKNOWN IMPORTED)
    endif()
    if(NOT TARGET harfbuzz::harfbuzz)
        add_library(harfbuzz::harfbuzz ALIAS harfbuzz)
    endif()

    if(HARFBUZZ_LIBRARY_RELEASE AND HARFBUZZ_LIBRARY_DEBUG)
        set_target_properties(harfbuzz PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${HARFBUZZ_LIBRARY_DEBUG}"
        )
    else()
        set_target_properties(harfbuzz PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_LIBRARY_RELEASE}"
        )
    endif()

    if(NOT TARGET harfbuzz::harfbuzz-subset)
        add_library(harfbuzz::harfbuzz-subset UNKNOWN IMPORTED)
    endif()

    if(HARFBUZZ_SUBSET_LIBRARY_RELEASE AND HARFBUZZ_SUBSET_LIBRARY_DEBUG)
        set_target_properties(harfbuzz::harfbuzz-subset PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_SUBSET_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${HARFBUZZ_SUBSET_LIBRARY_DEBUG}"
        )
    else()
        set_target_properties(harfbuzz::harfbuzz-subset PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_LOCATION_RELEASE "${HARFBUZZ_SUBSET_LIBRARY_RELEASE}"
        )
    endif()

    target_link_libraries(harfbuzz::harfbuzz-subset INTERFACE harfbuzz)
    set(FEATHERDOC_HARFBUZZ_SUBSET_AVAILABLE TRUE PARENT_SCOPE)
endfunction()

function(featherdoc_add_freetype_target)
    if(TARGET Freetype::Freetype)
        return()
    endif()

    if(NOT EXISTS "${FEATHERDOC_PDF_THIRD_PARTY_FREETYPE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Bundled FreeType source is missing. Expected: "
            "${FEATHERDOC_PDF_THIRD_PARTY_FREETYPE_DIR}")
    endif()

    set(FT_DISABLE_BZIP2 TRUE CACHE BOOL
        "Disable bzip2 support in the bundled FreeType build." FORCE)
    set(FT_DISABLE_BROTLI TRUE CACHE BOOL
        "Disable Brotli support in the bundled FreeType build." FORCE)
    set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL
        "Disable HarfBuzz support in the bundled FreeType build." FORCE)
    set(FT_DISABLE_HVF TRUE CACHE BOOL
        "Disable HVF support in the bundled FreeType build." FORCE)
    set(FT_DISABLE_PNG TRUE CACHE BOOL
        "Disable PNG support in the bundled FreeType build." FORCE)
    set(FT_DISABLE_ZLIB TRUE CACHE BOOL
        "Disable zlib support in the bundled FreeType build." FORCE)

    add_subdirectory(
        "${FEATHERDOC_PDF_THIRD_PARTY_FREETYPE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/freetype"
        EXCLUDE_FROM_ALL
    )

    if(WIN32 AND TARGET freetype)
        get_target_property(freetype_sources freetype SOURCES)
        if(freetype_sources)
            list(REMOVE_ITEM freetype_sources src/base/ftver.rc)
            set_target_properties(freetype PROPERTIES
                SOURCES "${freetype_sources}")
        endif()
    endif()

    if(TARGET freetype-interface AND NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype ALIAS freetype-interface)
    endif()
endfunction()

function(featherdoc_add_png_target target_name)
    if(TARGET ${target_name})
        return()
    endif()

    if(NOT EXISTS "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/png.h")
        message(FATAL_ERROR
            "Bundled libpng source is missing. Expected: "
            "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}")
    endif()

    if(NOT TARGET zlibstatic)
        featherdoc_add_zlib_target()
    endif()

    add_library(${target_name} STATIC
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/png.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngerror.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngget.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngmem.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngpread.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngread.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngrio.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngrtran.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngrutil.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngset.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngtrans.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngwio.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngwrite.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngwtran.c"
        "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}/pngwutil.c"
    )

    add_library(PNG::PNG ALIAS ${target_name})

    target_compile_features(${target_name} PUBLIC c_std_99)
    target_compile_definitions(${target_name} PRIVATE PNG_SET_OPTION_SUPPORTED)
    target_include_directories(${target_name}
        PUBLIC
            "${FEATHERDOC_PDF_THIRD_PARTY_LIBPNG_DIR}"
        PRIVATE
            "${FEATHERDOC_PDF_THIRD_PARTY_ZLIB_DIR}"
    )
    target_link_libraries(${target_name} PUBLIC zlibstatic)
endfunction()

function(featherdoc_add_zlib_target)
    if(TARGET zlibstatic)
        return()
    endif()

    if(NOT EXISTS "${FEATHERDOC_PDF_THIRD_PARTY_ZLIB_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Bundled zlib source is missing. Expected: "
            "${FEATHERDOC_PDF_THIRD_PARTY_ZLIB_DIR}")
    endif()

    set(ZLIB_BUILD_SHARED OFF CACHE BOOL
        "Disable the shared zlib build in the bundled PDF configuration." FORCE)
    set(ZLIB_BUILD_STATIC ON CACHE BOOL
        "Enable the static zlib build in the bundled PDF configuration." FORCE)
    set(ZLIB_INSTALL OFF CACHE BOOL
        "Disable zlib install rules in the bundled PDF configuration." FORCE)
    set(ZLIB_BUILD_TESTING OFF CACHE BOOL
        "Disable zlib tests in the bundled PDF configuration." FORCE)
    set(BUILD_UNITTESTS OFF CACHE BOOL
        "Disable zlib unit tests in the bundled PDF configuration." FORCE)
    set(BUILD_MINIZIP_BIN OFF CACHE BOOL
        "Disable zlib minizip binary in the bundled PDF configuration." FORCE)
    set(BUILD_ZPIPE OFF CACHE BOOL
        "Disable zlib zpipe tool in the bundled PDF configuration." FORCE)
    set(BUILD_MINIGZIP OFF CACHE BOOL
        "Disable zlib minigzip tool in the bundled PDF configuration." FORCE)
    set(ENABLE_SIMD_OPTIMIZATIONS OFF CACHE BOOL
        "Disable zlib SIMD optimizations in the bundled PDF configuration." FORCE)
    set(ENABLE_SIMD_AVX512 OFF CACHE BOOL
        "Disable zlib AVX512 optimizations in the bundled PDF configuration." FORCE)
    set(USE_ZLIB_RABIN_KARP_HASH OFF CACHE BOOL
        "Disable zlib rolling hash changes in the bundled PDF configuration." FORCE)
    set(ENABLE_INTEL_QAT_COMPRESSION OFF CACHE BOOL
        "Disable zlib QAT support in the bundled PDF configuration." FORCE)
    set(ZLIB_PREFIX OFF CACHE BOOL
        "Disable zlib symbol prefixing in the bundled PDF configuration." FORCE)

    add_subdirectory(
        "${FEATHERDOC_PDF_THIRD_PARTY_ZLIB_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/zlib"
        EXCLUDE_FROM_ALL
    )
endfunction()
