#include "BigInt.h"
#include <cmath>
#include <ctype.h>
#include <string_view>
#include <format>
#include <span>
#include <bit>
#include <limits>
#include <stdexcept>

using namespace mpal;
void ThrowIfCharInvalid(size_t index, const std::string& str, const std::string& message) {
    throw std::invalid_argument("Invalid character at index " + std::to_string(index) + " in string \"" + str + "\": " + message);
}

namespace {

using word_type = uint32_t;

void trim(std::vector<word_type>& value) {
    while(!value.empty() && value.back() == 0)
        value.pop_back();
}
std::vector<word_type> slice(
    const std::span<const word_type> value, const size_t offset, const size_t count) {
    if(offset >= value.size() || count == 0)
        return {};
    const size_t length = std::min(count, value.size() - offset);
    std::vector<word_type> result(value.begin() + static_cast<std::ptrdiff_t>(offset),
                                  value.begin() + static_cast<std::ptrdiff_t>(offset + length));
    trim(result);
    return result;
}

std::vector<word_type> shiftLeft(const std::span<const word_type> value, const size_t shift) {
    if(value.empty())
        return {};
    if(shift == 0)
        return std::vector<word_type>(value.begin(), value.end());
    const size_t wordShift = shift / 32;
    const unsigned bitShift = static_cast<unsigned>(shift % 32);
    std::vector<word_type> result(wordShift + value.size(), 0);
    uint64_t carry = 0;
    for(size_t i = 0; i < value.size(); ++i) {
        const uint64_t current = (static_cast<uint64_t>(value[i]) << bitShift) | carry;
        result[wordShift + i] = static_cast<word_type>(current);
        carry = current >> 32;
    }
    if(carry != 0)
        result.push_back(static_cast<word_type>(carry));
    return result;
}

std::vector<word_type> shiftRight(const std::span<const word_type> value, const size_t shift) {
    if(shift == 0)
        return std::vector<word_type>(value.begin(), value.end());
    const size_t wordShift = shift / 32;
    const unsigned bitShift = static_cast<unsigned>(shift % 32);
    if(wordShift >= value.size())
        return {};
    std::vector<word_type> result(value.size() - wordShift, 0);
    for(size_t i = 0; i < result.size(); ++i) {
        const size_t source = i + wordShift;
        uint64_t current = static_cast<uint64_t>(value[source]) >> bitShift;
        if(bitShift != 0 && source + 1 < value.size())
            current |= static_cast<uint64_t>(value[source + 1]) << (32 - bitShift);
        result[i] = static_cast<word_type>(current);
    }
    trim(result);
    return result;
}

bool powerOfTwo(const std::span<const word_type> value, size_t& shift) {
    bool found = false;
    shift = 0;
    for(size_t i = 0; i < value.size(); ++i) {
        if(value[i] == 0)
            continue;
        if(found || !std::has_single_bit(value[i]))
            return false;
        found = true;
        shift = i * 32 + static_cast<size_t>(std::countr_zero(value[i]));
    }
    return found;
}

void decrement(std::vector<word_type>& value) {
    for(word_type& word : value) {
        if(word-- != 0) {
            trim(value);
            return;
        }
    }
    throw std::logic_error("Burnikel-Ziegler quotient correction underflow");
}

std::vector<word_type> concatenate(
    const std::span<const word_type> low,
    const std::span<const word_type> high,
    const size_t highOffset) {
    std::vector<word_type> result(std::max(low.size(), highOffset + high.size()), 0);
    std::copy(low.begin(), low.end(), result.begin());
    std::copy(high.begin(), high.end(), result.begin() + static_cast<std::ptrdiff_t>(highOffset));
    trim(result);
    return result;
}

void subtractInPlace(std::vector<word_type>& value, const std::span<const word_type> subtrahend) {
    word_type borrow = 0;
    size_t i = 0;
    for(; i < subtrahend.size(); ++i) {
        const uint64_t sub = static_cast<uint64_t>(subtrahend[i]) + borrow;
        const word_type current = value[i];
        value[i] = static_cast<word_type>(static_cast<uint64_t>(current) - sub);
        borrow = static_cast<word_type>(static_cast<uint64_t>(current) < sub);
    }
    while(borrow != 0 && i < value.size()) {
        const word_type current = value[i];
        value[i] = current - 1;
        borrow = static_cast<word_type>(current == 0);
        ++i;
    }
    trim(value);
}

void addAt(std::vector<word_type>& destination,
           const std::span<const word_type> source,
           const size_t offset) {
    if(offset + source.size() > destination.size())
        destination.resize(offset + source.size(), 0);
    uint64_t carry = 0;
    size_t i = 0;
    for(; i < source.size(); ++i) {
        const uint64_t sum = static_cast<uint64_t>(destination[offset + i]) + source[i] + carry;
        destination[offset + i] = static_cast<word_type>(sum);
        carry = sum >> 32;
    }
    size_t position = offset + i;
    while(carry != 0) {
        if(position == destination.size())
            destination.push_back(0);
        const uint64_t sum = static_cast<uint64_t>(destination[position]) + carry;
        destination[position] = static_cast<word_type>(sum);
        carry = sum >> 32;
        ++position;
    }
}

} // namespace

