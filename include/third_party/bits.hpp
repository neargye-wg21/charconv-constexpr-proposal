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

#pragma once

#include <type_traits>

#if __has_include(<bit>)
#include <bit>
#endif

#if !defined(__cpp_lib_bit_cast)
#include "third_party/ieee754.hpp"
#endif

// TODO _BitScanReverse

namespace third_party {

template <class _To, class _From>
constexpr _To bit_cast(const _From& _From_obj) noexcept {
    static_assert(sizeof(_To) == sizeof(_From));
    static_assert(std::is_trivially_copyable_v<_To>);
    static_assert(std::is_trivially_copyable_v<_From>);
#if defined(__cpp_lib_bit_cast)
    return __builtin_bit_cast(_To_obj, _From_obj);
#else
    if constexpr (std::is_floating_point_v<_From> || std::is_floating_point_v<_To>) {
        return static_cast<_To>(third_party::ieee754(_From_obj));
    } else {
        static_assert(sizeof(_To) == 0, "not implemented");
    }
#endif
}

constexpr bool bit_scan_forward(unsigned long* _Index, uint32_t _Mask) noexcept {
    if(_Mask == 0) {
        return false;
    }
#if defined(__cpp_lib_bit_cast)
    (void)_BitScanForward(_Index, _Mask);
#else
    *_Index = __builtin_ctzl(_Mask);
#endif
    return true;
}

constexpr bool bit_scan_reverse(unsigned long* _Index, uint32_t _Mask) noexcept {
    if(_Mask == 0) {
        return false;
    }
#if defined(__cpp_lib_bit_cast)
    (void)_BitScanReverse(_Index, _Mask);
#else
    *_Index = __builtin_clzl(_Mask);
#endif
    return true;
}

#if defined(_WIN64) || defined(__x86_64__)
constexpr bool bit_scan_forward(unsigned long* _Index, uint64_t _Mask) noexcept {
    if(_Mask == 0) {
        return false;
    }
#if defined(__cpp_lib_bit_cast)
    (void)_BitScanForward64(_Index, _Mask);
#else
    *_Index = __builtin_ctzll(_Mask);
#endif
    return true;
}

constexpr bool bit_scan_reverse(unsigned long* _Index, uint64_t _Mask) noexcept {
    if(_Mask == 0) {
        return false;
    }
#if defined(__cpp_lib_bit_cast)
    (void)_BitScanReverse64(_Index, _Mask);
#else
    *_Index = __builtin_clzll(_Mask);
#endif
    return true;
}
#endif

} // namespace third_party
