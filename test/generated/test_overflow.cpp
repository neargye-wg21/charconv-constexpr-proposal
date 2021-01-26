#include "../test.cxx"

constexpr auto _test_overflow = []{
    // Test overflow scenarios.
    test_from_chars<unsigned int>("4294967289", 10, 10, errc{}, 4294967289U); // not risky
    test_from_chars<unsigned int>("4294967294", 10, 10, errc{}, 4294967294U); // risky with good digit
    test_from_chars<unsigned int>("4294967295", 10, 10, errc{}, 4294967295U); // risky with max digit
    test_from_chars<unsigned int>("4294967296", 10, 10, out_ran); // risky with bad digit
    test_from_chars<unsigned int>("4294967300", 10, 10, out_ran); // beyond risky

    test_from_chars<int>("2147483639", 10, 10, errc{}, 2147483639); // not risky
    test_from_chars<int>("2147483646", 10, 10, errc{}, 2147483646); // risky with good digit
    test_from_chars<int>("2147483647", 10, 10, errc{}, 2147483647); // risky with max digit
    test_from_chars<int>("2147483648", 10, 10, out_ran); // risky with bad digit
    test_from_chars<int>("2147483650", 10, 10, out_ran); // beyond risky

    test_from_chars<int>("-2147483639", 10, 11, errc{}, -2147483639); // not risky
    test_from_chars<int>("-2147483647", 10, 11, errc{}, -2147483647); // risky with good digit
    test_from_chars<int>("-2147483648", 10, 11, errc{}, -2147483647 - 1); // risky with max digit
    test_from_chars<int>("-2147483649", 10, 11, out_ran); // risky with bad digit
    test_from_chars<int>("-2147483650", 10, 11, out_ran); // beyond risky
    return true;
}();

int main(int, char**) {}
