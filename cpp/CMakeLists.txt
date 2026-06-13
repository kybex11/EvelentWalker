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
file(GLOB_RECURSE EVW_CORE_SOURCES
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

enable_testing()
add_test(NAME evw_tests COMMAND evw_tests)