mpal::BigInt::BigInt() : BigInt(0) {}
mpal::BigInt::BigInt(int value) 
{
    if(value < 0){
        isNegative_ = true;
    }
    const unsigned int magnitude = value < 0
        ? 0u - static_cast<unsigned int>(value)
        : static_cast<unsigned int>(value);
    if(magnitude != 0)
        digits_.emplace_back(magnitude);
}

const mpal::BigInt mpal::BigInt::ZERO{0};
const mpal::BigInt mpal::BigInt::ONE{1};
mpal::BigInt::BigInt(const std::string &str)
{
    digits_ = std::move(string_to_digits(str, &isNegative_));
    std::ranges::reverse(digits_);
    while(!digits_.empty() && digits_.back() == 0)
        digits_.pop_back();
    if(digits_.empty())
        isNegative_ = false;
}

mpal::BigInt::BigInt(const BigInt &other)
    : BigBase(other.isNegative_), digits_(other.digits_) {}

mpal::BigInt::BigInt(BigInt &&other) noexcept
    : BigBase(other.isNegative_), digits_(std::move(other.digits_))
{
    other.isNegative_ = false;
}

std::string mpal::BigInt::ToString(unsigned int base) const
{
    if(digits_.empty()){
        if(base < 2 || base > 16)
            throw std::invalid_argument("Base must be between 2 and 16");
        return "0";
    }
    std::vector<u32_t> digitsCopy = digits_;
    std::ranges::reverse(digitsCopy);
    return digits_to_string(digitsCopy, isNegative_, base);
}

bool mpal::BigInt::operator==(const BigInt &other) const noexcept
{
    return (isNegative_ == other.isNegative_) && (digits_ == other.digits_);
}

bool mpal::BigInt::operator!=(const BigInt &other) const noexcept
{
    return !(*this == other);
}

bool mpal::BigInt::operator<(const BigInt &other) const noexcept
{
    if(isNegative_ != other.isNegative_)
        return isNegative_;
    const std::partial_ordering comparison = compareAbs(digits_, other.digits_);
    return isNegative_ ? comparison == std::partial_ordering::greater
                       : comparison == std::partial_ordering::less;
}

bool mpal::BigInt::operator>(const BigInt &other) const noexcept
{
    return other < *this;
}

mpal::BigInt &mpal::BigInt::operator=(const BigInt &other)
{
    if(this != &other){
        digits_ = other.digits_;
        isNegative_ = other.isNegative_;
    }
    return *this;
}

mpal::BigInt &mpal::BigInt::operator=(BigInt &&other) noexcept
{
    if(this != &other){
        digits_ = std::move(other.digits_);
        isNegative_ = other.isNegative_;
        other.isNegative_ = false;
    }
    return *this;
}

std::strong_ordering mpal::BigInt::operator<=>(const BigInt &other) const noexcept
{
    if(isNegative_ != other.isNegative_)
        return isNegative_ ? std::strong_ordering::less : std::strong_ordering::greater;
    const std::partial_ordering comparison = compareAbs(digits_, other.digits_);
    if(comparison == std::partial_ordering::equivalent)
        return std::strong_ordering::equal;
    const bool less = comparison == std::partial_ordering::less;
    return less != isNegative_ ? std::strong_ordering::less : std::strong_ordering::greater;
}

