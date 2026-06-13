include("${CMAKE_CURRENT_LIST_DIR}/cmake/TestSupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CoreCppTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PdfCppTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PdfImportCppTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/PdfRegressionTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CliCoreCppTests.cmake")

if(WIN32)
    find_program(FEATHERDOC_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
    if(FEATHERDOC_POWERSHELL_EXECUTABLE)
        set(FEATHERDOC_POWERSHELL_TEST_COMMAND
            ${FEATHERDOC_POWERSHELL_EXECUTABLE}
            -NoLogo
            -NoProfile
            -NonInteractive
        )
        find_program(FEATHERDOC_PYTHON_EXECUTABLE NAMES python python3 py)

        include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsScriptReleaseTests.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsScriptPdfVisualTests.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsScriptReviewTests.cmake")
        include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsScriptVisualSyncTests.cmake")

        if(TARGET featherdoc_cli)
            include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsCliGovernanceScriptTests.cmake")
            include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsCliDocsScriptTests.cmake")
            include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsCliEditTemplateScriptTests.cmake")
        endif()

        if(TARGET featherdoc_cli AND TARGET featherdoc_sample_bookmark_block_visibility_visual AND TARGET featherdoc_sample_part_template_validation)
            include("${CMAKE_CURRENT_LIST_DIR}/cmake/WindowsCliTemplateRenderScriptTests.cmake")
        endif()
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/CrossPlatformScriptTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CliExecutableCppTests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/TestPostlude.cmake")
