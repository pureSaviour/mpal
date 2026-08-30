// #ifndef INTEGRAL_128_STREAM_H
// #define INTEGRAL_128_STREAM_H

// #include <algorithm>
// #include <array>
// #include <climits>
// #include <cstddef>
// #include <ios>
// #include <istream>
// #include <locale>
// #include <ostream>
// #include <string>
// #include <string_view>
// #include <utility>
// #include <vector>
// #include "utils_stream.h"

// namespace integral128_stream_detail {

// inline int stream_base(std::ios_base::fmtflags flags) noexcept {
//     switch(flags & std::ios_base::basefield) {
//         case std::ios_base::oct: return 8;
//         case std::ios_base::hex: return 16;
//         case std::ios_base::dec: return 10;
//         default: return 0;
//     }
// }

// struct parsed_integer {
//     std::array<uint32_t, 4> words{};
//     std::ios_base::iostate state = std::ios_base::goodbit;
//     bool sentry_ok = false;
//     bool negative = false;
//     bool overflow = false;
//     bool any_digit = false;
// };

// inline void append_digit(parsed_integer& parsed, unsigned int base, unsigned int digit) noexcept {
//     uint64_t carry = digit;
//     for(int index = parsed.words.size() - 1; index >= 0; --index) {
//         const uint64_t value = static_cast<uint64_t>(parsed.words[index]) * base + carry;
//         parsed.words[index] = static_cast<uint32_t>(value);
//         carry = value >> 32U;
//     }
//     parsed.overflow = parsed.overflow || carry != 0;
// }

// inline int compare_words(
//     const std::array<uint32_t, 4>& lhs,
//     const std::array<uint32_t, 4>& rhs) noexcept {
//     for(std::size_t index = 0; index < lhs.size(); ++index) {
//         if(lhs[index] < rhs[index]) return -1;
//         if(lhs[index] > rhs[index]) return 1;
//     }
//     return 0;
// }

// // inline std::string words_to_base(std::array<uint32_t, 4> words, unsigned int base) {
// //     constexpr char alphabet[] = "0123456789abcdef";
// //     std::string result;
// //     do {
// //         uint64_t remainder = 0;
// //         for(uint32_t& word : words) {
// //             const uint64_t current = (remainder << 32U) | word;
// //             word = static_cast<uint32_t>(current / base);
// //             remainder = current % base;
// //         }
// //         result.push_back(alphabet[remainder]);
// //     } while(std::ranges::any_of(words, [](uint32_t word) { return word != 0; }));
// //     std::ranges::reverse(result);
// //     return result;
// // }

// inline bool valid_grouping(const std::vector<std::size_t>& groups, const std::string& grouping) {
//     if(groups.size() <= 1 || grouping.empty()) return true;

//     std::size_t pattern = 0;
//     for(std::size_t group = groups.size(); group-- > 1;) {
//         const unsigned char expected = static_cast<unsigned char>(grouping[pattern]);
//         if(expected == 0 || expected == UCHAR_MAX || groups[group] != expected) return false;
//         if(pattern + 1 < grouping.size()) ++pattern;
//     }

//     const unsigned char expected = static_cast<unsigned char>(grouping[pattern]);
//     return expected != 0 && expected != UCHAR_MAX
//         && groups.front() != 0 && groups.front() <= expected;
// }

// // template<class CharT, class Traits>
// // parsed_integer read_integer(std::basic_istream<CharT, Traits>& stream) {
// //     utils::stream::parsed_integer_fixed<4> a = utils::stream::read_integer<CharT, Traits, 4>(stream);
// //     parsed_integer result;
// //     result.words = a.words;
// //     result.state = a.state;
// //     result.sentry_ok = a.sentry_ok;
// //     result.negative = a.negative;
// //     result.overflow = a.overflow;
// //     result.any_digit = a.any_digit;
// //     return result;
    
// //     parsed_integer parsed;
// //     typename std::basic_istream<CharT, Traits>::sentry sentry(stream);
// //     if(!sentry) return parsed;
// //     parsed.sentry_ok = true;

// //     // auto* buffer = stream.rdbuf();
// //     std::basic_streambuf<CharT, Traits>* buffer = stream.rdbuf();
// //     const auto eof = Traits::eof();
// //     auto current = buffer->sgetc();
// //     auto is_char = [&](char expected) {
// //         return !Traits::eq_int_type(current, eof)
// //             && Traits::eq(Traits::to_char_type(current), stream.widen(expected));
// //     };
// //     auto advance = [&] {
// //         current = buffer->snextc();
// //     };

// //     if(is_char('+') || is_char('-')) {
// //         parsed.negative = is_char('-');
// //         advance();
// //     }

// //     int base = stream_base(stream.flags());
// //     bool leading_zero_is_digit = false;
// //     if(base == 0) {
// //         base = 10;
// //         if(is_char('0')) {
// //             advance();
// //             if(is_char('x') || is_char('X')) {
// //                 base = 16;
// //                 advance();
// //             }
// //             else {
// //                 base = 8;
// //                 leading_zero_is_digit = true;
// //             }
// //         }
// //     }
// //     else if(base == 16 && is_char('0')) {
// //         advance();
// //         if(is_char('x') || is_char('X')) {
// //             advance();
// //         }
// //         else {
// //             leading_zero_is_digit = true;
// //         }
// //     }

