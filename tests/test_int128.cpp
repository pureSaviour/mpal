#include <gtest/gtest.h>
#include "uint128.h"
#include "int128.h"

#include <array>
#include <bit>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>

// int128_t 只有 ToString()，没有 operator<<，为 gtest 的打印补一个。
std::ostream& operator<<(std::ostream& os, const int128_t& v) {
    return os << v.ToString();
}

TEST(Int128Test, ConstructAndToString) {
    EXPECT_EQ(int128_t(0).ToString(), std::string("0"));
    EXPECT_EQ(int128_t(-42).ToString(), std::string("-42"));
    EXPECT_EQ(int128_t("18446744073709551616").ToString(),   // 2^64
              std::string("18446744073709551616"));
    EXPECT_EQ(int128_t("-170141183460469231731687303715884105728").ToString(),  // INT128_MIN
              std::string("-170141183460469231731687303715884105728"));
    EXPECT_EQ(int128_t::max().ToString(),
              std::string("170141183460469231731687303715884105727"));
}

TEST(Int128Test, Compare) {
    int128_t neg("-100"), zero(0), pos("100");
    EXPECT_TRUE(neg < zero);
    EXPECT_TRUE(zero < pos);
    EXPECT_TRUE(neg < pos);
    EXPECT_TRUE(pos > neg);
    EXPECT_EQ(neg, int128_t("-100"));
    EXPECT_TRUE(neg <= neg);
    EXPECT_TRUE(pos >= pos);
    EXPECT_TRUE(neg != pos);
}

TEST(Int128Test, AddSub) {
    EXPECT_EQ(int128_t(5) + int128_t(3), int128_t(8));
    EXPECT_EQ(int128_t(5) - int128_t(3), int128_t(2));
    EXPECT_EQ(int128_t(-5) + int128_t(3), int128_t(-2));
    EXPECT_EQ(int128_t(-5) - int128_t(3), int128_t(-8));
    // 回绕：MAX + 1 = MIN
    EXPECT_EQ(int128_t::max() + int128_t(1), int128_t::min());
    // 回绕：MIN - 1 = MAX
    EXPECT_EQ(int128_t::min() - int128_t(1), int128_t::max());
}

TEST(Int128Test, Negate) {
    EXPECT_EQ(-int128_t(5), int128_t(-5));
    EXPECT_EQ(-int128_t(-5), int128_t(5));
    EXPECT_EQ(-int128_t(0), int128_t(0));
}

TEST(Int128Test, Mul) {
    EXPECT_EQ(int128_t(6) * int128_t(7), int128_t(42));
    EXPECT_EQ(int128_t(-6) * int128_t(7), int128_t(-42));
    EXPECT_EQ(int128_t(-6) * int128_t(-7), int128_t(42));
    EXPECT_EQ(int128_t("18446744073709551616") * int128_t(2),  // 2^64 * 2 = 2^65
              int128_t("36893488147419103232"));
}

TEST(Int128Test, Div) {
    // 向零截断
    EXPECT_EQ(int128_t(7) / int128_t(2), int128_t(3));
    EXPECT_EQ(int128_t(-7) / int128_t(2), int128_t(-3));
    EXPECT_EQ(int128_t(7) / int128_t(-2), int128_t(-3));
    EXPECT_EQ(int128_t(-7) / int128_t(-2), int128_t(3));
    // 除以 ±1
    EXPECT_EQ(int128_t(42) / int128_t(1), int128_t(42));
    EXPECT_EQ(int128_t(42) / int128_t(-1), int128_t(-42));
    // 相同数相除
    EXPECT_EQ(int128_t(100) / int128_t(100), int128_t(1));
}

TEST(Int128Test, Mod) {
    EXPECT_EQ(int128_t(7) % int128_t(2), int128_t(1));
    EXPECT_EQ(int128_t(-7) % int128_t(2), int128_t(-1));  // 余数符号随被除数
    EXPECT_EQ(int128_t(7) % int128_t(-2), int128_t(1));
    EXPECT_EQ(int128_t(100) % int128_t(7), int128_t(2));
}

TEST(Int128Test, Shift) {
    EXPECT_EQ(int128_t(1) << 64, int128_t("18446744073709551616"));
    EXPECT_EQ(int128_t("18446744073709551616") >> 64, int128_t(1));
    // 算术右移：负数高位补 1
    EXPECT_EQ(int128_t(-8) >> 1, int128_t(-4));
    EXPECT_EQ(int128_t(-1) >> 1, int128_t(-1));
}

TEST(Int128Test, Bitwise) {
    EXPECT_EQ(int128_t(0b1010) & int128_t(0b1100), int128_t(0b1000));
    EXPECT_EQ(int128_t(0b1010) | int128_t(0b1100), int128_t(0b1110));
    EXPECT_EQ(int128_t(0b1010) ^ int128_t(0b1100), int128_t(0b0110));
    EXPECT_EQ(~int128_t(0), int128_t(-1));
}