mpal::BigInt mpal::BigInt::operator-() const
{
    BigInt res(*this);
    if(!res.digits_.empty())
        res.isNegative_ = !isNegative_;
    return res;
}
mpal::BigInt mpal::BigInt::operator-(const BigInt &other) const
{
    if(this == &other)
        return ZERO;
    if(other.digits_.empty())
        return *this;
    if(digits_.empty())
        return -other;
    if(isNegative_ ^ other.isNegative_){
        return BigInt(mpal::BigInt::addAbs(digits_, other.digits_), isNegative_);
    }else{
        std::partial_ordering cmp = mpal::BigInt::compareAbs(digits_, other.digits_);        
        if(cmp == std::partial_ordering::equivalent){
            return mpal::BigInt::ZERO;
        }
        return (cmp == std::partial_ordering::greater) ?
            BigInt(mpal::BigInt::subAbs(digits_, other.digits_), isNegative_) :
            BigInt(mpal::BigInt::subAbs(other.digits_, digits_), !isNegative_);
    }
}

mpal::BigInt mpal::BigInt::operator+(const BigInt &other) const
{
    if(digits_.empty())
        return other;
    if(other.digits_.empty())
        return *this;
    if(!(isNegative_ ^ other.isNegative_)){
        return BigInt(mpal::BigInt::addAbs(digits_, other.digits_), isNegative_);
    }else{
        std::partial_ordering cmp = mpal::BigInt::compareAbs(digits_, other.digits_);
        if(cmp == std::partial_ordering::equivalent){
            return mpal::BigInt::ZERO;
        }
        return (cmp == std::partial_ordering::greater) ?
            BigInt(mpal::BigInt::subAbs(digits_, other.digits_), isNegative_) :
            BigInt(mpal::BigInt::subAbs(other.digits_, digits_), other.isNegative_);
    }
}

BigInt mpal::BigInt::operator*(const BigInt &other) const
{
    if(digits_.empty() || other.digits_.empty())
        return ZERO;
    const bool negative = isNegative_ != other.isNegative_;
    if(digits_.size() == 1) {
        if(digits_.front() == 1)
            return BigInt(other.digits_, negative);
        if(std::has_single_bit(digits_.front()))
            return BigInt(shiftLeft(other.digits_, static_cast<size_t>(std::countr_zero(digits_.front()))), negative);
    }
    if(other.digits_.size() == 1) {
        if(other.digits_.front() == 1)
            return BigInt(digits_, negative);
        if(std::has_single_bit(other.digits_.front()))
            return BigInt(shiftLeft(digits_, static_cast<size_t>(std::countr_zero(other.digits_.front()))), negative);
    }
    if(std::min(digits_.size(), other.digits_.size()) <= KARATSUBA_THRESHOLD)
        return BigInt(multiplyAbs(digits_, other.digits_), negative);
    return BigInt(KaratsubaImpl(digits_, other.digits_), negative);
}

BigInt mpal::BigInt::operator/(const BigInt &other) const
{
    if(other.digits_.empty())
        throw std::runtime_error("BigInt division by zero");
    if(digits_.empty() || digits_.size() < other.digits_.size())
        return ZERO;
    if(this == &other)
        return ONE;
    if(other.digits_.size() == 1 && other.digits_.front() == 1)
        return BigInt(digits_, isNegative_ != other.isNegative_);
    divmod_result result = divmodAbs(digits_, other.digits_);
    return BigInt(std::move(result.quotient), isNegative_ != other.isNegative_);
}

BigInt mpal::BigInt::operator%(const BigInt &other) const
{
    if(other.digits_.empty())
        throw std::runtime_error("BigInt division by zero");
    if(digits_.empty())
        return ZERO;
    if(digits_.size() < other.digits_.size())
        return *this;
    if(this == &other || (other.digits_.size() == 1 && other.digits_.front() == 1))
        return ZERO;
    divmod_result result = divmodAbs(digits_, other.digits_);
    return BigInt(std::move(result.remainder), isNegative_);
}

BigInt &mpal::BigInt::operator/=(const BigInt &other)
{
    *this = *this / other;
    return *this;
}

BigInt &mpal::BigInt::operator%=(const BigInt &other)
{
    *this = *this % other;
    return *this;
}

