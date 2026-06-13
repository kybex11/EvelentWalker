cmake_minimum_required(VERSION 3.20)
project(EvelentWalkerCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ---- Core library (mirror of EvelentWalker.Core) ----
file(GLOB_RECURSE EVW_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.cpp")

add_library(evw_core STATIC ${EVW_CORE_SOURCES})
target_include_directories(evw_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/core/include")

# ---- Self-test executable ----
add_executable(evw_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp")
target_link_libraries(evw_tests PRIVATE evw_core)

# ---- Command-line tool ----
add_executable(evw_cli "${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp")
target_link_libraries(evw_cli PRIVATE evw_core)

# ---- ImGui GUI (RPF explorer), optional ----
# Uses Dear ImGui with the Win32 + Direct3D 11 backends (matches CodeWalker's
# DX11 renderer; HLSL shaders can be reused). Only ImGui is fetched; d3d11/dxgi/
# d3dcompiler come from the Windows SDK.
# Enable with: cmake -S . -B build -DEVW_BUILD_GUI=ON
option(EVW_BUILD_GUI "Build the Dear ImGui (Win32 + D3D11) front-end" OFF)
if(EVW_BUILD_GUI)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.5)
    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp")
    target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")

    add_executable(evw_gui WIN32
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/viewport.cpp")
    target_link_libraries(evw_gui PRIVATE evw_core imgui
        d3d11 dxgi d3dcompiler dwmapi)
endif()

enable_testing()
add_test(NAME evw_tests COMMAND evw_tests)
