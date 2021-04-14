# Add Constexpr Modifiers to Functions `to_chars` and `from_chars` for Integral Types

## References

* <https://github.com/microsoft/STL/blob/master/stl/inc/charconv>

## Tested

Tested on [MS/STL charconv set tests](/test) and small constexpr set tests.

* [GCC 10](https://github.com/Neargye/charconv-constexpr-proposal/actions/workflows/ubuntu.yml)
* [Clang 10](https://github.com/Neargye/charconv-constexpr-proposal/actions/workflows/ubuntu.yml)
* [Visual Studio 2019](https://ci.appveyor.com/project/Neargye/charconv-constexpr-proposal/branch/integral)
