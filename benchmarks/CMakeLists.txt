add_executable(mpal_benchmarks
    benchmark_int128.cpp
    benchmark_uint128.cpp
)

target_compile_features(mpal_benchmarks PRIVATE cxx_std_23)
target_link_libraries(mpal_benchmarks PRIVATE MPAL benchmark::benchmark_main)
