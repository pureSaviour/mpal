#ifndef UINT128_H
#define UINT128_H

#include <bit>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <format>
#include <assert.h>
#include <intrin.h>

#include "integral_128.h"
#include "utils.h"
#include "utils_stream.h"

namespace u128_impl{
    #if MPAL_HAS_NATIVE_I128
        using rep = integral128_detail::unsigned_rep;
        template <integral_128 T>
        inline constexpr rep make(const T& value) { 
            if constexpr(std::is_same_v<T, uint128_t>)
                return value.v_;
            else if constexpr(std::is_same_v<T, int128_t>)
                return std::bit_cast<rep>(value.v_);
            else
                return static_cast<rep>(value);
        }
        inline constexpr rep add(const rep& a, const rep& b) { return a + b; }
        inline constexpr rep sub(const rep& a, const rep& b) { return a - b; }
        inline constexpr void add_assign(rep& a, const rep& b) { a += b; }
        inline constexpr void sub_assign(rep& a, const rep& b) { a -= b; }
        inline constexpr rep mul(const rep& a, const rep& b) { return a * b; }
        using divmod_result = integral128_detail::unsigned_divmod_result;
        inline constexpr divmod_result divmod(const rep& a, const rep& b) {
            return integral128_detail::unsigned_divmod(a, b);
        }
        inline constexpr rep div(const rep& a, const rep& b) { return divmod(a, b).quotient; }
        inline constexpr rep mod(const rep& a, const rep& b) { return divmod(a, b).remainder; }
        inline constexpr rep shl(const rep& a, unsigned int n) { return n >= 128 ? 0 : a << n; }
        inline constexpr rep shr(const rep& a, unsigned int n) { return n >= 128 ? 0 : a >> n; }
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
        inline constexpr rep max(){return static_cast<rep>(-1);}    
    #else    
        using u64_t = uint64_t;
        using rep = integral128_detail::unsigned_rep;
        template <integral_128 T>
        inline constexpr rep make(const T& value) {            
            if constexpr (std::is_same_v<T, uint128_t>){
                return value.v_;
            }
            else if constexpr (std::is_same_v<T, int128_t>){                
                return std::bit_cast<rep>(value.v_);
            }            
            else
                return { static_cast<u64_t>(value < static_cast<T>(0) ? -1 : 0), static_cast<u64_t>(value)};
            
        }
        inline constexpr rep shl(const rep& a, unsigned int n) {
            if(n >= 128) return {0, 0};
            if(n >= 64) return {a.low << (n - 64), 0};
            if(n == 0) return a;   
            return {(a.high << n) | (a.low >> (64 - n)), a.low << n};
        }
        inline constexpr rep shr(const rep& a, unsigned int n) {
            if(n >= 128) return {0, 0};
            if(n >= 64) return {0, a.high >> (n - 64)};
            if(n == 0) return a;            
            return { a.high >> n, (a.low >> n) | (a.high << (64 - n))};
        }
        inline constexpr rep bit_and(const rep& a, const rep& b) { return { a.high & b.high, a.low & b.low}; }
        inline constexpr rep bit_or(const rep& a, const rep& b) { return { a.high | b.high, a.low | b.low}; }
        inline constexpr rep bit_xor(const rep& a, const rep& b) { return { a.high ^ b.high, a.low ^ b.low}; }
        inline constexpr rep bit_not(const rep& a) { return { ~a.high, ~a.low}; }
        inline constexpr bool eq(const rep& a, const rep& b) { return a.low == b.low && a.high == b.high; }
        inline constexpr bool lt(const rep& a, const rep& b) { return (a.high < b.high) || (a.high == b.high && a.low < b.low); }
        inline constexpr bool gt(const rep& a, const rep& b) { return (a.high > b.high) || (a.high == b.high && a.low > b.low); }
        inline constexpr bool truthy(const rep& a) { return a.low != 0 || a.high != 0; }
        inline constexpr uint64_t low(const rep& a){return a.low;} 
        inline constexpr uint64_t high(const rep& a){return a.high;} 
        inline constexpr rep add(const rep& a, const rep& b) { 
            u64_t lo = a.low + b.low;        
            return {a.high + b.high + (lo < a.low ? 1 : 0), lo};
        }
        inline constexpr rep sub(const rep& a, const rep& b) { 
            u64_t lo = a.low - b.low;        
            return {a.high - b.high - (lo > a.low ? 1 : 0), lo};
        }
        inline constexpr void add_assign(rep& a, const rep& b) {
            const u64_t oldLow = a.low;
            a.low += b.low;
            a.high += b.high + static_cast<u64_t>(a.low < oldLow);
        }
        inline constexpr void sub_assign(rep& a, const rep& b) {
            const u64_t oldLow = a.low;
            a.low -= b.low;
            a.high -= b.high + static_cast<u64_t>(a.low > oldLow);
        }
        inline constexpr rep mul(const rep& a, const rep& b) {
            u64_t low, high, res[2];
            low = _umul128(a.low, b.low, &high);
            res[1] = low;
            res[0] = high;
            res[0] += _umul128(a.low, b.high, &high) + _umul128(a.high, b.low, &high);
            return {res[0], res[1]};
        }
        using divmod_result = integral128_detail::unsigned_divmod_result;
        inline constexpr divmod_result divmod(const rep& a, const rep& b) {
            return integral128_detail::unsigned_divmod(a, b);
        }
        inline constexpr rep div(const rep& a, const rep& b) { return divmod(a, b).quotient; }
        inline constexpr rep mod(const rep& a, const rep& b) { return divmod(a, b).remainder; }
        inline constexpr rep max(){return {0xFFFF'FFFF'FFFF'FFFFULL, 0xFFFF'FFFF'FFFF'FFFFULL};}
    #endif
}

