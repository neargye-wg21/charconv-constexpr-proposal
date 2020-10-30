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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <charconv/charconv.hpp>

namespace proposal = nstd;

#include <array>
#include <cstring>
#include <iterator>
#include <optional>

namespace Catch {
    template <typename T>
    struct StringMaker<std::optional<T>> {
        static std::string convert(const std::optional<T>& value ) {
            if (value)
                return StringMaker<T>::convert(*value);
            else
                return "null";
        }
    };
}

TEST_CASE("[to_chars] float") {
    auto test = []() constexpr -> bool {
        std::array<char, 10> str = {};
        if (auto [p, ec] = proposal::to_chars(str.data(), str.data() + str.size(), 42.2F); ec == std::errc{}) {
            return str[0] == '4' && str[1] == '2' && str[2] == '.' && str[3] == '2';
        }
        return false;
    };

    constexpr auto test_to_chars_float = test();
    static_assert(test_to_chars_float);
    REQUIRE(test());
}

TEST_CASE("[from_chars] float") {
    Catch::StringMaker<float>::precision = 15;

    auto parse_float = [](std::string_view str) constexpr -> std::optional<float> {
        float result = -1;
        if (auto [p, ec] = proposal::from_chars(str.data(), str.data() + str.size(), result); ec == std::errc{}) {
            return result;
        }
        return std::nullopt;
    };

#define TEST_NUMBER(X) { constexpr auto result = parse_float(#X); CHECK(result == Approx(X)); }

    TEST_NUMBER(42.2)
    TEST_NUMBER(-1000.0)
    TEST_NUMBER(-0.0)
    // TEST_NUMBER(+0.0) // valid?
    TEST_NUMBER(1e-5)
    TEST_NUMBER(1e+5)
    TEST_NUMBER(1e+8)
    // TEST_NUMBER(1e+10)
    // TEST_NUMBER(1e-10)

#undef TEST_NUMBER
}
