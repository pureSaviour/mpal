#ifndef INTEGRAL_128_H
#define INTEGRAL_128_H

#include <bit>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

struct int128_t;
struct uint128_t;

#ifndef MPAL_USE_NATIVE_INT128
    #define MPAL_USE_NATIVE_INT128 1
#endif

#if defined(MPAL_USE_NATIVE_INT128) && MPAL_USE_NATIVE_INT128 && defined(__SIZEOF_INT128__)
    #define MPAL_HAS_NATIVE_I128 1
#else
    #define MPAL_HAS_NATIVE_I128 0
#endif

#if defined(_MSC_VER)
    #define MPAL_FORCE_INLINE __forceinline
#else
    #define MPAL_FORCE_INLINE inline
#endif

#if !MPAL_HAS_NATIVE_I128
    #include <intrin.h>
#endif

namespace integral128_detail {
#if MPAL_HAS_NATIVE_I128
    using unsigned_rep = __uint128_t;

    struct unsigned_divmod_result {
        unsigned_rep quotient;
        unsigned_rep remainder;
    };

    inline constexpr unsigned_rep unsigned_zero() { return 0; }
    inline constexpr unsigned_rep unsigned_sub(unsigned_rep a, unsigned_rep b) { return a - b; }

    inline constexpr unsigned_divmod_result unsigned_divmod(
        unsigned_rep dividend, unsigned_rep divisor) {
        if(divisor == 0) {
            throw std::runtime_error("Division by zero");
        }
        return {dividend / divisor, dividend % divisor};
    }
#else
    struct unsigned_rep {
        uint64_t high;
        uint64_t low;
    };

    struct unsigned_divmod_result {
        unsigned_rep quotient;
        unsigned_rep remainder;
    };

    inline constexpr unsigned_rep unsigned_zero() { return {0, 0}; }
    inline constexpr unsigned_rep unsigned_one() { return {0, 1}; }
    inline constexpr bool unsigned_truthy(const unsigned_rep& value) {
        return value.high != 0 || value.low != 0;
    }
    inline constexpr bool unsigned_eq(const unsigned_rep& a, const unsigned_rep& b) {
        return a.high == b.high && a.low == b.low;
    }
    inline constexpr bool unsigned_lt(const unsigned_rep& a, const unsigned_rep& b) {
        return a.high < b.high || (a.high == b.high && a.low < b.low);
    }
    inline constexpr unsigned_rep unsigned_sub(const unsigned_rep& a, const unsigned_rep& b) {
        const uint64_t low = a.low - b.low;
        return {a.high - b.high - static_cast<uint64_t>(low > a.low), low};
    }
    inline constexpr unsigned_rep unsigned_shl(const unsigned_rep& value, unsigned int shift) {
        if(shift >= 128) return {0, 0};
        if(shift >= 64) return {value.low << (shift - 64), 0};
        if(shift == 0) return value;
        return {(value.high << shift) | (value.low >> (64 - shift)), value.low << shift};
    }
    inline constexpr unsigned_rep unsigned_shr(const unsigned_rep& value, unsigned int shift) {
        if(shift >= 128) return {0, 0};
        if(shift >= 64) return {0, value.high >> (shift - 64)};
        if(shift == 0) return value;
        return {value.high >> shift, (value.low >> shift) | (value.high << (64 - shift))};
    }
    inline constexpr unsigned_rep unsigned_and(const unsigned_rep& a, const unsigned_rep& b) {
        return {a.high & b.high, a.low & b.low};
    }
    inline constexpr unsigned int unsigned_bit_width(const unsigned_rep& value) {
        if(value.high != 0) {
            return 128U - static_cast<unsigned int>(std::countl_zero(value.high));
        }
        return 64U - static_cast<unsigned int>(std::countl_zero(value.low));
    }
    inline constexpr bool unsigned_is_power_of_two(const unsigned_rep& value) {
        if(value.high == 0) {
            return value.low != 0 && (value.low & (value.low - 1)) == 0;
        }
        return value.low == 0 && (value.high & (value.high - 1)) == 0;
    }
    inline constexpr unsigned int unsigned_countr_zero(const unsigned_rep& value) {
        if(value.low != 0) {
            return static_cast<unsigned int>(std::countr_zero(value.low));
        }
        return 64U + static_cast<unsigned int>(std::countr_zero(value.high));
    }

