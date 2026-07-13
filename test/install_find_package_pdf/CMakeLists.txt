cmake_minimum_required(VERSION 3.20)

project(FeatherDocInstallPdfSmoke LANGUAGES CXX)

find_package(FeatherDoc CONFIG REQUIRED COMPONENTS Core Pdf)

add_executable(featherdoc_install_pdf_smoke main.cpp)
target_link_libraries(featherdoc_install_pdf_smoke PRIVATE FeatherDoc::Pdf)
target_compile_features(featherdoc_install_pdf_smoke PRIVATE cxx_std_20)

set_target_properties(
    featherdoc_install_pdf_smoke
    PROPERTIES
        CXX_EXTENSIONS NO
        CXX_STANDARD_REQUIRED YES
)
