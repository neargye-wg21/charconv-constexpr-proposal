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
// * add constexpr modifiers to 'to_chars' and 'from_chars'

#pragma once

#include "charconv/detail/entity.hpp"

#include "floating_point_to_chars.hpp"
#include "floating_point_from_chars.hpp"

namespace nstd {

constexpr to_chars_result to_chars(char* const _First, char* const _Last, const float _Value) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(_First, _Last, _Value, chars_format{}, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const double _Value) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(_First, _Last, _Value, chars_format{}, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const long double _Value) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Plain>(_First, _Last, static_cast<double>(_Value), chars_format{}, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const float _Value, const chars_format _Fmt) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(_First, _Last, _Value, _Fmt, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const double _Value, const chars_format _Fmt) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(_First, _Last, _Value, _Fmt, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const long double _Value, const chars_format _Fmt) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_only>(_First, _Last, static_cast<double>(_Value), _Fmt, 0);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const float _Value, const chars_format _Fmt, const int _Precision) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(_First, _Last, _Value, _Fmt, _Precision);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const double _Value, const chars_format _Fmt, const int _Precision) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(_First, _Last, _Value, _Fmt, _Precision);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const long double _Value, const chars_format _Fmt, const int _Precision) noexcept {
    return _Floating_to_chars<_Floating_to_chars_overload::_Format_precision>(_First, _Last, static_cast<double>(_Value), _Fmt, _Precision);
}

constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, float& _Value,
    const chars_format _Fmt = chars_format::general) noexcept /* strengthened */ {
    return _Floating_from_chars(_First, _Last, _Value, _Fmt);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, double& _Value,
    const chars_format _Fmt = chars_format::general) noexcept /* strengthened */ {
    return _Floating_from_chars(_First, _Last, _Value, _Fmt);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, long double& _Value,
    const chars_format _Fmt = chars_format::general) noexcept /* strengthened */ {
    double _Dbl;
    const from_chars_result _Result = _Floating_from_chars(_First, _Last, _Dbl, _Fmt);

    if (_Result.ec == errc{}) {
        _Value = _Dbl;
    }

    return _Result;
}

} // namespace nstd
