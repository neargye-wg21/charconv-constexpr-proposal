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

TEST_CASE("[to_chars] int") {
    auto test = []() constexpr -> bool {
        std::array<char, 10> str = {};
        if (auto [p, ec] = proposal::to_chars(str.data(), str.data() + str.size(), 42); ec == std::errc{}) {
            return str[0] == '4' && str[1] == '2';
        }
        return false;
    };

    constexpr auto test_to_chars_int = test();
    static_assert(test_to_chars_int);
    REQUIRE(test());
}

TEST_CASE("[from_chars] int") {
    auto test = []() constexpr -> bool {
        std::array<char, 10> str{"42"};
        int result = -1;
        if (auto [p, ec] = proposal::from_chars(str.data(), str.data() + str.size(), result); ec == std::errc{}) {
            return result == 42;
        }
        return false;
    };

    constexpr auto test_from_chars_int = test();
    static_assert(test_from_chars_int);
    REQUIRE(test());
}