struct uint128_t {    
    u128_impl::rep v_;
    
    uint128_t() noexcept : v_{u128_impl::make((unsigned int)0)} {
        
    }
    explicit uint128_t(u128_impl::rep v) : v_(v) {
        #if MPAL_HAS_NATIVE_U128
        #warning "This kind of constructor is only supported when the compiler supports native 128-bit integers."
        #endif
    }
    uint128_t(const std::string& str){
        // Implement string to uint128_t conversion
        v_ = u128_impl::make(static_cast<uint64_t>(0));
        
        if(str.empty()){
            throw std::invalid_argument("Empty string is not a valid uint128_t");
        }
        bool isNegative = false;
        std::vector<uint32_t> digits = string_to_digits(str, &isNegative);
        if(digits.size() <= 4){
            for(size_t i = 0; i < digits.size(); ++i){
                v_ = u128_impl::add(u128_impl::shl(v_, 32), u128_impl::make(digits[i]));
            }
        }
        else
            throw std::invalid_argument("String represents a number larger than uint128_t can hold");
        if(isNegative){
            v_ = u128_impl::sub(u128_impl::make(0), v_);            
        }
    }
    
    template <integral_128 T>
    explicit uint128_t(const T& value) : v_(u128_impl::make(value)) {}

    explicit inline constexpr operator u128_impl::rep() const noexcept { return v_; }    
    