// //     std::vector<std::size_t> groups(1, leading_zero_is_digit ? 1U : 0U);
// //     if(leading_zero_is_digit) {
// //         parsed.any_digit = true;
// //         append_digit(parsed, static_cast<unsigned int>(base), 0);
// //     }

// //     const auto& punctuation = std::use_facet<std::numpunct<CharT>>(stream.getloc());
// //     const std::string grouping = punctuation.grouping();
// //     const CharT separator = punctuation.thousands_sep();

// //     while(!Traits::eq_int_type(current, eof)) {
// //         const CharT character = Traits::to_char_type(current);
// //         int digit = -1;
// //         for(char candidate = '0'; candidate <= '9'; ++candidate) {
// //             if(Traits::eq(character, stream.widen(candidate))) {
// //                 digit = candidate - '0';
// //                 break;
// //             }
// //         }
// //         if(digit < 0 && base == 16) {
// //             for(char candidate = 'a'; candidate <= 'f'; ++candidate) {
// //                 if(Traits::eq(character, stream.widen(candidate))
// //                     || Traits::eq(character, stream.widen(static_cast<char>(candidate - 'a' + 'A')))) {
// //                     digit = candidate - 'a' + 10;
// //                     break;
// //                 }
// //             }
// //         }

// //         if(digit >= 0 && digit < base) {
// //             parsed.any_digit = true;
// //             ++groups.back();
// //             append_digit(parsed, static_cast<unsigned int>(base), static_cast<unsigned int>(digit));
// //             advance();
// //             continue;
// //         }
// //         if(!grouping.empty() && parsed.any_digit && Traits::eq(character, separator)) {
// //             groups.push_back(0);
// //             advance();
// //             continue;
// //         }
// //         break;
// //     }

// //     if(Traits::eq_int_type(current, eof)) parsed.state |= std::ios_base::eofbit;
// //     if(!parsed.any_digit || !valid_grouping(groups, grouping)) {
// //         parsed.state |= std::ios_base::failbit;
// //     }
// //     if(parsed.overflow) parsed.state |= std::ios_base::failbit;
// //     return parsed;
// // }

// template<class CharT, class Traits>
// std::basic_string<CharT> widen(
//     std::basic_ios<CharT, Traits>& stream, std::string_view text) {
//     std::basic_string<CharT> result;
//     result.reserve(text.size());
//     for(char character : text) result.push_back(stream.widen(character));
//     return result;
// }

// template<class CharT>
// std::basic_string<CharT> apply_grouping(
//     std::basic_string<CharT> digits,
//     const std::string& grouping,
//     CharT separator) {
//     if(grouping.empty() || digits.size() <= 1) return digits;

//     std::basic_string<CharT> result;
//     result.reserve(digits.size() + digits.size() / 3);
//     std::size_t end = digits.size();
//     std::size_t pattern = 0;
//     while(end != 0) {
//         const unsigned char size = static_cast<unsigned char>(grouping[pattern]);
//         if(size == 0 || size == UCHAR_MAX) {
//             result.insert(0, digits.substr(0, end));
//             break;
//         }
//         const std::size_t begin = end > size ? end - size : 0;
//         if(!result.empty()) result.insert(result.begin(), separator);
//         result.insert(0, digits.substr(begin, end - begin));
//         end = begin;
//         if(pattern + 1 < grouping.size()) ++pattern;
//     }
//     return result;
// }

// template<class CharT, class Traits>
// std::basic_ostream<CharT, Traits>& write_integer(
//     std::basic_ostream<CharT, Traits>& stream,
//     std::string digits,
//     std::string sign,
//     std::string prefix) {
//     typename std::basic_ostream<CharT, Traits>::sentry sentry(stream);
//     if(!sentry) return stream;

//     const std::streamsize width = stream.width(0);
//     if((stream.flags() & std::ios_base::uppercase) != 0) {
//         std::ranges::transform(digits, digits.begin(), [](char character) {
//             return character >= 'a' && character <= 'f'
//                 ? static_cast<char>(character - 'a' + 'A') : character;
//         });
//     }

//     const auto& punctuation = std::use_facet<std::numpunct<CharT>>(stream.getloc());
//     auto wide_digits = apply_grouping(
//         widen(stream, digits), punctuation.grouping(), punctuation.thousands_sep());
//     auto head = widen(stream, sign + prefix);
//     const std::size_t content_size = head.size() + wide_digits.size();
//     const std::size_t padding = width > 0 && static_cast<std::size_t>(width) > content_size
//         ? static_cast<std::size_t>(width) - content_size : 0;
//     const std::basic_string<CharT> fill(padding, stream.fill());

//     std::basic_string<CharT> result;
//     result.reserve(content_size + padding);
//     const auto adjustment = stream.flags() & std::ios_base::adjustfield;
//     if(adjustment == std::ios_base::left) {
//         result = head + wide_digits + fill;
//     }
//     else if(adjustment == std::ios_base::internal) {
//         result = head + fill + wide_digits;
//     }
//     else {
//         result = fill + head + wide_digits;
//     }

//     stream.write(result.data(), static_cast<std::streamsize>(result.size()));
//     return stream;
// }

// } // namespace integral128_stream_detail

// #endif // INTEGRAL_128_STREAM_H
