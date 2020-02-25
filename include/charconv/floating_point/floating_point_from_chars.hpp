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

// * add constexpr modifiers to '_Floating_from_chars'
// * change '_Bit_cast' to 'third_party::bit_cast'

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "charconv/detail/entity.hpp"
#include "charconv/detail/detail.hpp"

#include "floating_traits.hpp"
#include "big_integer.hpp"

#include "third_party/bits.hpp"
#include "third_party/constexpr_utility.hpp"

#pragma once

namespace nstd {

struct _Floating_point_string {
    bool _Myis_negative;
    int32_t _Myexponent;
    uint32_t _Mymantissa_count;
    uint8_t _Mymantissa[768];
};

// Stores a positive or negative zero into the _Result object
template <class _FloatingType>
void constexpr _Assemble_floating_point_zero(const bool _Is_negative, _FloatingType& _Result) noexcept {
    using _Floating_traits = _Floating_type_traits<_FloatingType>;
    using _Uint_type       = typename _Floating_traits::_Uint_type;

    _Uint_type _Sign_component = _Is_negative;
    _Sign_component <<= _Floating_traits::_Sign_shift;

    _Result = third_party::bit_cast<_FloatingType>(_Sign_component);
}

// Stores a positive or negative infinity into the _Result object
template <class _FloatingType>
void constexpr _Assemble_floating_point_infinity(const bool _Is_negative, _FloatingType& _Result) noexcept {
    using _Floating_traits = _Floating_type_traits<_FloatingType>;
    using _Uint_type       = typename _Floating_traits::_Uint_type;

    _Uint_type _Sign_component = _Is_negative;
    _Sign_component <<= _Floating_traits::_Sign_shift;

    const _Uint_type _Exponent_component = _Floating_traits::_Shifted_exponent_mask;

    _Result = third_party::bit_cast<_FloatingType>(_Sign_component | _Exponent_component);
}

// Determines whether a mantissa should be rounded up according to round_to_nearest given [1] the value of the least
// significant bit of the mantissa, [2] the value of the next bit after the least significant bit (the "round" bit)
// and [3] whether any trailing bits after the round bit are set.

// The mantissa is treated as an unsigned integer magnitude.

// For this function, "round up" is defined as "increase the magnitude" of the mantissa. (Note that this means that
// if we need to round a negative value to the next largest representable value, we return false, because the next
// largest representable value has a smaller magnitude.)
_NODISCARD constexpr bool _Should_round_up(
    const bool _Lsb_bit, const bool _Round_bit, const bool _Has_tail_bits) noexcept {
    // If there are no insignificant set bits, the value is exactly-representable and should not be rounded.
    // We could detect this with:
    // const bool _Is_exactly_representable = !_Round_bit && !_Has_tail_bits;
    // if (_Is_exactly_representable) { return false; }
    // However, this is unnecessary given the logic below.

    // If there are insignificant set bits, we need to round according to round_to_nearest.
    // We need to handle two cases: we round up if either [1] the value is slightly greater
    // than the midpoint between two exactly-representable values or [2] the value is exactly the midpoint
    // between two exactly-representable values and the greater of the two is even (this is "round-to-even").
    return _Round_bit && (_Has_tail_bits || _Lsb_bit);
}

// Computes _Value / 2^_Shift, then rounds the result according to round_to_nearest.
// By the time we call this function, we will already have discarded most digits.
// The caller must pass true for _Has_zero_tail if all discarded bits were zeroes.
_NODISCARD constexpr uint64_t _Right_shift_with_rounding(
    const uint64_t _Value, const uint32_t _Shift, const bool _Has_zero_tail) noexcept {
    // If we'd need to shift further than it is possible to shift, the answer is always zero:
    if (_Shift >= 64) {
        return 0;
    }

    const uint64_t _Extra_bits_mask = (1ULL << (_Shift - 1)) - 1;
    const uint64_t _Round_bit_mask  = (1ULL << (_Shift - 1));
    const uint64_t _Lsb_bit_mask    = 1ULL << _Shift;

    const bool _Lsb_bit   = (_Value & _Lsb_bit_mask) != 0;
    const bool _Round_bit = (_Value & _Round_bit_mask) != 0;
    const bool _Tail_bits = !_Has_zero_tail || (_Value & _Extra_bits_mask) != 0;

    return (_Value >> _Shift) + _Should_round_up(_Lsb_bit, _Round_bit, _Tail_bits);
}

// Converts the floating-point value [sign] 0.mantissa * 2^exponent into the correct form for _FloatingType and
// stores the result into the _Result object. The caller must ensure that the mantissa and exponent are correctly
// computed such that either [1] the most significant bit of the mantissa is in the correct position for the
// _FloatingType, or [2] the exponent has been correctly adjusted to account for the shift of the mantissa that
// will be required.

// This function correctly handles range errors and stores a zero or infinity in the _Result object
// on underflow and overflow errors, respectively. This function correctly forms denormal numbers when required.

// If the provided mantissa has more bits of precision than can be stored in the _Result object, the mantissa is
// rounded to the available precision. Thus, if possible, the caller should provide a mantissa with at least one
// more bit of precision than is required, to ensure that the mantissa is correctly rounded.
// (The caller should not round the mantissa before calling this function.)
template <class _FloatingType>
_NODISCARD constexpr errc _Assemble_floating_point_value_t(const bool _Is_negative, const int32_t _Exponent,
    const typename _Floating_type_traits<_FloatingType>::_Uint_type _Mantissa, _FloatingType& _Result) noexcept {
    using _Floating_traits = _Floating_type_traits<_FloatingType>;
    using _Uint_type       = typename _Floating_traits::_Uint_type;

    _Uint_type _Sign_component = _Is_negative;
    _Sign_component <<= _Floating_traits::_Sign_shift;

    _Uint_type _Exponent_component = static_cast<uint32_t>(_Exponent + _Floating_traits::_Exponent_bias);
    _Exponent_component <<= _Floating_traits::_Exponent_shift;

    _Result = third_party::bit_cast<_FloatingType>(_Sign_component | _Exponent_component | _Mantissa);

    return errc{};
}

template <class _FloatingType>
_NODISCARD constexpr errc _Assemble_floating_point_value(const uint64_t _Initial_mantissa, const int32_t _Initial_exponent,
    const bool _Is_negative, const bool _Has_zero_tail, _FloatingType& _Result) noexcept {
    using _Traits = _Floating_type_traits<_FloatingType>;

    // Assume that the number is representable as a normal value.
    // Compute the number of bits by which we must adjust the mantissa to shift it into the correct position,
    // and compute the resulting base two exponent for the normalized mantissa:
    const uint32_t _Initial_mantissa_bits = _Bit_scan_reverse(_Initial_mantissa);
    const int32_t _Normal_mantissa_shift  = static_cast<int32_t>(_Traits::_Mantissa_bits - _Initial_mantissa_bits);
    const int32_t _Normal_exponent        = _Initial_exponent - _Normal_mantissa_shift;

    uint64_t _Mantissa = _Initial_mantissa;
    int32_t _Exponent  = _Normal_exponent;

    if (_Normal_exponent > _Traits::_Maximum_binary_exponent) {
        // The exponent is too large to be represented by the floating-point type; report the overflow condition:
        _Assemble_floating_point_infinity(_Is_negative, _Result);
        return errc::result_out_of_range; // Overflow example: "1e+1000"
    }

    if (_Normal_exponent < _Traits::_Minimum_binary_exponent) {
        // The exponent is too small to be represented by the floating-point type as a normal value, but it may be
        // representable as a denormal value. Compute the number of bits by which we need to shift the mantissa
        // in order to form a denormal number. (The subtraction of an extra 1 is to account for the hidden bit of
        // the mantissa that is not available for use when representing a denormal.)
        const int32_t _Denormal_mantissa_shift =
            _Normal_mantissa_shift + _Normal_exponent + _Traits::_Exponent_bias - 1;

        // Denormal values have an exponent of zero, so the debiased exponent is the negation of the exponent bias:
        _Exponent = -_Traits::_Exponent_bias;

        if (_Denormal_mantissa_shift < 0) {
            // Use two steps for right shifts: for a shift of N bits, we first shift by N-1 bits,
            // then shift the last bit and use its value to round the mantissa.
            _Mantissa =
                _Right_shift_with_rounding(_Mantissa, static_cast<uint32_t>(-_Denormal_mantissa_shift), _Has_zero_tail);

            // If the mantissa is now zero, we have underflowed:
            if (_Mantissa == 0) {
                _Assemble_floating_point_zero(_Is_negative, _Result);
                return errc::result_out_of_range; // Underflow example: "1e-1000"
            }

            // When we round the mantissa, the result may be so large that the number becomes a normal value.
            // For example, consider the single-precision case where the mantissa is 0x01ffffff and a right shift
            // of 2 is required to shift the value into position. We perform the shift in two steps: we shift by
            // one bit, then we shift again and round using the dropped bit. The initial shift yields 0x00ffffff.
            // The rounding shift then yields 0x007fffff and because the least significant bit was 1, we add 1
            // to this number to round it. The final result is 0x00800000.

            // 0x00800000 is 24 bits, which is more than the 23 bits available in the mantissa.
            // Thus, we have rounded our denormal number into a normal number.

            // We detect this case here and re-adjust the mantissa and exponent appropriately, to form a normal number:
            if (_Mantissa > _Traits::_Denormal_mantissa_mask) {
                // We add one to the _Denormal_mantissa_shift to account for the hidden mantissa bit
                // (we subtracted one to account for this bit when we computed the _Denormal_mantissa_shift above).
                _Exponent = _Initial_exponent - (_Denormal_mantissa_shift + 1) - _Normal_mantissa_shift;
            }
        } else {
            _Mantissa <<= _Denormal_mantissa_shift;
        }
    } else {
        if (_Normal_mantissa_shift < 0) {
            // Use two steps for right shifts: for a shift of N bits, we first shift by N-1 bits,
            // then shift the last bit and use its value to round the mantissa.
            _Mantissa =
                _Right_shift_with_rounding(_Mantissa, static_cast<uint32_t>(-_Normal_mantissa_shift), _Has_zero_tail);

            // When we round the mantissa, it may produce a result that is too large. In this case,
            // we divide the mantissa by two and increment the exponent (this does not change the value).
            if (_Mantissa > _Traits::_Normal_mantissa_mask) {
                _Mantissa >>= 1;
                ++_Exponent;

                // The increment of the exponent may have generated a value too large to be represented.
                // In this case, report the overflow:
                if (_Exponent > _Traits::_Maximum_binary_exponent) {
                    _Assemble_floating_point_infinity(_Is_negative, _Result);
                    return errc::result_out_of_range; // Overflow example: "1.ffffffp+127" for float
                                                      // Overflow example: "1.fffffffffffff8p+1023" for double
                }
            }
        } else if (_Normal_mantissa_shift > 0) {
            _Mantissa <<= _Normal_mantissa_shift;
        }
    }

    // Unset the hidden bit in the mantissa and assemble the floating-point value from the computed components:
    _Mantissa &= _Traits::_Denormal_mantissa_mask;

    using _Uint_type = typename _Traits::_Uint_type;

    return _Assemble_floating_point_value_t(_Is_negative, _Exponent, static_cast<_Uint_type>(_Mantissa), _Result);
}

// This function is part of the fast track for integer floating-point strings. It takes an integer and a sign and
// converts the value into its _FloatingType representation, storing the result in the _Result object. If the value
// is not representable, +/-infinity is stored and overflow is reported (since this function deals with only integers,
// underflow is impossible).
template <class _FloatingType>
_NODISCARD constexpr errc _Assemble_floating_point_value_from_big_integer_flt(const _Big_integer_flt& _Integer_value,
    const uint32_t _Integer_bits_of_precision, const bool _Is_negative, const bool _Has_nonzero_fractional_part,
    _FloatingType& _Result) noexcept {
    using _Traits = _Floating_type_traits<_FloatingType>;

    const int32_t _Base_exponent = _Traits::_Mantissa_bits - 1;

    // Very fast case: If we have 64 bits of precision or fewer,
    // we can just take the two low order elements from the _Big_integer_flt:
    if (_Integer_bits_of_precision <= 64) {
        const int32_t _Exponent = _Base_exponent;

        const uint32_t _Mantissa_low  = _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
        const uint32_t _Mantissa_high = _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
        const uint64_t _Mantissa      = _Mantissa_low + (static_cast<uint64_t>(_Mantissa_high) << 32);

        return _Assemble_floating_point_value(
            _Mantissa, _Exponent, _Is_negative, !_Has_nonzero_fractional_part, _Result);
    }

    const uint32_t _Top_element_bits  = _Integer_bits_of_precision % 32;
    const uint32_t _Top_element_index = _Integer_bits_of_precision / 32;

    const uint32_t _Middle_element_index = _Top_element_index - 1;
    const uint32_t _Bottom_element_index = _Top_element_index - 2;

    // Pretty fast case: If the top 64 bits occupy only two elements, we can just combine those two elements:
    if (_Top_element_bits == 0) {
        const int32_t _Exponent = static_cast<int32_t>(_Base_exponent + _Bottom_element_index * 32);

        const uint64_t _Mantissa = _Integer_value._Mydata[_Bottom_element_index]
                                   + (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index]) << 32);

