# Runtime DLL helper functions for Windows test/sample/CLI targets.
# Included from the top-level CMakeLists.txt after the core target is declared.


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

    set(runtime_property_names
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
        IMPORTED_IMPLIB)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(runtime_property_names
            IMPORTED_LOCATION_DEBUG
            IMPORTED_IMPLIB_DEBUG
            IMPORTED_LOCATION_RELEASE
            IMPORTED_LOCATION_RELWITHDEBINFO
            IMPORTED_LOCATION_MINSIZEREL
            IMPORTED_LOCATION_NOCONFIG
            IMPORTED_LOCATION
            IMPORTED_IMPLIB_RELEASE
            IMPORTED_IMPLIB_RELWITHDEBINFO
            IMPORTED_IMPLIB_MINSIZEREL
            IMPORTED_IMPLIB_NOCONFIG
            IMPORTED_IMPLIB)
    endif()

    set(runtime_location "")
    foreach(property_name IN LISTS runtime_property_names)
        if(runtime_location STREQUAL "")
            get_target_property(runtime_location ${target_name} ${property_name})
            if(runtime_location STREQUAL "runtime_location-NOTFOUND")
                set(runtime_location "")
            endif()
        endif()
    endforeach()

    if(runtime_location STREQUAL "")
        if(target_name STREQUAL "PNG::PNG")
            set(runtime_variable_names PNG_LIBRARY_RELEASE PNG_LIBRARY_DEBUG PNG_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names PNG_LIBRARY_DEBUG PNG_LIBRARY_RELEASE PNG_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "Freetype::Freetype")
            set(runtime_variable_names
                FREETYPE_LIBRARY_RELEASE
                FREETYPE_LIBRARY_DEBUG
                FEATHERDOC_FREETYPE_LIBRARY_RELEASE
                FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                FREETYPE_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    FREETYPE_LIBRARY_DEBUG
                    FEATHERDOC_FREETYPE_LIBRARY_DEBUG
                    FREETYPE_LIBRARY_RELEASE
                    FEATHERDOC_FREETYPE_LIBRARY_RELEASE
                    FREETYPE_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz-subset")
            set(runtime_variable_names
                HARFBUZZ_SUBSET_LIBRARY_RELEASE
                HARFBUZZ_SUBSET_LIBRARY_DEBUG
                HARFBUZZ_SUBSET_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    HARFBUZZ_SUBSET_LIBRARY_DEBUG
                    HARFBUZZ_SUBSET_LIBRARY_RELEASE
                    HARFBUZZ_SUBSET_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
                if(runtime_location STREQUAL "" AND DEFINED ${variable_name})
                    set(runtime_location "${${variable_name}}")
                    if(runtime_location MATCHES "-NOTFOUND$")
                        set(runtime_location "")
                    endif()
                endif()
            endforeach()
        elseif(target_name STREQUAL "harfbuzz::harfbuzz")
            set(runtime_variable_names
                HARFBUZZ_LIBRARY_RELEASE
                HARFBUZZ_LIBRARY_DEBUG
                HARFBUZZ_LIBRARY)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(runtime_variable_names
                    HARFBUZZ_LIBRARY_DEBUG
                    HARFBUZZ_LIBRARY_RELEASE
                    HARFBUZZ_LIBRARY)
            endif()
            foreach(variable_name IN LISTS runtime_variable_names)
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
