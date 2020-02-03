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

#pragma once

#include <cfloat>
#include <climits>
#include <cstdint>

namespace nstd {

template <class _FloatingType>
struct _Floating_type_traits;

template <>
struct _Floating_type_traits<float> {
    static constexpr int32_t _Mantissa_bits = FLT_MANT_DIG;
    static constexpr int32_t _Exponent_bits = sizeof(float) * CHAR_BIT - FLT_MANT_DIG;

    static constexpr int32_t _Maximum_binary_exponent = FLT_MAX_EXP - 1;
    static constexpr int32_t _Minimum_binary_exponent = FLT_MIN_EXP - 1;

    static constexpr int32_t _Exponent_bias = 127;

    static constexpr int32_t _Sign_shift     = _Exponent_bits + _Mantissa_bits - 1;
    static constexpr int32_t _Exponent_shift = _Mantissa_bits - 1;

    using _Uint_type = uint32_t;

    static constexpr uint32_t _Exponent_mask             = (1u << _Exponent_bits) - 1;
    static constexpr uint32_t _Normal_mantissa_mask      = (1u << _Mantissa_bits) - 1;
    static constexpr uint32_t _Denormal_mantissa_mask    = (1u << (_Mantissa_bits - 1)) - 1;
    static constexpr uint32_t _Special_nan_mantissa_mask = 1u << (_Mantissa_bits - 2);
    static constexpr uint32_t _Shifted_sign_mask         = 1u << _Sign_shift;
    static constexpr uint32_t _Shifted_exponent_mask     = _Exponent_mask << _Exponent_shift;
};

template <>
struct _Floating_type_traits<double> {
    static constexpr int32_t _Mantissa_bits = DBL_MANT_DIG;
    static constexpr int32_t _Exponent_bits = sizeof(double) * CHAR_BIT - DBL_MANT_DIG;

    static constexpr int32_t _Maximum_binary_exponent = DBL_MAX_EXP - 1;
    static constexpr int32_t _Minimum_binary_exponent = DBL_MIN_EXP - 1;

    static constexpr int32_t _Exponent_bias = 1023;

    static constexpr int32_t _Sign_shift     = _Exponent_bits + _Mantissa_bits - 1;
    static constexpr int32_t _Exponent_shift = _Mantissa_bits - 1;

    using _Uint_type = uint64_t;

    static constexpr uint64_t _Exponent_mask             = (1ULL << _Exponent_bits) - 1;
    static constexpr uint64_t _Normal_mantissa_mask      = (1ULL << _Mantissa_bits) - 1;
    static constexpr uint64_t _Denormal_mantissa_mask    = (1ULL << (_Mantissa_bits - 1)) - 1;
    static constexpr uint64_t _Special_nan_mantissa_mask = 1ULL << (_Mantissa_bits - 2);
    static constexpr uint64_t _Shifted_sign_mask         = 1ULL << _Sign_shift;
    static constexpr uint64_t _Shifted_exponent_mask     = _Exponent_mask << _Exponent_shift;
};

} // namespace nstd
