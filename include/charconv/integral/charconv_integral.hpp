// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 Daniil Goncharov <neargye@gmail.com>, Karaev Alexander <akaraevz@mail.ru>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// https://github.com/llvm/llvm-project/commits/main/libcxx/include/charconv
// a354fd56c5046cc4317a301d90908520f6c4717a
// Changes:
// * add constexpr modifiers
// * add constexpr log2f
// * change 'memcpy' 'memmove'
// move charconv.cpp to charconv.hpp

#pragma once

#if defined(_MSC_VER)
#define _LIBCPP_COMPILER_MSVC 1
#endif

#include <type_traits>
#include <bit>

#include "entity.hpp"
#include "third_party/constexpr_utility.hpp"

namespace nstd {

template <typename I>
constexpr I log2(I value) noexcept {
  auto ret = I{0};
  for (; value > I{1}; value >>= I{1}, ++ret) {}

  return ret;
}

namespace __itoa
{

inline constexpr char cDigitsLut[200] = { //[neargye] static -> inline
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0',
    '7', '0', '8', '0', '9', '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0', '2', '1', '2',
    '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3',
    '7', '3', '8', '3', '9', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', '5', '0', '5', '1', '5',
    '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6',
    '7', '6', '8', '6', '9', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '8', '0', '8', '1', '8',
    '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9',
    '7', '9', '8', '9', '9'};

template <typename T>
constexpr char*
append1(char* buffer, T i) noexcept
{
    *buffer = '0' + static_cast<char>(i);
    return buffer + 1;
}

template <typename T>
constexpr char*
append2(char* buffer, T i) noexcept
{
    third_party::trivial_copy(buffer, &cDigitsLut[(i)*2], 2);
    return buffer + 2;
}

template <typename T>
constexpr char*
append3(char* buffer, T i) noexcept
{
    return append2(append1(buffer, (i) / 100), (i) % 100);
}

template <typename T>
constexpr char*
append4(char* buffer, T i) noexcept
{
    return append2(append2(buffer, (i) / 100), (i) % 100);
}

template <typename T>
constexpr char*
append2_no_zeros(char* buffer, T v) noexcept
{
    if (v < 10)
        return append1(buffer, v);
    else
        return append2(buffer, v);
}

template <typename T>
constexpr char*
append4_no_zeros(char* buffer, T v) noexcept
{
    if (v < 100)
        return append2_no_zeros(buffer, v);
    else if (v < 1000)
        return append3(buffer, v);
    else
        return append4(buffer, v);
}

template <typename T>
constexpr char*
append8_no_zeros(char* buffer, T v) noexcept
{
    if (v < 10000)
    {
        buffer = append4_no_zeros(buffer, v);
    }
    else
    {
        buffer = append4_no_zeros(buffer, v / 10000);
        buffer = append4(buffer, v % 10000);
    }
    return buffer;
}

constexpr char*
__u32toa(uint32_t value, char* buffer) noexcept
{
    if (value < 100000000)
    {
        buffer = append8_no_zeros(buffer, value);
    }
    else
    {
        // value = aabbbbcccc in decimal
        const uint32_t a = value / 100000000;  // 1 to 42
        value %= 100000000;

        buffer = append2_no_zeros(buffer, a);
        buffer = append4(buffer, value / 10000);
        buffer = append4(buffer, value % 10000);
    }

    return buffer;
}

constexpr char*
__u64toa(uint64_t value, char* buffer) noexcept
{
    if (value < 100000000)
    {
        uint32_t v = static_cast<uint32_t>(value);
        buffer = append8_no_zeros(buffer, v);
    }
    else if (value < 10000000000000000)
    {
        const uint32_t v0 = static_cast<uint32_t>(value / 100000000);
        const uint32_t v1 = static_cast<uint32_t>(value % 100000000);

        buffer = append8_no_zeros(buffer, v0);
        buffer = append4(buffer, v1 / 10000);
        buffer = append4(buffer, v1 % 10000);
    }
    else
    {
        const uint32_t a =
            static_cast<uint32_t>(value / 10000000000000000);  // 1 to 1844
        value %= 10000000000000000;

        buffer = append4_no_zeros(buffer, a);

        const uint32_t v0 = static_cast<uint32_t>(value / 100000000);
        const uint32_t v1 = static_cast<uint32_t>(value % 100000000);
        buffer = append4(buffer, v0 / 10000);
        buffer = append4(buffer, v0 % 10000);
        buffer = append4(buffer, v1 / 10000);
        buffer = append4(buffer, v1 % 10000);
    }

    return buffer;
}

}  // namespace __itoa

void to_chars(char*, char*, bool, int = 10) = delete;
void from_chars(const char*, const char*, bool, int = 10) = delete;

namespace __itoa
{

inline constexpr uint64_t __pow10_64[] = { //[neargye] static -> inline
    UINT64_C(0),
    UINT64_C(10),
    UINT64_C(100),
    UINT64_C(1000),
    UINT64_C(10000),
    UINT64_C(100000),
    UINT64_C(1000000),
    UINT64_C(10000000),
    UINT64_C(100000000),
    UINT64_C(1000000000),
    UINT64_C(10000000000),
    UINT64_C(100000000000),
    UINT64_C(1000000000000),
    UINT64_C(10000000000000),
    UINT64_C(100000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(10000000000000000),
    UINT64_C(100000000000000000),
    UINT64_C(1000000000000000000),
    UINT64_C(10000000000000000000),
};

inline constexpr uint32_t __pow10_32[] = { //[neargye] static -> inline
    UINT32_C(0),          UINT32_C(10),       UINT32_C(100),
    UINT32_C(1000),       UINT32_C(10000),    UINT32_C(100000),
    UINT32_C(1000000),    UINT32_C(10000000), UINT32_C(100000000),
    UINT32_C(1000000000),
};

template <typename _Tp, typename = void>
struct __traits_base
{
    using type = uint64_t;

#if !defined(_LIBCPP_COMPILER_MSVC)
    static constexpr int __width(_Tp __v)
    {
        auto __t = (64 - __builtin_clzll(__v | 1)) * 1233 >> 12;
        return __t - (__v < __pow10_64[__t]) + 1;
    }
#endif

    
    static constexpr char* __convert(_Tp __v, char* __p)
    {
        return __u64toa(__v, __p);
    }

    static constexpr decltype(__pow10_64)& __pow() { return __pow10_64; }
};

template <typename _Tp>
struct __traits_base<_Tp, decltype(void(uint32_t{std::declval<_Tp>()}))>
{
    using type = uint32_t;

#if !defined(_LIBCPP_COMPILER_MSVC)
    static constexpr int __width(_Tp __v)
    {
        auto __t = (32 - __builtin_clz(__v | 1)) * 1233 >> 12;
        return __t - (__v < __pow10_32[__t]) + 1;
    }
#endif

    
    static constexpr char* __convert(_Tp __v, char* __p)
    {
        return __u32toa(__v, __p);
    }

    static constexpr decltype(__pow10_32)& __pow() { return __pow10_32; }
};

template <typename _Tp>
constexpr bool
__mul_overflowed(unsigned char __a, _Tp __b, unsigned char& __r)
{
    auto __c = __a * __b;
    __r = __c;
    return __c > std::numeric_limits<unsigned char>::max();
}

template <typename _Tp>
constexpr bool
__mul_overflowed(unsigned short __a, _Tp __b, unsigned short& __r)
{
    auto __c = __a * __b;
    __r = __c;
    return __c > std::numeric_limits<unsigned short>::max();
}

template <typename _Tp>
constexpr bool
__mul_overflowed(_Tp __a, _Tp __b, _Tp& __r)
{
    static_assert(std::is_unsigned<_Tp>::value, "");
#if !defined(_LIBCPP_COMPILER_MSVC)
    return __builtin_mul_overflow(__a, __b, &__r);
#else
    bool __did = __b && (std::numeric_limits<_Tp>::max() / __b) < __a;
    __r = __a * __b;
    return __did;
#endif
}

template <typename _Tp, typename _Up>
constexpr bool
__mul_overflowed(_Tp __a, _Up __b, _Tp& __r)
{
    return __mul_overflowed(__a, static_cast<_Tp>(__b), __r);
}

template <typename _Tp>
struct __traits : __traits_base<_Tp>
{
    static constexpr int digits = std::numeric_limits<_Tp>::digits10 + 1;
    using __traits_base<_Tp>::__pow;
    using typename __traits_base<_Tp>::type;

    // precondition: at least one non-zero character available
    static constexpr char const*
    __read(char const* __p, char const* __ep, type& __a, type& __b)
    {
        type __cprod[digits];
        int __j = digits - 1;
        int __i = digits;
        do
        {
            if (!('0' <= *__p && *__p <= '9'))
                break;
            __cprod[--__i] = *__p++ - '0';
        } while (__p != __ep && __i != 0);

        __a = __inner_product(__cprod + __i + 1, __cprod + __j, __pow() + 1,
                              __cprod[__i]);
        if (__mul_overflowed(__cprod[__j], __pow()[__j - __i], __b))
            --__p;
        return __p;
    }

    template <typename _It1, typename _It2, class _Up>
    static constexpr _Up
    __inner_product(_It1 __first1, _It1 __last1, _It2 __first2, _Up __init)
    {
        for (; __first1 < __last1; ++__first1, ++__first2)
            __init = __init + *__first1 * *__first2;
        return __init;
    }
};

}  // namespace __itoa

template <typename _Tp>
constexpr _Tp
__complement(_Tp __x)
{
    static_assert(std::is_unsigned<_Tp>::value, "cast to unsigned first");
    return _Tp(~__x + 1);
}

template <typename _Tp>
constexpr typename std::make_unsigned<_Tp>::type
__to_unsigned(_Tp __x)
{
    return static_cast<typename std::make_unsigned<_Tp>::type>(__x);
}

template <typename _Tp>
constexpr to_chars_result
__to_chars_itoa(char* __first, char* __last, _Tp __value, std::false_type)
{
  using __tx = __itoa::__traits<_Tp>;
  auto __diff = __last - __first;

#if !defined(_LIBCPP_COMPILER_MSVC)
  if (__tx::digits <= __diff || __tx::__width(__value) <= __diff)
    return { __tx::__convert(__value, __first), errc(0) };
  else
    return { __last, errc::value_too_large };
#else
  if (__tx::digits <= __diff)
    return { __tx::__convert(__value, __first), {} };
  else
  {
    char __buf[__tx::digits];
    auto __p = __tx::__convert(__value, __buf);
    auto __len = __p - __buf;
    if (__len <= __diff)
    {
      third_party::trivial_copy(__first, __buf, __len);
      return { __first + __len, {} };
    }
    else
      return { __last, errc::value_too_large };
  }
#endif
}

template <typename _Tp>
constexpr to_chars_result
__to_chars_itoa(char* __first, char* __last, _Tp __value, std::true_type)
{
    auto __x = __to_unsigned(__value);
    if (__value < 0 && __first != __last)
    {
        *__first++ = '-';
        __x = __complement(__x);
    }

    return __to_chars_itoa(__first, __last, __x, std::false_type());
}

template <typename _Tp>
constexpr to_chars_result
__to_chars_integral(char* __first, char* __last, _Tp __value, int __base, std::false_type)
{
    if (__base == 10)
        return __to_chars_itoa(__first, __last, __value, std::false_type());

    auto __p = __last;
    while (__p != __first)
    {
        auto __c = __value % __base;
        __value /= __base;
        *--__p = "0123456789abcdefghijklmnopqrstuvwxyz"[__c];
        if (__value == 0)
            break;
    }

    auto __len = __last - __p;
    if (__value != 0 || !__len)
        return {__last, errc::value_too_large};
    else
    {
        third_party::trivial_move(__first, __p, __len);
        return {__first + __len, {}};
    }
}

template <typename _Tp>
constexpr to_chars_result
__to_chars_integral(char* __first, char* __last, _Tp __value, int __base, std::true_type)
{
    auto __x = __to_unsigned(__value);
    if (__value < 0 && __first != __last)
    {
        *__first++ = '-';
        __x = __complement(__x);
    }

    return __to_chars_integral(__first, __last, __x, __base, std::false_type());
}

template <typename _Tp, typename std::enable_if<std::is_integral<_Tp>::value, int>::type = 0>
constexpr to_chars_result
to_chars(char* __first, char* __last, _Tp __value)
{
    return __to_chars_itoa(__first, __last, __value, std::is_signed<_Tp>());
}

template <typename _Tp, typename std::enable_if<std::is_integral<_Tp>::value, int>::type = 0>
constexpr to_chars_result
to_chars(char* __first, char* __last, _Tp __value, int __base)
{
    //_LIBCPP_ASSERT(2 <= __base && __base <= 36, "base not in [2, 36]");
    return __to_chars_integral(__first, __last, __value, __base,
      std::is_signed<_Tp>());
}

template <typename _It, typename _Tp, typename _Fn, typename... _Ts>
constexpr from_chars_result
__sign_combinator(_It __first, _It __last, _Tp& __value, _Fn __f, _Ts... __args)
{
    using __tl = std::numeric_limits<_Tp>;
    decltype(__to_unsigned(__value)) __x;

    bool __neg = (__first != __last && *__first == '-');
    auto __r = __f(__neg ? __first + 1 : __first, __last, __x, __args...);
    switch (__r.ec)
    {
    case errc::invalid_argument:
        return {__first, __r.ec};
    case errc::result_out_of_range:
        return __r;
    default:
        break;
    }

    if (__neg)
    {
        if (__x <= __complement(__to_unsigned(__tl::min())))
        {
            __x = __complement(__x);
            //third_party::trivial_copy(&__value, &__x, sizeof(__x));
            //__value = std::bit_cast<std::remove_cvref_t<decltype(__value)>>(__x);
            __value = static_cast<std::remove_cvref_t<decltype(__value)>>(__x);
            return __r;
        }
    }
    else
    {
        if (__x <= __tl::max())
        {
            __value = __x;
            return __r;
        }
    }

    return {__r.ptr, errc::result_out_of_range};
}

template <typename _Tp>
constexpr bool
__in_pattern(_Tp __c)
{
    return '0' <= __c && __c <= '9';
}

struct __in_pattern_result
{
    bool __ok;
    int __val;

