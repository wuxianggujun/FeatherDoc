if(NOT DEFINED FEATHERDOC_PDFIUM_RUN_STEP)
    message(FATAL_ERROR "FEATHERDOC_PDFIUM_RUN_STEP is required")
endif()

if(NOT DEFINED FEATHERDOC_PDFIUM_SOURCE_DIR)
    message(FATAL_ERROR "FEATHERDOC_PDFIUM_SOURCE_DIR is required")
endif()

function(featherdoc_check_tcp_connectivity host port timeout_seconds result_var)
    if(WIN32)
        execute_process(
            COMMAND powershell -NoProfile -NonInteractive
                -ExecutionPolicy Bypass
                -Command
                "$hostName='${host}'; $port=${port}; "
                "$timeoutMs=${timeout_seconds} * 1000; "
                "$client=New-Object System.Net.Sockets.TcpClient; "
                "try { "
                "  $async=$client.BeginConnect($hostName, $port, $null, $null); "
                "  if(-not $async.AsyncWaitHandle.WaitOne($timeoutMs)) { exit 2 } "
                "  $null=$client.EndConnect($async); "
                "  exit 0 "
                "} catch { "
                "  exit 3 "
                "} finally { "
                "  $client.Dispose() "
                "}"
            RESULT_VARIABLE connectivity_result
            OUTPUT_QUIET
            ERROR_QUIET)
    else()
        set(connectivity_result 0)
    endif()

    set(${result_var} "${connectivity_result}" PARENT_SCOPE)
endfunction()

set(ENV{DEPOT_TOOLS_WIN_TOOLCHAIN} "0")

if(DEFINED FEATHERDOC_DEPOT_TOOLS_DIR AND NOT FEATHERDOC_DEPOT_TOOLS_DIR STREQUAL "")
    if(WIN32)
        set(ENV{PATH} "${FEATHERDOC_DEPOT_TOOLS_DIR};$ENV{PATH}")
    else()
        set(ENV{PATH} "${FEATHERDOC_DEPOT_TOOLS_DIR}:$ENV{PATH}")
    endif()

    set(depot_tools_python_marker
        "${FEATHERDOC_DEPOT_TOOLS_DIR}/python3_bin_reldir.txt")
    if(NOT EXISTS "${depot_tools_python_marker}")
        if(WIN32)
            set(depot_tools_bootstrap
                "${FEATHERDOC_DEPOT_TOOLS_DIR}/bootstrap/win_tools.bat")
            set(depot_tools_bootstrap_command
                cmd /c "${depot_tools_bootstrap}")
        else()
            set(depot_tools_bootstrap
                "${FEATHERDOC_DEPOT_TOOLS_DIR}/update_depot_tools")
            set(depot_tools_bootstrap_command "${depot_tools_bootstrap}")
        endif()

        if(NOT EXISTS "${depot_tools_bootstrap}")
            message(FATAL_ERROR
                "depot_tools is present but not initialized, and the bootstrap "
                "script is missing: ${depot_tools_bootstrap}")
        endif()

        featherdoc_check_tcp_connectivity(
            "chrome-infra-packages.appspot.com" 443 8
            depot_tools_cipd_connectivity_result)
        if(NOT depot_tools_cipd_connectivity_result EQUAL 0)
            message(FATAL_ERROR
                "depot_tools bootstrap requires access to "
                "chrome-infra-packages.appspot.com:443, but the current "
                "environment cannot reach that CIPD backend. Initialize "
                "depot_tools manually or restore network access before "
                "building PDFium from source.")
        endif()

        message(STATUS
            "Bootstrapping depot_tools because "
            "python3_bin_reldir.txt is missing")
        execute_process(
            COMMAND ${depot_tools_bootstrap_command}
            WORKING_DIRECTORY "${FEATHERDOC_DEPOT_TOOLS_DIR}"
            RESULT_VARIABLE depot_tools_bootstrap_result
            OUTPUT_VARIABLE depot_tools_bootstrap_stdout
            ERROR_VARIABLE depot_tools_bootstrap_stderr
            TIMEOUT 180)
        if(NOT depot_tools_bootstrap_result EQUAL 0)
            message(FATAL_ERROR
                "depot_tools bootstrap failed with exit code "
                "${depot_tools_bootstrap_result}\n"
                "stdout:\n${depot_tools_bootstrap_stdout}\n"
                "stderr:\n${depot_tools_bootstrap_stderr}")
        endif()
        if(NOT EXISTS "${depot_tools_python_marker}")
            message(FATAL_ERROR
                "depot_tools bootstrap completed but "
                "python3_bin_reldir.txt is still missing: "
                "${depot_tools_python_marker}")
        endif()
    endif()
endif()

if(WIN32
        AND DEFINED FEATHERDOC_PDFIUM_VS_YEAR
        AND NOT FEATHERDOC_PDFIUM_VS_YEAR STREQUAL ""
        AND DEFINED FEATHERDOC_PDFIUM_VS_INSTALL
        AND NOT FEATHERDOC_PDFIUM_VS_INSTALL STREQUAL "")
    set("ENV{vs${FEATHERDOC_PDFIUM_VS_YEAR}_install}"
        "${FEATHERDOC_PDFIUM_VS_INSTALL}")
endif()

if(FEATHERDOC_PDFIUM_RUN_STEP STREQUAL "configure")
    execute_process(
        COMMAND "${FEATHERDOC_GN_EXECUTABLE}" gen
            "${FEATHERDOC_PDFIUM_OUT_DIR}"
            "--args=${FEATHERDOC_PDFIUM_GN_ARGS}"
        WORKING_DIRECTORY "${FEATHERDOC_PDFIUM_SOURCE_DIR}"
        RESULT_VARIABLE result)
elseif(FEATHERDOC_PDFIUM_RUN_STEP STREQUAL "build")
    execute_process(
        COMMAND "${FEATHERDOC_NINJA_EXECUTABLE}" -C
            "${FEATHERDOC_PDFIUM_OUT_DIR}"
            "${FEATHERDOC_PDFIUM_NINJA_TARGET}"
        WORKING_DIRECTORY "${FEATHERDOC_PDFIUM_SOURCE_DIR}"
        RESULT_VARIABLE result)
else()
    message(FATAL_ERROR
        "Unsupported FEATHERDOC_PDFIUM_RUN_STEP: ${FEATHERDOC_PDFIUM_RUN_STEP}")
endif()

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "PDFium ${FEATHERDOC_PDFIUM_RUN_STEP} step failed with exit code ${result}")
endif()
