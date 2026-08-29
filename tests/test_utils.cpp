#include <gtest/gtest.h>

#include "utils.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

TEST(StringToDigitsTest, ParsesSupportedBasesIntoBigEndianLimbs) {
    bool negative = true;

    EXPECT_EQ(StringToDigits("4294967296", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_FALSE(negative);

    EXPECT_EQ(StringToDigits("0x100000000", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_EQ(StringToDigits("0o40000000000", &negative),
              (std::vector<uint32_t>{1, 0}));
    EXPECT_EQ(StringToDigits("0b100000000000000000000000000000000", &negative),
              (std::vector<uint32_t>{1, 0}));
}

TEST(StringToDigitsTest, ParsesSignsAndLeadingZeroes) {
    bool negative = false;

    EXPECT_EQ(StringToDigits("+42", &negative),
              (std::vector<uint32_t>{42}));
    EXPECT_FALSE(negative);

    EXPECT_EQ(StringToDigits("-42", &negative),
              (std::vector<uint32_t>{42}));
    EXPECT_TRUE(negative);

    // Contract-level regression: every spelling of zero should normalize to one zero limb.
    EXPECT_EQ(StringToDigits("00", &negative),
              (std::vector<uint32_t>{0}));
    EXPECT_FALSE(negative);
}

TEST(StringToDigitsTest, RejectsMalformedInput) {
    bool negative = false;

    EXPECT_THROW(StringToDigits("", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("+", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("-", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("12z", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("0b102", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("0o128", &negative), std::invalid_argument);
    EXPECT_THROW(StringToDigits("0x12g", &negative), std::invalid_argument);
}

TEST(StringToDigitsTest, HandlesMaximum128BitMagnitude) {
    bool negative = true;
    const std::vector<uint32_t> expected{
        0xFFFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU
    };

    EXPECT_EQ(StringToDigits("340282366920938463463374607431768211455", &negative),
              expected);
    EXPECT_FALSE(negative);
    EXPECT_EQ(StringToDigits("0xffffffffffffffffffffffffffffffff", &negative),
              expected);
}