TEST(Int128Test, LargeOperandArithmetic) {
    // 大于 2^64 的操作数
    int128_t a("18446744073709551616");   // 2^64
    int128_t b("18446744073709551616");
    EXPECT_EQ(a + b, int128_t("36893488147419103232"));  // 2^65
    EXPECT_EQ(a * int128_t(2), int128_t("36893488147419103232"));
}

TEST(Int128BoundaryTest, StringConstructionAtLimits) {
    EXPECT_EQ(int128_t("170141183460469231731687303715884105727"), int128_t::max());
    EXPECT_EQ(int128_t("-170141183460469231731687303715884105728"), int128_t::min());
    EXPECT_EQ(int128_t("170141183460469231731687303715884105728"), int128_t::min());
    EXPECT_EQ(int128_t("-170141183460469231731687303715884105729"), int128_t::max());

    EXPECT_THROW(int128_t(""), std::invalid_argument);
    EXPECT_THROW(int128_t("-"), std::invalid_argument);
    EXPECT_THROW(int128_t("12z"), std::invalid_argument);
}

TEST(Int128BoundaryTest, OrderingAroundSignBit) {
    const std::array<int128_t, 7> ordered{
        int128_t::min(), int128_t(-2), int128_t(-1), int128_t(0),
        int128_t(1), int128_t(2), int128_t::max()
    };

    for (size_t i = 0; i < ordered.size(); ++i) {
        EXPECT_EQ(ordered[i], ordered[i]);
        for (size_t j = i + 1; j < ordered.size(); ++j) {
            EXPECT_LT(ordered[i], ordered[j]);
            EXPECT_GT(ordered[j], ordered[i]);
        }
    }
}

TEST(Int128BoundaryTest, ArithmeticWraparound) {
    EXPECT_EQ(int128_t::max() + int128_t(1), int128_t::min());
    EXPECT_EQ(int128_t::min() - int128_t(1), int128_t::max());
    EXPECT_EQ(int128_t::min() + int128_t::min(), int128_t(0));
    EXPECT_EQ(-int128_t::min(), int128_t::min());
    EXPECT_EQ(int128_t::max() * int128_t(2), int128_t(-2));
    EXPECT_EQ(int128_t::min() * int128_t(-1), int128_t::min());
    EXPECT_EQ(int128_t(-1) * int128_t(-1), int128_t(1));
}

TEST(Int128BoundaryTest, DivisionSpecialCases) {
    const int128_t min = int128_t::min();
    const int128_t max = int128_t::max();
    const int128_t minDiv3("-56713727820156410577229101238628035242");

    EXPECT_EQ(min / int128_t(1), min);
    EXPECT_EQ(min / int128_t(-1), min); // library's documented modulo-2^128 policy
    EXPECT_EQ(min / int128_t(2), int128_t("-85070591730234615865843651857942052864"));
    EXPECT_EQ(min / int128_t(3), minDiv3);
    EXPECT_EQ(min / int128_t(-3), -minDiv3);
    EXPECT_EQ(min % int128_t(3), int128_t(-2));
    EXPECT_EQ(min % int128_t(-3), int128_t(-2));
    EXPECT_EQ(min / min, int128_t(1));
    EXPECT_EQ(min % min, int128_t(0));

    EXPECT_EQ(max / min, int128_t(0));
    EXPECT_EQ(max % min, max);
    EXPECT_EQ(int128_t(1) / min, int128_t(0));
    EXPECT_EQ(int128_t(1) % min, int128_t(1));
    EXPECT_EQ(int128_t(-1) / min, int128_t(0));
    EXPECT_EQ(int128_t(-1) % min, int128_t(-1));

    EXPECT_THROW(max / int128_t(0), std::invalid_argument);
    EXPECT_THROW(max % int128_t(0), std::invalid_argument);
}

