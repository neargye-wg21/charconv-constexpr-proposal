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

namespace nstd {

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
