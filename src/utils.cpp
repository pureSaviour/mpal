#include "utils.h"

#include <stdexcept>
#include <string>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>

void ThrowIfStrEmpty(const std::string& str){
    if(str.empty()){
        throw std::invalid_argument("String is empty");
    }
}

static uint32_t checkc(const StringFormat format, char c){
    bool cIsDigit = isdigit(c);
    switch(format){
        case HEX:
            return (cIsDigit || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ?
                (cIsDigit ? (c - '0') : (tolower(c) - 'a' + 10)) : -1;
        case DEC:
            return isdigit(c) ? (c - '0') : -1;
        case OCT:
            return (c >= '0' && c <= '7') ? (c - '0') : -1;
        case BIN:
            return (c == '0' || c == '1') ? (c - '0') : -1;
        default:
            return -1;
    }
}

std::vector<uint32_t> string_to_digits(const std::string& str, bool* isNegative) {
    ThrowIfStrEmpty(str);
    size_t startIndex = 0;
    bool isNeg = false; 
    if(str.size() == 1){
        if(isdigit(str[0])){
            if(isNegative != nullptr)
                *isNegative = false;
            return {static_cast<uint32_t>(str[0] - '0')};            
        }            
        else
            throw std::invalid_argument("String is not a valid integer");
    }        
    
    // 判断符号位    
    if(str[0] == '-'){
        isNeg = true;
        startIndex = 1;
    }
    else if(str[0] == '+'){
        isNeg = false;
        startIndex = 1;
    }else{
        isNeg = false;
    }
    if(isNegative != nullptr)
        *isNegative = isNeg;
    
    // 判断进制
    StringFormat format = DEC;    
    if(str[startIndex] == '0'){
        if(str.size() > startIndex + 1){
            switch(str[startIndex + 1]){
                case 'x':
                case 'X':
                    format = HEX;
                    startIndex += 2;                
                    break;
                case 'b':
                case 'B':
                    format = BIN;
                    startIndex += 2;                    
                    break;
                case 'o':
                case 'O':
                    format = OCT;
                    startIndex += 2;                   
                    break;
                default:
                    if(isdigit(str[startIndex + 1])){                        
                        startIndex += 1;                        
                    }else{
                        throw std::invalid_argument(str + "  has invalid character" + " at index " + std::to_string(startIndex + 1));
                    }
            }            
        }
        else
        {
            *isNegative = false;
            return {0};
        }
    }        
        
    for(; startIndex < str.size() && str[startIndex] == '0'; ++startIndex){
        uint32_t val = checkc(format, str[startIndex]);
        if(val == -1){
            throw std::invalid_argument(str + "  has invalid character" + " at index " + std::to_string(startIndex));
        }
    }
    if(startIndex == str.size()){
        return {0};
    }
    size_t ss = str.size() - startIndex;    // 纯数字串的长度
    size_t blockSize = 9;
    uint64_t base = 1E9;
    switch (format)
    {
    case HEX:    
        blockSize = 7;
        base = 268435456; // 16^7
        break;    
    case OCT:
        blockSize = 10;
        base = 1073741824; // 8^10
        break;
    case BIN:
        blockSize = 32;
        base = 4294967296; // 2^32
        break;
    default:
        break;
    }
    size_t digitsSize = (ss + blockSize - 1) / blockSize;
    size_t firstBlockSize = ss % blockSize;
    std::vector<uint32_t> digits(digitsSize, 0);
    if(firstBlockSize == 0)
        firstBlockSize = blockSize;
    
    std::string substr = str.substr(startIndex, firstBlockSize);
    size_t pos = 0;
    digits[0] = std::stoul(substr, &pos, format);
    if(pos != firstBlockSize){
        throw std::invalid_argument(str + "  has invalid character" + " at index " + std::to_string(startIndex + pos));
    }
    for(size_t i = 1; i < digitsSize; ++i){
        substr = str.substr(startIndex + firstBlockSize + (i - 1) * blockSize, blockSize);
        pos = 0;
        digits[i] = std::stoul(substr, &pos, format);
        if(pos != blockSize){
            throw std::invalid_argument(str + "  has invalid character" + " at index " + std::to_string(startIndex + pos)); 
        }
    }
           
    constexpr uint64_t TARGET_BASE = 0x1'00'00'00'00U;    
    std::vector<uint32_t> result;
    while(true){
        uint32_t carry = 0;
        bool allZero = true;
        for(size_t i = 0; i < digits.size(); ++i){
            uint64_t val = static_cast<uint64_t>(digits[i]) + (static_cast<uint64_t>(carry) * base); ;
            digits[i] = static_cast<uint32_t>(val / TARGET_BASE);
            carry = static_cast<uint32_t>(val % TARGET_BASE);
            if(digits[i] != 0)
                allZero = false;
        }
        result.emplace_back(carry);
        if(allZero)
            break;
    }

    std::ranges::reverse(result);        
    return result;
}

std::string digits_to_string(const std::span<uint32_t> &digits, bool isNegative, const uint32_t base)
{
    if(digits.empty()){
        throw std::invalid_argument("Digits vector is empty");
    }
    if(base > 16 || base < 2){
        throw std::invalid_argument("Base must be between 2 and 16");
    }
    constexpr char alphabet[] = "0123456789abcdef";
    std::string str;    
    
    do{
        uint32_t carry = 0;
        for(uint32_t& digit : digits){
            uint64_t val = static_cast<uint64_t>(digit) | (static_cast<uint64_t>(carry) << 32U);
            digit = static_cast<uint32_t>(val / base);
            carry = static_cast<uint32_t>(val % base);
        }
        str.push_back(alphabet[carry]);
    }while(std::ranges::any_of(digits, [](uint32_t digit){ return digit != 0; }));
    if(isNegative)
        str.push_back('-');
    std::ranges::reverse(str);    
    return str;
}
