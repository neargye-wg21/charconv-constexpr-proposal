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

// Changes:
// * add constexpr modifiers to '_Integer_to_chars'
// * add default initialize '_Buff'
// * change '_CSTD memcpy' to 'third_party::trivial_copy'
// * add static_assert(std::is_integral_v<_RawTy>)

#pragma once

#include <cstddef>
#include <climits>
#include <type_traits>

#include "charconv/detail/entity.hpp"
#include "charconv/detail/detail.hpp"

#include "third_party/constexpr_utility.hpp"

namespace nstd {

template <class _RawTy>
_NODISCARD constexpr to_chars_result _Integer_to_chars(char* _First, char* const _Last, const _RawTy _Raw_value, const int _Base) noexcept {
    static_assert(std::is_integral_v<_RawTy>);
    _Adl_verify_range(_First, _Last);
    _STL_ASSERT(_Base >= 2 && _Base <= 36, "invalid base in to_chars()");

    using _Unsigned = std::make_unsigned_t<_RawTy>;

    _Unsigned _Value = static_cast<_Unsigned>(_Raw_value);

    if constexpr (std::is_signed_v<_RawTy>) {
        if (_Raw_value < 0) {
            if (_First == _Last) {
                return {_Last, errc::value_too_large};
            }

            *_First++ = '-';

            _Value = static_cast<_Unsigned>(0 - _Value);
        }
    }

    constexpr std::size_t _Buff_size = sizeof(_Unsigned) * CHAR_BIT; // enough for base 2
    char _Buff[_Buff_size]           = {}; //[neargye] default initialize for constexpr context. P1331 fix this?
    char* const _Buff_end            = _Buff + _Buff_size;
    char* _RNext                     = _Buff_end;

    switch (_Base) {
    case 10: { // Derived from _UIntegral_to_buff()
        // Performance note: Ryu's digit table should be faster here.
        constexpr bool _Use_chunks = sizeof(_Unsigned) > sizeof(size_t);

        if constexpr (_Use_chunks) { // For 64-bit numbers on 32-bit platforms, work in chunks to avoid 64-bit
                                     // divisions.
            while (_Value > 0xFFFF'FFFFU) {
                // Performance note: Ryu's division workaround would be faster here.
                unsigned long _Chunk = static_cast<unsigned long>(_Value % 1'000'000'000);
                _Value               = static_cast<_Unsigned>(_Value / 1'000'000'000);

                for (int _Idx = 0; _Idx != 9; ++_Idx) {
                    *--_RNext = static_cast<char>('0' + _Chunk % 10);
                    _Chunk /= 10;
                }
            }
        }

        using _Truncated = std::conditional_t<_Use_chunks, unsigned long, _Unsigned>;

        _Truncated _Trunc = static_cast<_Truncated>(_Value);

        do {
            *--_RNext = static_cast<char>('0' + _Trunc % 10);
            _Trunc /= 10;
        } while (_Trunc != 0);
        break;
    }

    case 2:
        do {
            *--_RNext = static_cast<char>('0' + (_Value & 0b1));
            _Value >>= 1;
        } while (_Value != 0);
        break;

    case 4:
        do {
            *--_RNext = static_cast<char>('0' + (_Value & 0b11));
            _Value >>= 2;
        } while (_Value != 0);
        break;

    case 8:
        do {
            *--_RNext = static_cast<char>('0' + (_Value & 0b111));
            _Value >>= 3;
        } while (_Value != 0);
        break;

    case 16:
        do {
            *--_RNext = _Charconv_digits[_Value & 0b1111];
            _Value >>= 4;
        } while (_Value != 0);
        break;

    case 32:
        do {
            *--_RNext = _Charconv_digits[_Value & 0b11111];
            _Value >>= 5;
        } while (_Value != 0);
        break;

    default:
        do {
            *--_RNext = _Charconv_digits[_Value % _Base];
            _Value    = static_cast<_Unsigned>(_Value / _Base);
        } while (_Value != 0);
        break;
    }

    const std::ptrdiff_t _Digits_written = _Buff_end - _RNext;

    if (_Last - _First < _Digits_written) {
        return {_Last, errc::value_too_large};
    }

    //[neargye] constexpr copy chars. P1944 fix this?
    third_party::trivial_copy(_First, _RNext, static_cast<size_t>(_Digits_written));

    return {_First + _Digits_written, errc{}};
}

} // namespace nstd
