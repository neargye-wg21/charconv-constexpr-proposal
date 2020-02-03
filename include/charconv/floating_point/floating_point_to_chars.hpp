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

// * add constexpr modifiers to '_Floating_to_chars'

#pragma once

#include "charconv/detail/entity.hpp"
#include "charconv/detail/detail.hpp"

#include "floating_traits.hpp"
#include "bit_cast.hpp"
#include "ryu.hpp"

namespace nstd {

// FUNCTION to_chars (FLOATING-POINT TO STRING)
template <class _Floating>
_NODISCARD to_chars_result _Floating_to_chars_hex_precision(
    char* _First, char* const _Last, const _Floating _Value, int _Precision) noexcept {

    // * Determine the effective _Precision.
    // * Later, we'll decrement _Precision when printing each hexit after the decimal point.

    // The hexits after the decimal point correspond to the explicitly stored fraction bits.
    // float explicitly stores 23 fraction bits. 23 / 4 == 5.75, which is 6 hexits.
    // double explicitly stores 52 fraction bits. 52 / 4 == 13, which is 13 hexits.
    constexpr int _Full_precision         = is_same_v<_Floating, float> ? 6 : 13;
    constexpr int _Adjusted_explicit_bits = _Full_precision * 4;

    if (_Precision < 0) {
        // C11 7.21.6.1 "The fprintf function"/5: "A negative precision argument is taken as if the precision were
        // omitted." /8: "if the precision is missing and FLT_RADIX is a power of 2, then the precision is sufficient
        // for an exact representation of the value"
        _Precision = _Full_precision;
    }

    // * Extract the _Ieee_mantissa and _Ieee_exponent.
    using _Traits    = _Floating_type_traits<_Floating>;
    using _Uint_type = typename _Traits::_Uint_type;

    const _Uint_type _Uint_value    = _Bit_cast<_Uint_type>(_Value);
    const _Uint_type _Ieee_mantissa = _Uint_value & _Traits::_Denormal_mantissa_mask;
    const int32_t _Ieee_exponent    = static_cast<int32_t>(_Uint_value >> _Traits::_Exponent_shift);

    // * Prepare the _Adjusted_mantissa. This is aligned to hexit boundaries,
    // * with the implicit bit restored (0 for zero values and subnormal values, 1 for normal values).
    // * Also calculate the _Unbiased_exponent. This unifies the processing of zero, subnormal, and normal values.
    _Uint_type _Adjusted_mantissa;

    if constexpr (is_same_v<_Floating, float>) {
        _Adjusted_mantissa = _Ieee_mantissa << 1; // align to hexit boundary (23 isn't divisible by 4)
    } else {
        _Adjusted_mantissa = _Ieee_mantissa; // already aligned (52 is divisible by 4)
    }

    int32_t _Unbiased_exponent;

    if (_Ieee_exponent == 0) { // zero or subnormal
        // implicit bit is 0

        if (_Ieee_mantissa == 0) { // zero
            // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the exponent is zero."
            _Unbiased_exponent = 0;
        } else { // subnormal
            _Unbiased_exponent = 1 - _Traits::_Exponent_bias;
        }
    } else { // normal
        _Adjusted_mantissa |= _Uint_type{1} << _Adjusted_explicit_bits; // implicit bit is 1
        _Unbiased_exponent = _Ieee_exponent - _Traits::_Exponent_bias;
    }

    // _Unbiased_exponent is within [-126, 127] for float, [-1022, 1023] for double.

    // * Decompose _Unbiased_exponent into _Sign_character and _Absolute_exponent.
    char _Sign_character;
    uint32_t _Absolute_exponent;

    if (_Unbiased_exponent < 0) {
        _Sign_character    = '-';
        _Absolute_exponent = static_cast<uint32_t>(-_Unbiased_exponent);
    } else {
        _Sign_character    = '+';
        _Absolute_exponent = static_cast<uint32_t>(_Unbiased_exponent);
    }

    // _Absolute_exponent is within [0, 127] for float, [0, 1023] for double.

    // * Perform a single bounds check.
    {
        int32_t _Exponent_length;

        if (_Absolute_exponent < 10) {
            _Exponent_length = 1;
        } else if (_Absolute_exponent < 100) {
            _Exponent_length = 2;
        } else if constexpr (is_same_v<_Floating, float>) {
            _Exponent_length = 3;
        } else if (_Absolute_exponent < 1000) {
            _Exponent_length = 3;
        } else {
            _Exponent_length = 4;
        }

        // _Precision might be enormous; avoid integer overflow by testing it separately.
        ptrdiff_t _Buffer_size = _Last - _First;

        if (_Buffer_size < _Precision) {
            return {_Last, errc::value_too_large};
        }

        _Buffer_size -= _Precision;

        const int32_t _Length_excluding_precision = 1 // leading hexit
                                                    + static_cast<int32_t>(_Precision > 0) // possible decimal point
                                                    // excluding `+ _Precision`, hexits after decimal point
                                                    + 2 // "p+" or "p-"
                                                    + _Exponent_length; // exponent

        if (_Buffer_size < _Length_excluding_precision) {
            return {_Last, errc::value_too_large};
        }
    }

    // * Perform rounding when we've been asked to omit hexits.
    if (_Precision < _Full_precision) {
        // _Precision is within [0, 5] for float, [0, 12] for double.

        // _Dropped_bits is within [4, 24] for float, [4, 52] for double.
        const int _Dropped_bits = (_Full_precision - _Precision) * 4;

        // Perform rounding by adding an appropriately-shifted bit.

        // This can propagate carries all the way into the leading hexit. Examples:
        // "0.ff9" rounded to a precision of 2 is "1.00".
        // "1.ff9" rounded to a precision of 2 is "2.00".

        // Note that the leading hexit participates in the rounding decision. Examples:
        // "0.8" rounded to a precision of 0 is "0".
        // "1.8" rounded to a precision of 0 is "2".

        // Reference implementation with suboptimal codegen:
        // const bool _Lsb_bit       = (_Adjusted_mantissa & (_Uint_type{1} << _Dropped_bits)) != 0;
        // const bool _Round_bit     = (_Adjusted_mantissa & (_Uint_type{1} << (_Dropped_bits - 1))) != 0;
        // const bool _Has_tail_bits = (_Adjusted_mantissa & ((_Uint_type{1} << (_Dropped_bits - 1)) - 1)) != 0;
        // const bool _Should_round = _Should_round_up(_Lsb_bit, _Round_bit, _Has_tail_bits);
        // _Adjusted_mantissa += _Uint_type{_Should_round} << _Dropped_bits;

        // Example for optimized implementation: Let _Dropped_bits be 8.
        //          Bit index: ...[8]76543210
        // _Adjusted_mantissa: ...[L]RTTTTTTT (not depicting known details, like hexit alignment)
        // By focusing on the bit at index _Dropped_bits, we can avoid unnecessary branching and shifting.

        // Bit index: ...[8]76543210
        //  _Lsb_bit: ...[L]RTTTTTTT
        const _Uint_type _Lsb_bit = _Adjusted_mantissa;

        //  Bit index: ...9[8]76543210
        // _Round_bit: ...L[R]TTTTTTT0
        const _Uint_type _Round_bit = _Adjusted_mantissa << 1;

        // We can detect (without branching) whether any of the trailing bits are set.
        // Due to _Should_round below, this computation will be used if and only if R is 1, so we can assume that here.
        //      Bit index: ...9[8]76543210
        //     _Round_bit: ...L[1]TTTTTTT0
        // _Has_tail_bits: ....[H]........

        // If all of the trailing bits T are 0, then `_Round_bit - 1` will produce 0 for H (due to R being 1).
        // If any of the trailing bits T are 1, then `_Round_bit - 1` will produce 1 for H (due to R being 1).
        const _Uint_type _Has_tail_bits = _Round_bit - 1;

        // Finally, we can use _Should_round_up() logic with bitwise-AND and bitwise-OR,
        // selecting just the bit at index _Dropped_bits. This is the appropriately-shifted bit that we want.
        const _Uint_type _Should_round = _Round_bit & (_Has_tail_bits | _Lsb_bit) & (_Uint_type{1} << _Dropped_bits);

        // This rounding technique is dedicated to the memory of Peppermint. =^..^=
        _Adjusted_mantissa += _Should_round;
    }

    // * Print the leading hexit, then mask it away.
    {
        const uint32_t _Nibble = static_cast<uint32_t>(_Adjusted_mantissa >> _Adjusted_explicit_bits);
        _STL_INTERNAL_CHECK(_Nibble < 3);
        const char _Leading_hexit = static_cast<char>('0' + _Nibble);

        *_First++ = _Leading_hexit;

        constexpr _Uint_type _Mask = (_Uint_type{1} << _Adjusted_explicit_bits) - 1;
        _Adjusted_mantissa &= _Mask;
    }

    // * Print the decimal point and trailing hexits.

    // C11 7.21.6.1 "The fprintf function"/8:
    // "if the precision is zero and the # flag is not specified, no decimal-point character appears."
    if (_Precision > 0) {
        *_First++ = '.';

        int32_t _Number_of_bits_remaining = _Adjusted_explicit_bits; // 24 for float, 52 for double

        for (;;) {
            _STL_INTERNAL_CHECK(_Number_of_bits_remaining >= 4);
            _STL_INTERNAL_CHECK(_Number_of_bits_remaining % 4 == 0);
            _Number_of_bits_remaining -= 4;

            const uint32_t _Nibble = static_cast<uint32_t>(_Adjusted_mantissa >> _Number_of_bits_remaining);
            _STL_INTERNAL_CHECK(_Nibble < 16);
            const char _Hexit = _Charconv_digits[_Nibble];

            *_First++ = _Hexit;

            // _Precision is the number of hexits that still need to be printed.
            --_Precision;
            if (_Precision == 0) {
                break; // We're completely done with this phase.
            }
            // Otherwise, we need to keep printing hexits.

            if (_Number_of_bits_remaining == 0) {
                // We've finished printing _Adjusted_mantissa, so all remaining hexits are '0'.
                _CSTD memset(_First, '0', static_cast<size_t>(_Precision));
                _First += _Precision;
                break;
            }

            // Mask away the hexit that we just printed, then keep looping.
            // (We skip this when breaking out of the loop above, because _Adjusted_mantissa isn't used later.)
            const _Uint_type _Mask = (_Uint_type{1} << _Number_of_bits_remaining) - 1;
            _Adjusted_mantissa &= _Mask;
        }
    }

    // * Print the exponent.

    // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at least one digit, and only as many more
    // digits as necessary to represent the decimal exponent of 2."

    // Performance note: We should take advantage of the known ranges of possible exponents.

    *_First++ = 'p';
    *_First++ = _Sign_character;

    // We've already printed '-' if necessary, so uint32_t _Absolute_exponent avoids testing that again.
    return _STD to_chars(_First, _Last, _Absolute_exponent);
}

template <class _Floating>
_NODISCARD to_chars_result _Floating_to_chars_hex_shortest(
    char* _First, char* const _Last, const _Floating _Value) noexcept {

    // This prints "1.728p+0" instead of "2.e5p-1".
    // This prints "0.000002p-126" instead of "1p-149" for float.
    // This prints "0.0000000000001p-1022" instead of "1p-1074" for double.
    // This prioritizes being consistent with printf's de facto behavior (and hex-precision's behavior)
    // over minimizing the number of characters printed.

    using _Traits    = _Floating_type_traits<_Floating>;
    using _Uint_type = typename _Traits::_Uint_type;

    const _Uint_type _Uint_value = _Bit_cast<_Uint_type>(_Value);

    if (_Uint_value == 0) { // zero detected; write "0p+0" and return
        // C11 7.21.6.1 "The fprintf function"/8: "If the value is zero, the exponent is zero."
        // Special-casing zero is necessary because of the exponent.
        const char* const _Str = "0p+0";
        const size_t _Len      = 4;

        if (_Last - _First < static_cast<ptrdiff_t>(_Len)) {
            return {_Last, errc::value_too_large};
        }

        _CSTD memcpy(_First, _Str, _Len);

        return {_First + _Len, errc{}};
    }

    const _Uint_type _Ieee_mantissa = _Uint_value & _Traits::_Denormal_mantissa_mask;
    const int32_t _Ieee_exponent    = static_cast<int32_t>(_Uint_value >> _Traits::_Exponent_shift);

    char _Leading_hexit; // implicit bit
    int32_t _Unbiased_exponent;

    if (_Ieee_exponent == 0) { // subnormal
        _Leading_hexit     = '0';
        _Unbiased_exponent = 1 - _Traits::_Exponent_bias;
    } else { // normal
        _Leading_hexit     = '1';
        _Unbiased_exponent = _Ieee_exponent - _Traits::_Exponent_bias;
    }

    // Performance note: Consider avoiding per-character bounds checking when there's plenty of space.

    if (_First == _Last) {
        return {_Last, errc::value_too_large};
    }

    *_First++ = _Leading_hexit;

    if (_Ieee_mantissa == 0) {
        // The fraction bits are all 0. Trim them away, including the decimal point.
    } else {
        if (_First == _Last) {
            return {_Last, errc::value_too_large};
        }

        *_First++ = '.';

        // The hexits after the decimal point correspond to the explicitly stored fraction bits.
        // float explicitly stores 23 fraction bits. 23 / 4 == 5.75, so we'll print at most 6 hexits.
        // double explicitly stores 52 fraction bits. 52 / 4 == 13, so we'll print at most 13 hexits.
        _Uint_type _Adjusted_mantissa;
        int32_t _Number_of_bits_remaining;

        if constexpr (is_same_v<_Floating, float>) {
            _Adjusted_mantissa        = _Ieee_mantissa << 1; // align to hexit boundary (23 isn't divisible by 4)
            _Number_of_bits_remaining = 24; // 23 fraction bits + 1 alignment bit
        } else {
            _Adjusted_mantissa        = _Ieee_mantissa; // already aligned (52 is divisible by 4)
            _Number_of_bits_remaining = 52; // 52 fraction bits
        }

        // do-while: The condition _Adjusted_mantissa != 0 is initially true - we have nonzero fraction bits and we've
        // printed the decimal point. Each iteration, we print a hexit, mask it away, and keep looping if we still have
        // nonzero fraction bits. If there would be trailing '0' hexits, this trims them. If there wouldn't be trailing
        // '0' hexits, the same condition works (as we print the final hexit and mask it away); we don't need to test
        // _Number_of_bits_remaining.
        do {
            _STL_INTERNAL_CHECK(_Number_of_bits_remaining >= 4);
            _STL_INTERNAL_CHECK(_Number_of_bits_remaining % 4 == 0);
            _Number_of_bits_remaining -= 4;

            const uint32_t _Nibble = static_cast<uint32_t>(_Adjusted_mantissa >> _Number_of_bits_remaining);
            _STL_INTERNAL_CHECK(_Nibble < 16);
            const char _Hexit = _Charconv_digits[_Nibble];

            if (_First == _Last) {
                return {_Last, errc::value_too_large};
            }

            *_First++ = _Hexit;

            const _Uint_type _Mask = (_Uint_type{1} << _Number_of_bits_remaining) - 1;
            _Adjusted_mantissa &= _Mask;

        } while (_Adjusted_mantissa != 0);
    }

    // C11 7.21.6.1 "The fprintf function"/8: "The exponent always contains at least one digit, and only as many more
    // digits as necessary to represent the decimal exponent of 2."

    // Performance note: We should take advantage of the known ranges of possible exponents.

    // float: _Unbiased_exponent is within [-126, 127].
    // double: _Unbiased_exponent is within [-1022, 1023].

    if (_Last - _First < 2) {
        return {_Last, errc::value_too_large};
    }

    *_First++ = 'p';

    if (_Unbiased_exponent < 0) {
        *_First++          = '-';
        _Unbiased_exponent = -_Unbiased_exponent;
    } else {
        *_First++ = '+';
    }

    // We've already printed '-' if necessary, so static_cast<uint32_t> avoids testing that again.
    return _STD to_chars(_First, _Last, static_cast<uint32_t>(_Unbiased_exponent));
}

enum class _Floating_to_chars_overload { _Plain, _Format_only, _Format_precision };

template <_Floating_to_chars_overload _Overload, class _Floating>
_NODISCARD constexpr to_chars_result _Floating_to_chars(
    char* _First, char* const _Last, _Floating _Value, const chars_format _Fmt, const int _Precision) noexcept {
    _Adl_verify_range(_First, _Last);

    if constexpr (_Overload == _Floating_to_chars_overload::_Plain) {
        _STL_INTERNAL_CHECK(_Fmt == chars_format{}); // plain overload must pass chars_format{} internally
    } else {
        _STL_ASSERT(_Fmt == chars_format::general || _Fmt == chars_format::scientific || _Fmt == chars_format::fixed
                        || _Fmt == chars_format::hex,
            "invalid format in to_chars()");
    }

    using _Traits    = _Floating_type_traits<_Floating>;
    using _Uint_type = typename _Traits::_Uint_type;

    _Uint_type _Uint_value = _Bit_cast<_Uint_type>(_Value);

    const bool _Was_negative = (_Uint_value & _Traits::_Shifted_sign_mask) != 0;

    if (_Was_negative) { // sign bit detected; write minus sign and clear sign bit
        if (_First == _Last) {
            return {_Last, errc::value_too_large};
        }

        *_First++ = '-';

        _Uint_value &= ~_Traits::_Shifted_sign_mask;
        _Value = _Bit_cast<_Floating>(_Uint_value);
    }

    if ((_Uint_value & _Traits::_Shifted_exponent_mask) == _Traits::_Shifted_exponent_mask) {
        // inf/nan detected; write appropriate string and return
        const char* _Str;
        size_t _Len;

        const _Uint_type _Mantissa = _Uint_value & _Traits::_Denormal_mantissa_mask;

        if (_Mantissa == 0) {
            _Str = "inf";
            _Len = 3;
        } else if (_Was_negative && _Mantissa == _Traits::_Special_nan_mantissa_mask) {
            // When a NaN value has the sign bit set, the quiet bit set, and all other mantissa bits cleared,
            // the UCRT interprets it to mean "indeterminate", and indicates this by printing "-nan(ind)".
            _Str = "nan(ind)";
            _Len = 8;
        } else if ((_Mantissa & _Traits::_Special_nan_mantissa_mask) != 0) {
            _Str = "nan";
            _Len = 3;
        } else {
            _Str = "nan(snan)";
            _Len = 9;
        }

        if (_Last - _First < static_cast<ptrdiff_t>(_Len)) {
            return {_Last, errc::value_too_large};
        }

        _CSTD memcpy(_First, _Str, _Len);

        return {_First + _Len, errc{}};
    }

    if constexpr (_Overload == _Floating_to_chars_overload::_Plain) {
        (void) _Fmt;
        (void) _Precision;

        return _Floating_to_chars_ryu(_First, _Last, _Value, chars_format{});
    } else if constexpr (_Overload == _Floating_to_chars_overload::_Format_only) {
        (void) _Precision;

        if (_Fmt == chars_format::hex) {
            return _Floating_to_chars_hex_shortest(_First, _Last, _Value);
        }

        return _Floating_to_chars_ryu(_First, _Last, _Value, _Fmt);
    } else if constexpr (_Overload == _Floating_to_chars_overload::_Format_precision) {
        switch (_Fmt) {
        case chars_format::scientific:
            return _Floating_to_chars_scientific_precision(_First, _Last, _Value, _Precision);
        case chars_format::fixed:
            return _Floating_to_chars_fixed_precision(_First, _Last, _Value, _Precision);
        case chars_format::general:
            return _Floating_to_chars_general_precision(_First, _Last, _Value, _Precision);
        case chars_format::hex:
        default: // avoid warning C4715: not all control paths return a value
            return _Floating_to_chars_hex_precision(_First, _Last, _Value, _Precision);
        }
    }
}

} // namespace nstd
