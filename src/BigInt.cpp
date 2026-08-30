#include "BigInt.h"
#include <cmath>
#include <ctype.h>
#include <string_view>
#include <format>
#include <span>

using namespace mpal;
void ThrowIfCharInvalid(size_t index, const std::string& str, const std::string& message) {
    throw std::invalid_argument("Invalid character at index " + std::to_string(index) + " in string \"" + str + "\": " + message);
}

mpal::BigInt::BigInt() : BigInt(0) {}
mpal::BigInt::BigInt(int value) 
{
    if(value < 0){
        isNegative_ = true;
    }
    const unsigned int magnitude = value < 0
        ? 0u - static_cast<unsigned int>(value)
        : static_cast<unsigned int>(value);
    digits_.emplace_back(magnitude);
}

const mpal::BigInt mpal::BigInt::ZERO{0};
const mpal::BigInt mpal::BigInt::ONE{1};
mpal::BigInt::BigInt(const std::string &str)
{
    digits_ = std::move(string_to_digits(str, &isNegative_));
}

mpal::BigInt::BigInt(const BigInt &other)
{
    digits_ = other.digits_;
    isNegative_ = other.isNegative_;
}

mpal::BigInt::BigInt(BigInt &&other) noexcept
{
    digits_ = std::move(other.digits_);
    isNegative_ = other.isNegative_;
    other.isNegative_ = false;
}

std::string mpal::BigInt::ToString(StringFormat format) const
{
    std::vector<u32_t> digitsCopy = digits_;
    return digits_to_string(digitsCopy, isNegative_, format);
}

bool mpal::BigInt::operator==(const BigInt &other) const
{
    return (isNegative_ == other.isNegative_) && (digits_ == other.digits_);
}

bool mpal::BigInt::operator<(const BigInt &other) const
{
    if(isNegative_ && !other.isNegative_){
        return true;
    }if(!isNegative_ && other.isNegative_){
        return false;
    }
    bool absBigger = false;
    bool isEqual = true;
    if(digits_.size() != other.digits_.size()){
        absBigger = digits_.size() > other.digits_.size();
        isEqual = false;
    }
    else{
        for(std::ptrdiff_t i = static_cast<std::ptrdiff_t>(digits_.size()) - 1; i >= 0; --i){
            if(digits_[i] != other.digits_[i]){
                absBigger = digits_[i] > other.digits_[i];
                isEqual = false;
                break;
            }
        }
    }
    if(isEqual)
        return false;    
    return isNegative_ ? absBigger : !absBigger;

}

