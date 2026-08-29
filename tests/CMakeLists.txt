add_executable(mpal_tests
    test_utils.cpp
    test_uint128.cpp
    test_int128.cpp
    test_int128_standalone.cpp
)

target_include_directories(mpal_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(mpal_tests PRIVATE cxx_std_23)
target_link_libraries(mpal_tests PRIVATE MPAL GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(mpal_tests)
