#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <span>
#include <iomanip>

enum StringFormat{
    HEX = 16,
    DEC = 10,
    OCT = 8,
    BIN = 2
};

/// @brief 将字符串转换为数字数组
/// @details 支持十进制、十六进制、八进制和二进制的字符串转换，返回一个存储数字的数组。可以通过 isNegative 参数获取字符串是否为负数。
/// @param str 要转换的字符串
/// @param isNegative 指向一个布尔值的指针，用于返回字符串是否为负数
/// @throws std::invalid_argument 如果字符串为空、大小大于字符串长度或包含无效字符
/// @return 存储uint32_t的数组
std::vector<uint32_t> string_to_digits(const std::string& str, bool* isNegative);

/// @brief 将数字数组转换为字符串(数组会被修改)
/// @details 支持2进制到16进制数字数组转换，返回一个字符串。可以通过 isNegative 参数指定字符串是否为负数。
/// @param digits 要转换的数字数组
/// @param isNegative 指定字符串是否为负数
/// @param format 指定转换的进制
/// @return 转换后的字符串
std::string digits_to_string(const std::span<uint32_t>& digits, bool isNegative, const uint32_t base = 10);

#endif // UTILS_H