    inline constexpr unsigned_divmod_result unsigned_divmod_shift(
        const unsigned_rep& dividend, const unsigned_rep& divisor) {
        if(!unsigned_truthy(divisor)) {
            throw std::runtime_error("Division by zero");
        }
        if(unsigned_lt(dividend, divisor)) {
            return {unsigned_zero(), dividend};
        }

        unsigned_rep quotient = unsigned_zero();
        unsigned_rep remainder = dividend;
        const int shift = static_cast<int>(
            unsigned_bit_width(dividend) - unsigned_bit_width(divisor));
        unsigned_rep shiftedDivisor = unsigned_shl(divisor, static_cast<unsigned int>(shift));
        for(int bit = shift; bit >= 0; --bit) {
            if(!unsigned_lt(remainder, shiftedDivisor)) {
                remainder = unsigned_sub(remainder, shiftedDivisor);
                if(bit >= 64) {
                    quotient.high |= uint64_t{1} << static_cast<unsigned int>(bit - 64);
                }
                else {
                    quotient.low |= uint64_t{1} << static_cast<unsigned int>(bit);
                }
            }
            shiftedDivisor = unsigned_shr(shiftedDivisor, 1U);
        }
        return {quotient, remainder};
    }

    inline constexpr uint64_t unsigned_div_128_by_64_portable(
        uint64_t high, uint64_t low, uint64_t divisor, uint64_t& remainder) {
        uint64_t quotient = 0;
        uint64_t rem = high;
        for(int bit = 63; bit >= 0; --bit) {
            const bool overflow = (rem >> 63U) != 0;
            rem = (rem << 1U) | ((low >> static_cast<unsigned int>(bit)) & 1U);
            if(overflow || rem >= divisor) {
                rem -= divisor;
                quotient |= uint64_t{1} << static_cast<unsigned int>(bit);
            }
        }
        remainder = rem;
        return quotient;
    }

    inline uint64_t unsigned_div_128_by_64_runtime(
        uint64_t high, uint64_t low, uint64_t divisor, uint64_t& remainder) {
        #if defined(_MSC_VER) && defined(_M_X64)
            return ::_udiv128(high, low, divisor, &remainder);
        #elif defined(__GNUC__) && defined(__x86_64__)
            uint64_t quotient;
            asm volatile(
                "divq %[divisor]"
                : "=a"(quotient), "=d"(remainder)
                : "a"(low), "d"(high), [divisor] "r"(divisor)
                : "cc");
            return quotient;
        #else
            return unsigned_div_128_by_64_portable(high, low, divisor, remainder);
        #endif
    }

    inline unsigned_divmod_result unsigned_divmod_128_by_64(
        const unsigned_rep& dividend, uint64_t divisor) {
        const uint64_t quotientHigh = dividend.high / divisor;
        uint64_t remainder = dividend.high % divisor;
        const uint64_t quotientLow = unsigned_div_128_by_64_runtime(
            remainder, dividend.low, divisor, remainder);
        return {{quotientHigh, quotientLow}, {0, remainder}};
    }

