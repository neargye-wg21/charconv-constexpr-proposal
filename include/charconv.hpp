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

// Add Constexpr Modifiers to Functions 'to_chars' and `from_chars'

#pragma once

#include "detail/charconv_entity.hpp"
#include "detail/charconv_integral_to_chars.hpp"
#include "detail/charconv_integral_from_chars.hpp"

namespace nstd {

constexpr to_chars_result to_chars(char* const _First, char* const _Last, const char _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const signed char _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const unsigned char _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const short _Value,const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const unsigned short _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const int _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const unsigned int _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const long _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const unsigned long _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const long long _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}
constexpr to_chars_result to_chars(char* const _First, char* const _Last, const unsigned long long _Value, const int _Base = 10) noexcept {
  return _Integer_to_chars(_First, _Last, _Value, _Base);
}

to_chars_result to_chars(char* _First, char* _Last, const bool _Value, const int _Base = 10) = delete;

constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, char& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, signed char& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, unsigned char& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, short& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, unsigned short& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, int& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, unsigned int& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, long& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, unsigned long& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, long long& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}
constexpr from_chars_result from_chars(const char* const _First, const char* const _Last, unsigned long long& _Value, const int _Base = 10) noexcept {
  return _Integer_from_chars(_First, _Last, _Value, _Base);
}

from_chars_result from_chars(const char* _First, const char* _Last, bool& _Value, const int _Base = 10) = delete;

} // namespace nstd