bool mpal::BigInt::operator>(const BigInt &other) const
{
    return !(*this < other) && !(*this == other);
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

std::partial_ordering mpal::BigInt::operator<=>(const BigInt &other) const
{
    if(*this == other)
        return std::partial_ordering::equivalent;
    if(*this < other)
        return std::partial_ordering::less;
    return std::partial_ordering::greater;
}

mpal::BigInt mpal::BigInt::operator-() const
{
    BigInt res(*this);
    res.isNegative_ = !isNegative_;
    return res;
}

mpal::BigInt mpal::BigInt::operator-(const BigInt &other) const
{
    if(isNegative_ ^ other.isNegative_){
        return BigInt(std::move(mpal::BigInt::addAbs(digits_, other.digits_)), isNegative_);
    }else{
        std::partial_ordering cmp = mpal::BigInt::compareAbs(digits_, other.digits_);        
        if(cmp == std::partial_ordering::equivalent){
            return mpal::BigInt::ZERO;
        }
        return (cmp == std::partial_ordering::greater) ?
            BigInt(std::move(mpal::BigInt::subAbs(digits_, other.digits_)), isNegative_) :
            BigInt(std::move(mpal::BigInt::subAbs(other.digits_, digits_)), !isNegative_);        
    }
}

mpal::BigInt mpal::BigInt::operator+(const BigInt &other) const
{   
    if(!(isNegative_ ^ other.isNegative_)){
        return BigInt(std::move(mpal::BigInt::addAbs(digits_, other.digits_)), isNegative_);
    }else{
        std::partial_ordering cmp = mpal::BigInt::compareAbs(digits_, other.digits_);
        if(cmp == std::partial_ordering::equivalent){
            return mpal::BigInt::ZERO;
        }
        return (cmp == std::partial_ordering::greater) ?
            BigInt(std::move(mpal::BigInt::subAbs(digits_, other.digits_)), isNegative_) :
            BigInt(std::move(mpal::BigInt::subAbs(other.digits_, digits_)), other.isNegative_);        
    }
}

// 朴素乘法O(n * m)
BigInt mpal::BigInt::operator*(const BigInt &other) const
{
    return BigInt(std::move(mpal::BigInt::multiplyAbs(digits_, other.digits_)), isNegative_ ^ other.isNegative_);    
}

BigInt mpal::BigInt::multiply(const BigInt &a, const BigInt &b)
{
    auto data = KaratsubaImpl(a.digits_, b.digits_);
    return BigInt(std::move(data), a.isNegative_ ^ b.isNegative_);
}

mpal::BigInt::BigInt(std::vector<u32_t> digits, bool isNegative)
    : digits_(std::move(digits)), BigBase(isNegative) {}

std::vector<mpal::BigInt::u32_t> mpal::BigInt::addAbs(const std::span<const mpal::BigInt::u32_t>& a, const std::span<const mpal::BigInt::u32_t>& b)
{
    std::vector<u32_t> resDigits;
    size_t minSize = std::min(a.size(), b.size());
    size_t maxSize = std::max(a.size(), b.size());
    resDigits.reserve(maxSize + 1);
    u32_t carry = 0;
    for(size_t i = 0; i < minSize; ++i){
        u64_t sum = static_cast<u64_t>(a[i]) + static_cast<u64_t>(b[i]) + static_cast<u64_t>(carry);
        resDigits.emplace_back(static_cast<u32_t>(sum & 0xFFFFFFFF));
        carry = static_cast<u32_t>(sum >> 32);        
    }
    for(size_t i = minSize; i < maxSize; ++i){
        u64_t sum = static_cast<u64_t>(carry);
        if(a.size() > b.size())
            sum += static_cast<u64_t>(a[i]);
        else
            sum += static_cast<u64_t>(b[i]);            
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
    std::vector<u32_t> resDigits;    
    size_t minSize = smaller.size();
    size_t maxSize = bigger.size();    
    resDigits.reserve(maxSize);
    bool hasCarry = false;
    for(size_t i = 0; i < minSize; ++i){            
        u64_t minuend, subtrahend;
        bool needToCarry = false;
        minuend = static_cast<u64_t>(bigger[i]);
        subtrahend = static_cast<u64_t>(smaller[i]);
        if(hasCarry){
            if(minuend == 0){
                minuend = 0xFF'FF'FF'FF;
                needToCarry = true;
            }else{
                minuend -= 1;
            }
        }
        if(minuend < subtrahend){
            minuend += 0x1'00'00'00'00ULL;
            hasCarry = true;
        }else{
            hasCarry = false;
        }
        hasCarry |= needToCarry;            
        u32_t diff = static_cast<u32_t>(minuend - subtrahend);
        resDigits.emplace_back(diff);
    }
    for(size_t i = minSize; i < maxSize; ++i){
        u64_t minuend = static_cast<u64_t>(bigger[i]);
        if(hasCarry){
            if(minuend == 0){
                minuend = 0xFF'FF'FF'FF;
                hasCarry = true;
            }else{
                minuend -= 1;
                hasCarry = false;
            }
        }else{
            hasCarry = false;
        }
        resDigits.emplace_back(static_cast<u32_t>(minuend));
    }
    
    while(!resDigits.empty() && resDigits.back() == 0)
        resDigits.pop_back();
    
    return resDigits;
}

std::vector<BigInt::u32_t> mpal::BigInt::multiplyAbs(const std::span<const u32_t> &a, const std::span<const u32_t> &b)
{
    std::vector<u32_t> res(a.size() + b.size(), 0);
    for(size_t i = 0; i < a.size(); ++i){
        u64_t carry = 0;
        for(size_t j = 0; j < b.size(); ++j){
            u64_t product = static_cast<u64_t>(a[i]) * static_cast<u64_t>(b[j]) + static_cast<u64_t>(res[i + j]) + carry;
            res[i + j] = static_cast<u32_t>(product & 0xFFFFFFFF);
            carry = product >> 32;
        }
        for(size_t k = i + b.size(); carry; ++k){
            u64_t sum = static_cast<u64_t>(res[k]) + carry;
            res[k] = static_cast<u32_t>(sum & 0xFFFFFFFF);
            carry = sum >> 32;
        }
    }
    while(!res.empty() && res.back() == 0)
        res.pop_back();
    return res;
}

std::partial_ordering mpal::BigInt::compareAbs(const std::span<const u32_t> &a, const std::span<const u32_t> &b)
{
    if(a.size() != b.size()){
        return a.size() < b.size() ? std::partial_ordering::less : std::partial_ordering::greater;
    }
    return std::lexicographical_compare_three_way(a.rbegin(), a.rend(), b.rbegin(), b.rend());
}

// std::tuple<const BigInt *, const BigInt *, bool> mpal::BigInt::GetAbsBiggerAndSmaller(const BigInt &a, const BigInt &b)
// {
//     const BigInt* absBigger = &a;
//     const BigInt* absSmaller = &b;
//     bool isEqual = true;
//     if(a.digits_.size() != b.digits_.size()){
//         absBigger = (a.digits_.size() > b.digits_.size()) ? &a : &b;
//         absSmaller = (a.digits_.size() > b.digits_.size()) ? &b : &a;
//         isEqual = false;
//     }else{
//         for(ssize_t i = a.digits_.size() - 1; i >= 0; --i){
//             if(a.digits_[i] != b.digits_[i]){
//                 absBigger = (a.digits_[i] > b.digits_[i]) ? &a : &b;
//                 absSmaller = (a.digits_[i] > b.digits_[i]) ? &b : &a;
//                 isEqual = false;
//                 break;
//             }
//         }
//     }
//     return std::make_tuple(absBigger, absSmaller, isEqual);
// }

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
    if(std::min(num1.size(), num2.size()) <= 32){
        return multiplyAbs(num1, num2);
    }

    // 拆分：num = high * B^half + low，其中 B = 2^32（high 可能为空）
    const size_t half = std::max(num1.size(), num2.size()) / 2;
    std::span<const u32_t> a = half < num1.size() ? num1.subspan(half) : std::span<const u32_t>{};
    std::span<const u32_t> b = num1.first(std::min(num1.size(), half));
    std::span<const u32_t> c = half < num2.size() ? num2.subspan(half) : std::span<const u32_t>{};
    std::span<const u32_t> d = num2.first(std::min(num2.size(), half));

    std::vector<u32_t> q = KaratsubaImpl(a, c);   // 高×高
    std::vector<u32_t> p = KaratsubaImpl(b, d);   // 低×低

    // 中间项 mid = (a+b)(c+d) - p - q，就地相减，省掉 subAbs 的临时对象
    std::vector<u32_t> mid = KaratsubaImpl(addAbs(a, b), addAbs(c, d));
    auto subFrom = [](std::vector<u32_t>& x, const std::span<const u32_t>& y){
        u32_t borrow = 0;
        size_t i = 0;
        for(; i < y.size(); ++i){
            const u64_t sub = static_cast<u64_t>(y[i]) + borrow;
            if(x[i] < sub){
                x[i] = static_cast<u32_t>(x[i] + 0x1'00'00'00'00ULL - sub);
                borrow = 1;
            }else{
                x[i] = static_cast<u32_t>(x[i] - sub);
                borrow = 0;
            }
        }
        for(; borrow && i < x.size(); ++i){
            if(x[i] == 0){
                x[i] = 0xFF'FF'FF'FF;
            }else{
                --x[i];
                borrow = 0;
            }
        }
        while(!x.empty() && x.back() == 0)
            x.pop_back();
    };
    subFrom(mid, p);
    subFrom(mid, q);

    // 重组：res = p + (mid << half) + (q << 2*half)，一次分配、就地累加进位
    std::vector<u32_t> res;
    res.reserve(num1.size() + num2.size());
    auto addInto = [&res](const std::span<const u32_t>& src, size_t offset){
        if(offset + src.size() > res.size())
            res.resize(offset + src.size(), 0);
        u64_t carry = 0;
        size_t i = 0;
        for(; i < src.size(); ++i){
            const u64_t sum = static_cast<u64_t>(res[offset + i]) + src[i] + carry;
            res[offset + i] = static_cast<u32_t>(sum & 0xFFFF'FFFF);
            carry = sum >> 32;
        }
        for(size_t j = offset + i; carry; ++j){
            if(j >= res.size())
                res.push_back(0);
            const u64_t sum = static_cast<u64_t>(res[j]) + carry;
            res[j] = static_cast<u32_t>(sum & 0xFFFF'FFFF);
            carry = sum >> 32;
        }
    };
    addInto(p, 0);
    addInto(mid, half);
    addInto(q, half * 2);

    while(!res.empty() && res.back() == 0)
        res.pop_back();
    return res;
}