        bool _Has_zero_tail = !_Has_nonzero_fractional_part;
        for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index; ++_Ix) {
            _Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
        }

        return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative, _Has_zero_tail, _Result);
    }

    // Not quite so fast case: The top 64 bits span three elements in the _Big_integer_flt. Assemble the three pieces:
    const uint32_t _Top_element_mask  = (1u << _Top_element_bits) - 1;
    const uint32_t _Top_element_shift = 64 - _Top_element_bits; // Left

    const uint32_t _Middle_element_shift = _Top_element_shift - 32; // Left

    const uint32_t _Bottom_element_bits  = 32 - _Top_element_bits;
    const uint32_t _Bottom_element_mask  = ~_Top_element_mask;
    const uint32_t _Bottom_element_shift = 32 - _Bottom_element_bits; // Right

    const int32_t _Exponent = static_cast<int32_t>(_Base_exponent + _Bottom_element_index * 32 + _Top_element_bits);

    const uint64_t _Mantissa =
        (static_cast<uint64_t>(_Integer_value._Mydata[_Top_element_index] & _Top_element_mask) << _Top_element_shift)
        + (static_cast<uint64_t>(_Integer_value._Mydata[_Middle_element_index]) << _Middle_element_shift)
        + (static_cast<uint64_t>(_Integer_value._Mydata[_Bottom_element_index] & _Bottom_element_mask)
            >> _Bottom_element_shift);

    bool _Has_zero_tail =
        !_Has_nonzero_fractional_part && (_Integer_value._Mydata[_Bottom_element_index] & _Top_element_mask) == 0;

    for (uint32_t _Ix = 0; _Has_zero_tail && _Ix != _Bottom_element_index; ++_Ix) {
        _Has_zero_tail = _Integer_value._Mydata[_Ix] == 0;
    }

    return _Assemble_floating_point_value(_Mantissa, _Exponent, _Is_negative, _Has_zero_tail, _Result);
}

