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

#include <cmath>
#include <climits>

#include "ieee754.hpp"
#include "detail.hpp"

namespace nstd {

// FUNCTION TEMPLATE _Bit_cast
template <class _To, class _From, std::enable_if_t<sizeof(_To) == sizeof(_From) && std::is_trivially_copyable_v<_To> && std::is_trivially_copyable_v<_From>, int> = 0>
constexpr _To _Bit_cast(const _From& _From_obj) noexcept {
#if 0
	if (detail::is_constant_evaluated()) {
#endif
		if constexpr (std::is_floating_point_v<_From> || std::is_floating_point_v<_To>) {
			return static_cast<_To>(detail::ieee754(_From_obj));
		}
		else {
			static_assert(sizeof(_To) == 0, "not implemented");
		}
#if 0
	}
	else {
		_To _To_obj; // assumes default-init
		std::memcpy(std::addressof(_To_obj), std::addressof(_From_obj), sizeof(_To));
		return _To_obj;
	}
#endif
}

}