std::pair<BigInt, BigInt> mpal::BigInt::divmod(const BigInt &other) const
{
    if(other.digits_.empty())
        throw std::runtime_error("BigInt division by zero");
    if(digits_.empty())
        return {ZERO, ZERO};
    if(digits_.size() < other.digits_.size())
        return {ZERO, *this};
    if(this == &other)
        return {ONE, ZERO};
    if(other.digits_.size() == 1 && other.digits_.front() == 1)
        return {BigInt(digits_, isNegative_ != other.isNegative_), ZERO};
    divmod_result result = divmodAbs(digits_, other.digits_);
    BigInt quotient(std::move(result.quotient), isNegative_ != other.isNegative_);
    BigInt remainder(std::move(result.remainder), isNegative_);
    return {std::move(quotient), std::move(remainder)};
}

BigInt mpal::BigInt::multiply(const BigInt &a, const BigInt &b)
{
    return BigInt(KaratsubaImpl(a.digits_, b.digits_), a.isNegative_ ^ b.isNegative_);
}

mpal::BigInt::BigInt(std::vector<u32_t> digits, bool isNegative)
    : BigBase(isNegative), digits_(std::move(digits))
{
    while(!digits_.empty() && digits_.back() == 0)
        digits_.pop_back();
    if(digits_.empty())
        isNegative_ = false;
}

std::vector<mpal::BigInt::u32_t> mpal::BigInt::addAbs(const std::span<const mpal::BigInt::u32_t>& a, const std::span<const mpal::BigInt::u32_t>& b)
{
    if(a.empty())
        return std::vector<u32_t>(b.begin(), b.end());
    if(b.empty())
        return std::vector<u32_t>(a.begin(), a.end());
    std::vector<u32_t> resDigits;
    size_t minSize = std::min(a.size(), b.size());
    size_t maxSize = std::max(a.size(), b.size());
    const std::span<const u32_t> longer = a.size() >= b.size() ? a : b;
    resDigits.reserve(maxSize + 1);
    u32_t carry = 0;
    for(size_t i = 0; i < minSize; ++i){
        u64_t sum = static_cast<u64_t>(a[i]) + static_cast<u64_t>(b[i]) + static_cast<u64_t>(carry);
        resDigits.emplace_back(static_cast<u32_t>(sum & 0xFFFFFFFF));
        carry = static_cast<u32_t>(sum >> 32);        
    }
    for(size_t i = minSize; i < maxSize; ++i){
        const u64_t sum = static_cast<u64_t>(longer[i]) + carry;
        resDigits.emplace_back(static_cast<u32_t>(sum & 0xFFFFFFFF));
        carry = static_cast<u32_t>(sum >> 32);
    }
    if(carry > 0){
        resDigits.emplace_back(carry);
    }
    return resDigits;
}

std::vector<mpal::BigInt::u32_t> mpal::BigInt::subAbs(const std::span<const mpal::BigInt::u32_t>& bigger, const std::span<const mpal::BigInt::u32_t> &smaller)
{
    std::vector<u32_t> result;
    result.reserve(bigger.size());
    u64_t borrow = 0;
    size_t i = 0;
    for(; i < smaller.size(); ++i) {
        const u64_t subtrahend = static_cast<u64_t>(smaller[i]) + borrow;
        result.emplace_back(static_cast<u32_t>(static_cast<u64_t>(bigger[i]) - subtrahend));
        borrow = static_cast<u64_t>(bigger[i]) < subtrahend;
    }
    for(; i < bigger.size(); ++i) {
        const u32_t current = bigger[i];
        result.emplace_back(current - static_cast<u32_t>(borrow));
        borrow = borrow != 0 && current == 0;
    }
    trim(result);
    return result;
}

std::vector<BigInt::u32_t> mpal::BigInt::multiplyAbs(const std::span<const u32_t> &a, const std::span<const u32_t> &b)
{
    const std::span<const u32_t> outer = a.size() <= b.size() ? a : b;
    const std::span<const u32_t> inner = a.size() <= b.size() ? b : a;
    if(outer.empty())
        return {};
    std::vector<u32_t> res(outer.size() + inner.size(), 0);
    for(size_t i = 0; i < outer.size(); ++i){
        u64_t carry = 0;
        for(size_t j = 0; j < inner.size(); ++j){
            u64_t product = static_cast<u64_t>(outer[i]) * static_cast<u64_t>(inner[j]) + static_cast<u64_t>(res[i + j]) + carry;
            res[i + j] = static_cast<u32_t>(product & 0xFFFFFFFF);
            carry = product >> 32;
        }
        res[i + inner.size()] = static_cast<u32_t>(carry);
    }
    while(!res.empty() && res.back() == 0)
        res.pop_back();
    return res;
}

