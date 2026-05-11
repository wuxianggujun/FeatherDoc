include_guard(GLOBAL)

function(featherdoc_add_freetype_target)
    if(TARGET Freetype::Freetype)
        return()
    endif()

    find_package(Freetype QUIET)
    if(TARGET Freetype::Freetype)
        return()
    endif()

    set(freetype_hint_roots)

    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        get_filename_component(freetype_vcpkg_root "$ENV{VCPKG_ROOT}" ABSOLUTE)
        list(APPEND freetype_hint_roots
            "${freetype_vcpkg_root}/installed/x64-windows"
            "${freetype_vcpkg_root}/installed/x64-windows-static")
        file(GLOB freetype_installed_roots LIST_DIRECTORIES true
            "${freetype_vcpkg_root}/installed/*")
        list(APPEND freetype_hint_roots ${freetype_installed_roots})
    endif()

    list(REMOVE_DUPLICATES freetype_hint_roots)

    find_path(FEATHERDOC_FREETYPE_INCLUDE_DIR
        NAMES ft2build.h
        HINTS ${freetype_hint_roots}
        PATH_SUFFIXES include include/freetype2 freetype2
        DOC "FreeType include directory")

    find_library(FEATHERDOC_FREETYPE_LIBRARY_RELEASE
        NAMES freetype freetype2 libfreetype
        HINTS ${freetype_hint_roots}
        PATH_SUFFIXES lib lib64
        DOC "FreeType release library")

    find_library(FEATHERDOC_FREETYPE_LIBRARY_DEBUG
        NAMES freetyped freetyped libfreetyped
        HINTS ${freetype_hint_roots}
        PATH_SUFFIXES debug/lib lib/debug debug/lib64
        DOC "FreeType debug library")

    find_file(FEATHERDOC_FREETYPE_RUNTIME_RELEASE
        NAMES freetype.dll freetyped.dll libfreetype.dll
        HINTS ${freetype_hint_roots}
        PATH_SUFFIXES bin debug/bin
        DOC "FreeType release runtime DLL")

    find_file(FEATHERDOC_FREETYPE_RUNTIME_DEBUG
        NAMES freetyped.dll libfreetyped.dll freetype.dll
        HINTS ${freetype_hint_roots}
        PATH_SUFFIXES debug/bin bin/debug bin
        DOC "FreeType debug runtime DLL")

    if(NOT FEATHERDOC_FREETYPE_LIBRARY_DEBUG)
        set(FEATHERDOC_FREETYPE_LIBRARY_DEBUG
            "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}")
    endif()

    if(NOT FEATHERDOC_FREETYPE_INCLUDE_DIR OR
       NOT FEATHERDOC_FREETYPE_LIBRARY_RELEASE)
        message(FATAL_ERROR
            "Could not find FreeType. Set VCPKG_ROOT or point "
            "FEATHERDOC_FREETYPE_INCLUDE_DIR / FEATHERDOC_FREETYPE_LIBRARY_RELEASE "
            "to a valid installation.")
    endif()

    get_filename_component(freetype_library_extension
        "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}" EXT)
    string(TOLOWER "${freetype_library_extension}"
        freetype_library_extension)

    set(freetype_is_shared FALSE)
    if(freetype_library_extension STREQUAL ".so" OR
       freetype_library_extension STREQUAL ".dylib" OR
       freetype_library_extension STREQUAL ".dll" OR
       FEATHERDOC_FREETYPE_RUNTIME_RELEASE OR
       FEATHERDOC_FREETYPE_RUNTIME_DEBUG)
        set(freetype_is_shared TRUE)
    endif()

    if(freetype_is_shared)
        if(NOT FEATHERDOC_FREETYPE_LIBRARY_DEBUG)
            set(FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}")
        endif()
        if(NOT FEATHERDOC_FREETYPE_RUNTIME_RELEASE)
            if(FEATHERDOC_FREETYPE_RUNTIME_DEBUG)
                set(FEATHERDOC_FREETYPE_RUNTIME_RELEASE
                    "${FEATHERDOC_FREETYPE_RUNTIME_DEBUG}")
            else()
                set(FEATHERDOC_FREETYPE_RUNTIME_RELEASE
                    "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}")
            endif()
        endif()
        if(NOT FEATHERDOC_FREETYPE_RUNTIME_DEBUG)
            set(FEATHERDOC_FREETYPE_RUNTIME_DEBUG
                "${FEATHERDOC_FREETYPE_RUNTIME_RELEASE}")
        endif()

        add_library(Freetype::Freetype SHARED IMPORTED GLOBAL)
        set_target_properties(Freetype::Freetype PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            INTERFACE_INCLUDE_DIRECTORIES "${FEATHERDOC_FREETYPE_INCLUDE_DIR}"
            IMPORTED_LOCATION_RELEASE "${FEATHERDOC_FREETYPE_RUNTIME_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${FEATHERDOC_FREETYPE_RUNTIME_DEBUG}"
        )

        if(NOT freetype_library_extension STREQUAL ".so" AND
           NOT freetype_library_extension STREQUAL ".dylib" AND
           NOT freetype_library_extension STREQUAL ".dll")
            set_target_properties(Freetype::Freetype PROPERTIES
                IMPORTED_IMPLIB_RELEASE "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}"
                IMPORTED_IMPLIB_DEBUG "${FEATHERDOC_FREETYPE_LIBRARY_DEBUG}"
            )
        endif()
    else()
        add_library(Freetype::Freetype STATIC IMPORTED GLOBAL)
        set_target_properties(Freetype::Freetype PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            INTERFACE_INCLUDE_DIRECTORIES "${FEATHERDOC_FREETYPE_INCLUDE_DIR}"
            IMPORTED_LOCATION_RELEASE "${FEATHERDOC_FREETYPE_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${FEATHERDOC_FREETYPE_LIBRARY_DEBUG}"
        )
    endif()
endfunction()
