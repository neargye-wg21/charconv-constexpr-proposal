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

#include <cfloat>
#include <climits>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace third_party {

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

template <class Floating>
class ieee754 {
    static_assert(std::is_same_v<Floating, float> || std::is_same_v<Floating, double>);

    using floating_t = Floating;
    using unsigned_t = std::conditional_t<std::is_same_v<Floating, float>, std::uint32_t, std::uint64_t>;
    static_assert(sizeof(floating_t) == sizeof(unsigned_t));

    using limits_t = std::numeric_limits<floating_t>;

    static constexpr std::size_t total_bits    = sizeof(floating_t) * CHAR_BIT;
    static constexpr std::size_t exponent_bits = _Floating_type_traits<floating_t>::_Exponent_bits;
    static constexpr std::size_t mantissa_bits = _Floating_type_traits<floating_t>::_Mantissa_bits - 1;
    static_assert(1 + exponent_bits + mantissa_bits == total_bits);

    static constexpr int exponent_bias = _Floating_type_traits<floating_t>::_Exponent_bias;

    static constexpr unsigned_t sign_mask     = unsigned_t(1) << (total_bits - 1);
    static constexpr unsigned_t exponent_mask = ((unsigned_t(1) << exponent_bits) - 1) << mantissa_bits;
    static constexpr unsigned_t mantissa_mask = (unsigned_t(1) << mantissa_bits) - 1;

    constexpr bool signbit(floating_t f) {
        return f == 0 ? 1 / f < 0 : f < 0;
    }

    constexpr bool isfinite(floating_t f) {
        return 1 / f != 0;
    }

public:
    constexpr explicit ieee754(floating_t f) {
        if (f != f) {
            m_exponent = (unsigned_t(1) << exponent_bits) - 1;
            m_mantissa = 1; // any non-zero value
        } else if (!isfinite(f)) {
            m_exponent = (unsigned_t(1) << exponent_bits) - 1;
            m_mantissa = 0;
            m_negative = signbit(f);
        } else if (f == 0) {
            m_exponent = 0;
            m_mantissa = 0;
            m_negative = signbit(f);
        } else {
            m_negative = signbit(f);
            if (m_negative) f = -f;

            m_exponent = exponent_bias;
            while (f >= 2) { m_exponent++; f /= 2; }
            while (f < 1) { m_exponent--; f *= 2; if (m_exponent == 0) break; }

            if (m_exponent == 0) {
                // denorm
                m_mantissa = f / 2;
                assert(0 <= m_mantissa && m_mantissa < 1);
            } else {
                // norm
                m_mantissa = f - 1;
                assert(0 <= m_mantissa && m_mantissa < 1);
            }
        }
    }

    constexpr explicit ieee754(unsigned_t u) {
        m_negative = (u & sign_mask) != 0;
        m_exponent = (u & exponent_mask) >> mantissa_bits;
        m_mantissa = floating_t(u & mantissa_mask) / (mantissa_mask + 1);
    }

    constexpr operator floating_t() const {
        if (m_exponent == 0 && m_mantissa == floating_t(0))
            return m_negative ? floating_t(-0.0) : floating_t(+0.0);

        if (m_exponent + 1 == (unsigned_t(1) << exponent_bits)) {
            if (m_mantissa)
                return limits_t::quiet_NaN();
            else
                return m_negative ? -limits_t::infinity() : limits_t::infinity();
        }

        floating_t value = m_mantissa;

        int exp = static_cast<int>(m_exponent);
        if (exp == 0) {
            // denorm
            exp -= (exponent_bias - 1);
        } else {
            // norm
            value += 1;
            exp -= exponent_bias;
        }

        while (exp > 0) { exp--; value *= 2; }
        while (exp < 0) { exp++; value /= 2; }

        return m_negative ? -value : value;
    }

    constexpr operator unsigned_t() const {
        unsigned_t s = (m_negative ? sign_mask : unsigned_t(0));
        unsigned_t e = m_exponent << mantissa_bits;
        unsigned_t m = unsigned_t(m_mantissa * (mantissa_mask + 1));
        return s | e | m;
    }

    friend bool operator==(const ieee754& lhs, const ieee754& rhs) {
        auto proj = [](const ieee754& v) { return std::tie(v.m_negative, v.m_exponent, v.m_mantissa); };
        return (lhs.is_nan() && rhs.is_nan()) || proj(lhs) == proj(rhs);
    }

    constexpr bool is_negative() const {
        return m_negative;
    }

    constexpr unsigned_t exponent() const {
        return m_exponent;
    }

    constexpr floating_t mantissa() const {
        return m_mantissa;
    }

    constexpr bool is_nan() const {
        return (m_exponent + 1 == (unsigned_t(1) << exponent_bits)) && (m_mantissa != 0);
    }

private:
    bool       m_negative = false;
    unsigned_t m_exponent = 0; // [0, ...)
    floating_t m_mantissa = 0; // [0, 1)
};

ieee754(float) -> ieee754<float>;
ieee754(double) -> ieee754<double>;
ieee754(uint32_t) -> ieee754<float>;
ieee754(uint64_t) -> ieee754<double>;

} // namespace third_party
