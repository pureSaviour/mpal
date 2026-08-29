#ifndef BIT_INT_H
#define BIT_INT_H

#include <vector>
#include <iostream>
#include <stdint.h>
#include <tuple>
#include <string>
#include <string_view>
#include <compare>
#include <utility>
#include <algorithm>
#include <span>

#include "BigBase.h"

namespace mpal{

class ArithmeticManager;

class BigInt : public BigBase{
friend class ArithmeticManager;
public:
    BigInt();
    BigInt(int value);
    BigInt(const std::string& str);
    BigInt(const BigInt& other);
    BigInt(BigInt&& other) noexcept;
    ~BigInt()=default;    
    
    // friend std::ostream& operator<<(std::ostream& os, const BigInt& bigInt);
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] bool operator ==(const BigInt& other) const;
    [[nodiscard]] bool operator <(const BigInt& other) const;
    [[nodiscard]] bool operator >(const BigInt& other) const;
    BigInt& operator=(const BigInt& other);
    BigInt& operator=(BigInt&& other) noexcept;
    [[nodiscard]] std::partial_ordering operator<=>(const BigInt& other) const;
    [[nodiscard]] BigInt operator-() const;
    [[nodiscard]] BigInt operator-(const BigInt& other) const;
    [[nodiscard]] BigInt operator+(const BigInt& other) const;
    [[nodiscard]] BigInt operator*(const BigInt& other) const;
    
    static BigInt multiply(const BigInt& a, const BigInt& b);
public:
    static const BigInt ZERO;
    static const BigInt ONE;
private:
    
    using u32_t = uint32_t;    
    using u64_t = uint64_t;
    static constexpr u64_t _2E32 = 0x1'00'00'00'00ULL;
    static constexpr u32_t _1E9 = 1'000'000'000U;    
    BigInt(std::vector<u32_t> digits, bool isNegative);
    static std::vector<u32_t> addAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b) ;
    static std::vector<u32_t> subAbs(const std::span<const u32_t>& bigger, const std::span<const u32_t>& smaller);
    static std::vector<u32_t> multiplyAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b);    
    static std::partial_ordering compareAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b);  
    static std::vector<u32_t> KaratsubaImpl(const std::span<const u32_t>& num1, const std::span<const u32_t>& num2);

    bool isNegative_ = false;
    std::vector<u32_t> digits_;
};

}
#endif