TEST(Int128BoundaryTest, DivisionRandomDifferential) {
    uint64_t state = 0x94D049BB133111EBULL;
    auto next = [&state]() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DULL;
    };
    const auto makeSigned = [](uint64_t high, uint64_t low) {
        const uint128_t bits = (uint128_t(high) << 64U) | uint128_t(low);
        return int128_t(bits);
    };

    for(int iteration = 0; iteration < 20'000; ++iteration) {
        const uint64_t dividendHigh = next();
        const uint64_t dividendLow = next();
        uint64_t divisorHigh = iteration % 3 == 0 ? 0 : next();
        uint64_t divisorLow = next();
        if(divisorHigh == 0 && divisorLow == 0) {
            divisorLow = 1;
        }

        const int128_t dividend = makeSigned(dividendHigh, dividendLow);
        const int128_t divisor = makeSigned(divisorHigh, divisorLow);
        const int128_t quotient = dividend / divisor;
        const int128_t remainder = dividend % divisor;

        EXPECT_EQ(quotient * divisor + remainder, dividend);

        #if defined(__SIZEOF_INT128__)
            const __uint128_t nativeDividendBits =
                (static_cast<__uint128_t>(dividendHigh) << 64U) | dividendLow;
            const __uint128_t nativeDivisorBits =
                (static_cast<__uint128_t>(divisorHigh) << 64U) | divisorLow;
            const __int128_t nativeDividend = std::bit_cast<__int128_t>(nativeDividendBits);
            const __int128_t nativeDivisor = std::bit_cast<__int128_t>(nativeDivisorBits);
            const bool nativeOverflow =
                nativeDividendBits == (static_cast<__uint128_t>(1) << 127U)
                && nativeDivisor == static_cast<__int128_t>(-1);
            if(!nativeOverflow) {
                const __int128_t nativeQuotient = nativeDividend / nativeDivisor;
                const __int128_t nativeRemainder = nativeDividend % nativeDivisor;
                const __uint128_t nativeQuotientBits = std::bit_cast<__uint128_t>(nativeQuotient);
                const __uint128_t nativeRemainderBits = std::bit_cast<__uint128_t>(nativeRemainder);
                const uint128_t quotientBits(quotient);
                const uint128_t remainderBits(remainder);

                EXPECT_EQ(static_cast<uint64_t>(quotientBits), static_cast<uint64_t>(nativeQuotientBits));
                EXPECT_EQ(
                    static_cast<uint64_t>(quotientBits >> 64U),
                    static_cast<uint64_t>(nativeQuotientBits >> 64U));
                EXPECT_EQ(static_cast<uint64_t>(remainderBits), static_cast<uint64_t>(nativeRemainderBits));
                EXPECT_EQ(
                    static_cast<uint64_t>(remainderBits >> 64U),
                    static_cast<uint64_t>(nativeRemainderBits >> 64U));
            }
        #endif
    }
}

TEST(Int128BoundaryTest, ShiftBoundariesAndSignExtension) {
    const int128_t one(1);
    const std::array<unsigned int, 4> oversized{128, 129, 255, UINT32_MAX};

    EXPECT_EQ(one << 0, one);
    EXPECT_EQ(one << 63, int128_t("9223372036854775808"));
    EXPECT_EQ(one << 64, int128_t("18446744073709551616"));
    EXPECT_EQ(one << 127, int128_t::min());
    EXPECT_EQ(int128_t::max() << 1, int128_t(-2));
    EXPECT_EQ(int128_t::min() << 1, int128_t(0));
    EXPECT_EQ(int128_t::min() >> 127, int128_t(-1));
    EXPECT_EQ(int128_t::max() >> 127, int128_t(0));
    EXPECT_EQ(int128_t(-3) >> 1, int128_t(-2));

    for (const unsigned int shift : oversized) {
        EXPECT_EQ(int128_t::max() << shift, int128_t(0));
        EXPECT_EQ(int128_t::max() >> shift, int128_t(0));
        EXPECT_EQ(int128_t(-1) >> shift, int128_t(-1));
    }
}

TEST(Int128BoundaryTest, BitPatternsAndCrossSignednessConversions) {
    EXPECT_EQ(~int128_t::max(), int128_t::min());
    EXPECT_EQ(~int128_t::min(), int128_t::max());
    EXPECT_EQ(int128_t::min() | int128_t::max(), int128_t(-1));
    EXPECT_EQ(int128_t::min() & int128_t::max(), int128_t(0));
    EXPECT_EQ(int128_t::min() ^ int128_t::max(), int128_t(-1));

    EXPECT_EQ(int128_t(uint128_t::max()), int128_t(-1));
    EXPECT_EQ(uint128_t(int128_t(-1)), uint128_t::max());
    EXPECT_EQ(static_cast<uint64_t>(int128_t(-1)), UINT64_MAX);
    EXPECT_FALSE(static_cast<bool>(int128_t(0)));
    EXPECT_TRUE(static_cast<bool>(int128_t::min()));
}

TEST(Int128BoundaryTest, CompoundAssignmentsAtBoundaries) {
    int128_t value = int128_t::max();
    value += int128_t(1);
    EXPECT_EQ(value, int128_t::min());
    value -= int128_t(1);
    EXPECT_EQ(value, int128_t::max());
    value *= int128_t(2);
    EXPECT_EQ(value, int128_t(-2));
    value /= int128_t(2);
    EXPECT_EQ(value, int128_t(-1));
    value %= int128_t(2);
    EXPECT_EQ(value, int128_t(-1));

    value &= int128_t::max();
    EXPECT_EQ(value, int128_t::max());
    value ^= int128_t::min();
    EXPECT_EQ(value, int128_t(-1));
    value >>= 127;
    EXPECT_EQ(value, int128_t(-1));
    value <<= 128;
    EXPECT_EQ(value, int128_t(0));
    value |= int128_t::min();
    EXPECT_EQ(value, int128_t::min());
}
