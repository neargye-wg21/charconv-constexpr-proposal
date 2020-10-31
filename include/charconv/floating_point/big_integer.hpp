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

// * change '_CSTD memcpy' to 'third_party::trivial_copy'
// * change '_CSTD memmove' to 'third_party::trivial_move'
// * change '_CSTD memset' to 'third_party::trivial_fill'
// * change '_Min_value' to 'std::min'

#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>

#include "charconv/detail/detail.hpp"

#include "third_party/bits.hpp"
#include "third_party/constexpr_utility.hpp"

namespace nstd {

struct _Big_integer_flt {
    constexpr _Big_integer_flt() noexcept : _Myused(0), _Mydata{} {}

    constexpr _Big_integer_flt(const _Big_integer_flt& _Other) noexcept : _Myused(_Other._Myused), _Mydata{} {
        //[neargye] constexpr copy uint32_t. P1944 fix this?
        third_party::trivial_copy(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
    }

    constexpr _Big_integer_flt& operator=(const _Big_integer_flt& _Other) noexcept {
        _Myused = _Other._Myused;
        //[neargye] constexpr move uint32_t. P1944 fix this?
        third_party::trivial_move(_Mydata, _Other._Mydata, _Other._Myused * sizeof(uint32_t));
        return *this;
    }

    _NODISCARD constexpr bool operator<(const _Big_integer_flt& _Rhs) const noexcept {
        if (_Myused != _Rhs._Myused) {
            return _Myused < _Rhs._Myused;
        }

        for (uint32_t _Ix = _Myused - 1; _Ix != static_cast<uint32_t>(-1); --_Ix) {
            if (_Mydata[_Ix] != _Rhs._Mydata[_Ix]) {
                return _Mydata[_Ix] < _Rhs._Mydata[_Ix];
            }
        }

        return false;
    }

    static constexpr uint32_t _Maximum_bits = 1074 // 1074 bits required to represent 2^1074
                                              + 2552 // ceil(log2(10^768))
                                              + 54; // shift space

    static constexpr uint32_t _Element_bits = 32;

    static constexpr uint32_t _Element_count = (_Maximum_bits + _Element_bits - 1) / _Element_bits;

    uint32_t _Myused; // The number of elements currently in use
    uint32_t _Mydata[_Element_count]; // The number, stored in little-endian form
};

_NODISCARD constexpr _Big_integer_flt _Make_big_integer_flt_one() noexcept {
    _Big_integer_flt _Xval{};
    _Xval._Mydata[0] = 1;
    _Xval._Myused    = 1;
    return _Xval;
}

_NODISCARD constexpr _Big_integer_flt _Make_big_integer_flt_u32(const uint32_t _Value) noexcept {
    _Big_integer_flt _Xval{};
    _Xval._Mydata[0] = _Value;
    _Xval._Myused    = 1;
    return _Xval;
}

_NODISCARD constexpr _Big_integer_flt _Make_big_integer_flt_u64(const uint64_t _Value) noexcept {
    _Big_integer_flt _Xval{};
    _Xval._Mydata[0] = static_cast<uint32_t>(_Value);
    _Xval._Mydata[1] = static_cast<uint32_t>(_Value >> 32);
    _Xval._Myused    = _Xval._Mydata[1] == 0 ? 1u : 2u;
    return _Xval;
}

_NODISCARD constexpr _Big_integer_flt _Make_big_integer_flt_power_of_two(const uint32_t _Power) noexcept {
    const uint32_t _Element_index = _Power / _Big_integer_flt::_Element_bits;
    const uint32_t _Bit_index     = _Power % _Big_integer_flt::_Element_bits;

    _Big_integer_flt _Xval{};
    third_party::trivial_fill(_Xval._Mydata, (uint32_t)0, _Element_index * sizeof(uint32_t));
    _Xval._Mydata[_Element_index] = 1u << _Bit_index;
    _Xval._Myused                 = _Element_index + 1;
    return _Xval;
}

_NODISCARD constexpr uint32_t _Bit_scan_reverse(const uint32_t _Value) noexcept {
    unsigned long _Index;

    if (third_party::bit_scan_reverse(&_Index, _Value)) {
        return _Index + 1;
    }

    return 0;
}

_NODISCARD constexpr uint32_t _Bit_scan_reverse(const uint64_t _Value) noexcept {
    unsigned long _Index;

#ifdef _WIN64
    if (third_party::bit_scan_reverse(&_Index, _Value)) {
        return _Index + 1;
    }
#else // ^^^ 64-bit ^^^ / vvv 32-bit vvv
    uint32_t _Ui32 = static_cast<uint32_t>(_Value >> 32);

    if (third_party::bit_scan_reverse(&_Index, _Ui32)) {
        return _Index + 1 + 32;
    }

    _Ui32 = static_cast<uint32_t>(_Value);

    if (third_party::bit_scan_reverse(&_Index, _Ui32)) {
        return _Index + 1;
    }
#endif // ^^^ 32-bit ^^^

    return 0;
}

_NODISCARD constexpr uint32_t _Bit_scan_reverse(const _Big_integer_flt& _Xval) noexcept {
    if (_Xval._Myused == 0) {
        return 0;
    }

    const uint32_t _Bx = _Xval._Myused - 1;

    nstd_assert(_Xval._Mydata[_Bx] != 0); // _Big_integer_flt should always be trimmed

    unsigned long _Index;

    third_party::bit_scan_reverse(&_Index, _Xval._Mydata[_Bx]); // assumes _Xval._Mydata[_Bx] != 0

    return _Index + 1 + _Bx * _Big_integer_flt::_Element_bits;
}

// Shifts the high-precision integer _Xval by _Nx bits to the left. Returns true if the left shift was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
_NODISCARD constexpr bool _Shift_left(_Big_integer_flt& _Xval, const uint32_t _Nx) noexcept {
    if (_Xval._Myused == 0) {
        return true;
    }

    const uint32_t _Unit_shift = _Nx / _Big_integer_flt::_Element_bits;
    const uint32_t _Bit_shift  = _Nx % _Big_integer_flt::_Element_bits;

    if (_Xval._Myused + _Unit_shift > _Big_integer_flt::_Element_count) {
        // Unit shift will overflow.
        _Xval._Myused = 0;
        return false;
    }

    if (_Bit_shift == 0) {
        //[neargye] constexpr move uint32_t. P1944 fix this?
        third_party::trivial_move(_Xval._Mydata + _Unit_shift, _Xval._Mydata, _Xval._Myused * sizeof(uint32_t));
        _Xval._Myused += _Unit_shift;
    } else {
        const bool _Bit_shifts_into_next_unit =
            _Bit_shift > (_Big_integer_flt::_Element_bits - _Bit_scan_reverse(_Xval._Mydata[_Xval._Myused - 1]));

        const uint32_t _New_used = _Xval._Myused + _Unit_shift + static_cast<uint32_t>(_Bit_shifts_into_next_unit);

        if (_New_used > _Big_integer_flt::_Element_count) {
            // Bit shift will overflow.
            _Xval._Myused = 0;
            return false;
        }

        const uint32_t _Msb_bits = _Bit_shift;
        const uint32_t _Lsb_bits = _Big_integer_flt::_Element_bits - _Msb_bits;

        const uint32_t _Lsb_mask = (1UL << _Lsb_bits) - 1UL;
        const uint32_t _Msb_mask = ~_Lsb_mask;

        // If _Unit_shift == 0, this will wraparound, which is okay.
        for (uint32_t _Dest_index = _New_used - 1; _Dest_index != _Unit_shift - 1; --_Dest_index) {
            // performance note: PSLLDQ and PALIGNR instructions could be more efficient here

            // If _Bit_shifts_into_next_unit, the first iteration will trigger the bounds check below, which is okay.
            const uint32_t _Upper_source_index = _Dest_index - _Unit_shift;

            // When _Dest_index == _Unit_shift, this will wraparound, which is okay (see bounds check below).
            const uint32_t _Lower_source_index = _Dest_index - _Unit_shift - 1;

            const uint32_t _Upper_source = _Upper_source_index < _Xval._Myused ? _Xval._Mydata[_Upper_source_index] : 0;
            const uint32_t _Lower_source = _Lower_source_index < _Xval._Myused ? _Xval._Mydata[_Lower_source_index] : 0;

            const uint32_t _Shifted_upper_source = (_Upper_source & _Lsb_mask) << _Msb_bits;
            const uint32_t _Shifted_lower_source = (_Lower_source & _Msb_mask) >> _Lsb_bits;

            const uint32_t _Combined_shifted_source = _Shifted_upper_source | _Shifted_lower_source;

            _Xval._Mydata[_Dest_index] = _Combined_shifted_source;
        }

        _Xval._Myused = _New_used;
    }

    //[neargye] constexpr fill uint32_t. P1944 fix this?
    third_party::trivial_fill(_Xval._Mydata, (uint32_t)0, _Unit_shift * sizeof(uint32_t));

    return true;
}

// Adds a 32-bit _Value to the high-precision integer _Xval. Returns true if the addition was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
_NODISCARD constexpr bool _Add(_Big_integer_flt& _Xval, const uint32_t _Value) noexcept {
    if (_Value == 0) {
        return true;
    }

    uint32_t _Carry = _Value;
    for (uint32_t _Ix = 0; _Ix != _Xval._Myused; ++_Ix) {
        const uint64_t _Result = static_cast<uint64_t>(_Xval._Mydata[_Ix]) + _Carry;
        _Xval._Mydata[_Ix]     = static_cast<uint32_t>(_Result);
        _Carry                 = static_cast<uint32_t>(_Result >> 32);
    }

    if (_Carry != 0) {
        if (_Xval._Myused < _Big_integer_flt::_Element_count) {
            _Xval._Mydata[_Xval._Myused] = _Carry;
            ++_Xval._Myused;
        } else {
            _Xval._Myused = 0;
            return false;
        }
    }

    return true;
}

_NODISCARD constexpr uint32_t _Add_carry(uint32_t& _U1, const uint32_t _U2, const uint32_t _U_carry) noexcept {
    const uint64_t _Uu = static_cast<uint64_t>(_U1) + _U2 + _U_carry;
    _U1                = static_cast<uint32_t>(_Uu);
    return static_cast<uint32_t>(_Uu >> 32);
}

_NODISCARD constexpr uint32_t _Add_multiply_carry(
    uint32_t& _U_add, const uint32_t _U_mul_1, const uint32_t _U_mul_2, const uint32_t _U_carry) noexcept {
    const uint64_t _Uu_res = static_cast<uint64_t>(_U_mul_1) * _U_mul_2 + _U_add + _U_carry;
    _U_add                 = static_cast<uint32_t>(_Uu_res);
    return static_cast<uint32_t>(_Uu_res >> 32);
}

_NODISCARD constexpr uint32_t _Multiply_core(
    uint32_t* const _Multiplicand, const uint32_t _Multiplicand_count, const uint32_t _Multiplier) noexcept {
    uint32_t _Carry = 0;
    for (uint32_t _Ix = 0; _Ix != _Multiplicand_count; ++_Ix) {
        const uint64_t _Result = static_cast<uint64_t>(_Multiplicand[_Ix]) * _Multiplier + _Carry;
        _Multiplicand[_Ix]     = static_cast<uint32_t>(_Result);
        _Carry                 = static_cast<uint32_t>(_Result >> 32);
    }

    return _Carry;
}

// Multiplies the high-precision _Multiplicand by a 32-bit _Multiplier. Returns true if the multiplication
// was successful; false if it overflowed. When overflow occurs, the _Multiplicand is reset to zero.
_NODISCARD constexpr bool _Multiply(_Big_integer_flt& _Multiplicand, const uint32_t _Multiplier) noexcept {
    if (_Multiplier == 0) {
        _Multiplicand._Myused = 0;
        return true;
    }

    if (_Multiplier == 1) {
        return true;
    }

    if (_Multiplicand._Myused == 0) {
        return true;
    }

    const uint32_t _Carry = _Multiply_core(_Multiplicand._Mydata, _Multiplicand._Myused, _Multiplier);
    if (_Carry != 0) {
        if (_Multiplicand._Myused < _Big_integer_flt::_Element_count) {
            _Multiplicand._Mydata[_Multiplicand._Myused] = _Carry;
            ++_Multiplicand._Myused;
        } else {
            _Multiplicand._Myused = 0;
            return false;
        }
    }

    return true;
}

// This high-precision integer multiplication implementation was translated from the implementation of
// System.Numerics.BigIntegerBuilder.Mul in the .NET Framework sources. It multiplies the _Multiplicand
// by the _Multiplier and returns true if the multiplication was successful; false if it overflowed.
// When overflow occurs, the _Multiplicand is reset to zero.
_NODISCARD constexpr bool _Multiply(_Big_integer_flt& _Multiplicand, const _Big_integer_flt& _Multiplier) noexcept {
    if (_Multiplicand._Myused == 0) {
        return true;
    }

    if (_Multiplier._Myused == 0) {
        _Multiplicand._Myused = 0;
        return true;
    }

    if (_Multiplier._Myused == 1) {
        return _Multiply(_Multiplicand, _Multiplier._Mydata[0]); // when overflow occurs, resets to zero
    }

    if (_Multiplicand._Myused == 1) {
        const uint32_t _Small_multiplier = _Multiplicand._Mydata[0];
        _Multiplicand                    = _Multiplier;
        return _Multiply(_Multiplicand, _Small_multiplier); // when overflow occurs, resets to zero
    }

    // We prefer more iterations on the inner loop and fewer on the outer:
    const bool _Multiplier_is_shorter = _Multiplier._Myused < _Multiplicand._Myused;
    const uint32_t* const _Rgu1       = _Multiplier_is_shorter ? _Multiplier._Mydata : _Multiplicand._Mydata;
    const uint32_t* const _Rgu2       = _Multiplier_is_shorter ? _Multiplicand._Mydata : _Multiplier._Mydata;

    const uint32_t _Cu1 = _Multiplier_is_shorter ? _Multiplier._Myused : _Multiplicand._Myused;
    const uint32_t _Cu2 = _Multiplier_is_shorter ? _Multiplicand._Myused : _Multiplier._Myused;

    _Big_integer_flt _Result{};
    for (uint32_t _Iu1 = 0; _Iu1 != _Cu1; ++_Iu1) {
        const uint32_t _U_cur = _Rgu1[_Iu1];
        if (_U_cur == 0) {
            if (_Iu1 == _Result._Myused) {
                _Result._Mydata[_Iu1] = 0;
                _Result._Myused       = _Iu1 + 1;
            }

            continue;
        }

        uint32_t _U_carry = 0;
        uint32_t _Iu_res  = _Iu1;
        for (uint32_t _Iu2 = 0; _Iu2 != _Cu2 && _Iu_res != _Big_integer_flt::_Element_count; ++_Iu2, ++_Iu_res) {
            if (_Iu_res == _Result._Myused) {
                _Result._Mydata[_Iu_res] = 0;
                _Result._Myused          = _Iu_res + 1;
            }

            _U_carry = _Add_multiply_carry(_Result._Mydata[_Iu_res], _U_cur, _Rgu2[_Iu2], _U_carry);
        }

        while (_U_carry != 0 && _Iu_res != _Big_integer_flt::_Element_count) {
            if (_Iu_res == _Result._Myused) {
                _Result._Mydata[_Iu_res] = 0;
                _Result._Myused          = _Iu_res + 1;
            }

            _U_carry = _Add_carry(_Result._Mydata[_Iu_res++], 0, _U_carry);
        }

        if (_Iu_res == _Big_integer_flt::_Element_count) {
            _Multiplicand._Myused = 0;
            return false;
        }
    }

    // Store the _Result in the _Multiplicand and compute the actual number of elements used:
    _Multiplicand = _Result;
    return true;
}

// Multiplies the high-precision integer _Xval by 10^_Power. Returns true if the multiplication was successful;
// false if it overflowed. When overflow occurs, the high-precision integer is reset to zero.
_NODISCARD constexpr bool _Multiply_by_power_of_ten(_Big_integer_flt& _Xval, const uint32_t _Power) noexcept {
    // To improve performance, we use a table of precomputed powers of ten, from 10^10 through 10^380, in increments
    // of ten. In its unpacked form, as an array of _Big_integer_flt objects, this table consists mostly of zero
    // elements. Thus, we store the table in a packed form, trimming leading and trailing zero elements. We provide an
    // index that is used to unpack powers from the table, using the function that appears after this function in this
    // file.

    // The minimum value representable with double-precision is 5E-324.
    // With this table we can thus compute most multiplications with a single multiply.

    constexpr uint32_t _Large_power_data[] = {0x540be400, 0x00000002, 0x63100000, 0x6bc75e2d, 0x00000005,
        0x40000000, 0x4674edea, 0x9f2c9cd0, 0x0000000c, 0xb9f56100, 0x5ca4bfab, 0x6329f1c3, 0x0000001d, 0xb5640000,
        0xc40534fd, 0x926687d2, 0x6c3b15f9, 0x00000044, 0x10000000, 0x946590d9, 0xd762422c, 0x9a224501, 0x4f272617,
        0x0000009f, 0x07950240, 0x245689c1, 0xc5faa71c, 0x73c86d67, 0xebad6ddc, 0x00000172, 0xcec10000, 0x63a22764,
        0xefa418ca, 0xcdd17b25, 0x6bdfef70, 0x9dea3e1f, 0x0000035f, 0xe4000000, 0xcdc3fe6e, 0x66bc0c6a, 0x2e391f32,
        0x5a450203, 0x71d2f825, 0xc3c24a56, 0x000007da, 0xa82e8f10, 0xaab24308, 0x8e211a7c, 0xf38ace40, 0x84c4ce0b,
        0x7ceb0b27, 0xad2594c3, 0x00001249, 0xdd1a4000, 0xcc9f54da, 0xdc5961bf, 0xc75cabab, 0xf505440c, 0xd1bc1667,
        0xfbb7af52, 0x608f8d29, 0x00002a94, 0x21000000, 0x17bb8a0c, 0x56af8ea4, 0x06479fa9, 0x5d4bb236, 0x80dc5fe0,
        0xf0feaa0a, 0xa88ed940, 0x6b1a80d0, 0x00006323, 0x324c3864, 0x8357c796, 0xe44a42d5, 0xd9a92261, 0xbd3c103d,
        0x91e5f372, 0xc0591574, 0xec1da60d, 0x102ad96c, 0x0000e6d3, 0x1e851000, 0x6e4f615b, 0x187b2a69, 0x0450e21c,
        0x2fdd342b, 0x635027ee, 0xa6c97199, 0x8e4ae916, 0x17082e28, 0x1a496e6f, 0x0002196e, 0x32400000, 0x04ad4026,
        0xf91e7250, 0x2994d1d5, 0x665bcdbb, 0xa23b2e96, 0x65fa7ddb, 0x77de53ac, 0xb020a29b, 0xc6bff953, 0x4b9425ab,
        0x0004e34d, 0xfbc32d81, 0x5222d0f4, 0xb70f2850, 0x5713f2f3, 0xdc421413, 0xd6395d7d, 0xf8591999, 0x0092381c,
        0x86b314d6, 0x7aa577b9, 0x12b7fe61, 0x000b616a, 0x1d11e400, 0x56c3678d, 0x3a941f20, 0x9b09368b, 0xbd706908,
        0x207665be, 0x9b26c4eb, 0x1567e89d, 0x9d15096e, 0x7132f22b, 0xbe485113, 0x45e5a2ce, 0x001a7f52, 0xbb100000,
        0x02f79478, 0x8c1b74c0, 0xb0f05d00, 0xa9dbc675, 0xe2d9b914, 0x650f72df, 0x77284b4c, 0x6df6e016, 0x514391c2,
        0x2795c9cf, 0xd6e2ab55, 0x9ca8e627, 0x003db1a6, 0x40000000, 0xf4ecd04a, 0x7f2388f0, 0x580a6dc5, 0x43bf046f,
        0xf82d5dc3, 0xee110848, 0xfaa0591c, 0xcdf4f028, 0x192ea53f, 0xbcd671a0, 0x7d694487, 0x10f96e01, 0x791a569d,
        0x008fa475, 0xb9b2e100, 0x8288753c, 0xcd3f1693, 0x89b43a6b, 0x089e87de, 0x684d4546, 0xfddba60c, 0xdf249391,
        0x3068ec13, 0x99b44427, 0xb68141ee, 0x5802cac3, 0xd96851f1, 0x7d7625a2, 0x014e718d, 0xfb640000, 0xf25a83e6,
        0x9457ad0f, 0x0080b511, 0x2029b566, 0xd7c5d2cf, 0xa53f6d7d, 0xcdb74d1c, 0xda9d70de, 0xb716413d, 0x71d0ca4e,
        0xd7e41398, 0x4f403a90, 0xf9ab3fe2, 0x264d776f, 0x030aafe6, 0x10000000, 0x09ab5531, 0xa60c58d2, 0x566126cb,
        0x6a1c8387, 0x7587f4c1, 0x2c44e876, 0x41a047cf, 0xc908059e, 0xa0ba063e, 0xe7cfc8e8, 0xe1fac055, 0xef0144b2,
        0x24207eb0, 0xd1722573, 0xe4b8f981, 0x071505ae, 0x7a3b6240, 0xcea45d4f, 0x4fe24133, 0x210f6d6d, 0xe55633f2,
        0x25c11356, 0x28ebd797, 0xd396eb84, 0x1e493b77, 0x471f2dae, 0x96ad3820, 0x8afaced1, 0x4edecddb, 0x5568c086,
        0xb2695da1, 0x24123c89, 0x107d4571, 0x1c410000, 0x6e174a27, 0xec62ae57, 0xef2289aa, 0xb6a2fbdd, 0x17e1efe4,
        0x3366bdf2, 0x37b48880, 0xbfb82c3e, 0x19acde91, 0xd4f46408, 0x35ff6a4e, 0x67566a0e, 0x40dbb914, 0x782a3bca,
        0x6b329b68, 0xf5afc5d9, 0x266469bc, 0xe4000000, 0xfb805ff4, 0xed55d1af, 0x9b4a20a8, 0xab9757f8, 0x01aefe0a,
        0x4a2ca67b, 0x1ebf9569, 0xc7c41c29, 0xd8d5d2aa, 0xd136c776, 0x93da550c, 0x9ac79d90, 0x254bcba8, 0x0df07618,
        0xf7a88809, 0x3a1f1074, 0xe54811fc, 0x59638ead, 0x97cbe710, 0x26d769e8, 0xb4e4723e, 0x5b90aa86, 0x9c333922,
        0x4b7a0775, 0x2d47e991, 0x9a6ef977, 0x160b40e7, 0x0c92f8c4, 0xf25ff010, 0x25c36c11, 0xc9f98b42, 0x730b919d,
        0x05ff7caf, 0xb0432d85, 0x2d2b7569, 0xa657842c, 0xd01fef10, 0xc77a4000, 0xe8b862e5, 0x10d8886a, 0xc8cd98e5,
        0x108955c5, 0xd059b655, 0x58fbbed4, 0x03b88231, 0x034c4519, 0x194dc939, 0x1fc500ac, 0x794cc0e2, 0x3bc980a1,
        0xe9b12dd1, 0x5e6d22f8, 0x7b38899a, 0xce7919d8, 0x78c67672, 0x79e5b99f, 0xe494034e, 0x00000001, 0xa1000000,
        0x6c5cd4e9, 0x9be47d6f, 0xf93bd9e7, 0x77626fa1, 0xc68b3451, 0xde2b59e8, 0xcf3cde58, 0x2246ff58, 0xa8577c15,
        0x26e77559, 0x17776753, 0xebe6b763, 0xe3fd0a5f, 0x33e83969, 0xa805a035, 0xf631b987, 0x211f0f43, 0xd85a43db,
        0xab1bf596, 0x683f19a2, 0x00000004, 0xbe7dfe64, 0x4bc9042f, 0xe1f5edb0, 0x8fa14eda, 0xe409db73, 0x674fee9c,
        0xa9159f0d, 0xf6b5b5d6, 0x7338960e, 0xeb49c291, 0x5f2b97cc, 0x0f383f95, 0x2091b3f6, 0xd1783714, 0xc1d142df,
        0x153e22de, 0x8aafdf57, 0x77f5e55f, 0xa3e7ca8b, 0x032f525b, 0x42e74f3d, 0x0000000a, 0xf4dd1000, 0x5d450952,
        0xaeb442e1, 0xa3b3342e, 0x3fcda36f, 0xb4287a6e, 0x4bc177f7, 0x67d2c8d0, 0xaea8f8e0, 0xadc93b67, 0x6cc856b3,
        0x959d9d0b, 0x5b48c100, 0x4abe8a3d, 0x52d936f4, 0x71dbe84d, 0xf91c21c5, 0x4a458109, 0xd7aad86a, 0x08e14c7c,
        0x759ba59c, 0xe43c8800, 0x00000017, 0x92400000, 0x04f110d4, 0x186472be, 0x8736c10c, 0x1478abfb, 0xfc51af29,
        0x25eb9739, 0x4c2b3015, 0xa1030e0b, 0x28fe3c3b, 0x7788fcba, 0xb89e4358, 0x733de4a4, 0x7c46f2c2, 0x8f746298,
        0xdb19210f, 0x2ea3b6ae, 0xaa5014b2, 0xea39ab8d, 0x97963442, 0x01dfdfa9, 0xd2f3d3fe, 0xa0790280, 0x00000037,
        0x509c9b01, 0xc7dcadf1, 0x383dad2c, 0x73c64d37, 0xea6d67d0, 0x519ba806, 0xc403f2f8, 0xa052e1a2, 0xd710233a,
        0x448573a9, 0xcf12d9ba, 0x70871803, 0x52dc3a9b, 0xe5b252e8, 0x0717fb4e, 0xbe4da62f, 0x0aabd7e1, 0x8c62ed4f,
        0xceb9ec7b, 0xd4664021, 0xa1158300, 0xcce375e6, 0x842f29f2, 0x00000081, 0x7717e400, 0xd3f5fb64, 0xa0763d71,
        0x7d142fe9, 0x33f44c66, 0xf3b8f12e, 0x130f0d8e, 0x734c9469, 0x60260fa8, 0x3c011340, 0xcc71880a, 0x37a52d21,
        0x8adac9ef, 0x42bb31b4, 0xd6f94c41, 0xc88b056c, 0xe20501b8, 0x5297ed7c, 0x62c361c4, 0x87dad8aa, 0xb833eade,
        0x94f06861, 0x13cc9abd, 0x8dc1d56a, 0x0000012d, 0x13100000, 0xc67a36e8, 0xf416299e, 0xf3493f0a, 0x77a5a6cf,
        0xa4be23a3, 0xcca25b82, 0x3510722f, 0xbe9d447f, 0xa8c213b8, 0xc94c324e, 0xbc9e33ad, 0x76acfeba, 0x2e4c2132,
        0x3e13cd32, 0x70fe91b4, 0xbb5cd936, 0x42149785, 0x46cc1afd, 0xe638ddf8, 0x690787d2, 0x1a02d117, 0x3eb5f1fe,
        0xc3b9abae, 0x1c08ee6f, 0x000002be, 0x40000000, 0x8140c2aa, 0x2cf877d9, 0x71e1d73d, 0xd5e72f98, 0x72516309,
        0xafa819dd, 0xd62a5a46, 0x2a02dcce, 0xce46ddfe, 0x2713248d, 0xb723d2ad, 0xc404bb19, 0xb706cc2b, 0x47b1ebca,
        0x9d094bdc, 0xc5dc02ca, 0x31e6518e, 0x8ec35680, 0x342f58a8, 0x8b041e42, 0xfebfe514, 0x05fffc13, 0x6763790f,
        0x66d536fd, 0xb9e15076, 0x00000662, 0x67b06100, 0xd2010a1a, 0xd005e1c0, 0xdb12733b, 0xa39f2e3f, 0x61b29de2,
        0x2a63dce2, 0x942604bc, 0x6170d59b, 0xc2e32596, 0x140b75b9, 0x1f1d2c21, 0xb8136a60, 0x89d23ba2, 0x60f17d73,
        0xc6cad7df, 0x0669df2b, 0x24b88737, 0x669306ed, 0x19496eeb, 0x938ddb6f, 0x5e748275, 0xc56e9a36, 0x3690b731,
        0xc82842c5, 0x24ae798e, 0x00000ede, 0x41640000, 0xd5889ac1, 0xd9432c99, 0xa280e71a, 0x6bf63d2e, 0x8249793d,
        0x79e7a943, 0x22fde64a, 0xe0d6709a, 0x05cacfef, 0xbd8da4d7, 0xe364006c, 0xa54edcb3, 0xa1a8086e, 0x748f459e,
        0xfc8e54c8, 0xcc74c657, 0x42b8c3d4, 0x57d9636e, 0x35b55bcc, 0x6c13fee9, 0x1ac45161, 0xb595badb, 0xa1f14e9d,
        0xdcf9e750, 0x07637f71, 0xde2f9f2b, 0x0000229d, 0x10000000, 0x3c5ebd89, 0xe3773756, 0x3dcba338, 0x81d29e4f,
        0xa4f79e2c, 0xc3f9c774, 0x6a1ce797, 0xac5fe438, 0x07f38b9c, 0xd588ecfa, 0x3e5ac1ac, 0x85afccce, 0x9d1f3f70,
        0xe82d6dd3, 0x177d180c, 0x5e69946f, 0x648e2ce1, 0x95a13948, 0x340fe011, 0xb4173c58, 0x2748f694, 0x7c2657bd,
        0x758bda2e, 0x3b8090a0, 0x2ddbb613, 0x6dcf4890, 0x24e4047e, 0x00005099};

    struct _Unpack_index {
        uint16_t _Offset; // The offset of this power's initial element in the array
        uint8_t _Zeroes; // The number of omitted leading zero elements
        uint8_t _Size; // The number of elements present for this power
    };

    constexpr _Unpack_index _Large_power_indices[] = {{0, 0, 2}, {2, 0, 3}, {5, 0, 4}, {9, 1, 4}, {13, 1, 5},
        {18, 1, 6}, {24, 2, 6}, {30, 2, 7}, {37, 2, 8}, {45, 3, 8}, {53, 3, 9}, {62, 3, 10}, {72, 4, 10}, {82, 4, 11},
        {93, 4, 12}, {105, 5, 12}, {117, 5, 13}, {130, 5, 14}, {144, 5, 15}, {159, 6, 15}, {174, 6, 16}, {190, 6, 17},
        {207, 7, 17}, {224, 7, 18}, {242, 7, 19}, {261, 8, 19}, {280, 8, 21}, {301, 8, 22}, {323, 9, 22}, {345, 9, 23},
        {368, 9, 24}, {392, 10, 24}, {416, 10, 25}, {441, 10, 26}, {467, 10, 27}, {494, 11, 27}, {521, 11, 28},
        {549, 11, 29}};

    for (uint32_t _Large_power = _Power / 10; _Large_power != 0;) {
        const uint32_t _Current_power = std::min(_Large_power, static_cast<uint32_t>(std::size(_Large_power_indices)));

        const _Unpack_index& _Index = _Large_power_indices[_Current_power - 1];
        _Big_integer_flt _Multiplier{};
        _Multiplier._Myused = static_cast<uint32_t>(_Index._Size + _Index._Zeroes);

        const uint32_t* const _Source = _Large_power_data + _Index._Offset;

        //[neargye] constexpr fill uint32_t. P1944 fix this?
        third_party::trivial_fill(_Multiplier._Mydata, (uint32_t)0, _Index._Zeroes * sizeof(uint32_t));
        //[neargye] constexpr copy uint32_t. P1944 fix this?
        third_party::trivial_copy(_Multiplier._Mydata + _Index._Zeroes, _Source, _Index._Size * sizeof(uint32_t));

        if (!_Multiply(_Xval, _Multiplier)) { // when overflow occurs, resets to zero
            return false;
        }

        _Large_power -= _Current_power;
    }

    constexpr uint32_t _Small_powers_of_ten[9] = {10, 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000, 1'000'000'000};

    const uint32_t _Small_power = _Power % 10;

    if (_Small_power == 0) {
        return true;
    }

    return _Multiply(_Xval, _Small_powers_of_ten[_Small_power - 1]); // when overflow occurs, resets to zero
}


// Computes the number of zeroes higher than the most significant set bit in _Ux
_NODISCARD constexpr uint32_t _Count_sequential_high_zeroes(const uint32_t _Ux) noexcept {
    unsigned long _Index;
    return third_party::bit_scan_reverse(&_Index, _Ux) ? 31 - _Index : 32;
}

// This high-precision integer division implementation was translated from the implementation of
// System.Numerics.BigIntegerBuilder.ModDivCore in the .NET Framework sources.
// It computes both quotient and remainder: the remainder is stored in the _Numerator argument,
// and the least significant 64 bits of the quotient are returned from the function.
_NODISCARD constexpr uint64_t _Divide(_Big_integer_flt& _Numerator, const _Big_integer_flt& _Denominator) noexcept {
    // If the _Numerator is zero, then both the quotient and remainder are zero:
    if (_Numerator._Myused == 0) {
        return 0;
    }

    // If the _Denominator is zero, then uh oh. We can't divide by zero:
    nstd_assert(_Denominator._Myused != 0); // Division by zero

    uint32_t _Max_numerator_element_index         = _Numerator._Myused - 1;
    const uint32_t _Max_denominator_element_index = _Denominator._Myused - 1;

    // The _Numerator and _Denominator are both nonzero.
    // If the _Denominator is only one element wide, we can take the fast route:
    if (_Max_denominator_element_index == 0) {
        const uint32_t _Small_denominator = _Denominator._Mydata[0];

        if (_Max_numerator_element_index == 0) {
            const uint32_t _Small_numerator = _Numerator._Mydata[0];

            if (_Small_denominator == 1) {
                _Numerator._Myused = 0;
                return _Small_numerator;
            }

            _Numerator._Mydata[0] = _Small_numerator % _Small_denominator;
            _Numerator._Myused    = _Numerator._Mydata[0] > 0 ? 1u : 0u;
            return _Small_numerator / _Small_denominator;
        }

        if (_Small_denominator == 1) {
            uint64_t _Quotient = _Numerator._Mydata[1];
            _Quotient <<= 32;
            _Quotient |= _Numerator._Mydata[0];
            _Numerator._Myused = 0;
            return _Quotient;
        }

        // We count down in the next loop, so the last assignment to _Quotient will be the correct one.
        uint64_t _Quotient = 0;

        uint64_t _Uu = 0;
        for (uint32_t _Iv = _Max_numerator_element_index; _Iv != static_cast<uint32_t>(-1); --_Iv) {
            _Uu       = (_Uu << 32) | _Numerator._Mydata[_Iv];
            _Quotient = (_Quotient << 32) + static_cast<uint32_t>(_Uu / _Small_denominator);
            _Uu %= _Small_denominator;
        }

        _Numerator._Mydata[1] = static_cast<uint32_t>(_Uu >> 32);
        _Numerator._Mydata[0] = static_cast<uint32_t>(_Uu);
        _Numerator._Myused    = _Numerator._Mydata[1] > 0 ? 2u : 1u;
        return _Quotient;
    }

    if (_Max_denominator_element_index > _Max_numerator_element_index) {
        return 0;
    }

    const uint32_t _Cu_den = _Max_denominator_element_index + 1;
    const int32_t _Cu_diff = static_cast<int32_t>(_Max_numerator_element_index - _Max_denominator_element_index);

    // Determine whether the result will have _Cu_diff or _Cu_diff + 1 digits:
    int32_t _Cu_quo = _Cu_diff;
    for (int32_t _Iu = static_cast<int32_t>(_Max_numerator_element_index);; --_Iu) {
        if (_Iu < _Cu_diff) {
            ++_Cu_quo;
            break;
        }

        if (_Denominator._Mydata[_Iu - _Cu_diff] != _Numerator._Mydata[_Iu]) {
            if (_Denominator._Mydata[_Iu - _Cu_diff] < _Numerator._Mydata[_Iu]) {
                ++_Cu_quo;
            }

            break;
        }
    }

    if (_Cu_quo == 0) {
        return 0;
    }

    // Get the uint to use for the trial divisions. We normalize so the high bit is set:
    uint32_t _U_den      = _Denominator._Mydata[_Cu_den - 1];
    uint32_t _U_den_next = _Denominator._Mydata[_Cu_den - 2];

    const uint32_t _Cbit_shift_left  = _Count_sequential_high_zeroes(_U_den);
    const uint32_t _Cbit_shift_right = 32 - _Cbit_shift_left;
    if (_Cbit_shift_left > 0) {
        _U_den = (_U_den << _Cbit_shift_left) | (_U_den_next >> _Cbit_shift_right);
        _U_den_next <<= _Cbit_shift_left;

        if (_Cu_den > 2) {
            _U_den_next |= _Denominator._Mydata[_Cu_den - 3] >> _Cbit_shift_right;
        }
    }

    uint64_t _Quotient = 0;
    for (int32_t _Iu = _Cu_quo; --_Iu >= 0;) {
        // Get the high (normalized) bits of the _Numerator:
        const uint32_t _U_num_hi =
            (_Iu + _Cu_den <= _Max_numerator_element_index) ? _Numerator._Mydata[_Iu + _Cu_den] : 0;

        uint64_t _Uu_num =
            (static_cast<uint64_t>(_U_num_hi) << 32) | static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Cu_den - 1]);

        uint32_t _U_num_next = _Numerator._Mydata[_Iu + _Cu_den - 2];
        if (_Cbit_shift_left > 0) {
            _Uu_num = (_Uu_num << _Cbit_shift_left) | (_U_num_next >> _Cbit_shift_right);
            _U_num_next <<= _Cbit_shift_left;

            if (_Iu + _Cu_den >= 3) {
                _U_num_next |= _Numerator._Mydata[_Iu + _Cu_den - 3] >> _Cbit_shift_right;
            }
        }

        // Divide to get the quotient digit:
        uint64_t _Uu_quo = _Uu_num / _U_den;
        uint64_t _Uu_rem = static_cast<uint32_t>(_Uu_num % _U_den);

        if (_Uu_quo > UINT32_MAX) {
            _Uu_rem += _U_den * (_Uu_quo - UINT32_MAX);
            _Uu_quo = UINT32_MAX;
        }

        while (_Uu_rem <= UINT32_MAX && _Uu_quo * _U_den_next > ((_Uu_rem << 32) | _U_num_next)) {
            --_Uu_quo;
            _Uu_rem += _U_den;
        }

        // Multiply and subtract. Note that _Uu_quo may be one too large.
        // If we have a borrow at the end, we'll add the _Denominator back on and decrement _Uu_quo.
        if (_Uu_quo > 0) {
            uint64_t _Uu_borrow = 0;

            for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
                _Uu_borrow += _Uu_quo * _Denominator._Mydata[_Iu2];

                const uint32_t _U_sub = static_cast<uint32_t>(_Uu_borrow);
                _Uu_borrow >>= 32;
                if (_Numerator._Mydata[_Iu + _Iu2] < _U_sub) {
                    ++_Uu_borrow;
                }

                _Numerator._Mydata[_Iu + _Iu2] -= _U_sub;
            }

            if (_U_num_hi < _Uu_borrow) {
                // Add, tracking carry:
                uint32_t _U_carry = 0;
                for (uint32_t _Iu2 = 0; _Iu2 < _Cu_den; ++_Iu2) {
                    const uint64_t _Sum = static_cast<uint64_t>(_Numerator._Mydata[_Iu + _Iu2])
                                          + static_cast<uint64_t>(_Denominator._Mydata[_Iu2]) + _U_carry;

                    _Numerator._Mydata[_Iu + _Iu2] = static_cast<uint32_t>(_Sum);
                    _U_carry                       = static_cast<uint32_t>(_Sum >> 32);
                }

                --_Uu_quo;
            }

            _Max_numerator_element_index = _Iu + _Cu_den - 1;
        }

        _Quotient = (_Quotient << 32) + static_cast<uint32_t>(_Uu_quo);
    }

    // Trim the remainder:
    for (uint32_t _Ix = _Max_numerator_element_index + 1; _Ix < _Numerator._Myused; ++_Ix) {
        _Numerator._Mydata[_Ix] = 0;
    }

    uint32_t _Used = _Max_numerator_element_index + 1;

    while (_Used != 0 && _Numerator._Mydata[_Used - 1] == 0) {
        --_Used;
    }

    _Numerator._Myused = _Used;

    return _Quotient;
}

} // namespace nstd
