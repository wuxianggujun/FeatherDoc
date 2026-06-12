function(featherdoc_add_cpp_test target_name test_name)
    add_executable(${target_name} ${ARGN})

    target_link_libraries(
        ${target_name}
        PRIVATE FeatherDoc::FeatherDoc
    )
    featherdoc_copy_runtime_dlls(${target_name})

    add_test(
        NAME
        ${test_name}
        COMMAND
        ${target_name}
    )
endfunction()

function(featherdoc_set_test_labels test_name)
    set_tests_properties(
        ${test_name}
        PROPERTIES
        LABELS
        "${ARGN}"
    )
    if("pdf" IN_LIST ARGN)
        # PDF tests may touch font, renderer, or fixture stacks; keep CTest bounded.
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
    endif()
endfunction()

function(featherdoc_get_runtime_dir out_var target_name)
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

    get_filename_component(runtime_dir "${runtime_location}" DIRECTORY)
    set(${out_var} "${runtime_dir}" PARENT_SCOPE)
endfunction()

function(featherdoc_build_pdf_test_environment out_var)
    set(runtime_dirs)

    featherdoc_get_runtime_dir(png_runtime_dir PNG::PNG)
    if(NOT png_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${png_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(freetype_runtime_dir Freetype::Freetype)
    if(NOT freetype_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${freetype_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(harfbuzz_runtime_dir harfbuzz::harfbuzz)
    if(NOT harfbuzz_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_runtime_dir}")
    endif()

    featherdoc_get_runtime_dir(harfbuzz_subset_runtime_dir
        harfbuzz::harfbuzz-subset)
    if(NOT harfbuzz_subset_runtime_dir STREQUAL "")
        list(APPEND runtime_dirs "${harfbuzz_subset_runtime_dir}")
    endif()

    if(DEFINED FEATHERDOC_RESOLVED_PDFIUM_OUT_DIR AND
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

set(FEATHERDOC_PDF_TEST_ENVIRONMENT "")
featherdoc_build_pdf_test_environment(FEATHERDOC_PDF_TEST_ENVIRONMENT)
