#include <benchmark/benchmark.h>

#include "BigInt.h"

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

using mpal::BigInt;

std::string HexWithLimbs(std::size_t limbs, uint32_t seed) {
    std::ostringstream text;
    text << "0x" << std::hex << (seed | 1U);
    uint32_t value = seed;
    for(std::size_t i = 1; i < limbs; ++i) {
        value = value * 1664525U + 1013904223U;
        text << std::setw(8) << std::setfill('0') << value;
    }
    return text.str();
}

void BigIntLimbRanges(benchmark::internal::Benchmark* benchmark) {
    for(int limbs : {1, 4, 16, 32, 64, 128, 256})
        benchmark->Arg(limbs);
}

void BigIntMultiplyRanges(benchmark::internal::Benchmark* benchmark) {
    for(int limbs : {1, 4, 16, 32, 33, 48, 64, 96, 128, 256, 512, 1024})
        benchmark->Arg(limbs);
}

void BM_BigInt_Add(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(limbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = lhs + rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_Add)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_AddOppositeSigns(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs = -BigInt(HexWithLimbs(limbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = lhs + rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_AddOppositeSigns)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_Subtract(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(limbs, 0x71234567U));
    for(auto _ : state) {
        BigInt result = lhs - rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_Subtract)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_Compare(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    std::string lhsText = HexWithLimbs(limbs, 0xF1234567U);
    std::string rhsText = lhsText;
    rhsText.back() = rhsText.back() == 'f' ? 'e' : 'f';
    const BigInt lhs(lhsText);
    const BigInt rhs(rhsText);
    for(auto _ : state) {
        bool result = lhs < rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_Compare)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_MultiplyAdaptive(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(limbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = lhs * rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_MultiplyAdaptive)->Apply(BigIntMultiplyRanges)->Complexity();

void BM_BigInt_MultiplyByPowerOfTwo(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs(65536);
    for(auto _ : state) {
        BigInt result = lhs * rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_MultiplyByPowerOfTwo)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_MultiplyExplicit(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(limbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = BigInt::multiply(lhs, rhs);
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_MultiplyExplicit)->Apply(BigIntMultiplyRanges)->Complexity();

void BM_BigInt_MultiplyKaratsubaUnbalanced(benchmark::State& state) {
    const auto longLimbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(33, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(longLimbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = BigInt::multiply(lhs, rhs);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(33 * longLimbs));
}
BENCHMARK(BM_BigInt_MultiplyKaratsubaUnbalanced)->Arg(64)->Arg(128)->Arg(256)->Arg(512);

void BM_BigInt_MultiplyChunkedUnbalanced(benchmark::State& state) {
    const auto longLimbs = static_cast<std::size_t>(state.range(0));
    const BigInt lhs(HexWithLimbs(96, 0xF1234567U));
    const BigInt rhs(HexWithLimbs(longLimbs, 0x89ABCDEFU));
    for(auto _ : state) {
        BigInt result = lhs * rhs;
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(96 * longLimbs));
}
BENCHMARK(BM_BigInt_MultiplyChunkedUnbalanced)->Arg(256)->Arg(512)->Arg(1024)->Arg(2048);

void BM_BigInt_ParseDecimal(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const std::string decimal = BigInt(HexWithLimbs(limbs, 0xF1234567U)).ToString();
    for(auto _ : state) {
        BigInt result(decimal);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(decimal.size()));
}
BENCHMARK(BM_BigInt_ParseDecimal)->Apply(BigIntLimbRanges);

void BM_BigInt_ParseHex(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const std::string hexadecimal = HexWithLimbs(limbs, 0xF1234567U);
    for(auto _ : state) {
        BigInt result(hexadecimal);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(hexadecimal.size()));
}
BENCHMARK(BM_BigInt_ParseHex)->Apply(BigIntLimbRanges);

void BM_BigInt_ToStringDecimal(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt value(HexWithLimbs(limbs, 0xF1234567U));
    for(auto _ : state) {
        std::string result = value.ToString();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(limbs));
}
BENCHMARK(BM_BigInt_ToStringDecimal)->Apply(BigIntLimbRanges);

void BM_BigInt_ToStringHex(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt value(HexWithLimbs(limbs, 0xF1234567U));
    for(auto _ : state) {
        std::string result = value.ToString(16);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(limbs));
}
BENCHMARK(BM_BigInt_ToStringHex)->Apply(BigIntLimbRanges);

void BM_BigInt_DivideByWord(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt dividend(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt divisor("4294967291");
    for(auto _ : state) {
        BigInt result = dividend / divisor;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_DivideByWord)->Apply(BigIntLimbRanges)->Complexity();

void BM_BigInt_DivideByPowerOfTwo(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt dividend(HexWithLimbs(limbs * 2, 0xF1234567U));
    const BigInt divisor("0x1" + std::string(limbs * 4, '0'));
    for(auto _ : state) {
        BigInt result = dividend / divisor;
        benchmark::DoNotOptimize(result);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BigInt_DivideByPowerOfTwo)->Apply(BigIntLimbRanges)->Complexity();

void BigIntKnuthDivisionRanges(benchmark::internal::Benchmark* benchmark) {
    for(int limbs : {2, 8, 32, 64, 127, 128, 256, 512})
        benchmark->Arg(limbs);
}

void BigIntBurnikelDivisionRanges(benchmark::internal::Benchmark* benchmark) {
    for(int limbs : {1024, 1025, 1152, 1280, 1408, 1536, 2048})
        benchmark->Arg(limbs);
}

BigInt MakeDivisionDividend(std::size_t divisorLimbs, BigInt& divisor) {
    divisor = BigInt(HexWithLimbs(divisorLimbs, 0xF1234567U));
    const BigInt quotient(HexWithLimbs(divisorLimbs, 0x89ABCDEFU));
    const BigInt remainder(HexWithLimbs(divisorLimbs, 0x01234567U));
    return BigInt::multiply(divisor, quotient) + remainder;
}

void BM_BigInt_DivideKnuth(benchmark::State& state) {
    BigInt divisor;
    const BigInt dividend = MakeDivisionDividend(static_cast<std::size_t>(state.range(0)), divisor);
    for(auto _ : state) {
        BigInt result = dividend / divisor;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BigInt_DivideKnuth)->Apply(BigIntKnuthDivisionRanges);

void BM_BigInt_DivmodKnuth(benchmark::State& state) {
    BigInt divisor;
    const BigInt dividend = MakeDivisionDividend(static_cast<std::size_t>(state.range(0)), divisor);
    for(auto _ : state) {
        auto result = dividend.divmod(divisor);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BigInt_DivmodKnuth)->Apply(BigIntKnuthDivisionRanges);

void BM_BigInt_DivideBurnikelZiegler(benchmark::State& state) {
    BigInt divisor;
    const BigInt dividend = MakeDivisionDividend(static_cast<std::size_t>(state.range(0)), divisor);
    for(auto _ : state) {
        BigInt result = dividend / divisor;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BigInt_DivideBurnikelZiegler)->Apply(BigIntBurnikelDivisionRanges);

void BM_BigInt_DivmodBurnikelZiegler(benchmark::State& state) {
    BigInt divisor;
    const BigInt dividend = MakeDivisionDividend(static_cast<std::size_t>(state.range(0)), divisor);
    for(auto _ : state) {
        auto result = dividend.divmod(divisor);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BigInt_DivmodBurnikelZiegler)->Apply(BigIntBurnikelDivisionRanges);

void BM_BigInt_DivideLargeDivisorShortQuotient(benchmark::State& state) {
    const auto limbs = static_cast<std::size_t>(state.range(0));
    const BigInt divisor(HexWithLimbs(limbs, 0xF1234567U));
    const BigInt dividend = divisor * BigInt(0x1234567) + BigInt(42);
    for(auto _ : state) {
        BigInt result = dividend / divisor;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_BigInt_DivideLargeDivisorShortQuotient)->Arg(1024)->Arg(2048)->Arg(4096);

} // namespace
