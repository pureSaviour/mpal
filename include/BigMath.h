#ifndef BIG_MATH_H
#define BIG_MATH_H

#include <bit>
#include "uint128.h"
#include "int128.h"

template <unsigned128 T>
int countl_zero(T value){
    if constexpr (std::is_same_v<T, uint128_t>){
        if(value.high() == 0)
            return 64 + std::countl_zero(value.low());
        return std::countl_zero(value.high());
    }
    return std::countl_zero(value);
}

template <unsigned128 T>
int countr_zero(T value){
    if constexpr (std::is_same_v<T, uint128_t>){
        if(value.low() == 0)
            return 64 + std::countr_zero(value.high());
        return std::countr_zero(value.low());
    }
    return std::countr_zero(value);
}

template <unsigned128 T>
int countl_one(T value){
    if constexpr (std::is_same_v<T, uint128_t>){
        return countl_zero(~value);
    }
    return std::countl_one(value);
}

template <unsigned128 T>
int countr_one(T value){
    if constexpr (std::is_same_v<T, uint128_t>){
        return countr_zero(~value);
    }
    return std::countr_one(value);
}


#endif  