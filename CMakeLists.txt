cmake_minimum_required(VERSION 3.20)
project(MPAL VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(MPAL_BUILD_BENCHMARKS "Build MPAL microbenchmarks" OFF)
option(MPAL_BUILD_MAIN "Build the MPAL example executable" ON)
option(MPAL_USE_NATIVE_INT128 "Use compiler-native 128-bit integers when available" ON)

add_library(MPAL STATIC 
    src/BigInt.cpp
    src/utils.cpp
)
target_compile_features(MPAL PUBLIC cxx_std_23)
set_target_properties(MPAL PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(MPAL
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(MPAL
    PUBLIC MPAL_USE_NATIVE_INT128=$<BOOL:${MPAL_USE_NATIVE_INT128}>
)

if(MPAL_BUILD_MAIN)
    add_executable(main main.cpp)
    target_link_libraries(main MPAL)
endif()

if(BUILD_TESTING OR MPAL_BUILD_BENCHMARKS)
    find_package(Git REQUIRED)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
endif()

if(BUILD_TESTING)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
    )
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

if(MPAL_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
    )
    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()
