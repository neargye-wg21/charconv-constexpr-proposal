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

#include <catch.hpp>

#include <charconv/floating_point/bit_cast.hpp>

#include <cstring>

namespace third_party {
    template <class Floating>
    std::ostream& operator<<(std::ostream& os, const ieee754<Floating>& value) {
        return os << "(sign=" << std::boolalpha << value.is_negative() << ", exp=" << value.exponent() << ", mansissa=" << value.mantissa() << ")";
    }
}

namespace proposal = nstd;

TEST_CASE("[ieee754<float>]") {
    static_assert(proposal::_Bit_cast<uint32_t>(1.0f) == 0x3F80'0000);
    static_assert(proposal::_Bit_cast<uint32_t>(0.3f) == 0x3E99'999A);
    static_assert(proposal::_Bit_cast<uint32_t>(std::numeric_limits<float>::infinity()) == 0x7F80'0000);
    static_assert(proposal::_Bit_cast<uint32_t>(-std::numeric_limits<float>::infinity()) == 0xFF80'0000);

    uint32_t i = 0;
    while (true) {
        auto from_i = third_party::ieee754<float>(i);

        float f;
        std::memcpy(&f, &i, sizeof(i));

        auto from_f = third_party::ieee754<float>(f);

        if (std::isnan(f)) {
            REQUIRE(std::isnan(static_cast<float>(from_i)));
            REQUIRE(std::isnan(static_cast<float>(from_f)));
        }
        else {
            REQUIRE(from_i == from_f);
            REQUIRE(i == static_cast<uint32_t>(from_i));
            REQUIRE(i == static_cast<uint32_t>(from_f));
            REQUIRE(f == static_cast<float>(from_i));
            REQUIRE(f == static_cast<float>(from_f));
        }

        auto old_i = std::exchange(i, i + rand() % 1000);
        if (i < old_i) break;
    }
}

TEST_CASE("[ieee754<double>]") {
    static_assert(proposal::_Bit_cast<uint64_t>(1.0) == 0x3FF0'0000'0000'0000ull);
    static_assert(proposal::_Bit_cast<uint64_t>(0.3) == 0x3FD3'3333'3333'3333ull);
    static_assert(proposal::_Bit_cast<uint64_t>(std::numeric_limits<double>::infinity()) == 0x7FF0'0000'0000'0000ull);
    static_assert(proposal::_Bit_cast<uint64_t>(-std::numeric_limits<double>::infinity()) == 0xFFF0'0000'0000'0000ull);

    uint64_t i = 0;
    while (true) {
        auto from_i = third_party::ieee754<double>(i);

        double f;
        std::memcpy(&f, &i, sizeof(i));

        auto from_f = third_party::ieee754<double>(f);

        if (std::isnan(f)) {
            REQUIRE(std::isnan(static_cast<double>(from_i)));
            REQUIRE(std::isnan(static_cast<double>(from_f)));
        }
        else {
            REQUIRE(from_i == from_f);
            REQUIRE(i == static_cast<uint64_t>(from_i));
            REQUIRE(i == static_cast<uint64_t>(from_f));
            REQUIRE(f == static_cast<double>(from_i));
            REQUIRE(f == static_cast<double>(from_f));
        }

        auto old_i = std::exchange(i, i + rand() % 10000 + (uint64_t(1) << 42));
        if (i < old_i) break;
    }
}
