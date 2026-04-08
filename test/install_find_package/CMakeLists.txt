cmake_minimum_required(VERSION 3.20)

project(FeatherDocInstallSmoke LANGUAGES CXX)

find_package(FeatherDoc CONFIG REQUIRED)

add_executable(featherdoc_install_smoke main.cpp)
target_link_libraries(featherdoc_install_smoke PRIVATE FeatherDoc::FeatherDoc)
target_compile_features(featherdoc_install_smoke PRIVATE cxx_std_20)

set_target_properties(
    featherdoc_install_smoke
    PROPERTIES
        CXX_EXTENSIONS NO
        CXX_STANDARD_REQUIRED YES
)
