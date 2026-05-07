if(NOT DEFINED FEATHERDOC_COPY_RUNTIME_DLLS_DEST OR
   FEATHERDOC_COPY_RUNTIME_DLLS_DEST STREQUAL "")
    return()
endif()

if(NOT DEFINED FEATHERDOC_COPY_RUNTIME_DLLS_DLLS OR
   FEATHERDOC_COPY_RUNTIME_DLLS_DLLS STREQUAL "")
    return()
endif()

file(MAKE_DIRECTORY "${FEATHERDOC_COPY_RUNTIME_DLLS_DEST}")

foreach(runtime_dll IN LISTS FEATHERDOC_COPY_RUNTIME_DLLS_DLLS)
    if(NOT EXISTS "${runtime_dll}")
        continue()
    endif()

    get_filename_component(runtime_dll_name "${runtime_dll}" NAME)
    file(COPY_FILE
        "${runtime_dll}"
        "${FEATHERDOC_COPY_RUNTIME_DLLS_DEST}/${runtime_dll_name}"
        ONLY_IF_DIFFERENT
    )
endforeach()
