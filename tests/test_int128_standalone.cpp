#include <gtest/gtest.h>

#include "int128.h"

TEST(Int128StandaloneHeaderTest, DivisionDoesNotRequireUint128Header) {
    const int128_t dividend("-170141183460469231731687303715884105727");
    const int128_t divisor("18446744073709551617");

    const int128_t quotient = dividend / divisor;
    const int128_t remainder = dividend % divisor;

    EXPECT_EQ(quotient * divisor + remainder, dividend);
    EXPECT_LT(remainder, int128_t(0));
}
