#ifndef BIG_UTILS_H
#define BIG_UTILS_H

#include <ranges>
#include <functional>
#include <iostream>
#include <string>
#include <algorithm>
#include "BigInt.h"
#include "BigExcept.h"

namespace mpal{
namespace details{
    constexpr bool CheckIfValidStr(const std::string&& str){
        if(str.empty()){
            return false;
        }
        bool isValid = true;
        auto start = 0;
    
        switch(str[start]){
            case '-':
            case '+':
                start += 1;
                break;
            default:
                if(!isdigit(str[start])){
                    isValid = false;
                }
                break;
        }
        bool (*checkFunc)(const char&) = nullptr;
        if(str.length() > 2 && str[start] == '0'){
            if(str[start + 1] == 'x' || str[start + 1] == 'X'){
                checkFunc = [](const char& c){
                    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
                };
            }
            else if(str[start + 1] == 'b' || str[start + 1] == 'B'){
                checkFunc = [](const char& c){
                    return c == '0' || c == '1';
                };
            }
           start += 2;
        }else {
            checkFunc = [](const char& c){
                return static_cast<bool>(isdigit(c));
            };
            start += 1;
        }

        for(size_t i = start; i < str.length(); ++i){
            if(!checkFunc(str[i])){
                isValid = false;
                break;
            }
        }
    
        return isValid;
    }
}
    mpal::BigInt operator""_bi(const char* str, size_t len){        
        if (!details::CheckIfValidStr(std::string(str, len))){
            ThrowIfCharInvalid(0, std::string(str, len), "Invalid character");
        }
        return mpal::BigInt(std::string(str, len));
    }    
}
#endif  