    explicit constexpr operator bool() const { return __ok; }
};

template <typename _Tp>
constexpr __in_pattern_result
__in_pattern(_Tp __c, int __base)
{
    if (__base <= 10)
        return {'0' <= __c && __c < '0' + __base, __c - '0'};
    else if (__in_pattern(__c))
        return {true, __c - '0'};
    else if ('a' <= __c && __c < 'a' + __base - 10)
        return {true, __c - 'a' + 10};
    else
        return {'A' <= __c && __c < 'A' + __base - 10, __c - 'A' + 10};
}

template <typename _It, typename _Tp, typename _Fn, typename... _Ts>
constexpr from_chars_result
__subject_seq_combinator(_It __first, _It __last, _Tp& __value, _Fn __f,
                         _Ts... __args)
{
    auto __find_non_zero = [](_It __first, _It __last) {
        for (; __first != __last; ++__first)
            if (*__first != '0')
                break;
        return __first;
    };

    auto __p = __find_non_zero(__first, __last);
    if (__p == __last || !__in_pattern(*__p, __args...))
    {
        if (__p == __first)
            return {__first, errc::invalid_argument};
        else
        {
            __value = 0;
            return {__p, {}};
        }
    }

    auto __r = __f(__p, __last, __value, __args...);
    if (__r.ec == errc::result_out_of_range)
    {
        for (; __r.ptr != __last; ++__r.ptr)
        {
            if (!__in_pattern(*__r.ptr, __args...))
                break;
        }
    }

    return __r;
}

template <typename _Tp, typename std::enable_if<std::is_unsigned<_Tp>::value, int>::type = 0>
constexpr from_chars_result
__from_chars_atoi(const char* __first, const char* __last, _Tp& __value)
{
    using __tx = __itoa::__traits<_Tp>;
    using __output_type = typename __tx::type;

    return __subject_seq_combinator(
        __first, __last, __value,
        [](const char* __first, const char* __last,
           _Tp& __value) -> from_chars_result {
            __output_type __a, __b;
            auto __p = __tx::__read(__first, __last, __a, __b);
            if (__p == __last || !__in_pattern(*__p))
            {
                __output_type __m = std::numeric_limits<_Tp>::max();
                if (__m >= __a && __m - __a >= __b)
                {
                    __value = __a + __b;
                    return {__p, {}};
                }
            }
            return {__p, errc::result_out_of_range};
        });
}

template <typename _Tp, typename std::enable_if<std::is_signed<_Tp>::value, int>::type = 0>
constexpr from_chars_result
__from_chars_atoi(const char* __first, const char* __last, _Tp& __value)
{
    using __t = decltype(__to_unsigned(__value));
    return __sign_combinator(__first, __last, __value, __from_chars_atoi<__t>);
}

template <typename _Tp, typename std::enable_if<std::is_unsigned<_Tp>::value, int>::type = 0>
constexpr from_chars_result
__from_chars_integral(const char* __first, const char* __last, _Tp& __value,
                      int __base)
{
    if (__base == 10)
        return __from_chars_atoi(__first, __last, __value);

    return __subject_seq_combinator(
        __first, __last, __value,
        [](const char* __p, const char* __last, _Tp& __value,
           int __base) -> from_chars_result {
            using __tl = std::numeric_limits<_Tp>;
            auto __digits = __tl::digits / (float)log2(__base);
            _Tp __a = __in_pattern(*__p++, __base).__val, __b = 0;

            for (int __i = 1; __p != __last; ++__i, ++__p)
            {
                if (auto __c = __in_pattern(*__p, __base))
                {
                    if (__i < __digits - 1)
                        __a = __a * __base + __c.__val;
                    else
                    {
                        if (!__itoa::__mul_overflowed(__a, __base, __a))
                            ++__p;
                        __b = __c.__val;
                        break;
                    }
                }
                else
                    break;
            }

            if (__p == __last || !__in_pattern(*__p, __base))
            {
                if (__tl::max() - __a >= __b)
                {
                    __value = __a + __b;
                    return {__p, {}};
                }
            }
            return {__p, errc::result_out_of_range};
        },
        __base);
}

template <typename _Tp, typename std::enable_if<std::is_signed<_Tp>::value, int>::type = 0>
constexpr from_chars_result
__from_chars_integral(const char* __first, const char* __last, _Tp& __value,
                      int __base)
{
    using __t = decltype(__to_unsigned(__value));
    return __sign_combinator(__first, __last, __value,
                             __from_chars_integral<__t>, __base);
}

template <typename _Tp, typename std::enable_if<std::is_integral<_Tp>::value, int>::type = 0>
constexpr from_chars_result
from_chars(const char* __first, const char* __last, _Tp& __value)
{
    return __from_chars_atoi(__first, __last, __value);
}

template <typename _Tp, typename std::enable_if<std::is_integral<_Tp>::value, int>::type = 0>
constexpr from_chars_result
from_chars(const char* __first, const char* __last, _Tp& __value, int __base)
{
    //_LIBCPP_ASSERT(2 <= __base && __base <= 36, "base not in [2, 36]");
    return __from_chars_integral(__first, __last, __value, __base);
}

} // namespace nstd