// Accumulates the decimal digits in [_First_digit, _Last_digit) into the _Result high-precision integer.
// This function assumes that no overflow will occur.
constexpr void _Accumulate_decimal_digits_into_big_integer_flt(
    const uint8_t* const _First_digit, const uint8_t* const _Last_digit, _Big_integer_flt& _Result) noexcept {
    // We accumulate nine digit chunks, transforming the base ten string into base one billion on the fly,
    // allowing us to reduce the number of high-precision multiplication and addition operations by 8/9.
    uint32_t _Accumulator       = 0;
    uint32_t _Accumulator_count = 0;
    for (const uint8_t* _It = _First_digit; _It != _Last_digit; ++_It) {
        if (_Accumulator_count == 9) {
            [[maybe_unused]] const bool _Success1 = _Multiply(_Result, 1'000'000'000); // assumes no overflow
            _STL_INTERNAL_CHECK(_Success1);
            [[maybe_unused]] const bool _Success2 = _Add(_Result, _Accumulator); // assumes no overflow
            _STL_INTERNAL_CHECK(_Success2);

            _Accumulator       = 0;
            _Accumulator_count = 0;
        }

        _Accumulator *= 10;
        _Accumulator += *_It;
        ++_Accumulator_count;
    }

    if (_Accumulator_count != 0) {
        [[maybe_unused]] const bool _Success3 =
            _Multiply_by_power_of_ten(_Result, _Accumulator_count); // assumes no overflow
        _STL_INTERNAL_CHECK(_Success3);
        [[maybe_unused]] const bool _Success4 = _Add(_Result, _Accumulator); // assumes no overflow
        _STL_INTERNAL_CHECK(_Success4);
    }
}

// The core floating-point string parser for decimal strings. After a subject string is parsed and converted
// into a _Floating_point_string object, if the subject string was determined to be a decimal string,
// the object is passed to this function. This function converts the decimal real value to floating-point.
template <class _FloatingType>
_NODISCARD constexpr errc _Convert_decimal_string_to_floating_type(
    const _Floating_point_string& _Data, _FloatingType& _Result, bool _Has_zero_tail) noexcept {
    using _Traits = _Floating_type_traits<_FloatingType>;

    // To generate an N bit mantissa we require N + 1 bits of precision. The extra bit is used to correctly round
    // the mantissa (if there are fewer bits than this available, then that's totally okay;
    // in that case we use what we have and we don't need to round).
    const uint32_t _Required_bits_of_precision = static_cast<uint32_t>(_Traits::_Mantissa_bits + 1);

    // The input is of the form 0.mantissa * 10^exponent, where 'mantissa' are the decimal digits of the mantissa
    // and 'exponent' is the decimal exponent. We decompose the mantissa into two parts: an integer part and a
    // fractional part. If the exponent is positive, then the integer part consists of the first 'exponent' digits,
    // or all present digits if there are fewer digits. If the exponent is zero or negative, then the integer part
    // is empty. In either case, the remaining digits form the fractional part of the mantissa.
    const uint32_t _Positive_exponent      = static_cast<uint32_t>(std::max(0, _Data._Myexponent));
    const uint32_t _Integer_digits_present = std::min(_Positive_exponent, _Data._Mymantissa_count);
    const uint32_t _Integer_digits_missing = _Positive_exponent - _Integer_digits_present;
    const uint8_t* const _Integer_first    = _Data._Mymantissa;
    const uint8_t* const _Integer_last     = _Data._Mymantissa + _Integer_digits_present;

    const uint8_t* const _Fractional_first    = _Integer_last;
    const uint8_t* const _Fractional_last     = _Data._Mymantissa + _Data._Mymantissa_count;
    const uint32_t _Fractional_digits_present = static_cast<uint32_t>(_Fractional_last - _Fractional_first);

    // First, we accumulate the integer part of the mantissa into a _Big_integer_flt:
    _Big_integer_flt _Integer_value{};
    _Accumulate_decimal_digits_into_big_integer_flt(_Integer_first, _Integer_last, _Integer_value);

    if (_Integer_digits_missing > 0) {
        if (!_Multiply_by_power_of_ten(_Integer_value, _Integer_digits_missing)) {
            _Assemble_floating_point_infinity(_Data._Myis_negative, _Result);
            return errc::result_out_of_range; // Overflow example: "1e+2000"
        }
    }

    // At this point, the _Integer_value contains the value of the integer part of the mantissa. If either
    // [1] this number has more than the required number of bits of precision or
    // [2] the mantissa has no fractional part, then we can assemble the result immediately:
    const uint32_t _Integer_bits_of_precision = _Bit_scan_reverse(_Integer_value);
    {
        const bool _Has_zero_fractional_part = _Fractional_digits_present == 0 && _Has_zero_tail;

        if (_Integer_bits_of_precision >= _Required_bits_of_precision || _Has_zero_fractional_part) {
            return _Assemble_floating_point_value_from_big_integer_flt(
                _Integer_value, _Integer_bits_of_precision, _Data._Myis_negative, !_Has_zero_fractional_part, _Result);
        }
    }

    // Otherwise, we did not get enough bits of precision from the integer part, and the mantissa has a fractional
    // part. We parse the fractional part of the mantissa to obtain more bits of precision. To do this, we convert
    // the fractional part into an actual fraction N/M, where the numerator N is computed from the digits of the
    // fractional part, and the denominator M is computed as the power of 10 such that N/M is equal to the value
    // of the fractional part of the mantissa.
    _Big_integer_flt _Fractional_numerator{};
    _Accumulate_decimal_digits_into_big_integer_flt(_Fractional_first, _Fractional_last, _Fractional_numerator);

    const uint32_t _Fractional_denominator_exponent =
        _Data._Myexponent < 0 ? _Fractional_digits_present + static_cast<uint32_t>(-_Data._Myexponent)
                              : _Fractional_digits_present;

    _Big_integer_flt _Fractional_denominator = _Make_big_integer_flt_one();
    if (!_Multiply_by_power_of_ten(_Fractional_denominator, _Fractional_denominator_exponent)) {
        // If there were any digits in the integer part, it is impossible to underflow (because the exponent
        // cannot possibly be small enough), so if we underflow here it is a true underflow and we return zero.
        _Assemble_floating_point_zero(_Data._Myis_negative, _Result);
        return errc::result_out_of_range; // Underflow example: "1e-2000"
    }

    // Because we are using only the fractional part of the mantissa here, the numerator is guaranteed to be smaller
    // than the denominator. We normalize the fraction such that the most significant bit of the numerator is in the
    // same position as the most significant bit in the denominator. This ensures that when we later shift the
    // numerator N bits to the left, we will produce N bits of precision.
    const uint32_t _Fractional_numerator_bits   = _Bit_scan_reverse(_Fractional_numerator);
    const uint32_t _Fractional_denominator_bits = _Bit_scan_reverse(_Fractional_denominator);

    const uint32_t _Fractional_shift = _Fractional_denominator_bits > _Fractional_numerator_bits
                                           ? _Fractional_denominator_bits - _Fractional_numerator_bits
                                           : 0;

    if (_Fractional_shift > 0) {
        [[maybe_unused]] const bool _Shift_success1 =
            _Shift_left(_Fractional_numerator, _Fractional_shift); // assumes no overflow
        _STL_INTERNAL_CHECK(_Shift_success1);
    }

    const uint32_t _Required_fractional_bits_of_precision = _Required_bits_of_precision - _Integer_bits_of_precision;

    uint32_t _Remaining_bits_of_precision_required = _Required_fractional_bits_of_precision;
    if (_Integer_bits_of_precision > 0) {
        // If the fractional part of the mantissa provides no bits of precision and cannot affect rounding,
        // we can just take whatever bits we got from the integer part of the mantissa. This is the case for numbers
        // like 5.0000000000000000000001, where the significant digits of the fractional part start so far to the
        // right that they do not affect the floating-point representation.

        // If the fractional shift is exactly equal to the number of bits of precision that we require,
        // then no fractional bits will be part of the result, but the result may affect rounding.
        // This is e.g. the case for large, odd integers with a fractional part greater than or equal to .5.
        // Thus, we need to do the division to correctly round the result.
        if (_Fractional_shift > _Remaining_bits_of_precision_required) {
            return _Assemble_floating_point_value_from_big_integer_flt(_Integer_value, _Integer_bits_of_precision,
                _Data._Myis_negative, _Fractional_digits_present != 0 || !_Has_zero_tail, _Result);
        }

        _Remaining_bits_of_precision_required -= _Fractional_shift;
    }

    // If there was no integer part of the mantissa, we will need to compute the exponent from the fractional part.
    // The fractional exponent is the power of two by which we must multiply the fractional part to move it into the
    // range [1.0, 2.0). This will either be the same as the shift we computed earlier, or one greater than that shift:
    const uint32_t _Fractional_exponent =
        _Fractional_numerator < _Fractional_denominator ? _Fractional_shift + 1 : _Fractional_shift;

    [[maybe_unused]] const bool _Shift_success2 =
        _Shift_left(_Fractional_numerator, _Remaining_bits_of_precision_required); // assumes no overflow
    _STL_INTERNAL_CHECK(_Shift_success2);

    uint64_t _Fractional_mantissa = _Divide(_Fractional_numerator, _Fractional_denominator);

    _Has_zero_tail = _Has_zero_tail && _Fractional_numerator._Myused == 0;

    // We may have produced more bits of precision than were required. Check, and remove any "extra" bits:
    const uint32_t _Fractional_mantissa_bits = _Bit_scan_reverse(_Fractional_mantissa);
    if (_Fractional_mantissa_bits > _Required_fractional_bits_of_precision) {
        const uint32_t _Shift = _Fractional_mantissa_bits - _Required_fractional_bits_of_precision;
        _Has_zero_tail        = _Has_zero_tail && (_Fractional_mantissa & ((1ULL << _Shift) - 1)) == 0;
        _Fractional_mantissa >>= _Shift;
    }

    // Compose the mantissa from the integer and fractional parts:
    const uint32_t _Integer_mantissa_low  = _Integer_value._Myused > 0 ? _Integer_value._Mydata[0] : 0;
    const uint32_t _Integer_mantissa_high = _Integer_value._Myused > 1 ? _Integer_value._Mydata[1] : 0;
    const uint64_t _Integer_mantissa = _Integer_mantissa_low + (static_cast<uint64_t>(_Integer_mantissa_high) << 32);

    const uint64_t _Complete_mantissa =
        (_Integer_mantissa << _Required_fractional_bits_of_precision) + _Fractional_mantissa;

    // Compute the final exponent:
    // * If the mantissa had an integer part, then the exponent is one less than the number of bits we obtained
    // from the integer part. (It's one less because we are converting to the form 1.11111,
    // with one 1 to the left of the decimal point.)
    // * If the mantissa had no integer part, then the exponent is the fractional exponent that we computed.
    // Then, in both cases, we subtract an additional one from the exponent,
    // to account for the fact that we've generated an extra bit of precision, for use in rounding.
    const int32_t _Final_exponent = _Integer_bits_of_precision > 0
                                        ? static_cast<int32_t>(_Integer_bits_of_precision - 2)
                                        : -static_cast<int32_t>(_Fractional_exponent) - 1;

    return _Assemble_floating_point_value(
        _Complete_mantissa, _Final_exponent, _Data._Myis_negative, _Has_zero_tail, _Result);
}

template <class _FloatingType>
_NODISCARD constexpr errc _Convert_hexadecimal_string_to_floating_type(
    const _Floating_point_string& _Data, _FloatingType& _Result, bool _Has_zero_tail) noexcept {
    using _Traits = _Floating_type_traits<_FloatingType>;

    uint64_t _Mantissa = 0;
    int32_t _Exponent  = _Data._Myexponent + _Traits::_Mantissa_bits - 1;

    // Accumulate bits into the mantissa buffer
    const uint8_t* const _Mantissa_last = _Data._Mymantissa + _Data._Mymantissa_count;
    const uint8_t* _Mantissa_it         = _Data._Mymantissa;
    while (_Mantissa_it != _Mantissa_last && _Mantissa <= _Traits::_Normal_mantissa_mask) {
        _Mantissa *= 16;
        _Mantissa += *_Mantissa_it++;
        _Exponent -= 4; // The exponent is in binary; log2(16) == 4
    }

    while (_Has_zero_tail && _Mantissa_it != _Mantissa_last) {
        _Has_zero_tail = *_Mantissa_it++ == 0;
    }

    return _Assemble_floating_point_value(_Mantissa, _Exponent, _Data._Myis_negative, _Has_zero_tail, _Result);
}

template <class _Floating>
_NODISCARD constexpr from_chars_result _Ordinary_floating_from_chars(const char* const _First, const char* const _Last,
    _Floating& _Value, const chars_format _Fmt, const bool _Minus_sign, const char* _Next) noexcept {
    // vvvvvvvvvv DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS vvvvvvvvvv

    const bool _Is_hexadecimal = _Fmt == chars_format::hex;
    const int _Base{_Is_hexadecimal ? 16 : 10};

    // PERFORMANCE NOTE: _Fp_string is intentionally left uninitialized. Zero-initialization is quite expensive
    // and is unnecessary. The benefit of not zero-initializing is greatest for short inputs.
    _Floating_point_string _Fp_string = {}; //[neargye] default initialize for constexpr context. P1331 fix this?

    // Record the optional minus sign:
    _Fp_string._Myis_negative = _Minus_sign;

    uint8_t* const _Mantissa_first = _Fp_string._Mymantissa;
    uint8_t* const _Mantissa_last  = std::end(_Fp_string._Mymantissa);
    uint8_t* _Mantissa_it          = _Mantissa_first;

    // [_Whole_begin, _Whole_end) will contain 0 or more digits/hexits
    const char* const _Whole_begin = _Next;

    // Skip past any leading zeroes in the mantissa:
    for (; _Next != _Last && *_Next == '0'; ++_Next) {
    }
    const char* const _Leading_zero_end = _Next;

    // Scan the integer part of the mantissa:
    for (; _Next != _Last; ++_Next) {
        const unsigned char _Digit_value = _Digit_from_char(*_Next);

        if (_Digit_value >= _Base) {
            break;
        }

        if (_Mantissa_it != _Mantissa_last) {
            *_Mantissa_it++ = _Digit_value;
        }
    }
    const char* const _Whole_end = _Next;

    // Defend against _Exponent_adjustment integer overflow. (These values don't need to be strict.)
    constexpr ptrdiff_t _Maximum_adjustment = 1'000'000;
    constexpr ptrdiff_t _Minimum_adjustment = -1'000'000;

    // The exponent adjustment holds the number of digits in the mantissa buffer that appeared before the radix point.
    // It can be negative, and leading zeroes in the integer part are ignored. Examples:
    // For "03333.111", it is 4.
    // For "00000.111", it is 0.
    // For "00000.001", it is -2.
    int _Exponent_adjustment = static_cast<int>(std::min(_Whole_end - _Leading_zero_end, _Maximum_adjustment));

    // [_Whole_end, _Dot_end) will contain 0 or 1 '.' characters
    if (_Next != _Last && *_Next == '.') {
        ++_Next;
    }
    const char* const _Dot_end = _Next;

    // [_Dot_end, _Frac_end) will contain 0 or more digits/hexits

    // If we haven't yet scanned any nonzero digits, continue skipping over zeroes,
    // updating the exponent adjustment to account for the zeroes we are skipping:
    if (_Exponent_adjustment == 0) {
        for (; _Next != _Last && *_Next == '0'; ++_Next) {
        }

        _Exponent_adjustment = static_cast<int>(std::max(_Dot_end - _Next, _Minimum_adjustment));
    }

    // Scan the fractional part of the mantissa:
    bool _Has_zero_tail = true;

    for (; _Next != _Last; ++_Next) {
        const unsigned char _Digit_value = _Digit_from_char(*_Next);

        if (_Digit_value >= _Base) {
            break;
        }

        if (_Mantissa_it != _Mantissa_last) {
            *_Mantissa_it++ = _Digit_value;
        } else {
            _Has_zero_tail = _Has_zero_tail && _Digit_value == 0;
        }
    }
    const char* const _Frac_end = _Next;

    // We must have at least 1 digit/hexit
    if (_Whole_begin == _Whole_end && _Dot_end == _Frac_end) {
        return {_First, errc::invalid_argument};
    }

    const char _Exponent_prefix{_Is_hexadecimal ? 'p' : 'e'};

    bool _Exponent_is_negative = false;
    int _Exponent              = 0;

    constexpr int _Maximum_temporary_decimal_exponent = 5200;
    constexpr int _Minimum_temporary_decimal_exponent = -5200;

    if (_Fmt != chars_format::fixed // N4713 23.20.3 [charconv.from.chars]/7.3
                                    // "if fmt has chars_format::fixed set but not chars_format::scientific,
                                    // the optional exponent part shall not appear"
        && _Next != _Last && (static_cast<unsigned char>(*_Next) | 0x20) == _Exponent_prefix) { // found exponent prefix
        const char* _Unread = _Next + 1;

        if (_Unread != _Last && (*_Unread == '+' || *_Unread == '-')) { // found optional sign
            _Exponent_is_negative = *_Unread == '-';
            ++_Unread;
        }

        while (_Unread != _Last) {
            const unsigned char _Digit_value = _Digit_from_char(*_Unread);

            if (_Digit_value >= 10) {
                break;
            }

            // found decimal digit

            if (_Exponent <= _Maximum_temporary_decimal_exponent) {
                _Exponent = _Exponent * 10 + _Digit_value;
            }

            ++_Unread;
            _Next = _Unread; // consume exponent-part/binary-exponent-part
        }

        if (_Exponent_is_negative) {
            _Exponent = -_Exponent;
        }
    }

    // [_Frac_end, _Exponent_end) will either be empty or contain "[EPep] sign[opt] digit-sequence"
    const char* const _Exponent_end = _Next;

    if (_Fmt == chars_format::scientific
        && _Frac_end == _Exponent_end) { // N4713 23.20.3 [charconv.from.chars]/7.2
                                         // "if fmt has chars_format::scientific set but not chars_format::fixed,
                                         // the otherwise optional exponent part shall appear"
        return {_First, errc::invalid_argument};
    }

    // Remove trailing zeroes from mantissa:
    while (_Mantissa_it != _Mantissa_first && *(_Mantissa_it - 1) == 0) {
        --_Mantissa_it;
    }

    // If the mantissa buffer is empty, the mantissa was composed of all zeroes (so the mantissa is 0).
    // All such strings have the value zero, regardless of what the exponent is (because 0 * b^n == 0 for all b and n).
    // We can return now. Note that we defer this check until after we scan the exponent, so that we can correctly
    // update _Next to point past the end of the exponent.
    if (_Mantissa_it == _Mantissa_first) {
        _STL_INTERNAL_CHECK(_Has_zero_tail);
        _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
        return {_Next, errc{}};
    }

    // Before we adjust the exponent, handle the case where we detected a wildly
    // out of range exponent during parsing and clamped the value:
    if (_Exponent > _Maximum_temporary_decimal_exponent) {
        _Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
        return {_Next, errc::result_out_of_range}; // Overflow example: "1e+9999"
    }

    if (_Exponent < _Minimum_temporary_decimal_exponent) {
        _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
        return {_Next, errc::result_out_of_range}; // Underflow example: "1e-9999"
    }

    // In hexadecimal floating constants, the exponent is a base 2 exponent. The exponent adjustment computed during
    // parsing has the same base as the mantissa (so, 16 for hexadecimal floating constants).
    // We therefore need to scale the base 16 multiplier to base 2 by multiplying by log2(16):
    const int _Exponent_adjustment_multiplier{_Is_hexadecimal ? 4 : 1};

    _Exponent += _Exponent_adjustment * _Exponent_adjustment_multiplier;

    // Verify that after adjustment the exponent isn't wildly out of range (if it is, it isn't representable
    // in any supported floating-point format).
    if (_Exponent > _Maximum_temporary_decimal_exponent) {
        _Assemble_floating_point_infinity(_Fp_string._Myis_negative, _Value);
        return {_Next, errc::result_out_of_range}; // Overflow example: "10e+5199"
    }

    if (_Exponent < _Minimum_temporary_decimal_exponent) {
        _Assemble_floating_point_zero(_Fp_string._Myis_negative, _Value);
        return {_Next, errc::result_out_of_range}; // Underflow example: "0.001e-5199"
    }

    _Fp_string._Myexponent       = _Exponent;
    _Fp_string._Mymantissa_count = static_cast<uint32_t>(_Mantissa_it - _Mantissa_first);

    if (_Is_hexadecimal) {
        const errc _Ec = _Convert_hexadecimal_string_to_floating_type(_Fp_string, _Value, _Has_zero_tail);
        return {_Next, _Ec};
    } else {
        const errc _Ec = _Convert_decimal_string_to_floating_type(_Fp_string, _Value, _Has_zero_tail);
        return {_Next, _Ec};
    }

    // ^^^^^^^^^^ DERIVED FROM corecrt_internal_strtox.h WITH SIGNIFICANT MODIFICATIONS ^^^^^^^^^^
}

_NODISCARD constexpr bool _Starts_with_case_insensitive(
    const char* _First, const char* const _Last, const char* _Lowercase) noexcept {
    // pre: _Lowercase contains only ['a', 'z'] and is null-terminated
    for (; _First != _Last && *_Lowercase != '\0'; ++_First, ++_Lowercase) {
        if ((static_cast<unsigned char>(*_First) | 0x20) != *_Lowercase) {
            return false;
        }
    }

    return *_Lowercase == '\0';
}

template <class _Floating>
_NODISCARD constexpr from_chars_result _Infinity_from_chars(const char* const _First, const char* const _Last, _Floating& _Value,
    const bool _Minus_sign, const char* _Next) noexcept {
    // pre: _Next points at 'i' (case-insensitively)
    if (!_Starts_with_case_insensitive(_Next + 1, _Last, "nf")) { // definitely invalid
        return {_First, errc::invalid_argument};
    }

    // definitely inf
    _Next += 3;

    if (_Starts_with_case_insensitive(_Next, _Last, "inity")) { // definitely infinity
        _Next += 5;
    }

    _Assemble_floating_point_infinity(_Minus_sign, _Value);

    return {_Next, errc{}};
}

template <class _Floating>
_NODISCARD constexpr from_chars_result _Nan_from_chars(const char* const _First, const char* const _Last, _Floating& _Value,
    bool _Minus_sign, const char* _Next) noexcept {
    // pre: _Next points at 'n' (case-insensitively)
    if (!_Starts_with_case_insensitive(_Next + 1, _Last, "an")) { // definitely invalid
        return {_First, errc::invalid_argument};
    }

    // definitely nan
    _Next += 3;

    bool _Quiet = true;

    if (_Next != _Last && *_Next == '(') { // possibly nan(n-char-sequence[opt])
        const char* const _Seq_begin = _Next + 1;

        for (const char* _Temp = _Seq_begin; _Temp != _Last; ++_Temp) {
            if (*_Temp == ')') { // definitely nan(n-char-sequence[opt])
                _Next = _Temp + 1;

                if (_Temp - _Seq_begin == 3
                    && _Starts_with_case_insensitive(_Seq_begin, _Temp, "ind")) { // definitely nan(ind)
                    // The UCRT considers indeterminate NaN to be negative quiet NaN with no payload bits set.
                    // It parses "nan(ind)" and "-nan(ind)" identically.
                    _Minus_sign = true;
                } else if (_Temp - _Seq_begin == 4
                           && _Starts_with_case_insensitive(_Seq_begin, _Temp, "snan")) { // definitely nan(snan)
                    _Quiet = false;
                }

                break;
            } else if (*_Temp == '_' || ('0' <= *_Temp && *_Temp <= '9') || ('A' <= *_Temp && *_Temp <= 'Z')
                       || ('a' <= *_Temp && *_Temp <= 'z')) { // possibly nan(n-char-sequence[opt]), keep going
            } else { // definitely nan, not nan(n-char-sequence[opt])
                break;
            }
        }
    }

    // Intentional behavior difference between the UCRT and the STL:
    // strtod()/strtof() parse plain "nan" as being a quiet NaN with all payload bits set.
    // numeric_limits::quiet_NaN() returns a quiet NaN with no payload bits set.
    // This implementation of from_chars() has chosen to be consistent with numeric_limits.

    using _Traits    = _Floating_type_traits<_Floating>;
    using _Uint_type = typename _Traits::_Uint_type;

    _Uint_type _Uint_value = _Traits::_Shifted_exponent_mask;

    if (_Minus_sign) {
        _Uint_value |= _Traits::_Shifted_sign_mask;
    }

    if (_Quiet) {
        _Uint_value |= _Traits::_Special_nan_mantissa_mask;
    } else {
        _Uint_value |= 1;
    }

    _Value = third_party::bit_cast<_Floating>(_Uint_value);

    return {_Next, errc{}};
}

template <class _Floating>
_NODISCARD constexpr from_chars_result _Floating_from_chars(
    const char* const _First, const char* const _Last, _Floating& _Value, const chars_format _Fmt) noexcept {
    _Adl_verify_range(_First, _Last);

    _STL_ASSERT(_Fmt == chars_format::general || _Fmt == chars_format::scientific || _Fmt == chars_format::fixed
                    || _Fmt == chars_format::hex,
        "invalid format in from_chars()");

    bool _Minus_sign = false;

    const char* _Next = _First;

    if (_Next == _Last) {
        return {_First, errc::invalid_argument};
    }

    if (*_Next == '-') {
        _Minus_sign = true;
        ++_Next;

        if (_Next == _Last) {
            return {_First, errc::invalid_argument};
        }
    }

    // Distinguish ordinary numbers versus inf/nan with a single test.
    // ordinary numbers start with ['.'] ['0', '9'] ['A', 'F'] ['a', 'f']
    // inf/nan start with ['I'] ['N'] ['i'] ['n']
    // All other starting characters are invalid.
    // Setting the 0x20 bit folds these ranges in a useful manner.
    // ordinary (and some invalid) starting characters are folded to ['.'] ['0', '9'] ['a', 'f']
    // inf/nan starting characters are folded to ['i'] ['n']
    // These are ordered: ['.'] ['0', '9'] ['a', 'f'] < ['i'] ['n']
    // Note that invalid starting characters end up on both sides of this test.
    const unsigned char _Folded_start = static_cast<unsigned char>(static_cast<unsigned char>(*_Next) | 0x20);

    if (_Folded_start <= 'f') { // possibly an ordinary number
        return _Ordinary_floating_from_chars(_First, _Last, _Value, _Fmt, _Minus_sign, _Next);
    } else if (_Folded_start == 'i') { // possibly inf
        return _Infinity_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
    } else if (_Folded_start == 'n') { // possibly nan
        return _Nan_from_chars(_First, _Last, _Value, _Minus_sign, _Next);
    } else { // definitely invalid
        return {_First, errc::invalid_argument};
    }
}

} // namespace nstd
