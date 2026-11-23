#ifndef LESSAMPLER_TEST_SUPPORT_H
#define LESSAMPLER_TEST_SUPPORT_H

#include <cmath>
#include <iostream>
#include <string_view>

namespace test {

    inline int failures = 0;

    inline void check(bool condition, std::string_view expression, const char *file, int line) {
        if (!condition) {
            ++failures;
            std::cerr << file << ":" << line << ": CHECK failed: " << expression << '\n';
        }
    }

    inline int finish() {
        if (failures == 0) {
            std::cout << "All checks passed\n";
            return 0;
        }
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }

}// namespace test

#define CHECK(expression) ::test::check((expression), #expression, __FILE__, __LINE__)
#define REQUIRE(expression)                                        \
    do {                                                           \
        if (!(expression)) {                                       \
            ::test::check(false, #expression, __FILE__, __LINE__); \
            return ::test::finish();                               \
        }                                                          \
    } while (false)
#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))
#define CHECK_NEAR(actual, expected, tolerance) CHECK(std::abs((actual) - (expected)) <= (tolerance))

#endif//LESSAMPLER_TEST_SUPPORT_H