mpal::BigInt::divmod_result mpal::BigInt::divmodAbs(
    const std::span<const u32_t> dividend, const std::span<const u32_t> divisor)
{
    if(divisor.empty())
        throw std::runtime_error("BigInt division by zero");
    const auto comparison = compareAbs(dividend, divisor);
    if(comparison == std::partial_ordering::less)
        return {{}, std::vector<u32_t>(dividend.begin(), dividend.end())};
    if(comparison == std::partial_ordering::equivalent)
        return {{1}, {}};

    size_t powerShift = 0;
    if(powerOfTwo(divisor, powerShift)) {
        std::vector<u32_t> quotient = shiftRight(dividend, powerShift);
        const size_t fullWords = powerShift / 32;
        const unsigned remainingBits = static_cast<unsigned>(powerShift % 32);
        std::vector<u32_t> remainder;
        const size_t copiedWords = std::min(fullWords, dividend.size());
        remainder.assign(dividend.begin(), dividend.begin() + static_cast<std::ptrdiff_t>(copiedWords));
        if(remainingBits != 0 && fullWords < dividend.size()) {
            const u32_t mask = (u32_t{1} << remainingBits) - 1;
            remainder.push_back(dividend[fullWords] & mask);
        }
        trim(remainder);
        return {std::move(quotient), std::move(remainder)};
    }
    if(divisor.size() == 1)
        return divmodByWord(dividend, divisor.front());
    const size_t quotientWords = dividend.size() - divisor.size() + 1;
    if(divisor.size() < BURNIKEL_ZIEGLER_THRESHOLD
       || quotientWords < divisor.size() / 2)
        return divmodKnuth(dividend, divisor);
    return divmodBurnikelZiegler(dividend, divisor);
}

mpal::BigInt::divmod_result mpal::BigInt::divmodByWord(
    const std::span<const u32_t> dividend, const u32_t divisor)
{
    std::vector<u32_t> quotient(dividend.size(), 0);
    u64_t remainder = 0;
    for(size_t i = dividend.size(); i-- > 0;) {
        const u64_t numerator = (remainder << 32) | dividend[i];
        quotient[i] = static_cast<u32_t>(numerator / divisor);
        remainder = numerator % divisor;
    }
    trim(quotient);
    return {std::move(quotient), remainder == 0 ? std::vector<u32_t>{}
                                                : std::vector<u32_t>{static_cast<u32_t>(remainder)}};
}

mpal::BigInt::divmod_result mpal::BigInt::divmodKnuth(
    const std::span<const u32_t> dividend, const std::span<const u32_t> divisor)
{
    if(divisor.size() == 1)
        return divmodByWord(dividend, divisor.front());
    if(compareAbs(dividend, divisor) == std::partial_ordering::less)
        return {{}, std::vector<u32_t>(dividend.begin(), dividend.end())};

    const unsigned normalization = static_cast<unsigned>(std::countl_zero(divisor.back()));
    std::vector<u32_t> normalizedDivisor = shiftLeft(divisor, normalization);
    std::vector<u32_t> normalizedDividend = shiftLeft(dividend, normalization);
    normalizedDividend.resize(dividend.size() + 1, 0);

    const size_t n = normalizedDivisor.size();
    const size_t m = dividend.size() - divisor.size();
    std::vector<u32_t> quotient(m + 1, 0);
    constexpr u64_t base = u64_t{1} << 32;

    for(size_t position = m + 1; position-- > 0;) {
        const u64_t numerator = (static_cast<u64_t>(normalizedDividend[position + n]) << 32)
            | normalizedDividend[position + n - 1];
        u64_t estimate = numerator / normalizedDivisor[n - 1];
        u64_t estimateRemainder = numerator % normalizedDivisor[n - 1];
        while(estimate == base
              || estimate * normalizedDivisor[n - 2]
                    > base * estimateRemainder + normalizedDividend[position + n - 2]) {
            --estimate;
            estimateRemainder += normalizedDivisor[n - 1];
            if(estimateRemainder >= base)
                break;
        }

        u64_t borrow = 0;
        for(size_t i = 0; i < n; ++i) {
            const u64_t product = estimate * normalizedDivisor[i] + borrow;
            const u32_t low = static_cast<u32_t>(product);
            borrow = product >> 32;
            if(normalizedDividend[position + i] < low)
                ++borrow;
            normalizedDividend[position + i] -= low;
        }
        const bool negative = normalizedDividend[position + n] < borrow;
        normalizedDividend[position + n] -= static_cast<u32_t>(borrow);
        if(negative) {
            --estimate;
            u64_t carry = 0;
            for(size_t i = 0; i < n; ++i) {
                const u64_t sum = static_cast<u64_t>(normalizedDividend[position + i])
                    + normalizedDivisor[i] + carry;
                normalizedDividend[position + i] = static_cast<u32_t>(sum);
                carry = sum >> 32;
            }
            normalizedDividend[position + n] += static_cast<u32_t>(carry);
        }
        quotient[position] = static_cast<u32_t>(estimate);
    }

    trim(quotient);
    std::vector<u32_t> remainder(normalizedDividend.begin(),
        normalizedDividend.begin() + static_cast<std::ptrdiff_t>(n));
    remainder = shiftRight(remainder, normalization);
    return {std::move(quotient), std::move(remainder)};
}

