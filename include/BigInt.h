#ifndef BIT_INT_H
#define BIT_INT_H

#include <vector>
#include <stdint.h>
#include <tuple>
#include <string>
#include <string_view>
#include <compare>
#include <utility>
#include <algorithm>
#include <span>

#include "utils.h"
#include "utils_stream.h"
#include "BigBase.h"
namespace mpal{

class ArithmeticManager;

class BigInt : public BigBase{
friend class ArithmeticManager;
public:
    BigInt();
    BigInt(int value);
    BigInt(const ::std::string& str);
    BigInt(const BigInt& other);
    BigInt(BigInt&& other) noexcept;
    ~BigInt()=default;

    [[nodiscard]] ::std::string ToString(unsigned int base = 10) const;
    [[nodiscard]] bool operator ==(const BigInt& other) const noexcept;
    [[nodiscard]] bool operator !=(const BigInt& other) const noexcept;
    // [[nodiscard]] bool operator <=(const BigInt& other) const noexcept;
    // [[nodiscard]] bool operator >=(const BigInt& other) const noexcept;
    [[nodiscard]] bool operator <(const BigInt& other) const noexcept;
    [[nodiscard]] bool operator >(const BigInt& other) const noexcept;
    BigInt& operator=(const BigInt& other);
    BigInt& operator=(BigInt&& other) noexcept;        
    [[nodiscard]] ::std::strong_ordering operator<=>(const BigInt& other) const noexcept;
    [[nodiscard]] BigInt operator-() const;
    [[nodiscard]] BigInt operator-(const BigInt& other) const;
    [[nodiscard]] BigInt operator+(const BigInt& other) const;
    [[nodiscard]] BigInt operator*(const BigInt& other) const;
    [[nodiscard]] BigInt operator/(const BigInt& other) const;
    [[nodiscard]] BigInt operator%(const BigInt& other) const;
    BigInt& operator/=(const BigInt& other);
    BigInt& operator%=(const BigInt& other);
    /// @return 商和余数；商向零截断，余数与被除数同号。
    /// @throws std::runtime_error 除数为零时抛出。
    [[nodiscard]] ::std::pair<BigInt, BigInt> divmod(const BigInt& other) const;
    [[nodiscard]] bool IsNegative() const noexcept { return isNegative_; }
    [[nodiscard]] bool IsPositive() const noexcept { return !isNegative_; }
    void Inverse() noexcept {
        if(!digits_.empty()) isNegative_ = !isNegative_;
    }
    void SetSign(bool isPositive) noexcept {
        isNegative_ = !isPositive && !digits_.empty();
    }
    
    template<class CharT, class Traits>
    friend ::std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const BigInt& bigInt);

    template<class CharT, class Traits>
    friend ::std::basic_istream<CharT, Traits>& operator>>(::std::basic_istream<CharT, Traits>& is, BigInt& bigInt);
    static BigInt multiply(const BigInt& a, const BigInt& b);
public:
    static const BigInt ZERO;
    static const BigInt ONE;
private:
    using u32_t = uint32_t;    
    using u64_t = uint64_t;
    struct divmod_result{
        std::vector<u32_t> quotient;
        std::vector<u32_t> remainder;
    };
    static constexpr size_t BURNIKEL_ZIEGLER_THRESHOLD = 1024;
    static constexpr size_t BURNIKEL_ZIEGLER_BASECASE = 32;
    static constexpr size_t KARATSUBA_THRESHOLD = 64;
    static constexpr u64_t _2E32 = 0x1'00'00'00'00ULL;
    static constexpr u32_t _1E9 = 1'000'000'000U;    
    BigInt(std::vector<u32_t> digits, bool isNegative);
    static std::vector<u32_t> addAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b) ;
    static std::vector<u32_t> subAbs(const std::span<const u32_t>& bigger, const std::span<const u32_t>& smaller);
    static std::vector<u32_t> multiplyAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b);
    static divmod_result divmodAbs(std::span<const u32_t> dividend, std::span<const u32_t> divisor);
    static divmod_result divmodByWord(std::span<const u32_t> dividend, u32_t divisor);
    static divmod_result divmodKnuth(std::span<const u32_t> dividend, std::span<const u32_t> divisor);
    static divmod_result divmodBurnikelZiegler(std::span<const u32_t> dividend, std::span<const u32_t> divisor);
    static divmod_result divide2n1n(std::span<const u32_t> dividend, std::span<const u32_t> divisor);
    static divmod_result divide3n2n(std::span<const u32_t> dividend, std::span<const u32_t> divisor, size_t half);
    static std::partial_ordering compareAbs(const std::span<const u32_t>& a, const std::span<const u32_t>& b) noexcept;
    static std::vector<u32_t> KaratsubaImpl(const std::span<const u32_t>& num1, const std::span<const u32_t>& num2);
    std::vector<u32_t> digits_;
};

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, const BigInt &bigInt)
{
    int base = utils::stream::stream_base(os.flags());
    if(base == 0)
        base = 10;
    const bool zero = bigInt.digits_.empty();
    std::vector<uint32_t> digitsCopy = bigInt.digits_;
    std::ranges::reverse(digitsCopy);
    std::string digits = zero
        ? "0"
        : digits_to_string(digitsCopy, false, static_cast<unsigned int>(base));
    std::string sign;
    if(bigInt.isNegative_)
        sign = "-";
    else if((os.flags() & std::ios_base::showpos) != 0)
        sign = "+";
    std::string prefix;
    if(!zero && (os.flags() & std::ios_base::showbase) != 0) {
        if(base == 16)
            prefix = (os.flags() & std::ios_base::uppercase) != 0 ? "0X" : "0x";
        else if(base == 8)
            prefix = "0";
    }
    return utils::stream::write_integer(
        os,
        std::move(digits),
        std::move(sign),
        std::move(prefix));
}

template <class CharT, class Traits>
::std::basic_istream<CharT, Traits> &operator>>(::std::basic_istream<CharT, Traits> &is, BigInt &bigInt)
{
    utils::stream::parsed_integer parsed = utils::stream::read_integer<CharT, Traits>(is);
    if(!parsed.sentry_ok)return is;

    while(!parsed.words.empty() && parsed.words.back() == 0)
        parsed.words.pop_back();
    bigInt.digits_ = std::move(parsed.words);
    bigInt.isNegative_ = parsed.negative && !bigInt.digits_.empty();
    if(parsed.state != std::ios_base::goodbit) is.setstate(parsed.state);
    return is;
}
}
#endif
