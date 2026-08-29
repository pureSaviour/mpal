#include <gtest/gtest.h>
#include "uint128.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

#if !MPAL_HAS_NATIVE_I128
constexpr auto kConstexprDivBy64 = u128_impl::divmod(
    u128_impl::rep{UINT64_MAX, UINT64_MAX}, u128_impl::rep{0, 3});
static_assert(kConstexprDivBy64.quotient.high == 0x5555'5555'5555'5555ULL);
static_assert(kConstexprDivBy64.quotient.low == 0x5555'5555'5555'5555ULL);
static_assert(kConstexprDivBy64.remainder.high == 0 && kConstexprDivBy64.remainder.low == 0);

constexpr auto kConstexprDivBy128 = u128_impl::divmod(
    u128_impl::rep{UINT64_MAX, UINT64_MAX}, u128_impl::rep{1, 1});
static_assert(kConstexprDivBy128.quotient.high == 0);
static_assert(kConstexprDivBy128.quotient.low == UINT64_MAX);
static_assert(kConstexprDivBy128.remainder.high == 0 && kConstexprDivBy128.remainder.low == 0);
#endif

TEST(Uint128Test, ConstructAndToString) {
    EXPECT_EQ(uint128_t(0).ToString(), std::string("0"));
    EXPECT_EQ(uint128_t(42).ToString(), std::string("42"));
    // 2^64
    EXPECT_EQ(uint128_t("18446744073709551616").ToString(),
              std::string("18446744073709551616"));
    // 2^128 - 1
    EXPECT_EQ(uint128_t::max().ToString(),
              std::string("340282366920938463463374607431768211455"));
}

TEST(Uint128Test, Compare) {
    uint128_t a("100"), b("200");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(a != b);
    EXPECT_EQ(a, uint128_t("100"));
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
}

TEST(Uint128Test, Add) {
    EXPECT_EQ(uint128_t(5) + uint128_t(3), uint128_t(8));
    // 跨 64 位进位：(2^64 - 1) + 1 = 2^64
    EXPECT_EQ(uint128_t("18446744073709551615") + uint128_t(1),
              uint128_t("18446744073709551616"));
    // 回绕
    EXPECT_EQ(uint128_t::max() + uint128_t(1), uint128_t(0));
}

TEST(Uint128Test, Sub) {
    EXPECT_EQ(uint128_t(8) - uint128_t(3), uint128_t(5));
    // 回绕：0 - 1 = max
    EXPECT_EQ(uint128_t(0) - uint128_t(1), uint128_t::max());
}

TEST(Uint128Test, Mul) {
    EXPECT_EQ(uint128_t(6) * uint128_t(7), uint128_t(42));
    // 2^64 * 2^64 = 2^128，截断为 0
    EXPECT_EQ(uint128_t("18446744073709551616") * uint128_t("18446744073709551616"),
              uint128_t(0));
    // (2^64-1)^2 = 2^128 - 2^65 + 1 => low=1, high=2^64-2
    uint128_t x = uint128_t("18446744073709551615") * uint128_t("18446744073709551615");
    uint128_t expected = uint128_t(1) | (uint128_t("18446744073709551614") << 64);
    EXPECT_EQ(x, expected);
}

TEST(Uint128Test, DivMod) {
    EXPECT_EQ(uint128_t(100) / uint128_t(7), uint128_t(14));
    EXPECT_EQ(uint128_t(100) % uint128_t(7), uint128_t(2));
    // 2^64 / 2 = 2^63
    EXPECT_EQ(uint128_t("18446744073709551616") / uint128_t(2),
              uint128_t("9223372036854775808"));
    EXPECT_EQ(uint128_t(7) / uint128_t(1), uint128_t(7));
    EXPECT_EQ(uint128_t(7) % uint128_t(1), uint128_t(0));
}

TEST(Uint128Test, Bitwise) {
    EXPECT_EQ(uint128_t(0b1010) & uint128_t(0b1100), uint128_t(0b1000));
    EXPECT_EQ(uint128_t(0b1010) | uint128_t(0b1100), uint128_t(0b1110));
    EXPECT_EQ(uint128_t(0b1010) ^ uint128_t(0b1100), uint128_t(0b0110));
    EXPECT_EQ(~uint128_t(0), uint128_t::max());
}

TEST(Uint128Test, Shift) {
    EXPECT_EQ(uint128_t(1) << 64, uint128_t("18446744073709551616"));
    EXPECT_EQ(uint128_t("18446744073709551616") >> 64, uint128_t(1));
    EXPECT_EQ(uint128_t::max() >> 127, uint128_t(1));
    EXPECT_EQ(uint128_t::max() << 1, uint128_t::max() - uint128_t(1));
    EXPECT_EQ(uint128_t(1) << 128, uint128_t(0));  // 整体移出
}

TEST(Uint128Test, IncrementDecrement) {
    uint128_t x(0);
    EXPECT_EQ(++x, uint128_t(1));
    EXPECT_EQ(x++, uint128_t(1));
    EXPECT_EQ(x, uint128_t(2));
    EXPECT_EQ(--x, uint128_t(1));
    EXPECT_EQ(x--, uint128_t(1));
    EXPECT_EQ(x, uint128_t(0));
}

TEST(Uint128Test, CompoundAssign) {
    uint128_t x(10);
    x += 5;  EXPECT_EQ(x, uint128_t(15));
    x -= 3;  EXPECT_EQ(x, uint128_t(12));
    x *= 2;  EXPECT_EQ(x, uint128_t(24));
    x /= 4;  EXPECT_EQ(x, uint128_t(6));
    x %= 4;  EXPECT_EQ(x, uint128_t(2));
}

TEST(Uint128BoundaryTest, StringConstructionAndRangeChecks) {
    EXPECT_EQ(uint128_t("+42"), uint128_t(42));
    EXPECT_EQ(uint128_t("000000000000000000000000000000000000001"), uint128_t(1));
    EXPECT_EQ(uint128_t("0xffffffffffffffffffffffffffffffff"), uint128_t::max());
    EXPECT_EQ(uint128_t("0b1" + std::string(127, '0')), uint128_t(1) << 127);
    EXPECT_EQ(uint128_t("-1"), uint128_t::max());

    EXPECT_THROW(uint128_t(""), std::invalid_argument);
    EXPECT_THROW(uint128_t("12z"), std::invalid_argument);
    EXPECT_THROW(uint128_t("340282366920938463463374607431768211456"),
                 std::invalid_argument);
}

TEST(Uint128BoundaryTest, IntegralConstructionUsesFullUnsignedWidth) {
    EXPECT_EQ(uint128_t(uint64_t{0}), uint128_t(0));
    EXPECT_EQ(uint128_t(UINT64_MAX), uint128_t("18446744073709551615"));

    // Converting a negative built-in integer to an unsigned 128-bit value should
    // sign-extend before applying modulo 2^128, matching built-in unsigned conversion.
    EXPECT_EQ(uint128_t(-1), uint128_t::max());
}

TEST(Uint128BoundaryTest, CarryBorrowAndWraparound) {
    const uint128_t two64("18446744073709551616");
    const uint128_t lowMax("18446744073709551615");

    EXPECT_EQ(lowMax + uint128_t(1), two64);
    EXPECT_EQ(two64 - uint128_t(1), lowMax);
    EXPECT_EQ(uint128_t::max() + uint128_t::max(), uint128_t::max() - uint128_t(1));
    EXPECT_EQ(uint128_t(0) - uint128_t::max(), uint128_t(1));
}

TEST(Uint128BoundaryTest, MultiplicationTruncatesModuloTwoTo128) {
    const uint128_t two64("18446744073709551616");
    const uint128_t two127 = uint128_t(1) << 127;

    EXPECT_EQ(uint128_t::max() * uint128_t(0), uint128_t(0));
    EXPECT_EQ(uint128_t::max() * uint128_t(1), uint128_t::max());
    EXPECT_EQ(two64 * two64, uint128_t(0));
    EXPECT_EQ(two127 * uint128_t(2), uint128_t(0));
    EXPECT_EQ(uint128_t::max() * uint128_t::max(), uint128_t(1));
}

TEST(Uint128BoundaryTest, DivisionAndRemainderExtremes) {
    const uint128_t max = uint128_t::max();
    const uint128_t two64("18446744073709551616");

    EXPECT_EQ(uint128_t(0) / max, uint128_t(0));
    EXPECT_EQ(uint128_t(0) % max, uint128_t(0));
    EXPECT_EQ(max / uint128_t(1), max);
    EXPECT_EQ(max % uint128_t(1), uint128_t(0));
    EXPECT_EQ(max / max, uint128_t(1));
    EXPECT_EQ(max % max, uint128_t(0));
    EXPECT_EQ(max / uint128_t(2), (uint128_t(1) << 127) - uint128_t(1));
    EXPECT_EQ(max % uint128_t(2), uint128_t(1));
    EXPECT_EQ(max / two64, uint128_t("18446744073709551615"));
    EXPECT_EQ(max % two64, uint128_t("18446744073709551615"));

    EXPECT_THROW(max / uint128_t(0), std::runtime_error);
    EXPECT_THROW(max % uint128_t(0), std::runtime_error);
}

TEST(Uint128BoundaryTest, DivisionRandomDifferential) {
    uint64_t state = 0xD1B54A32D192ED03ULL;
    auto next = [&state]() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DULL;
    };
    const auto make128 = [](uint64_t high, uint64_t low) {
        return (uint128_t(high) << 64U) | uint128_t(low);
    };

    for(int iteration = 0; iteration < 20'000; ++iteration) {
        const uint64_t dividendHigh = next();
        const uint64_t dividendLow = next();
        uint64_t divisorHigh = iteration % 3 == 0 ? 0 : next();
        uint64_t divisorLow = next();
        if(divisorHigh == 0 && divisorLow == 0) {
            divisorLow = 1;
        }

        const uint128_t dividend = make128(dividendHigh, dividendLow);
        const uint128_t divisor = make128(divisorHigh, divisorLow);
        const uint128_t quotient = dividend / divisor;
        const uint128_t remainder = dividend % divisor;

        EXPECT_EQ(quotient * divisor + remainder, dividend);
        EXPECT_LT(remainder, divisor);

        #if defined(__SIZEOF_INT128__)
            const __uint128_t nativeDividend =
                (static_cast<__uint128_t>(dividendHigh) << 64U) | dividendLow;
            const __uint128_t nativeDivisor =
                (static_cast<__uint128_t>(divisorHigh) << 64U) | divisorLow;
            const __uint128_t nativeQuotient = nativeDividend / nativeDivisor;
            const __uint128_t nativeRemainder = nativeDividend % nativeDivisor;

            EXPECT_EQ(static_cast<uint64_t>(quotient), static_cast<uint64_t>(nativeQuotient));
            EXPECT_EQ(
                static_cast<uint64_t>(quotient >> 64U),
                static_cast<uint64_t>(nativeQuotient >> 64U));
            EXPECT_EQ(static_cast<uint64_t>(remainder), static_cast<uint64_t>(nativeRemainder));
            EXPECT_EQ(
                static_cast<uint64_t>(remainder >> 64U),
                static_cast<uint64_t>(nativeRemainder >> 64U));
        #endif
    }
}

TEST(Uint128BoundaryTest, ShiftBoundaries) {
    const uint128_t one(1);
    const uint128_t two64("18446744073709551616");
    const std::array<unsigned int, 4> oversized{128, 129, 255, UINT32_MAX};

    EXPECT_EQ(one << 0, one);
    EXPECT_EQ(one << 63, uint128_t("9223372036854775808"));
    EXPECT_EQ(one << 64, two64);
    EXPECT_EQ(one << 65, uint128_t("36893488147419103232"));
    EXPECT_EQ(one << 127, uint128_t("170141183460469231731687303715884105728"));
    EXPECT_EQ((one << 127) >> 127, one);
    EXPECT_EQ(uint128_t::max() >> 64, uint128_t("18446744073709551615"));

    for (const unsigned int shift : oversized) {
        EXPECT_EQ(uint128_t::max() << shift, uint128_t(0));
        EXPECT_EQ(uint128_t::max() >> shift, uint128_t(0));
    }
}

TEST(Uint128BoundaryTest, IncrementDecrementWrapAndConversions) {
    uint128_t value = uint128_t::max();
    EXPECT_EQ(value++, uint128_t::max());
    EXPECT_EQ(value, uint128_t(0));
    EXPECT_EQ(value--, uint128_t(0));
    EXPECT_EQ(value, uint128_t::max());
    EXPECT_EQ(++value, uint128_t(0));
    EXPECT_EQ(--value, uint128_t::max());

    EXPECT_FALSE(static_cast<bool>(uint128_t(0)));
    EXPECT_TRUE(static_cast<bool>(uint128_t(1) << 127));
    EXPECT_EQ(static_cast<uint64_t>(uint128_t::max()), UINT64_MAX);
    EXPECT_EQ(static_cast<uint32_t>(uint128_t::max()), UINT32_MAX);
}

TEST(Uint128BoundaryTest, BitwiseCompoundAssignments) {
    uint128_t value = uint128_t::max();
    value &= uint128_t("18446744073709551615");
    EXPECT_EQ(value, uint128_t("18446744073709551615"));
    value <<= 64;
    EXPECT_EQ(value, uint128_t::max() - uint128_t("18446744073709551615"));
    value >>= 64;
    EXPECT_EQ(value, uint128_t("18446744073709551615"));
    value ^= uint128_t::max();
    EXPECT_EQ(value, uint128_t::max() - uint128_t("18446744073709551615"));
    value |= uint128_t("18446744073709551615");
    EXPECT_EQ(value, uint128_t::max());
}