mpal::BigInt::divmod_result mpal::BigInt::divmodBurnikelZiegler(
    const std::span<const u32_t> dividend, const std::span<const u32_t> divisor)
{
    const size_t blockSize = std::bit_ceil(divisor.size());
    const size_t wordPadding = blockSize - divisor.size();
    const unsigned bitPadding = static_cast<unsigned>(std::countl_zero(divisor.back()));
    const size_t totalShift = wordPadding * 32 + bitPadding;
    std::vector<u32_t> normalizedDivisor = shiftLeft(divisor, totalShift);
    std::vector<u32_t> normalizedDividend = shiftLeft(dividend, totalShift);
    const size_t blockCount = (normalizedDividend.size() + blockSize - 1) / blockSize;
    std::vector<u32_t> quotient(blockCount * blockSize, 0);
    std::vector<u32_t> remainder;

    for(size_t block = blockCount; block-- > 0;) {
        std::vector<u32_t> lowBlock = slice(normalizedDividend, block * blockSize, blockSize);
        std::vector<u32_t> numerator = concatenate(lowBlock, remainder, blockSize);
        divmod_result partial = divide2n1n(numerator, normalizedDivisor);
        std::copy(partial.quotient.begin(), partial.quotient.end(),
                  quotient.begin() + static_cast<std::ptrdiff_t>(block * blockSize));
        remainder = std::move(partial.remainder);
    }
    trim(quotient);
    remainder = shiftRight(remainder, totalShift);
    return {std::move(quotient), std::move(remainder)};
}

mpal::BigInt::divmod_result mpal::BigInt::divide2n1n(
    const std::span<const u32_t> dividend, const std::span<const u32_t> divisor)
{
    const size_t n = divisor.size();
    if(n <= BURNIKEL_ZIEGLER_BASECASE || (n & 1U) != 0)
        return divmodKnuth(dividend, divisor);
    const size_t half = n / 2;
    std::vector<u32_t> upper = slice(dividend, half, 3 * half);
    divmod_result high = divide3n2n(upper, divisor, half);
    std::vector<u32_t> low = slice(dividend, 0, half);
    std::vector<u32_t> lower = concatenate(low, high.remainder, half);
    divmod_result lowResult = divide3n2n(lower, divisor, half);
    std::vector<u32_t> quotient = concatenate(lowResult.quotient, high.quotient, half);
    return {std::move(quotient), std::move(lowResult.remainder)};
}

