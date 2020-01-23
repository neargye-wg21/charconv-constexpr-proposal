// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Daniil Goncharov <neargye@gmail.com>.
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

// Add Constexpr Modifiers to Functions 'to_chars' and `from_chars'

#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <climits>
#include "charconv.hpp"
#include "detail.hpp"

namespace nstd {

template <class _RawTy>
constexpr to_chars_result _Integer_to_chars(char* _First, char* const _Last, const _RawTy _Raw_value, const int _Base) noexcept {
    static_assert(std::is_integral_v<_RawTy>);
    assert(_First <= _Last); //_Adl_verify_range(_First, _Last);
    assert(_First <= _Last); //_STL_ASSERT(_Base >= 2 && _Base <= 36, "invalid base in to_chars()");

    using _Unsigned = std::make_unsigned_t<_RawTy>;

    _Unsigned _Value = static_cast<_Unsigned>(_Raw_value);

    if constexpr (std::is_signed_v<_RawTy>) {
        if (_Raw_value < 0) {
            if (_First == _Last) {
                return {_Last, std::errc::value_too_large};
            }

            *_First++ = '-';

            _Value = static_cast<_Unsigned>(0 - _Value);
        }
    }

    constexpr std::size_t _Buff_size = sizeof(_Unsigned) * CHAR_BIT; // enough for base 2
    char _Buff[_Buff_size] = {}; //[neargye] default uninitialize for constexpr context. P1331 fix this?
    char* const _Buff_end = _Buff + _Buff_size;
    char* _RNext          = _Buff_end;

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
        return {_Last, std::errc::value_too_large};
    }

    //[neargye] constexpr copy chars. P1944 fix this?
    detail::chars_copy(_First, _RNext, static_cast<size_t>(_Digits_written));

    return {_First + _Digits_written, std::errc{}};
}

template <class _RawTy>
constexpr from_chars_result _Integer_from_chars(const char* const _First, const char* const _Last, _RawTy& _Raw_value, const int _Base) noexcept {
    static_assert(std::is_integral_v<_RawTy>);
    assert(_First <= _Last); //_Adl_verify_range(_First, _Last);
    assert(_First <= _Last); //_STL_ASSERT(_Base >= 2 && _Base <= 36, "invalid base in to_chars()");

    bool _Minus_sign = false;

    const char* _Next = _First;

    if constexpr (std::is_signed_v<_RawTy>) {
        if (_Next != _Last && *_Next == '-') {
            _Minus_sign = true;
            ++_Next;
        }
    }

    using _Unsigned = std::make_unsigned_t<_RawTy>;

    constexpr _Unsigned _Uint_max    = static_cast<_Unsigned>(-1);
    constexpr _Unsigned _Int_max     = static_cast<_Unsigned>(_Uint_max >> 1);
    constexpr _Unsigned _Abs_int_min = static_cast<_Unsigned>(_Int_max + 1);

    _Unsigned _Risky_val = {}; //[neargye] default uninitialize for constexpr context. P1331 fix this?
    _Unsigned _Max_digit = {}; //[neargye] default uninitialize for constexpr context. P1331 fix this?

    if constexpr (std::is_signed_v<_RawTy>) {
        if (_Minus_sign) {
            _Risky_val = static_cast<_Unsigned>(_Abs_int_min / _Base);
            _Max_digit = static_cast<_Unsigned>(_Abs_int_min % _Base);
        } else {
            _Risky_val = static_cast<_Unsigned>(_Int_max / _Base);
            _Max_digit = static_cast<_Unsigned>(_Int_max % _Base);
        }
    } else {
        _Risky_val = static_cast<_Unsigned>(_Uint_max / _Base);
        _Max_digit = static_cast<_Unsigned>(_Uint_max % _Base);
    }

    _Unsigned _Value = 0;

    bool _Overflowed = false;

    for (; _Next != _Last; ++_Next) {
        const unsigned char _Digit = _Digit_from_char(*_Next);

        if (_Digit >= _Base) {
            break;
        }

        if (_Value < _Risky_val // never overflows
            || (_Value == _Risky_val && _Digit <= _Max_digit)) { // overflows for certain digits
            _Value = static_cast<_Unsigned>(_Value * _Base + _Digit);
        } else { // _Value > _Risky_val always overflows
            _Overflowed = true; // keep going, _Next still needs to be updated, _Value is now irrelevant
        }
    }

    if (_Next - _First == static_cast<std::ptrdiff_t>(_Minus_sign)) {
        return {_First, std::errc::invalid_argument};
    }

    if (_Overflowed) {
        return {_Next, std::errc::result_out_of_range};
    }

    if constexpr (std::is_signed_v<_RawTy>) {
        if (_Minus_sign) {
            _Value = static_cast<_Unsigned>(0 - _Value);
        }
    }

    _Raw_value = static_cast<_RawTy>(_Value); // implementation-defined for negative, N4713 7.8 [conv.integral]/3

    return {_Next, std::errc{}};
}

} // namespace nstd
