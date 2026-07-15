if(NOT FEATHERDOC_BUILD_FUZZERS)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "FEATHERDOC_BUILD_FUZZERS requires Clang libFuzzer")
endif()
if(NOT FEATHERDOC_ENABLE_SANITIZERS)
    message(FATAL_ERROR
        "FEATHERDOC_BUILD_FUZZERS requires FEATHERDOC_ENABLE_SANITIZERS=ON")
endif()
if(NOT TARGET FeatherDocCliCore)
    message(FATAL_ERROR "FeatherDocCliCore is required by the JSON fuzz target")
endif()

function(featherdoc_add_fuzzer target_name source_file)
    add_executable(${target_name} "${source_file}")
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_options(${target_name} PRIVATE -fsanitize=fuzzer-no-link)
    target_link_options(${target_name} PRIVATE -fsanitize=fuzzer)
    set_target_properties(${target_name} PROPERTIES
        CXX_EXTENSIONS NO
        CXX_STANDARD_REQUIRED YES)
endfunction()

featherdoc_add_fuzzer(
    featherdoc_docx_open_fuzzer
    "${PROJECT_SOURCE_DIR}/fuzz/docx_open_fuzzer.cpp")
target_link_libraries(
    featherdoc_docx_open_fuzzer PRIVATE FeatherDoc::FeatherDoc)

featherdoc_add_fuzzer(
    featherdoc_cli_json_fuzzer
    "${PROJECT_SOURCE_DIR}/fuzz/cli_json_fuzzer.cpp")
target_link_libraries(
    featherdoc_cli_json_fuzzer PRIVATE FeatherDocCliCore)