    inline constexpr uint128_t operator^(const uint128_t& other) const noexcept{    
        return uint128_t(u128_impl::bit_xor(v_, other.v_));
    }
    inline constexpr uint128_t operator&(const uint128_t& other) const noexcept{
        return uint128_t(u128_impl::bit_and(v_, other.v_));
    }
    inline constexpr uint128_t operator<<(const unsigned int bit) const noexcept{
        return uint128_t(u128_impl::shl(v_, bit));
    }
    inline constexpr uint128_t operator>>(const unsigned int bit) const noexcept{
        return uint128_t(u128_impl::shr(v_, bit));
    }
    inline constexpr uint128_t operator|(const uint128_t& other) const noexcept{
        return uint128_t(u128_impl::bit_or(v_, other.v_));
    }
    inline constexpr uint128_t operator~() const noexcept{
        return uint128_t(u128_impl::bit_not(v_));
    }
    explicit inline constexpr operator bool() const noexcept{
        return u128_impl::truthy(v_);
    }
    explicit inline constexpr operator uint64_t() const noexcept{
        return u128_impl::low(v_);
    }
    explicit inline constexpr operator uint32_t() const noexcept{
        return static_cast<uint32_t>(u128_impl::low(v_));
    }    
    inline constexpr bool operator==(const uint128_t& other) const noexcept{
        return u128_impl::eq(v_, other.v_);
    }
    inline constexpr bool operator!=(const uint128_t& other) const noexcept{
        return !u128_impl::eq(v_, other.v_);
    }
    inline constexpr bool operator<(const uint128_t& other) const noexcept{
        return u128_impl::lt(v_, other.v_);
    }
    inline constexpr bool operator>(const uint128_t& other) const noexcept{
        return u128_impl::gt(v_, other.v_);
    }
    inline constexpr bool operator<=(const uint128_t& other) const noexcept{
        return !(*this > other);
    }
    inline constexpr bool operator>=(const uint128_t& other) const noexcept{
        return !(*this < other);
    }
    inline constexpr uint128_t& operator=(const uint128_t& other){
        v_ = other.v_;
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t operator+(const T& other) const noexcept{                  
        return uint128_t(u128_impl::add(v_, u128_impl::make(other)));
    }
    template <integral_128 T>
    inline constexpr uint128_t operator-(const T& other) const noexcept{
        return uint128_t(u128_impl::sub(v_, u128_impl::make(other)));
    }
    
    inline constexpr uint128_t operator-() const noexcept{
        return uint128_t(u128_impl::sub(u128_impl::make(0), v_));
    }
    inline constexpr uint128_t& operator++() noexcept {
        *this = *this + static_cast<unsigned int>(1);
        return *this;
    }
    inline constexpr uint128_t& operator--() noexcept {
        *this = *this - static_cast<unsigned int>(1);
        return *this;
    }
    inline constexpr uint128_t operator++(int) noexcept {
        uint128_t temp = *this;
        *this = *this + static_cast<unsigned int>(1);
        return temp;
    }
    inline constexpr uint128_t operator--(int) noexcept {
        uint128_t temp = *this;
        *this = *this - static_cast<unsigned int>(1);
        return temp;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t operator*(const T& other) const noexcept{
        return uint128_t(u128_impl::mul(v_, u128_impl::make(other)));
    }

    template <integral_128 T>
    inline constexpr uint128_t operator/(const T& other) const{        
        return uint128_t(u128_impl::div(v_, u128_impl::make(other)));
    }    
    
    template <integral_128 T>
    inline constexpr uint128_t operator%(const T& other) const{        
        return uint128_t(u128_impl::mod(v_, u128_impl::make(other)));
    }
    
    template <integral_128 T>
    inline constexpr uint128_t& operator+=(const T& other) noexcept{
        const u128_impl::rep rhs = u128_impl::make(other);
        u128_impl::add_assign(v_, rhs);
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t& operator-=(const T& other) noexcept{
        const u128_impl::rep rhs = u128_impl::make(other);
        u128_impl::sub_assign(v_, rhs);
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t& operator*=(const T& other) noexcept{
        *this = *this * other;
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t& operator/=(const T& other){
        *this = *this / other;
        return *this;
    }
    
    template <integral_128 T>
    inline constexpr uint128_t& operator%=(const T& other){
        *this = *this % other;
        return *this;
    }

    inline constexpr uint128_t& operator&=(const uint128_t& other) noexcept{
        *this = *this & other;
        return *this;
    }    
    inline constexpr uint128_t& operator|=(const uint128_t& other) noexcept{
        *this = *this | other;
        return *this;
    }
    inline constexpr uint128_t& operator^=(const uint128_t other) noexcept{
        *this = *this ^ other;
        return *this;
    }
    inline constexpr uint128_t& operator<<=(const unsigned int bit) noexcept{
        *this = *this << bit;
        return *this;
    }
    inline constexpr uint128_t& operator>>=(const unsigned int bit) noexcept{
        *this = *this >> bit;
        return *this;
    }        

    std::string ToString(unsigned int base = 10) const{    
        if(base < 2 || base > 16){
            throw std::invalid_argument("Base must be between 2 and 16");
        }
        if(!(*this)){
            return "0";
        }
        uint32_t digits[4] = {
            static_cast<uint32_t>(u128_impl::high(v_) >> 32),
            static_cast<uint32_t>(u128_impl::high(v_) & 0xFFFF'FFFF),
            static_cast<uint32_t>(u128_impl::low(v_) >> 32),
            static_cast<uint32_t>(u128_impl::low(v_) & 0xFFFF'FFFF)
        };
        return digits_to_string(digits, false, base);
    }
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(
        std::basic_ostream<CharT, Traits>& os, const uint128_t& num) {
        int base = utils::stream::stream_base(os.flags());
        if(base == 0) base = 10;
        std::array<uint32_t, 4> words{
            static_cast<uint32_t>(u128_impl::high(num.v_) >> 32U),
            static_cast<uint32_t>(u128_impl::high(num.v_)),
            static_cast<uint32_t>(u128_impl::low(num.v_) >> 32U),
            static_cast<uint32_t>(u128_impl::low(num.v_))
        };
        std::string prefix;
        if(num && (os.flags() & std::ios_base::showbase) != 0) {
            if(base == 16) {
                prefix = (os.flags() & std::ios_base::uppercase) != 0 ? "0X" : "0x";
            }
            else if(base == 8) {
                prefix = "0";
            }
        }
        return utils::stream::write_integer(
            os, digits_to_string(words, false, static_cast<unsigned int>(base)),
            {}, std::move(prefix));
    }

    template<class CharT, class Traits>
    friend std::basic_istream<CharT, Traits>& operator>>(
        std::basic_istream<CharT, Traits>& is, uint128_t& num) {
        auto parsed = utils::stream::read_integer<CharT, Traits, 4>(is);
        if(!parsed.sentry_ok) return is;

        uint128_t value(0U);
        if(parsed.overflow) {
            value = max();
        }
        else if(parsed.any_digit) {
            for(uint32_t word : parsed.words) {
                value = (value << 32U) + word;
            }
            if(parsed.negative) value = -value;
        }
        num = value;
        if(parsed.state != std::ios_base::goodbit) is.setstate(parsed.state);
        return is;
    }
    static inline constexpr uint128_t max(){
        return uint128_t(u128_impl::max());
    }
};

#endif // UINT128_H
