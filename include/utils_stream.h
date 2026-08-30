#ifndef UTILS_STREAM_H
#define UTILS_STREAM_H

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <locale>
#include <compare>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace utils{    
    namespace stream{

        /// @brief 存储解析后的整数值及状态信息，使用可变大小的字数组表示任意精度整数。        
        ///
        /// @details
        /// 该结构体用于 `read_integer` 等解析函数，保存解析结果及相关标志。
        /// 数值以二进制补码形式存储在 `words` 数组中，高位在前（小端序）。
        struct parsed_integer{
            std::vector<uint32_t> words{};  // 倒序存储的数字数组
            std::ios_base::iostate state = std::ios_base::goodbit;
            bool sentry_ok = false;
            bool negative = false;            
            bool any_digit = false;
        };

        /// @brief 存储解析后的整数值及状态信息，使用固定大小的字数组表示任意精度整数。
        /// @tparam N 数组 `words` 的大小，即用于存储整数的 `uint32_t` 元素的个数。
        ///         每个元素为一个“字”（word），用于构建任意长度的整数（大整数）。
        ///
        /// @details
        /// 该结构体用于 `read_integer` 等解析函数，保存解析结果及相关标志。
        /// 数值以二进制补码形式存储在 `words` 数组中，低位在前（小端序）。
        template<size_t N>
        struct parsed_integer_fixed{
            std::array<uint32_t, N> words{};
            std::ios_base::iostate state = std::ios_base::goodbit;
            bool sentry_ok = false;
            bool negative = false;
            bool overflow = false;
            bool any_digit = false;
        };

        inline int stream_base(std::ios_base::fmtflags flags) noexcept {
            switch(flags & std::ios_base::basefield) {
                case std::ios_base::oct: return 8;
                case std::ios_base::hex: return 16;
                case std::ios_base::dec: return 10;
                default: return 0;
            }
        }

        namespace detail{
            template < typename T, size_t N>
            concept parsed_interger_type = std::same_as<T, struct parsed_integer_fixed<N>> || std::same_as<T, struct parsed_integer>;

          
            inline void append_digit(std::vector<uint32_t>& words, unsigned int base, unsigned int digit) {
                if(words.empty()){
                    words.emplace_back(digit);
                    return;
                }
                uint32_t carry = digit;
                for(uint32_t& word : words) {
                    const uint64_t value = static_cast<uint64_t>(word) * base + carry;
                    word = static_cast<uint32_t>(value);
                    carry = static_cast<uint32_t>(value >> 32U);
                }                
                if(carry != 0) {
                    words.emplace_back(carry);
                }
            }

            template <size_t N>
            inline void append_digit(parsed_integer_fixed<N>& parsed, unsigned int base, unsigned int digit) noexcept {
                uint32_t carry = digit;
                for(size_t i = parsed.words.size(); i-- > 0;){
                    const uint64_t value = static_cast<uint64_t>(parsed.words[i]) * base + carry;
                    parsed.words[i] = static_cast<uint32_t>(value);
                    carry = static_cast<uint32_t>(value >> 32U);
                }
                parsed.overflow = parsed.overflow || carry != 0;
            }
            /// @brief 验证分组大小向量是否匹配给定的二进制模式字符串。
            /// @param groups 存储各组大小的向量（无符号整数），至少包含首元素和后续元素。
            /// @param grouping 二进制模式字符串，每个字节被解释为 unsigned char 数值（范围 0–255），
            ///                 其中 0 和 UCHAR_MAX（255）视为非法保留值。
            /// @details 验证规则如下：
            ///          1. 若 groups.size() <= 1 或 grouping.empty()，则直接返回 true（无约束）。
            ///          2. 从 groups 的最后一个元素开始，逆序遍历到索引 1，
            ///             每个元素必须等于 grouping 中对应位置的字节值（按顺序从索引 0 开始）。
            ///             若 grouping 较短，则后续所有元素必须等于最后一个字节的值；
            ///             若 grouping 较长，则只使用前 groups.size()-1 个字节，多余忽略。
            ///             如果遇到 expected == 0 或 expected == UCHAR_MAX，则直接返回 false。
            ///          3. 检查 groups 的首元素（索引 0）：必须非零且不大于最后匹配的字节值
            ///             （即循环结束后 pattern 指向的字节，若循环未执行则取 grouping[0]）。
            /// @example
            ///          // 完全匹配
            ///          std::vector<size_t> g1 = {3, 3, 3};
            ///          std::string p1 = "\x03\x03";
            ///          valid_grouping(g1, p1); // true
            ///
            ///          // 首元素可小于等于最后匹配值
            ///          std::vector<size_t> g2 = {2, 3, 3};
            ///          valid_grouping(g2, p1); // true
            ///
            ///          // 首元素过大
            ///          std::vector<size_t> g3 = {4, 3, 3};
            ///          valid_grouping(g3, p1); // false
            ///
            ///          // grouping 较短时，后部元素均需等于末字节
            ///          std::vector<size_t> g4 = {3, 3, 3, 3};
            ///          std::string p2 = "\x03";
            ///          valid_grouping(g4, p2); // true
            ///
            ///          // grouping 较长时，多余字节被忽略
            ///          std::vector<size_t> g5 = {3, 3};
            ///          std::string p3 = "\x03\x04\x05";
            ///          valid_grouping(g5, p3); // true（只检查 groups[1] 与 0x03）
            ///
            ///          // 非法字节 0 或 255
            ///          std::vector<size_t> g6 = {3, 3};
            ///          std::string p4 = "\x00\x03";
            ///          valid_grouping(g6, p4); // false
            /// @return 若所有检查通过则返回 true，否则返回 false。
            inline bool valid_grouping(const std::span<const std::size_t>& groups, const std::string& grouping){
                if(groups.size() <= 1 || grouping.empty()) return true;

                std::size_t pattern = 0;
                for(std::size_t group = groups.size(); group-- > 1;){
                    const unsigned char expected = static_cast<unsigned char>(grouping[pattern]);
                    if(expected == 0 || expected == UCHAR_MAX || groups[group] != expected) return false;
                    if(pattern + 1 < grouping.size()) ++pattern;
                }

                const unsigned char expected = static_cast<unsigned char>(grouping[pattern]);
                return expected != 0 && expected != UCHAR_MAX
                    && groups.front() != 0 && groups.front() <= expected;
            }  

            template<class CharT, class Traits, size_t N, parsed_interger_type<N> T>
            T read_integer(std::basic_istream<CharT, Traits>& stream){
                T parsed;                
                typename std::basic_istream<CharT, Traits>::sentry sentry(stream);
                if(!sentry) return parsed;
                parsed.sentry_ok = true;
                
                // auto* buffer = stream.rdbuf();
                std::basic_streambuf<CharT, Traits>* buffer = stream.rdbuf();
                const auto eof = Traits::eof();
                auto current = buffer->sgetc();
                auto is_char = [&](char expected) {
                    return !Traits::eq_int_type(current, eof)
                        && Traits::eq(Traits::to_char_type(current), stream.widen(expected));
                };
                auto advance = [&] {
                    current = buffer->snextc();
                };
            
                if(is_char('+') || is_char('-')) {
                    parsed.negative = is_char('-');
                    advance();
                }
            
                int base = stream_base(stream.flags());
                bool leading_zero_is_digit = false;
                if(base == 0) {
                    base = 10;
                    if(is_char('0')){
                        advance();
                        if(is_char('x') || is_char('X')){
                            base = 16;
                            advance();
                        }
                        else{
                            base = 8;
                            leading_zero_is_digit = true;
                        }
                    }
                }else if(base == 16 && is_char('0')) {
                    advance();
                    if(is_char('x') || is_char('X')) {
                        advance();
                    }
                    else {
                        leading_zero_is_digit = true;
                    }
                }
            
                std::vector<size_t> groups(1, leading_zero_is_digit ? 1 : 0);            
                if(leading_zero_is_digit) {
                    parsed.any_digit = true;
                    if constexpr (std::same_as<T, parsed_integer>) {
                        append_digit(parsed.words, static_cast<unsigned int>(base), 0);
                    } else {
                        append_digit(parsed, static_cast<unsigned int>(base), 0);
                    }                    
                }
                const auto& punctuation = std::use_facet<std::numpunct<CharT>>(stream.getloc());
                const std::string grouping = punctuation.grouping();
                const CharT separator = punctuation.thousands_sep();
            
                while(!Traits::eq_int_type(current, eof)) {
                    const CharT character = Traits::to_char_type(current);
                    int digit = -1;
                    for(char candidate = '0'; candidate <= '9'; ++candidate) {
                        if(Traits::eq(character, stream.widen(candidate))) {
                            digit = candidate - '0';
                            break;
                        }
                    }
                    if(digit < 0 && base == 16) {
                        for(char candidate = 'a'; candidate <= 'f'; ++candidate) {
                            if(Traits::eq(character, stream.widen(candidate))
                                || Traits::eq(character, stream.widen(static_cast<char>(candidate - 'a' + 'A')))) {
                                digit = candidate - 'a' + 10;
                                break;
                            }
                        }
                    }
                
                    if(digit >= 0 && digit < base) {
                        parsed.any_digit = true;
                        ++groups.back();
                        if constexpr (std::same_as<T, parsed_integer>) {
                            append_digit(parsed.words, static_cast<unsigned int>(base), static_cast<unsigned int>(digit));
                        } else {
                            append_digit(parsed, static_cast<unsigned int>(base), static_cast<unsigned int>(digit));
                        }                        
                        advance();
                        continue;
                    }
                    if(!grouping.empty() && parsed.any_digit && Traits::eq(character, separator)) {
                        groups.push_back(0);
                        advance();
                        continue;
                    }
                    break;
                }
            
                if(Traits::eq_int_type(current, eof)) parsed.state |= std::ios_base::eofbit;
                if(!parsed.any_digit || !valid_grouping(groups, grouping)) {
                    parsed.state |= std::ios_base::failbit;
                }
                if constexpr (std::same_as<T, parsed_integer_fixed<N>>) {
                    if(parsed.overflow) parsed.state |= std::ios_base::failbit;
                }                
                return parsed;
            }
        }
        

        inline std::strong_ordering compare_words(
            const std::span<const uint32_t>& lhs,
            const std::span<const uint32_t>& rhs) noexcept {            
            if(lhs.size() != rhs.size())
                return lhs.size() <=> rhs.size();            
            for(size_t i = 0; i < lhs.size(); ++i)
                if(lhs[i] != rhs[i])
                    return lhs[i] <=> rhs[i];            
            return std::strong_ordering::equal;
        }
      

        template<class CharT, class Traits>
        parsed_integer read_integer(std::basic_istream<CharT, Traits>& stream){
            return detail::read_integer<CharT, Traits, 0, struct parsed_integer>(stream);
        }
    
        template<class CharT, class Traits, size_t N>
        parsed_integer_fixed<N> read_integer(std::basic_istream<CharT, Traits>& stream) {            
            return detail::read_integer<CharT, Traits, N, struct parsed_integer_fixed<N>>(stream);
        }

        /// @brief 将窄字符字符串（std::string_view）转换为宽字符字符串，使用给定流的环境（locale）进行字符拓宽。
        /// @tparam CharT 目标字符类型（如 char、wchar_t、char8_t 等）。
        /// @tparam Traits 字符特性类型，用于流操作的字符处理（如 std::char_traits<CharT>）。
        /// @param stream 用于提供 widening 上下文（locale）的输出流，其 widen 方法将 char 转换为 CharT。
        /// @param text 要转换的窄字符字符串视图（std::string_view），仅包含基本源字符集字符。
        /// @return 转换后的宽字符串（std::basic_string<CharT>），长度与 text 相同，每个字符均通过 stream.widen() 转换。
        template<class CharT, class Traits>
        std::basic_string<CharT> widen(
            std::basic_ostream<CharT, Traits>& stream, const std::string_view text) {
            std::basic_string<CharT> result;
            result.reserve(text.size());
            for(char character : text) result.push_back(stream.widen(character));
            return result;
        }

        template<class CharT>
        std::basic_string<CharT> apply_grouping(
            std::basic_string<CharT> digits,
            const std::string& grouping,
            CharT separator) {
            if(grouping.empty() || digits.size() <= 1) return digits;
            
            std::basic_string<CharT> result;
            result.reserve(digits.size() + digits.size() / 3);
            std::size_t end = digits.size();
            std::size_t pattern = 0;
            while(end != 0) {
                const unsigned char size = static_cast<unsigned char>(grouping[pattern]);
                if(size == 0 || size == UCHAR_MAX) {
                    result.insert(0, digits.substr(0, end));
                    break;
                }
                const std::size_t begin = end > size ? end - size : 0;
                if(!result.empty()) result.insert(result.begin(), separator);
                result.insert(0, digits.substr(begin, end - begin));
                end = begin;
                if(pattern + 1 < grouping.size()) ++pattern;
            }
            return result;
        }

        template<class CharT, class Traits>
        std::basic_ostream<CharT, Traits>& write_integer(
            std::basic_ostream<CharT, Traits>& stream,
            std::string digits,
            std::string sign,
            std::string prefix) {
            
            typename std::basic_ostream<CharT, Traits>::sentry sentry(stream);
            if(!sentry) return stream;

            const std::streamsize width = stream.width(0);
            if((stream.flags() & std::ios_base::uppercase) != 0){
                std::ranges::transform(digits, digits.begin(), [](char c){ return std::toupper(static_cast<unsigned char>(c)); });
            }
            const auto& punctuation = std::use_facet<std::numpunct<CharT>>(stream.getloc());
            auto wide_digits = apply_grouping(widen(stream, digits), punctuation.grouping(), punctuation.thousands_sep());
            auto head = widen(stream, sign + prefix);
            const std::size_t content_size = head.size() + wide_digits.size();
            const std::size_t padding = width > 0 && static_cast<std::size_t>(width) > content_size
            ? static_cast<std::size_t>(width) - content_size : 0;
            const std::basic_string<CharT> fill(padding, stream.fill());

            std::basic_string<CharT> result;
            result.reserve(content_size + padding);
            const auto adjustment = stream.flags() & std::ios_base::adjustfield;
            if(adjustment == std::ios_base::left) {
                result = head + wide_digits + fill;
            }
            else if(adjustment == std::ios_base::internal) {
                result = head + fill + wide_digits;
            }
            else {
                result = fill + head + wide_digits;
            }

            stream.write(result.data(), static_cast<std::streamsize>(result.size()));
            return stream;
        }
    }   
}

#endif // UTILS_STREAM_H