#include <gtest/gtest.h>

#include "int128.h"
#include "uint128.h"

#include <iomanip>
#include <ios>
#include <locale>
#include <sstream>
#include <string>

namespace {

class underscore_grouping final : public std::numpunct<char> {
protected:
    char do_thousands_sep() const override { return '_'; }
    std::string do_grouping() const override { return "\3"; }
};

TEST(Uint128StreamTest, FormatsBasesPrefixesCaseAndSigns) {
    std::ostringstream output;
    output << uint128_t::max();
    EXPECT_EQ(output.str(), "340282366920938463463374607431768211455");

    output.str({});
    output.clear();
    output << std::showbase << std::uppercase << std::hex << uint128_t("340282366920938463463374607431768211455");
    EXPECT_EQ(output.str(), "0XFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

    output.str({});
    output.clear();
    output << std::showbase << std::oct << uint128_t(64);
    EXPECT_EQ(output.str(), "0100");

    output.str({});
    output.clear();
    output << std::showbase << std::showpos << std::hex << uint128_t(0);
    EXPECT_EQ(output.str(), "0");
}

TEST(Uint128StreamTest, AppliesWidthFillAndAlignmentLikeAnInteger) {
    std::ostringstream output;
    output << std::showbase << std::hex << std::setfill('_')
           << std::right << std::setw(8) << uint128_t(42);
    EXPECT_EQ(output.str(), "____0x2a");

    output.str({});
    output.clear();
    output << std::showbase << std::hex << std::setfill('_')
           << std::internal << std::setw(8) << uint128_t(42);
    EXPECT_EQ(output.str(), "0x____2a");

    output.str({});
    output.clear();
    output << std::showbase << std::hex << std::setfill('_')
           << std::left << std::setw(8) << uint128_t(42);
    EXPECT_EQ(output.str(), "0x2a____");

    output.str({});
    output.clear();
    output << std::dec << std::right << std::setfill(' ')
           << std::setw(4) << uint128_t(1) << uint128_t(2);
    EXPECT_EQ(output.str(), "   12");
}

TEST(Int128StreamTest, FormatsSignedDecimalAndTwosComplementNonDecimal) {
    std::ostringstream output;
    output << std::showpos << int128_t(42) << ' ' << int128_t(0) << ' ' << int128_t(-42);
    EXPECT_EQ(output.str(), "+42 +0 -42");

    output.str({});
    output.clear();
    output << std::showbase << std::hex << int128_t(-1);
    EXPECT_EQ(output.str(), "0xffffffffffffffffffffffffffffffff");

    output.str({});
    output.clear();
    output << std::showpos << std::setfill('_') << std::internal
           << std::setw(7) << std::dec << int128_t(42);
    EXPECT_EQ(output.str(), "+____42");
}

TEST(Integral128StreamTest, SupportsWideStreamsAndLocaleGrouping) {
    std::wostringstream wide_output;
    wide_output << std::showbase << std::uppercase << std::hex << uint128_t(0xABCDU);
    EXPECT_EQ(wide_output.str(), L"0XABCD");

    std::ostringstream grouped_output;
    grouped_output.imbue(std::locale(std::locale::classic(), new underscore_grouping));
    grouped_output << uint128_t(1'234'567U) << ' ' << int128_t(-1'234'567);
    EXPECT_EQ(grouped_output.str(), "1_234_567 -1_234_567");

    std::istringstream grouped_input("1_234_567 -1_234_567");
    grouped_input.imbue(std::locale(std::locale::classic(), new underscore_grouping));
    uint128_t unsigned_value;
    int128_t signed_value;
    grouped_input >> unsigned_value >> signed_value;
    EXPECT_TRUE(grouped_input.eof());
    EXPECT_FALSE(grouped_input.fail());
    EXPECT_EQ(unsigned_value, uint128_t(1'234'567U));
    EXPECT_EQ(signed_value, int128_t(-1'234'567));
}

TEST(Uint128StreamTest, ParsesBasesPrefixesSignsAndPartialTokens) {
    std::istringstream input("  42 052 0x2a -1 123xyz");
    uint128_t decimal, octal, hexadecimal, negative, partial;
    input >> decimal >> std::setbase(0) >> octal >> hexadecimal >> negative >> std::dec >> partial;

    EXPECT_EQ(decimal, uint128_t(42));
    EXPECT_EQ(octal, uint128_t(42));
    EXPECT_EQ(hexadecimal, uint128_t(42));
    EXPECT_EQ(negative, uint128_t::max());
    EXPECT_EQ(partial, uint128_t(123));
    EXPECT_EQ(input.peek(), 'x');
    EXPECT_FALSE(input.fail());
}

TEST(Int128StreamTest, ParsesLimitsInDecimalAndHexadecimal) {
    std::istringstream input(
        "170141183460469231731687303715884105727 "
        "-170141183460469231731687303715884105728 0x7f");
    int128_t maximum, minimum, hexadecimal;
    input >> maximum >> minimum >> std::setbase(0) >> hexadecimal;

    EXPECT_EQ(maximum, int128_t::max());
    EXPECT_EQ(minimum, int128_t::min());
    EXPECT_EQ(hexadecimal, int128_t(127));
    EXPECT_TRUE(input.eof());
    EXPECT_FALSE(input.fail());
}

TEST(Integral128StreamTest, ReportsMalformedAndOutOfRangeInput) {
    uint128_t unsigned_value(7U);
    std::istringstream invalid("xyz");
    invalid >> unsigned_value;
    EXPECT_TRUE(invalid.fail());
    EXPECT_EQ(unsigned_value, uint128_t(0U));

    std::istringstream empty("");
    unsigned_value = uint128_t(7U);
    empty >> unsigned_value;
    EXPECT_TRUE(empty.fail());
    EXPECT_TRUE(empty.eof());
    EXPECT_EQ(unsigned_value, uint128_t(7U));

    std::istringstream unsigned_overflow("340282366920938463463374607431768211456");
    unsigned_overflow >> unsigned_value;
    EXPECT_TRUE(unsigned_overflow.fail());
    EXPECT_TRUE(unsigned_overflow.eof());
    EXPECT_EQ(unsigned_value, uint128_t::max());

    int128_t signed_value;
    std::istringstream positive_overflow("170141183460469231731687303715884105728");
    positive_overflow >> signed_value;
    EXPECT_TRUE(positive_overflow.fail());
    EXPECT_EQ(signed_value, int128_t::max());

    std::istringstream negative_overflow("-170141183460469231731687303715884105729");
    negative_overflow >> signed_value;
    EXPECT_TRUE(negative_overflow.fail());
    EXPECT_EQ(signed_value, int128_t::min());
}

TEST(Integral128StreamTest, ValidatesGroupingAndHonorsExceptionMask) {
    std::istringstream malformed_group("12_34");
    malformed_group.imbue(std::locale(std::locale::classic(), new underscore_grouping));
    uint128_t value;
    malformed_group >> value;
    EXPECT_TRUE(malformed_group.fail());
    EXPECT_EQ(value, uint128_t(1234U));

    std::istringstream throwing("340282366920938463463374607431768211456");
    throwing.exceptions(std::ios_base::failbit);
    EXPECT_THROW(throwing >> value, std::ios_base::failure);
    EXPECT_EQ(value, uint128_t::max());
}

} // namespace
