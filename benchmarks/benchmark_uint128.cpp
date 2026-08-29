#include <benchmark/benchmark.h>

#include "uint128.h"

#include <string>

namespace {

const uint128_t kUintMax = uint128_t::max();
const uint128_t kUintLargeDivisor("18446744073709551617");
const uint128_t kUintMultiplier("11400714819323198485");

inline void DoNotOptimizeUint128Limbs(uint128_t& value) {
#if MPAL_HAS_NATIVE_I128
    auto low = u128_impl::low(value.v_);
    auto high = u128_impl::high(value.v_);
    benchmark::DoNotOptimize(low);
    benchmark::DoNotOptimize(high);
#else
    benchmark::DoNotOptimize(value.v_.low);
    benchmark::DoNotOptimize(value.v_.high);
#endif
}

void BM_Uint128_Add(benchmark::State& state) {
    uint128_t value = kUintMax - uint128_t(1024);
    uint128_t addend(11'400'714'819'323'198'485ULL);
    DoNotOptimizeUint128Limbs(addend);
    for (auto _ : state) {
        value += addend;
        DoNotOptimizeUint128Limbs(value);
    }
}
BENCHMARK(BM_Uint128_Add);

void BM_Uint128_Subtract(benchmark::State& state) {
    uint128_t value = kUintMax;
    uint128_t subtrahend(11'400'714'819'323'198'485ULL);
    DoNotOptimizeUint128Limbs(subtrahend);
    for (auto _ : state) {
        value -= subtrahend;
        DoNotOptimizeUint128Limbs(value);
    }
}
BENCHMARK(BM_Uint128_Subtract);

void BM_Uint128_Multiply(benchmark::State& state) {
    uint128_t value("170141183460469231731687303715884105727");
    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        uint128_t result = value * kUintMultiplier;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Uint128_Multiply);

void BM_Uint128_DivideBy64Bit(benchmark::State& state) {
    uint128_t dividend = kUintMax;
    uint128_t divisor(1'000'000'007U);
    DoNotOptimizeUint128Limbs(dividend);
    DoNotOptimizeUint128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        uint128_t result = dividend / divisor;
        DoNotOptimizeUint128Limbs(result);
    }
}
BENCHMARK(BM_Uint128_DivideBy64Bit);

void BM_Uint128_DivideBy128Bit(benchmark::State& state) {
    uint128_t dividend = kUintMax;
    uint128_t divisor = kUintLargeDivisor;
    DoNotOptimizeUint128Limbs(dividend);
    DoNotOptimizeUint128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        uint128_t result = dividend / divisor;
        DoNotOptimizeUint128Limbs(result);
    }
}
BENCHMARK(BM_Uint128_DivideBy128Bit);

void BM_Uint128_Modulo128Bit(benchmark::State& state) {
    uint128_t dividend = kUintMax;
    uint128_t divisor = kUintLargeDivisor;
    DoNotOptimizeUint128Limbs(dividend);
    DoNotOptimizeUint128Limbs(divisor);
    for (auto _ : state) {
        benchmark::DoNotOptimize(dividend);
        benchmark::DoNotOptimize(divisor);
        uint128_t result = dividend % divisor;
        DoNotOptimizeUint128Limbs(result);
    }
}
BENCHMARK(BM_Uint128_Modulo128Bit);

void BM_Uint128_Shift(benchmark::State& state) {
    uint128_t value = kUintMax;
    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        value = (value << 37U) | (value >> 91U);
        benchmark::DoNotOptimize(value);
    }
}
BENCHMARK(BM_Uint128_Shift);

void BM_Uint128_Compare(benchmark::State& state) {
    for (auto _ : state) {
        uint128_t lhs = kUintLargeDivisor;
        benchmark::DoNotOptimize(lhs);
        bool result = lhs < kUintMax;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Uint128_Compare);

void BM_Uint128_ParseDecimal(benchmark::State& state) {
    const std::string input = "340282366920938463463374607431768211455";
    for (auto _ : state) {
        uint128_t result(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Uint128_ParseDecimal);

void BM_Uint128_ToString(benchmark::State& state) {
    for (auto _ : state) {
        std::string result = kUintMax.ToString();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Uint128_ToString);

} // namespace