    inline unsigned_divmod_result unsigned_divmod_128_by_128(
        const unsigned_rep& dividend, const unsigned_rep& divisor) {
        const unsigned int shift = static_cast<unsigned int>(std::countl_zero(divisor.high));
        uint64_t numeratorHigh;
        uint64_t numeratorLow;
        uint64_t numeratorExtra;
        uint64_t divisorHigh;
        uint64_t divisorLow;

        if(shift == 0) {
            numeratorHigh = 0;
            numeratorLow = dividend.high;
            numeratorExtra = dividend.low;
            divisorHigh = divisor.high;
            divisorLow = divisor.low;
        }
        else {
            numeratorHigh = dividend.high >> (64U - shift);
            numeratorLow = (dividend.high << shift) | (dividend.low >> (64U - shift));
            numeratorExtra = dividend.low << shift;
            divisorHigh = (divisor.high << shift) | (divisor.low >> (64U - shift));
            divisorLow = divisor.low << shift;
        }

        uint64_t estimateRemainder = 0;
        uint64_t quotient = unsigned_div_128_by_64_runtime(
            numeratorHigh, numeratorLow, divisorHigh, estimateRemainder);

        for(int correction = 0; correction < 2; ++correction) {
            uint64_t productHigh = 0;
            const uint64_t productLow = _umul128(quotient, divisorLow, &productHigh);
            if(productHigh < estimateRemainder
                || (productHigh == estimateRemainder && productLow <= numeratorExtra)) {
                break;
            }
            --quotient;
            const uint64_t previousRemainder = estimateRemainder;
            estimateRemainder += divisorHigh;
            if(estimateRemainder < previousRemainder) {
                break;
            }
        }

        uint64_t productCarry = 0;
        const uint64_t productLow = _umul128(quotient, divisor.low, &productCarry);
        const unsigned_rep product{productCarry + quotient * divisor.high, productLow};
        return {{0, quotient}, unsigned_sub(dividend, product)};
    }

    inline unsigned_divmod_result unsigned_divmod_runtime(
        const unsigned_rep& dividend, const unsigned_rep& divisor) {
        if(!unsigned_truthy(divisor)) {
            throw std::runtime_error("Division by zero");
        }
        if(unsigned_lt(dividend, divisor)) {
            return {unsigned_zero(), dividend};
        }
        if(unsigned_eq(dividend, divisor)) {
            return {unsigned_one(), unsigned_zero()};
        }
        if(unsigned_eq(divisor, unsigned_one())) {
            return {dividend, unsigned_zero()};
        }
        if(unsigned_is_power_of_two(divisor)) {
            const unsigned int shift = unsigned_countr_zero(divisor);
            return {
                unsigned_shr(dividend, shift),
                unsigned_and(dividend, unsigned_sub(divisor, unsigned_one()))
            };
        }
        if(divisor.high == 0) {
            return unsigned_divmod_128_by_64(dividend, divisor.low);
        }
        return unsigned_divmod_128_by_128(dividend, divisor);
    }

    inline constexpr unsigned_divmod_result unsigned_divmod(
        const unsigned_rep& dividend, const unsigned_rep& divisor) {
        if consteval {
            return unsigned_divmod_shift(dividend, divisor);
        }
        else {
            return unsigned_divmod_runtime(dividend, divisor);
        }
    }
#endif
}

#if MPAL_HAS_NATIVE_I128
template<typename T>
concept integral_128 = std::is_integral_v<T> || std::is_same_v<T, int128_t>
    || std::is_same_v<T, uint128_t> || std::is_same_v<T, __int128_t>
    || std::is_same_v<T, __uint128_t>;

template<typename T>
concept signed128 = std::is_integral_v<T> || std::is_same_v<T, int128_t>
    || std::is_same_v<T, __int128_t>;

template<typename T>
concept unsigned128 = std::is_integral_v<T> || std::is_same_v<T, uint128_t>
    || std::is_same_v<T, __uint128_t>;
#else
template<typename T>
concept integral_128 = std::is_integral_v<T> || std::is_same_v<T, int128_t>
    || std::is_same_v<T, uint128_t>;

template<typename T>
concept signed128 = std::is_integral_v<T> || std::is_same_v<T, int128_t>;

template<typename T>
concept unsigned128 = std::unsigned_integral<T> || std::is_same_v<T, uint128_t>;
#endif

#endif // INTEGRAL_128_H
