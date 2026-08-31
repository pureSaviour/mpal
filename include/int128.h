#ifndef INT128_H
#define INT128_H

#include <bit>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <string>
#include <format>
#include "integral_128.h"
#include "utils.h"
#include "utils_stream.h"
namespace i128_impl{
    #if MPAL_HAS_NATIVE_I128
        using rep = __int128_t;        

        template <integral_128 T>
        inline constexpr rep make(const T& value) { 
            if constexpr(std::is_same_v<T, int128_t>)
                return value.v_;
            else if constexpr(std::is_same_v<T, uint128_t>)
                return std::bit_cast<rep>(value.v_);
            else
                return static_cast<rep>(value);            
        }

        inline constexpr rep add(const rep& a, const rep& b) { return a + b; }
        inline constexpr rep sub(const rep& a, const rep& b) { return a - b; }
        inline constexpr void add_assign(rep& a, const rep& b) {
            using unsigned_rep = __uint128_t;
            a = std::bit_cast<rep>(std::bit_cast<unsigned_rep>(a) + std::bit_cast<unsigned_rep>(b));
        }
        inline constexpr void sub_assign(rep& a, const rep& b) {
            using unsigned_rep = __uint128_t;
            a = std::bit_cast<rep>(std::bit_cast<unsigned_rep>(a) - std::bit_cast<unsigned_rep>(b));
        }
        inline constexpr rep mul(const rep& a, const rep& b) { return a * b; }
        struct divmod_result { rep quotient; rep remainder; };
        inline constexpr divmod_result divmod(const rep& a, const rep& b) {
            if(b == 0){
                throw std::invalid_argument("Division by zero");
            }
            const bool dividendNegative = a < 0;
            const bool divisorNegative = b < 0;
            auto dividend = std::bit_cast<integral128_detail::unsigned_rep>(a);
            auto divisor = std::bit_cast<integral128_detail::unsigned_rep>(b);
            if(dividendNegative){
                dividend = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), dividend);
            }
            if(divisorNegative){
                divisor = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), divisor);
            }
            auto result = integral128_detail::unsigned_divmod(dividend, divisor);
            if(dividendNegative != divisorNegative){
                result.quotient = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), result.quotient);
            }
            if(dividendNegative){
                result.remainder = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), result.remainder);
            }
            return {
                std::bit_cast<rep>(result.quotient),
                std::bit_cast<rep>(result.remainder)
            };
        }
        inline constexpr rep div(const rep& a, const rep& b) { return divmod(a, b).quotient; }
        inline constexpr rep mod(const rep& a, const rep& b) { return divmod(a, b).remainder; }
        inline constexpr rep shl(const rep& a, unsigned int n) {
            if(n >= 128) return 0;
            return std::bit_cast<rep>(std::bit_cast<__uint128_t>(a) << n);
        }
        inline constexpr rep shr(const rep& a, unsigned int n) {
            if(n >= 128) return a < 0 ? static_cast<rep>(-1) : 0;
            return a >> n;
        }
        inline constexpr rep bit_and(const rep& a, const rep& b) { return a & b; }
        inline constexpr rep bit_or(const rep& a, const rep& b) { return a | b; }
        inline constexpr rep bit_xor(const rep& a, const rep& b) { return a ^ b; }
        inline constexpr rep bit_not(const rep& a) { return ~a; }
        inline constexpr bool eq(const rep& a, const rep& b) { return a == b; }
        inline constexpr bool lt(const rep& a, const rep& b) { return a < b; }
        inline constexpr bool gt(const rep& a, const rep& b) { return a > b; }
        inline constexpr bool truthy(const rep& a) { return a != 0; }
        inline constexpr uint64_t low(const rep& a){return static_cast<uint64_t>(a & 0xFF'FF'FF'FF'FF'FF'FF'FFULL);}
        inline constexpr uint64_t high(const rep& a){return static_cast<uint64_t>(a >> 64);}
        inline constexpr rep max(){return (static_cast<rep>(0x7FFF'FFFF'FFFF'FFFF) << 64) | static_cast<rep>(0xFFFF'FFFF'FFFF'FFFF);}
        inline constexpr rep min(){return (static_cast<rep>(0x8000'0000'0000'0000) << 64);}
    #else        
        using u64_t = uint64_t;
        using i64_t = int64_t;
       
        struct rep{           
            i64_t high = 0;
            u64_t low = 0;            
            // rep(u64_t low, i64_t high) : low(low), high(high) {}            
        };        
        template <integral_128 T>
        inline constexpr rep make(const T& value) {
            if constexpr (std::is_same_v<T, int128_t>){
                return value.v_;
            }
            else if constexpr (std::is_same_v<T, uint128_t>){
                return std::bit_cast<rep>(value.v_);
            }
            else
                return {static_cast<i64_t>(value < static_cast<T>(0) ? -1 : 0), static_cast<u64_t>(value)};
                // return {static_cast<u64_t>(value), static_cast<i64_t>(value < static_cast<T>(0) ? -1 : 0)};
        }        
        inline constexpr rep shl(const rep& a, unsigned int n){
            if(n >= 128) return {0, 0};
            if(n >= 64) return {static_cast<i64_t>(a.low << (n - 64)), 0};
            if(n == 0) return a;
            return {a.high << n | static_cast<i64_t>(a.low >> (64 - n)), a.low << n};
        }
        inline constexpr rep shr(const rep& a, unsigned int n){
            if(n >= 128) {
                if(a.high < 0)
                    return {-1, 0xFFFF'FFFF'FFFF'FFFFULL};
                return {0, 0};
            }
            if(n >= 64) {
                if(a.high < 0)
                    return {-1, static_cast<u64_t>(a.high >> (n - 64))};
                return {0, static_cast<u64_t>(a.high >> (n - 64))};
            }
            if(n == 0) return a;
            return {a.high >> n, (a.low >> n) | (a.high << (64 - n))};
        }
        inline constexpr rep bit_and(const rep& a, const rep& b) { return {a.high & b.high, a.low & b.low}; }
        inline constexpr rep bit_or(const rep& a, const rep& b) { return {a.high | b.high, a.low | b.low}; }
        inline constexpr rep bit_xor(const rep& a, const rep& b) { return {a.high ^ b.high, a.low ^ b.low}; }        
        inline constexpr rep bit_not(const rep& a) { return {~a.high, ~a.low}; }
        inline constexpr bool eq(const rep& a, const rep& b) { return a.low == b.low && a.high == b.high; }
        inline constexpr bool lt(const rep& a, const rep& b) { return (a.high < b.high) || (a.high == b.high && a.low < b.low); }
        inline constexpr bool gt(const rep& a, const rep& b) { return (a.high > b.high) || (a.high == b.high && a.low > b.low); }        
        inline constexpr bool truthy(const rep& a){return a.low != 0 || a.high != 0;}
        inline constexpr u64_t low(const rep& a){return a.low;}
        inline constexpr i64_t high(const rep& a){return a.high;}        
        inline constexpr rep add(const rep& a, const rep& b) { 
            u64_t lo = a.low + b.low;
            u64_t hi = a.high + b.high + (lo < a.low ? 1 : 0);
            return {static_cast<i64_t>(hi), lo};
        }
        inline constexpr rep sub(const rep& a, const rep& b){
            u64_t lo = a.low - b.low;
            u64_t hi = a.high - b.high - (a.low < b.low ? 1 : 0);
            return {static_cast<i64_t>(hi), lo};
        }
        inline constexpr void add_assign(rep& a, const rep& b) {
            const u64_t oldLow = a.low;
            a.low += b.low;
            const u64_t high = std::bit_cast<u64_t>(a.high) + std::bit_cast<u64_t>(b.high)
                + static_cast<u64_t>(a.low < oldLow);
            a.high = std::bit_cast<i64_t>(high);
        }
        inline constexpr void sub_assign(rep& a, const rep& b) {
            const u64_t low = a.low - b.low;
            const u64_t high = std::bit_cast<u64_t>(a.high) - std::bit_cast<u64_t>(b.high)
                - static_cast<u64_t>(low > a.low);
            a.low = low;
            a.high = std::bit_cast<i64_t>(high);
        }
        inline constexpr rep abs_impl(const rep& a) {
            return a.high < 0 ? sub(make(0), a) : a;
        }
        inline constexpr rep mul(const rep& a, const rep& b){
            u64_t low, high, res[2];
            low = _umul128(static_cast<u64_t>(a.low), static_cast<u64_t>(b.low), &high);
            res[0] = low;
            res[1] = high;
            res[1] += _umul128(static_cast<u64_t>(a.low), static_cast<u64_t>(b.high), &high) + _umul128(static_cast<u64_t>(a.high), static_cast<u64_t>(b.low), &high);
            return {static_cast<i64_t>(res[1]), res[0]};
        }

        inline constexpr rep max(){return {static_cast<i64_t>(0x7FFF'FFFF'FFFF'FFFFULL), static_cast<u64_t>(0xFFFF'FFFF'FFFF'FFFFULL)};}
        inline constexpr rep min(){return {static_cast<i64_t>(0x8000'0000'0000'0000), static_cast<u64_t>(0)};}

        struct divmod_result { rep quotient; rep remainder; };
        MPAL_FORCE_INLINE constexpr divmod_result divmod(const rep& a, const rep& b){
            if(!i128_impl::truthy(b)){
                throw std::invalid_argument("Division by zero");
            }
            const bool dividendNegative = a.high < 0;
            const bool divisorNegative = b.high < 0;
            auto dividend = std::bit_cast<integral128_detail::unsigned_rep>(a);
            auto divisor = std::bit_cast<integral128_detail::unsigned_rep>(b);
            if(dividendNegative){
                dividend = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), dividend);
            }
            if(divisorNegative){
                divisor = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), divisor);
            }

            auto result = integral128_detail::unsigned_divmod(dividend, divisor);
            if(dividendNegative != divisorNegative){
                result.quotient = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), result.quotient);
            }
            if(dividendNegative){
                result.remainder = integral128_detail::unsigned_sub(
                    integral128_detail::unsigned_zero(), result.remainder);
            }

            return {
                std::bit_cast<rep>(result.quotient),
                std::bit_cast<rep>(result.remainder)
            };
        }
        MPAL_FORCE_INLINE constexpr rep div(const rep& a, const rep& b){ return divmod(a, b).quotient; }
        MPAL_FORCE_INLINE constexpr rep mod(const rep& a, const rep& b){ return divmod(a, b).remainder; }
        
    #endif
}

struct int128_t{
    i128_impl::rep v_;
    constexpr int128_t() noexcept : v_(i128_impl::make(0)) {}
    constexpr int128_t(i128_impl::rep v) noexcept : v_(v) {}
    constexpr int128_t(const std::string& str){
        v_ = i128_impl::make(0);
        
        if(str.empty()){
            throw std::invalid_argument("Empty string for int128_t");
        }
        if(str.size() > 40){
            throw std::invalid_argument("String too long for int128_t: " + str);
        }
        bool isNegative = false;
        std::vector<uint32_t> digits;
        try{
            digits = string_to_digits(str, &isNegative);            
        }catch(const std::invalid_argument& e){
            throw std::invalid_argument("Invalid string for int128_t: " + str);
        }
        if(digits.size() > 4){
            throw std::invalid_argument("String is too big for int128_t: " + str);
        }
        for(size_t i = 0; i < digits.size(); ++i){
            v_ = i128_impl::add(i128_impl::shl(v_, 32), i128_impl::make(digits[i]));
        }
        if(isNegative)
            *this = -(*this);
    }
    template <integral_128 T>
    explicit constexpr int128_t(const T& value) : v_(i128_impl::make(value)) {}

    template <integral_128 T>
    inline constexpr int128_t operator+(const T& other) const noexcept{
        return int128_t(i128_impl::add(v_, i128_impl::make(other)));        
    }

    template <integral_128 T>
    inline constexpr int128_t operator-(const T& other) const noexcept{
        return int128_t(i128_impl::sub(v_, i128_impl::make(other)));
    }

    inline constexpr int128_t operator-() const noexcept{
        return int128_t(i128_impl::add(i128_impl::bit_not(v_), i128_impl::make(1)));
    }

    template <integral_128 T>
    inline constexpr int128_t operator*(const T& other) const noexcept{
        return int128_t(i128_impl::mul(v_, i128_impl::make(other)));
    }

    template <integral_128 T>
    inline constexpr int128_t operator/(const T& other) const{
        return int128_t(i128_impl::div(v_, i128_impl::make(other)));
    }

    template <integral_128 T>
    inline constexpr int128_t operator%(const T& other) const{
        return int128_t(i128_impl::mod(v_, i128_impl::make(other)));
    }

    template <integral_128 T>
    inline constexpr int128_t& operator+=(const T& other) noexcept {
        const i128_impl::rep rhs = i128_impl::make(other);
        i128_impl::add_assign(v_, rhs);
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr int128_t& operator-=(const T& other) noexcept{
        const i128_impl::rep rhs = i128_impl::make(other);
        i128_impl::sub_assign(v_, rhs);
        return *this;
    }

    template <integral_128 T>
    inline constexpr int128_t& operator*=(const T& other) noexcept{
        *this = *this * other;
        return *this;
    }

    template <integral_128 T>
    inline constexpr int128_t& operator/=(const T& other){
        *this = *this / other;
        return *this;
    }

    template <integral_128 T>
    inline constexpr int128_t& operator%=(const T& other){
        *this = *this % other;
        return *this;
    }

    inline constexpr int128_t operator<<(const unsigned int bit) const noexcept{
        return int128_t(i128_impl::shl(v_, bit));        
    }    
    inline constexpr int128_t operator>>(const unsigned int bit) const noexcept{
        return int128_t(i128_impl::shr(v_, bit));
    }
    inline constexpr int128_t operator&(const int128_t& other) const noexcept{
        return int128_t(i128_impl::bit_and(v_, other.v_));
    }
    inline constexpr int128_t operator|(const int128_t& other) const noexcept{
        return int128_t(i128_impl::bit_or(v_, other.v_));
    }
    inline constexpr int128_t operator^(const int128_t& other) const noexcept{
        return int128_t(i128_impl::bit_xor(v_, other.v_));
    }
    inline constexpr int128_t operator~() const noexcept{
        return int128_t(i128_impl::bit_not(v_));
    }    
    inline constexpr int128_t& operator<<=(const unsigned int bit) noexcept{
        *this = *this << bit;
        return *this;
    }
    inline constexpr int128_t& operator>>=(const unsigned int bit) noexcept{
        *this = *this >> bit;
        return *this;
    }
    inline constexpr int128_t& operator&=(const int128_t& other) noexcept{
        *this = *this & other;
        return *this;
    }
    inline constexpr int128_t& operator|=(const int128_t& other) noexcept{
        *this = *this | other;
        return *this;
    }
    inline constexpr int128_t& operator^=(const int128_t& other) noexcept{
        *this = *this ^ other;
        return *this;
    }    
    inline constexpr bool operator==(const int128_t& other) const noexcept{
        return i128_impl::eq(v_, other.v_);
    }
    inline constexpr bool operator!=(const int128_t& other) const noexcept{
        return !i128_impl::eq(v_, other.v_);
    }
    inline constexpr bool operator<(const int128_t& other) const noexcept{
        return i128_impl::lt(v_, other.v_);
    }
    inline constexpr bool operator<=(const int128_t& other) const noexcept{
        return !(*this > other);
    }    
    inline constexpr bool operator>(const int128_t& other) const noexcept{
        return i128_impl::gt(v_, other.v_);
    }
    inline constexpr bool operator>=(const int128_t& other) const noexcept{
        return !(*this < other);
    }
    inline constexpr int128_t& operator=(const int128_t& other){
        v_ = other.v_;
        return *this;
    }
    inline constexpr explicit operator bool() const noexcept{
        return i128_impl::truthy(v_);
    }
    inline constexpr explicit operator uint64_t() const noexcept{
        return i128_impl::low(v_);
    }
    inline constexpr explicit operator uint32_t() const noexcept{
        return static_cast<uint32_t>(i128_impl::low(v_));
    }    
    std::string ToString(unsigned int base = 10) const{        
        if(base < 2 || base > 16){
            throw std::invalid_argument("Base must be between 2 and 16");
        }
        if(*this == min()){            
            static const std::string minStrs[] = {                
                "-10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
                "-101100201022001010121000102002120122110122221010202000122201220121120010200022002",
                "-2000000000000000000000000000000000000000000000000000000000000000",
                "-3013030220323124042102424341431241221233040112312340403",
                "-11324454543055553250455021551551121442554522203132",
                "-1406241064412313155000336513424310163013142502",
                "-2000000000000000000000000000000000000000000",
                "-11321261117012076573587122018656546120262",
                "-170141183460469231731687303715884105728",
                "-555A8020989A11327710815513A946A188727",
                "-2A695925806818735399A37A20A31B3534A8",
                "-2373464C8A3CB25BA2B7C6382B2963BB71B",
                "-27C22D5B9734A1517BB1DC612904A79D72",
                "-3E2480B3404D8BB9BCA3084369BA3E188",
                "-80000000000000000000000000000000"
            };
            return minStrs[base - 2];
        };            
        bool isNegative = i128_impl::lt(v_, i128_impl::make(0));
        i128_impl::rep absValue = isNegative ? i128_impl::sub(i128_impl::make(0), v_) : v_;
        
        std::array<uint32_t, 4> words = {
            static_cast<uint32_t>(std::bit_cast<uint64_t>(i128_impl::high(absValue)) >> 32U),
            static_cast<uint32_t>(std::bit_cast<uint64_t>(i128_impl::high(absValue))),
            static_cast<uint32_t>(i128_impl::low(absValue) >> 32U),
            static_cast<uint32_t>(i128_impl::low(absValue))
        };
        return digits_to_string(words, isNegative, base);
    }

    template<class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(
        std::basic_ostream<CharT, Traits>& os, const int128_t& num) {
        int base = utils::stream::stream_base(os.flags());
        if(base == 0) base = 10;

        uint64_t high = std::bit_cast<uint64_t>(i128_impl::high(num.v_));
        uint64_t low = i128_impl::low(num.v_);
        const bool negative = i128_impl::lt(num.v_, i128_impl::make(0));
        std::string sign;
        if(base == 10) {
            if(negative) {
                sign = "-";
                low = ~low + 1U;
                high = ~high + static_cast<uint64_t>(low == 0);
            }
            else if((os.flags() & std::ios_base::showpos) != 0) {
                sign = "+";
            }
        }
        
        std::array<uint32_t, 4> words{
            static_cast<uint32_t>(high >> 32U), static_cast<uint32_t>(high),
            static_cast<uint32_t>(low >> 32U), static_cast<uint32_t>(low)
        };
         
        const bool zero = std::ranges::all_of(words, [](uint32_t word) { return word == 0; });
        std::string prefix;
        if(!zero && (os.flags() & std::ios_base::showbase) != 0) {
            if(base == 16) {
                prefix = (os.flags() & std::ios_base::uppercase) != 0 ? "0X" : "0x";
            }
            else if(base == 8) {
                prefix = "0";
            }
        }
        return utils::stream::write_integer(
            os, std::move(digits_to_string(words, false, static_cast<unsigned int>(base))),
            std::move(sign), std::move(prefix));
    }

    template<class CharT, class Traits>
    friend std::basic_istream<CharT, Traits>& operator>>(
        std::basic_istream<CharT, Traits>& is, int128_t& num) {
        auto parsed = utils::stream::read_integer<CharT, Traits, 4>(is);
        if(!parsed.sentry_ok) return is;

        constexpr std::array<uint32_t, 4> positive_limit{
            0x7FFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU, 0xFFFF'FFFFU};
        constexpr std::array<uint32_t, 4> negative_limit{
            0x8000'0000U, 0, 0, 0};
        const auto& limit = parsed.negative ? negative_limit : positive_limit;
        const bool out_of_range = parsed.overflow
            || utils::stream::compare_words(parsed.words, limit) == std::strong_ordering::greater;

        int128_t value(0);
        if(out_of_range) {
            value = parsed.negative ? min() : max();
            parsed.state |= std::ios_base::failbit;
        }
        else if(parsed.any_digit) {
            if(parsed.negative && parsed.words == negative_limit) {
                value = min();
            }
            else {
                for(uint32_t word : parsed.words) {
                    value = (value << 32U) + word;
                }
                if(parsed.negative) value = -value;
            }
        }
        num = value;
        if(parsed.state != std::ios_base::goodbit) is.setstate(parsed.state);
        return is;
    }

    static inline constexpr int128_t max() noexcept {return int128_t(i128_impl::max());}
    static inline constexpr int128_t min() noexcept {return int128_t(i128_impl::min());}    
};

#endif
