#include <gtest/gtest.h>

#include "utils.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

TEST(StringToDigitsTest, ParsesSupportedBasesIntoBigEndianLimbs) {
    bool negative = true;

    EXPECT_EQ(string_to_digits("4294967296", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_FALSE(negative);

    EXPECT_EQ(string_to_digits("0x100000000", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_EQ(string_to_digits("0o40000000000", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_EQ(string_to_digits("0b100000000000000000000000000000000", &negative),
              (std::vector<uint32_t>{1, 0}));
}

TEST(StringToDigitsTest, ParsesSignsAndLeadingZeroes) {
    bool negative = false;

    EXPECT_EQ(string_to_digits("+42", &negative),
              (std::vector<uint32_t>{42}));
    EXPECT_FALSE(negative);

    EXPECT_EQ(string_to_digits("-42", &negative),
              (std::vector<uint32_t>{42}));
    EXPECT_TRUE(negative);

    // Contract-level regression: every spelling of zero should normalize to one zero limb.
    EXPECT_EQ(string_to_digits("00", &negative),
              (std::vector<uint32_t>{0}));
    EXPECT_FALSE(negative);
}

TEST(StringToDigitsTest, RejectsMalformedInput) {
    bool negative = false;

    EXPECT_THROW(string_to_digits("", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("+", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("-", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("12z", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("0b102", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("0o128", &negative), std::invalid_argument);
    EXPECT_THROW(string_to_digits("0x12g", &negative), std::invalid_argument);
}

TEST(StringToDigitsTest, HandlesMaximum128BitMagnitude) {
    bool negative = true;
    const std::vector<uint32_t> expected{
        0xFFFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU
    };

    EXPECT_EQ(string_to_digits("340282366920938463463374607431768211455", &negative),
              expected);
    EXPECT_FALSE(negative);
    EXPECT_EQ(string_to_digits("0xffffffffffffffffffffffffffffffff", &negative),
              expected);
}
