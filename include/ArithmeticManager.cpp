#include "ArithmeticManager.h"
using namespace mpal;
mpal::BigInt ArithmeticManager::add(const mpal::BigInt &a, const mpal::BigInt &b)
{
    return a + b;
}

mpal::BigInt ArithmeticManager::sub(const mpal::BigInt &a, const mpal::BigInt &b)
{
    return a - b;
}

mpal::BigInt ArithmeticManager::multiply(const mpal::BigInt &a, const mpal::BigInt &b)
{
    return a * b;
}


// comba
mpal::BigInt ArithmeticManager::mulParallel(const mpal::BigInt &a, const mpal::BigInt &b)
{
    size_t numThreads = threads_.size();
    if(numThreads == 0){
        return multiply(a, b);
    }
    std::vector<std::vector<mpal::BigInt::u32_t>> partialResults(numThreads);
    size_t chunkSize = (a.digits_.size() + numThreads - 1) / numThreads;
    for(size_t i = 0; i < numThreads; ++i){

    }
}

std::vector<mpal::BigInt::u32_t> mpal::ArithmeticManager::mulAbsParallel(const std::span<const mpal::BigInt::u32_t> &a, const std::span<const mpal::BigInt::u32_t> &b, size_t numThreads)
{    
    size_t ts = threads_.size();
    if(ts == 0){
        return mpal::BigInt::multiplyAbs(a, b);
    }
    size_t chunkSize = (a.size() + ts - 1) / ts;
    std::vector<std::vector<u64_t>> partialResults(ts, std::vector<u64_t>(chunkSize * b.size(), 0));    
    for(size_t i = 0; i < ts; ++i){        
        threads_[i] = std::thread([&, i]{            
            size_t start = i * chunkSize;
            size_t end = std::min(start + chunkSize, a.size());
            for(size_t j = start; j < end; ++j){
                for(size_t k = 0; k < b.size(); ++k){
                    u64_t product = static_cast<u64_t>(a[j]) * static_cast<u64_t>(b[k]);
                    partialResults[i][j - start + k] = product;
                }
            }   
        });
    }
}
