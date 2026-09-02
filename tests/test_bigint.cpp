#include <gtest/gtest.h>

#include "BigInt.h"

#include <compare>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <string>
#include <utility>

namespace {

using mpal::BigInt;

class underscore_grouping_bigint final : public std::numpunct<char> {
protected:
    char do_thousands_sep() const override { return '_'; }
    std::string do_grouping() const override { return "\3"; }
};

std::string RandomHex(std::mt19937_64& random, std::size_t limbs) {
    std::ostringstream text;
    text << "0x" << std::hex << ((static_cast<uint32_t>(random()) | 1U));
    for(std::size_t i = 1; i < limbs; ++i) {
        text << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(random());
    }
    return text.str();
}

std::string StructuredHex(std::size_t limbs, uint32_t top, uint32_t seed) {
    std::ostringstream text;
    text << "0x" << std::hex << top;
    uint32_t value = seed;
    for(std::size_t i = 1; i < limbs; ++i) {
        value = value * 1664525U + 1013904223U;
        text << std::setw(8) << std::setfill('0') << value;
    }
    return text.str();
}

TEST(BigIntConstructionTest, ConstructsCanonicalValuesAndConvertsBases) {
    EXPECT_EQ(BigInt().ToString(), "0");
    EXPECT_EQ(BigInt(42).ToString(), "42");
    EXPECT_EQ(BigInt(-42).ToString(), "-42");
    EXPECT_EQ(BigInt(std::numeric_limits<int>::min()).ToString(),
              std::to_string(std::numeric_limits<int>::min()));

    const BigInt two64("18446744073709551616");
    EXPECT_EQ(two64.ToString(), "18446744073709551616");
    EXPECT_EQ(two64.ToString(16), "10000000000000000");
    EXPECT_EQ(two64.ToString(8), "2000000000000000000000");
    EXPECT_EQ(two64.ToString(2), "1" + std::string(64, '0'));

    EXPECT_EQ(BigInt("0xffffffffffffffff").ToString(), "18446744073709551615");
    EXPECT_EQ(BigInt("0o1777777777777777777777").ToString(16), "ffffffffffffffff");
    EXPECT_EQ(BigInt("0b" + std::string(64, '1')).ToString(16), "ffffffffffffffff");
    EXPECT_EQ(BigInt("+00000000000042"), BigInt(42));
}

TEST(BigIntConstructionTest, RejectsMalformedInputAndInvalidOutputBases) {
    for(const std::string& input : {"", "+", "-", "0x", "-0b", "0o", "12z", "0b102", "0o128", "0x12g"}) {
        EXPECT_THROW((void)BigInt{input}, std::invalid_argument) << input;
    }
    EXPECT_THROW((void)BigInt(1).ToString(0), std::invalid_argument);
    EXPECT_THROW((void)BigInt(1).ToString(1), std::invalid_argument);
    EXPECT_THROW((void)BigInt(1).ToString(17), std::invalid_argument);
    EXPECT_THROW((void)BigInt(0).ToString(17), std::invalid_argument);
}

TEST(BigIntSignTest, ZeroAlwaysRemainsCanonical) {
    for(const std::string& spelling : {"0", "+0", "-0", "00", "-000", "0x0", "-0b0"}) {
        const BigInt value(spelling);
        EXPECT_EQ(value, BigInt::ZERO) << spelling;
        EXPECT_EQ(value.ToString(), "0") << spelling;
        EXPECT_FALSE(value.IsNegative()) << spelling;
    }

    BigInt zero;
    zero.Inverse();
    EXPECT_EQ(zero.ToString(), "0");
    zero.SetSign(false);
    EXPECT_FALSE(zero.IsNegative());
    EXPECT_EQ((-zero).ToString(), "0");
    EXPECT_EQ((BigInt(-7) * BigInt(0)).ToString(), "0");
    EXPECT_EQ(BigInt::multiply(BigInt(-7), BigInt(0)).ToString(), "0");
}

TEST(BigIntSignTest, SignInspectionAndMutationAffectArithmeticState) {
    BigInt value(42);
    EXPECT_TRUE(value.IsPositive());
    value.SetSign(false);
    EXPECT_TRUE(value.IsNegative());
    EXPECT_EQ(value.ToString(), "-42");
    value.Inverse();
    EXPECT_EQ(value, BigInt(42));
}

TEST(BigIntValueSemanticsTest, CopyMoveAndConstantsRemainUsable) {
    const BigInt original("-123456789012345678901234567890");
    BigInt copy(original);
    EXPECT_EQ(copy, original);

    BigInt assigned;
    assigned = copy;
    EXPECT_EQ(assigned, original);

    BigInt moved(std::move(copy));
    EXPECT_EQ(moved, original);
    EXPECT_EQ(copy.ToString(), "0");

    BigInt moveAssigned(7);
    moveAssigned = std::move(assigned);
    EXPECT_EQ(moveAssigned, original);
    EXPECT_EQ(assigned.ToString(), "0");
    EXPECT_EQ(BigInt::ZERO.ToString(), "0");
    EXPECT_EQ(BigInt::ONE.ToString(), "1");
}

TEST(BigIntComparisonTest, OrdersSignsAndMultiLimbMagnitudes) {
    const BigInt negativeLarge("-18446744073709551616");
    const BigInt negativeSmall(-1);
    const BigInt zero;
    const BigInt positiveSmall(1);
    const BigInt positiveLarge("18446744073709551616");

    EXPECT_LT(negativeLarge, negativeSmall);
    EXPECT_LT(negativeSmall, zero);
    EXPECT_LT(zero, positiveSmall);
    EXPECT_LT(positiveSmall, positiveLarge);
    EXPECT_GT(positiveLarge, negativeLarge);
    EXPECT_EQ(positiveLarge <=> BigInt("18446744073709551616"), std::strong_ordering::equal);
}

TEST(BigIntArithmeticTest, HandlesAllSignCombinations) {
    const BigInt a("1234567890123456789012345678901234567890");
    const BigInt b("987654321098765432109876543210987654321");

    EXPECT_EQ((a + b).ToString(), "2222222211222222221122222222112222222211");
    EXPECT_EQ((a - b).ToString(), "246913569024691356902469135690246913569");
    EXPECT_EQ((b - a).ToString(), "-246913569024691356902469135690246913569");
    EXPECT_EQ((-a + b).ToString(), "-246913569024691356902469135690246913569");
    EXPECT_EQ((-a - b).ToString(), "-2222222211222222221122222222112222222211");
    EXPECT_EQ((a * -b).ToString(),
              "-1219326311370217952261850327338667885944871208653362292333223746380111126352690");
}

TEST(BigIntArithmeticTest, PropagatesCarryAndBorrowAcrossManyLimbs) {
    const BigInt two32("4294967296");
    const BigInt two64("18446744073709551616");
    const BigInt two96("79228162514264337593543950336");

    EXPECT_EQ(BigInt("4294967295") + BigInt(1), two32);
    EXPECT_EQ(two32 - BigInt(1), BigInt("4294967295"));
    EXPECT_EQ(BigInt("18446744073709551615") + BigInt(1), two64);
    EXPECT_EQ(two64 - BigInt(1), BigInt("18446744073709551615"));
    EXPECT_EQ(two96 - BigInt(1), BigInt("79228162514264337593543950335"));
    EXPECT_EQ(two64 * two64, BigInt("340282366920938463463374607431768211456"));
}

TEST(BigIntArithmeticTest, OptimizesIdentityAndPowerOfTwoMultiplicationWithoutChangingSigns) {
    const BigInt value("-1234567890123456789012345678901234567890");
    EXPECT_EQ(value - value, BigInt::ZERO);
    EXPECT_EQ(value * BigInt::ONE, value);
    EXPECT_EQ(BigInt(-1) * value, -value);
    EXPECT_EQ(value * BigInt(256), BigInt("-316049379871604937987160493798716049379840"));
    EXPECT_EQ(BigInt(65536) * value, BigInt("-80908641247130864124713086412471308641239040"));
}

TEST(BigIntArithmeticTest, RandomSmallValuesMatchInt64Reference) {
    std::mt19937_64 random(0x6A09E667F3BCC909ULL);
    std::uniform_int_distribution<int64_t> values(-1'000'000'000LL, 1'000'000'000LL);
    for(int iteration = 0; iteration < 5000; ++iteration) {
        const int64_t lhs = values(random);
        const int64_t rhs = values(random);
        const BigInt a(std::to_string(lhs));
        const BigInt b(std::to_string(rhs));
        EXPECT_EQ((a + b).ToString(), std::to_string(lhs + rhs));
        EXPECT_EQ((a - b).ToString(), std::to_string(lhs - rhs));
        EXPECT_EQ((a * b).ToString(), std::to_string(lhs * rhs));
        EXPECT_EQ(a < b, lhs < rhs);
        EXPECT_EQ(a == b, lhs == rhs);
    }
}

TEST(BigIntDivisionTest, HandlesSpecialCasesAndSignedTruncation) {
    struct Case {
        const char* dividend;
        const char* divisor;
        const char* quotient;
        const char* remainder;
    };
    constexpr Case cases[] = {
        {"7", "3", "2", "1"},
        {"-7", "3", "-2", "-1"},
        {"7", "-3", "-2", "1"},
        {"-7", "-3", "2", "-1"},
        {"0", "37", "0", "0"},
        {"37", "37", "1", "0"},
        {"12", "37", "0", "12"},
    };
    for(const Case& test : cases) {
        const BigInt dividend(test.dividend);
        const BigInt divisor(test.divisor);
        const auto [quotient, remainder] = dividend.divmod(divisor);
        EXPECT_EQ(quotient, BigInt(test.quotient));
        EXPECT_EQ(remainder, BigInt(test.remainder));
        EXPECT_EQ(dividend / divisor, quotient);
        EXPECT_EQ(dividend % divisor, remainder);
        EXPECT_EQ(quotient * divisor + remainder, dividend);
    }

    EXPECT_THROW((void)(BigInt(1) / BigInt::ZERO), std::runtime_error);
    EXPECT_THROW((void)(BigInt(1) % BigInt::ZERO), std::runtime_error);
    EXPECT_THROW((void)BigInt(1).divmod(BigInt::ZERO), std::runtime_error);
}

TEST(BigIntDivisionTest, CompoundAssignmentsMatchBinaryOperators) {
    const BigInt divisor("9876543210987654321");
    BigInt quotient("123456789012345678901234567890");
    const BigInt original = quotient;
    quotient /= divisor;
    EXPECT_EQ(quotient, original / divisor);

    BigInt remainder = original;
    remainder %= divisor;
    EXPECT_EQ(remainder, original % divisor);
    EXPECT_EQ(quotient * divisor + remainder, original);
}

TEST(BigIntDivisionTest, RandomSmallValuesMatchInt64Reference) {
    std::mt19937_64 random(0xA54FF53A5F1D36F1ULL);
    std::uniform_int_distribution<int64_t> dividends(-1'000'000'000'000LL, 1'000'000'000'000LL);
    std::uniform_int_distribution<int64_t> divisors(-1'000'000'000LL, 1'000'000'000LL);
    for(int iteration = 0; iteration < 10'000; ++iteration) {
        const int64_t lhs = dividends(random);
        int64_t rhs = divisors(random);
        if(rhs == 0)
            rhs = 1;
        const BigInt dividend(std::to_string(lhs));
        const BigInt divisor(std::to_string(rhs));
        const auto [quotient, remainder] = dividend.divmod(divisor);
        EXPECT_EQ(quotient.ToString(), std::to_string(lhs / rhs));
        EXPECT_EQ(remainder.ToString(), std::to_string(lhs % rhs));
        EXPECT_EQ(quotient * divisor + remainder, dividend);
    }
}

TEST(BigIntDivisionTest, DividesByPowersOfTwoAcrossLimbBoundaries) {
    const BigInt dividend(StructuredHex(140, 0xE1234567U, 0x9E3779B9U));
    for(const size_t shift : {size_t{0}, size_t{1}, size_t{31}, size_t{32}, size_t{33},
                              size_t{63}, size_t{64}, size_t{127}, size_t{1024}, size_t{4096}}) {
        const BigInt divisor("0b1" + std::string(shift, '0'));
        const auto [quotient, remainder] = dividend.divmod(divisor);
        EXPECT_EQ(quotient * divisor + remainder, dividend) << "shift=" << shift;
        EXPECT_GE(remainder, BigInt::ZERO) << "shift=" << shift;
        EXPECT_LT(remainder, divisor) << "shift=" << shift;
    }
}

TEST(BigIntKnuthDivisionTest, RecoversConstructedQuotientAndRemainder) {
    for(const size_t divisorLimbs : {size_t{2}, size_t{3}, size_t{8}, size_t{31},
                                     size_t{32}, size_t{64}, size_t{127}, size_t{256}, size_t{512}}) {
        const BigInt divisor(StructuredHex(divisorLimbs, 0xF1234567U, 0x243F6A88U));
        const BigInt expectedRemainder(StructuredHex(divisorLimbs, 0x1234567U, 0x85A308D3U));
        for(const size_t quotientLimbs : {size_t{1}, size_t{2}, size_t{17}, size_t{65}}) {
            const BigInt expectedQuotient(StructuredHex(quotientLimbs, 0x81234567U, 0x13198A2EU));
            const BigInt dividend = divisor * expectedQuotient + expectedRemainder;
            const auto [quotient, remainder] = dividend.divmod(divisor);
            EXPECT_EQ(quotient, expectedQuotient)
                << "divisor limbs=" << divisorLimbs << ", quotient limbs=" << quotientLimbs;
            EXPECT_EQ(remainder, expectedRemainder)
                << "divisor limbs=" << divisorLimbs << ", quotient limbs=" << quotientLimbs;
        }
    }
}

TEST(BigIntBurnikelZieglerDivisionTest, RecoversConstructedQuotientAndRemainder) {
    for(const size_t divisorLimbs : {size_t{1024}, size_t{1025}}) {
        const BigInt divisor(StructuredHex(divisorLimbs, 0xF1234567U, 0xA4093822U));
        const BigInt expectedRemainder(StructuredHex(divisorLimbs, 0x1234567U, 0x299F31D0U));
        for(const size_t quotientLimbs : {size_t{1}, size_t{64}, size_t{257}, size_t{1025}}) {
            const BigInt expectedQuotient(StructuredHex(quotientLimbs, 0x91234567U, 0x082EFA98U));
            const BigInt dividend = BigInt::multiply(divisor, expectedQuotient) + expectedRemainder;
            const auto [quotient, remainder] = dividend.divmod(divisor);
            EXPECT_EQ(quotient, expectedQuotient)
                << "divisor limbs=" << divisorLimbs << ", quotient limbs=" << quotientLimbs;
            EXPECT_EQ(remainder, expectedRemainder)
                << "divisor limbs=" << divisorLimbs << ", quotient limbs=" << quotientLimbs;
            EXPECT_EQ(quotient * divisor + remainder, dividend);
        }
    }
}

TEST(BigIntDivisionTest, RandomLargeConstructedCasesRecoverUniqueQuotientAndRemainder) {
    std::mt19937_64 random(0x452821E638D01377ULL);
    constexpr size_t divisorSizes[] = {2, 7, 33, 64, 127, 128, 256, 512, 1024, 1025};
    for(const size_t divisorLimbs : divisorSizes) {
        const int repetitions = divisorLimbs < 128 ? 20 : (divisorLimbs < 1024 ? 8 : 2);
        for(int iteration = 0; iteration < repetitions; ++iteration) {
            const uint32_t seed = static_cast<uint32_t>(random());
            const BigInt divisor(StructuredHex(
                divisorLimbs, 0xE0000000U | (static_cast<uint32_t>(random()) & 0x0FFFFFFFU), seed));
            const BigInt remainder(StructuredHex(
                divisorLimbs, 0x10000000U | (static_cast<uint32_t>(random()) & 0x0FFFFFFFU), seed ^ 0x9E3779B9U));
            const size_t quotientLimbs = 1 + static_cast<size_t>(random() % (divisorLimbs + 37));
            const BigInt expectedQuotient(RandomHex(random, quotientLimbs));
            const BigInt dividend = BigInt::multiply(divisor, expectedQuotient) + remainder;
            const auto [quotient, actualRemainder] = dividend.divmod(divisor);
            EXPECT_EQ(quotient, expectedQuotient)
                << "divisor limbs=" << divisorLimbs << ", iteration=" << iteration;
            EXPECT_EQ(actualRemainder, remainder)
                << "divisor limbs=" << divisorLimbs << ", iteration=" << iteration;
        }
    }
}

TEST(BigIntKaratsubaTest, AdaptiveAndExplicitMultiplicationAgreeAroundThresholdAndBeyond) {
    std::mt19937_64 random(0xBB67AE8584CAA73BULL);
    constexpr std::size_t sizes[] = {31, 48, 63, 64, 65, 96, 128, 257};
    for(std::size_t lhsSize : sizes) {
        for(std::size_t rhsSize : sizes) {
            const BigInt lhs(RandomHex(random, lhsSize));
            const BigInt rhs(RandomHex(random, rhsSize));
            EXPECT_EQ(BigInt::multiply(lhs, rhs), lhs * rhs)
                << "limbs: " << lhsSize << " x " << rhsSize;
            EXPECT_EQ(BigInt::multiply(-lhs, rhs), -(lhs * rhs));
        }
    }
}

TEST(BigIntKaratsubaTest, HandlesHighlyUnbalancedOperands) {
    std::mt19937_64 random(0x3C6EF372FE94F82BULL);
    const BigInt shortValue(RandomHex(random, 96));
    const BigInt longValue(RandomHex(random, 513));
    const BigInt product = shortValue * longValue;
    EXPECT_EQ(BigInt::multiply(shortValue, longValue), product);
    EXPECT_EQ(BigInt::multiply(longValue, shortValue), product);

    const auto [quotient, remainder] = product.divmod(shortValue);
    EXPECT_EQ(quotient, longValue);
    EXPECT_EQ(remainder, BigInt(0));
}


TEST(BigIntStreamTest, FormatsLikeOtherProjectIntegerTypes) {
    const BigInt value("-18446744073709551615");
    std::ostringstream output;
    output << std::showbase << std::uppercase << std::hex << value;
    EXPECT_EQ(output.str(), "-0XFFFFFFFFFFFFFFFF");

    output.str({});
    output.clear();
    output << std::showpos << std::dec << BigInt(42);
    EXPECT_EQ(output.str(), "+42");

    output.str({});
    output.clear();
    output << std::nouppercase << std::showbase << std::hex << std::setfill('_')
           << std::internal << std::setw(10) << BigInt(-42);
    EXPECT_EQ(output.str(), "-0x_____2a");

    std::wostringstream wide;
    wide << std::showbase << std::hex << BigInt(0xABCD);
    EXPECT_EQ(wide.str(), L"0xabcd");
}

TEST(BigIntStreamTest, ParsesBasesGroupingPartialTokensAndErrors) {
    std::istringstream input("42 052 -0x2a 123xyz");
    BigInt decimal, octal, hexadecimal, partial;
    input >> decimal >> std::setbase(0) >> octal >> hexadecimal >> std::dec >> partial;
    EXPECT_EQ(decimal, BigInt(42));
    EXPECT_EQ(octal, BigInt(42));
    EXPECT_EQ(hexadecimal, BigInt(-42));
    EXPECT_EQ(partial, BigInt(123));
    EXPECT_EQ(input.peek(), 'x');
    EXPECT_FALSE(input.fail());

    std::istringstream grouped("1_234_567");
    grouped.imbue(std::locale(std::locale::classic(), new underscore_grouping_bigint));
    BigInt groupedValue;
    grouped >> groupedValue;
    EXPECT_EQ(groupedValue, BigInt("1234567"));
    EXPECT_TRUE(grouped.eof());
    EXPECT_FALSE(grouped.fail());

    BigInt invalidValue(7);
    std::istringstream invalid("xyz");
    invalid >> invalidValue;
    EXPECT_TRUE(invalid.fail());
    EXPECT_EQ(invalidValue, BigInt::ZERO);

    std::istringstream empty("");
    BigInt unchanged(7);
    empty >> unchanged;
    EXPECT_TRUE(empty.fail());
    EXPECT_TRUE(empty.eof());
    EXPECT_EQ(unchanged, BigInt(7));
}

} // namespace
