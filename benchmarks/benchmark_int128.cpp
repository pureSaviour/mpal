#include <benchmark/benchmark.h>

#include "int128.h"

#include <string>

namespace {

const int128_t kIntMax = int128_t::max();
const int128_t kIntMin = int128_t::min();
const int128_t kIntLargeDivisor("18446744073709551617");
const int128_t kIntMultiplier("11400714819323198485");

inline void DoNotOptimizeInt128Limbs(int128_t& value) {
#if MPAL_HAS_NATIVE_I128
    auto low = i128_impl::low(value.v_);
    auto high = i128_impl::high(value.v_);
    benchmark::DoNotOptimize(low);
    benchmark::DoNotOptimize(high);
#else
    benchmark::DoNotOptimize(value.v_.low);
    benchmark::DoNotOptimize(value.v_.high);
#endif
}

void BM_Int128_Add(benchmark::State& state) {
    int128_t value = kIntMax - int128_t(1024);
    int128_t addend(11'400'714'819'323'198'485ULL);
    DoNotOptimizeInt128Limbs(addend);
    for (auto _ : state) {
        value += addend;
        DoNotOptimizeInt128Limbs(value);
    }
}
BENCHMARK(BM_Int128_Add);

void BM_Int128_Subtract(benchmark::State& state) {
    int128_t value = kIntMin;
    int128_t subtrahend(11'400'714'819'323'198'485ULL);
    DoNotOptimizeInt128Limbs(subtrahend);
    for (auto _ : state) {
        value -= subtrahend;
        DoNotOptimizeInt128Limbs(value);
    }
}
BENCHMARK(BM_Int128_Subtract);

void BM_Int128_Multiply(benchmark::State& state) {
    int128_t value("-85070591730234615865843651857942052863");
    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        int128_t result = value * kIntMultiplier;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Int128_Multiply);

void BM_Int128_DividePositive(benchmark::State& state) {
    int128_t dividend = kIntMax;
    int128_t divisor = kIntLargeDivisor;
    DoNotOptimizeInt128Limbs(dividend);
    DoNotOptimizeInt128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        int128_t result = dividend / divisor;
        DoNotOptimizeInt128Limbs(result);
    }
}
BENCHMARK(BM_Int128_DividePositive);

void BM_Int128_DivideNegative(benchmark::State& state) {
    int128_t dividend = -kIntMax;
    int128_t divisor = kIntLargeDivisor;
    DoNotOptimizeInt128Limbs(dividend);
    DoNotOptimizeInt128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        int128_t result = dividend / divisor;
        DoNotOptimizeInt128Limbs(result);
    }
}
BENCHMARK(BM_Int128_DivideNegative);

void BM_Int128_Modulo(benchmark::State& state) {
    int128_t dividend = -kIntMax;
    int128_t divisor = kIntLargeDivisor;
    DoNotOptimizeInt128Limbs(dividend);
    DoNotOptimizeInt128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        int128_t result = dividend % divisor;
        DoNotOptimizeInt128Limbs(result);
    }
}
BENCHMARK(BM_Int128_Modulo);

void BM_Int128_ArithmeticShift(benchmark::State& state) {
    int128_t value = -kIntMax;
    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        value = (value >> 17U) ^ (value << 29U);
        benchmark::DoNotOptimize(value);
    }
}
BENCHMARK(BM_Int128_ArithmeticShift);

void BM_Int128_Compare(benchmark::State& state) {
    for (auto _ : state) {
        int128_t lhs = kIntMin;
        benchmark::DoNotOptimize(lhs);
        bool result = lhs < kIntMax;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Int128_Compare);

void BM_Int128_ParseDecimal(benchmark::State& state) {
    const std::string input = "-170141183460469231731687303715884105728";
    for (auto _ : state) {
        int128_t result(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Int128_ParseDecimal);

void BM_Int128_ToStringMax(benchmark::State& state) {
    for (auto _ : state) {
        std::string result = kIntMax.ToString();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Int128_ToStringMax);

void BM_Int128_ToStringMinFastPath(benchmark::State& state) {
    for (auto _ : state) {
        std::string result = kIntMin.ToString();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Int128_ToStringMinFastPath);

} // namespace
