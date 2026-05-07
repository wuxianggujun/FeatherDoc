include_guard(GLOBAL)

include(FetchContent)

set(FEATHERDOC_PDFIO_VERSION "v1.6.3" CACHE STRING
    "PDFio git tag or commit used by the experimental PDF writer module")
set(FEATHERDOC_PDFIO_SOURCE_DIR "" CACHE PATH
    "Existing PDFio source checkout. Leave empty to let CMake fetch PDFio.")
option(FEATHERDOC_FETCH_PDFIO
    "Download PDFio when FEATHERDOC_PDFIO_SOURCE_DIR is not provided" ON)

get_filename_component(FEATHERDOC_PDFIO_HELPER_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(FEATHERDOC_PDFIO_REPO_ROOT "${FEATHERDOC_PDFIO_HELPER_DIR}/.." ABSOLUTE)

function(featherdoc_add_pdfio_object_library target_name)
    set(pdfio_source_dir "${FEATHERDOC_PDFIO_SOURCE_DIR}")

    if(pdfio_source_dir STREQUAL "")
        if(NOT FEATHERDOC_FETCH_PDFIO)
            message(FATAL_ERROR
                "FEATHERDOC_BUILD_PDF requires PDFio. Set "
                "FEATHERDOC_PDFIO_SOURCE_DIR or enable FEATHERDOC_FETCH_PDFIO.")
        endif()

        FetchContent_Declare(
            featherdoc_pdfio
            GIT_REPOSITORY https://github.com/michaelrsweet/pdfio.git
            GIT_TAG ${FEATHERDOC_PDFIO_VERSION}
        )
        FetchContent_GetProperties(featherdoc_pdfio)
        if(NOT featherdoc_pdfio_POPULATED)
            FetchContent_Populate(featherdoc_pdfio)
        endif()
        set(pdfio_source_dir "${featherdoc_pdfio_SOURCE_DIR}")
    endif()

    if(NOT EXISTS "${pdfio_source_dir}/pdfio.h")
        message(FATAL_ERROR
            "PDFio source directory does not contain pdfio.h: ${pdfio_source_dir}")
    endif()

    add_library(${target_name} OBJECT
        "${pdfio_source_dir}/pdfio-aes.c"
        "${pdfio_source_dir}/pdfio-array.c"
        "${pdfio_source_dir}/pdfio-common.c"
        "${pdfio_source_dir}/pdfio-content.c"
        "${pdfio_source_dir}/pdfio-crypto.c"
        "${pdfio_source_dir}/pdfio-dict.c"
        "${pdfio_source_dir}/pdfio-file.c"
        "${pdfio_source_dir}/pdfio-md5.c"
        "${pdfio_source_dir}/pdfio-object.c"
        "${pdfio_source_dir}/pdfio-page.c"
        "${pdfio_source_dir}/pdfio-rc4.c"
        "${pdfio_source_dir}/pdfio-sha256.c"
        "${pdfio_source_dir}/pdfio-stream.c"
        "${pdfio_source_dir}/pdfio-string.c"
        "${pdfio_source_dir}/pdfio-token.c"
        "${pdfio_source_dir}/pdfio-value.c"
        "${pdfio_source_dir}/ttf.c"
    )

    target_compile_features(${target_name} PRIVATE c_std_99)
    target_include_directories(${target_name}
        PUBLIC
            "$<BUILD_INTERFACE:${pdfio_source_dir}>"
    )
    set_target_properties(${target_name} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )

    if(MSVC)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${target_name} PRIVATE /W3)
    else()
        target_compile_options(${target_name} PRIVATE -w)
    endif()

    set(FEATHERDOC_RESOLVED_PDFIO_SOURCE_DIR "${pdfio_source_dir}" PARENT_SCOPE)
endfunction()