mpal::BigInt::divmod_result mpal::BigInt::divide3n2n(
    const std::span<const u32_t> dividend,
    const std::span<const u32_t> divisor,
    const size_t half)
{
    std::vector<u32_t> top = slice(dividend, half, 2 * half);
    std::vector<u32_t> low = slice(dividend, 0, half);
    std::vector<u32_t> divisorHigh = slice(divisor, half, half);
    std::vector<u32_t> divisorLow = slice(divisor, 0, half);
    std::vector<u32_t> dividendHigh = slice(dividend, 2 * half, half);

    std::vector<u32_t> quotient;
    std::vector<u32_t> remainderHigh;
    if(compareAbs(dividendHigh, divisorHigh) == std::partial_ordering::less) {
        divmod_result estimate = divide2n1n(top, divisorHigh);
        quotient = std::move(estimate.quotient);
        remainderHigh = std::move(estimate.remainder);
    }
    else {
        quotient.assign(half, std::numeric_limits<u32_t>::max());
        std::vector<u32_t> product = KaratsubaImpl(quotient, divisorHigh);
        remainderHigh = subAbs(top, product);
    }

    std::vector<u32_t> remainder = concatenate(low, remainderHigh, half);
    std::vector<u32_t> product = KaratsubaImpl(quotient, divisorLow);
    while(compareAbs(remainder, product) == std::partial_ordering::less) {
        remainder = addAbs(remainder, divisor);
        decrement(quotient);
    }
    remainder = subAbs(remainder, product);
    return {std::move(quotient), std::move(remainder)};
}

std::partial_ordering mpal::BigInt::compareAbs(const std::span<const u32_t> &a, const std::span<const u32_t> &b) noexcept
{
    if(a.size() != b.size()){
        return a.size() < b.size() ? std::partial_ordering::less : std::partial_ordering::greater;
    }
    for(size_t i = a.size(); i-- > 0;) {
        if(a[i] != b[i])
            return a[i] < b[i] ? std::partial_ordering::less : std::partial_ordering::greater;
    }
    return std::partial_ordering::equivalent;
}

std::vector<mpal::BigInt::u32_t> mpal::BigInt::KaratsubaImpl(const std::span<const u32_t> &num1, const std::span<const u32_t> &num2)
{
    if(num1.empty() || num2.empty()){
        return {};
    }
    if(num1.size() == 1 && num2.size() == 1){
        u64_t product = static_cast<u64_t>(num1[0]) * static_cast<u64_t>(num2[0]);
        if(product >> 32 == 0)
            return {static_cast<u32_t>(product & 0xFF'FF'FF'FF)};
        return {static_cast<u32_t>(product & 0xFF'FF'FF'FF), static_cast<u32_t>(product >> 32)};
    }
    const size_t minSize = std::min(num1.size(), num2.size());
    const size_t maxSize = std::max(num1.size(), num2.size());
    if(minSize <= KARATSUBA_THRESHOLD){
        return multiplyAbs(num1, num2);
    }
    if(maxSize - minSize > minSize) {
        const std::span<const u32_t> shorter = num1.size() <= num2.size() ? num1 : num2;
        const std::span<const u32_t> longer = num1.size() <= num2.size() ? num2 : num1;
        std::vector<u32_t> result;
        result.reserve(num1.size() + num2.size());
        for(size_t offset = 0; offset < longer.size(); offset += shorter.size()) {
            const std::span<const u32_t> block = longer.subspan(
                offset, std::min(shorter.size(), longer.size() - offset));
            std::vector<u32_t> partial = block.size() <= KARATSUBA_THRESHOLD
                ? multiplyAbs(shorter, block)
                : KaratsubaImpl(shorter, block);
            addAt(result, partial, offset);
        }
        trim(result);
        return result;
    }

    // 拆分：num = high * B^half + low，其中 B = 2^32（high 可能为空）
    const size_t half = std::max(num1.size(), num2.size()) / 2;
    std::span<const u32_t> a = half < num1.size() ? num1.subspan(half) : std::span<const u32_t>{};
    std::span<const u32_t> b = num1.first(std::min(num1.size(), half));
    std::span<const u32_t> c = half < num2.size() ? num2.subspan(half) : std::span<const u32_t>{};
    std::span<const u32_t> d = num2.first(std::min(num2.size(), half));

    std::vector<u32_t> q = KaratsubaImpl(a, c);   // 高×高
    std::vector<u32_t> p = KaratsubaImpl(b, d);   // 低×低

    // 中间项 mid = (a+b)(c+d) - p - q
    std::vector<u32_t> mid = KaratsubaImpl(addAbs(a, b), addAbs(c, d));
    subtractInPlace(mid, p);
    subtractInPlace(mid, q);

    // 重组：res = p + (mid << half) + (q << 2*half)，一次分配、就地累加进位
    std::vector<u32_t> res;
    res.reserve(num1.size() + num2.size());
    addAt(res, p, 0);
    addAt(res, mid, half);
    addAt(res, q, half * 2);

    while(!res.empty() && res.back() == 0)
        res.pop_back();
    return res;
}
