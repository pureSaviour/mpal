#ifndef ARITHMETIC_MANAGER_H
#define ARITHMETIC_MANAGER_H

#include <thread>
#include <future>
#include "BigInt.h"

namespace mpal{

class ArithmeticManager{
public:
    
    ArithmeticManager() = default;
    ArithmeticManager(size_t numThreads) : threads_(numThreads) {}
    ArithmeticManager(ArithmeticManager&& other) noexcept : threads_(std::move(other.threads_)) {}
    ArithmeticManager& operator=(ArithmeticManager&& other) noexcept {
        if (this != &other) {
            threads_ = std::move(other.threads_);
        }
        return *this;
    }
    ArithmeticManager(const ArithmeticManager&) = delete;
    ArithmeticManager& operator=(const ArithmeticManager&) = delete;
    ~ArithmeticManager() = default;
    static mpal::BigInt add(const mpal::BigInt& a, const mpal::BigInt& b);
    static mpal::BigInt sub(const mpal::BigInt& a, const mpal::BigInt& b);
    static mpal::BigInt multiply(const mpal::BigInt& a, const mpal::BigInt& b);
    
    mpal::BigInt mulParallel(const mpal::BigInt& a, const mpal::BigInt& b);    
private:
    using u32_t = mpal::BigInt::u32_t;
    using u64_t = mpal::BigInt::u64_t;
    std::vector<std::thread> threads_;
    std::vector<mpal::BigInt::u32_t> mulAbsParallel(const std::span<const mpal::BigInt::u32_t>& a, const std::span<const mpal::BigInt::u32_t>& b, size_t numThreads);
};

}
#endif // ARITHMETIC_MANAGER_H