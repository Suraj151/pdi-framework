/******************************** PDI Test **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Minimal test harness for the host build. A test is declared with TEST(suite,
name) and registers itself before main runs, so adding a file to the build is
all it takes to add tests.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDI_TEST_H_
#define _PDI_TEST_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

namespace pditest
{

    typedef void (*TestFn)();

    /**
     * @brief One registered test. Instances are static and chained at load time.
     */
    struct TestCase
    {
        const char *suite;
        const char *name;
        TestFn fn;
        const char *file;
        int line;
        TestCase *next;
    };

    void registerTest(TestCase *testcase);

    /**
     * @brief Record a failure against the running test and print its location.
     */
    void reportFailure(const char *file, int line, const char *expr, const char *detail);

    /**
     * @brief Run every registered test, honouring --filter, --list and --verbose.
     * @return 0 when all selected tests pass.
     */
    int runAll(int argc, char **argv);

    struct Registrar
    {
        Registrar(TestCase *testcase) { registerTest(testcase); }
    };

    /**
     * @brief Render a value into buf for a failure message. Falls back to a byte
     *        count for types that carry no obvious text form.
     */
    template <typename T>
    void describe(char *buf, size_t size, const T &value)
    {
        if constexpr (std::is_same<T, bool>::value)
        {
            snprintf(buf, size, "%s", value ? "true" : "false");
        }
        else if constexpr (std::is_floating_point<T>::value)
        {
            snprintf(buf, size, "%f", (double)value);
        }
        else if constexpr (std::is_enum<T>::value)
        {
            snprintf(buf, size, "%lld", (long long)value);
        }
        else if constexpr (std::is_integral<T>::value && std::is_signed<T>::value)
        {
            snprintf(buf, size, "%lld", (long long)value);
        }
        else if constexpr (std::is_integral<T>::value)
        {
            snprintf(buf, size, "%llu", (unsigned long long)value);
        }
        else if constexpr (std::is_same<typename std::decay<T>::type, char *>::value ||
                           std::is_same<typename std::decay<T>::type, const char *>::value)
        {
            snprintf(buf, size, "\"%s\"", value ? (const char *)value : "(null)");
        }
        else if constexpr (std::is_pointer<T>::value)
        {
            snprintf(buf, size, "%p", (const void *)value);
        }
        else
        {
            snprintf(buf, size, "<%zu byte value>", sizeof(T));
        }
    }

    /**
     * @brief Build the "expected x, got y" tail of a comparison failure.
     */
    template <typename A, typename B>
    void describePair(char *buf, size_t size, const A &actual, const B &expected)
    {
        char abuf[128];
        char ebuf[128];
        describe(abuf, sizeof(abuf), actual);
        describe(ebuf, sizeof(ebuf), expected);
        snprintf(buf, size, "expected %s, got %s", ebuf, abuf);
    }

    /**
     * @brief Render a byte range as hex for a memory comparison failure.
     */
    void describeBytes(char *buf, size_t size, const void *data, size_t len);

    /**
     * @brief A copy of a string argument, taken while the argument is still
     *        alive. Comparing `foo().c_str()` would otherwise read a pointer
     *        into a temporary that died at the end of its own statement.
     */
    struct StrArg
    {
        bool isnull;
        std::string value;

        bool operator==(const StrArg &other) const
        {
            return isnull == other.isnull && value == other.value;
        }

        const char *text() const { return isnull ? "(null)" : value.c_str(); }
    };

    inline StrArg strArg(const char *text)
    {
        StrArg arg;
        arg.isnull = (nullptr == text);
        if (!arg.isnull)
        {
            arg.value = text;
        }
        return arg;
    }

} // namespace pditest

#define PDITEST_UNIQUE_(a, b) a##b
#define PDITEST_CONCAT_(a, b) PDITEST_UNIQUE_(a, b)

#define TEST(suite_, name_)                                                     \
    static void PDITEST_CONCAT_(pditest_fn_, PDITEST_CONCAT_(suite_, name_))(); \
    static pditest::TestCase PDITEST_CONCAT_(pditest_tc_, PDITEST_CONCAT_(suite_, name_)) = {              \
        #suite_, #name_, PDITEST_CONCAT_(pditest_fn_, PDITEST_CONCAT_(suite_, name_)), __FILE__, __LINE__, \
        nullptr};                                                                                          \
    static pditest::Registrar PDITEST_CONCAT_(pditest_reg_, PDITEST_CONCAT_(suite_, name_))(               \
        &PDITEST_CONCAT_(pditest_tc_, PDITEST_CONCAT_(suite_, name_)));                                    \
    static void PDITEST_CONCAT_(pditest_fn_, PDITEST_CONCAT_(suite_, name_))()

#define PDITEST_REPORT_(expr, detail) pditest::reportFailure(__FILE__, __LINE__, expr, detail)

#define ASSERT_TRUE(cond)                                     \
    do                                                        \
    {                                                         \
        if (!(cond))                                          \
        {                                                     \
            PDITEST_REPORT_(#cond, "expected true, got false"); \
            return;                                           \
        }                                                     \
    } while (0)

#define ASSERT_FALSE(cond)                                    \
    do                                                        \
    {                                                         \
        if ((cond))                                           \
        {                                                     \
            PDITEST_REPORT_(#cond, "expected false, got true"); \
            return;                                           \
        }                                                     \
    } while (0)

#define PDITEST_ASSERT_CMP_(actual, expected, op, text)                        \
    do                                                                         \
    {                                                                          \
        auto pditest_a_ = (actual);                                            \
        auto pditest_e_ = (expected);                                          \
        if (!(pditest_a_ op pditest_e_))                                       \
        {                                                                      \
            char pditest_detail_[288];                                         \
            pditest::describePair(pditest_detail_, sizeof(pditest_detail_),    \
                                  pditest_a_, pditest_e_);                     \
            PDITEST_REPORT_(#actual " " text " " #expected, pditest_detail_);  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_EQ(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, ==, "==")
#define ASSERT_NE(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, !=, "!=")
#define ASSERT_LT(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, <, "<")
#define ASSERT_LE(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, <=, "<=")
#define ASSERT_GT(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, >, ">")
#define ASSERT_GE(actual, expected) PDITEST_ASSERT_CMP_(actual, expected, >=, ">=")

#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == nullptr)
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != nullptr)

#define ASSERT_STREQ(actual, expected)                                            \
    do                                                                            \
    {                                                                             \
        pditest::StrArg pditest_a_ = pditest::strArg((const char *)(actual));     \
        pditest::StrArg pditest_e_ = pditest::strArg((const char *)(expected));   \
        if (!(pditest_a_ == pditest_e_))                                          \
        {                                                                         \
            char pditest_detail_[288];                                            \
            snprintf(pditest_detail_, sizeof(pditest_detail_),                    \
                     "expected \"%s\", got \"%s\"",                               \
                     pditest_e_.text(), pditest_a_.text());                       \
            PDITEST_REPORT_(#actual " == " #expected, pditest_detail_);           \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_STRNE(actual, expected)                                            \
    do                                                                            \
    {                                                                             \
        pditest::StrArg pditest_a_ = pditest::strArg((const char *)(actual));     \
        pditest::StrArg pditest_e_ = pditest::strArg((const char *)(expected));   \
        if (pditest_a_ == pditest_e_)                                             \
        {                                                                         \
            PDITEST_REPORT_(#actual " != " #expected, "strings are equal");       \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_MEMEQ(actual, expected, len)                                       \
    do                                                                            \
    {                                                                             \
        const void *pditest_a_ = (const void *)(actual);                          \
        const void *pditest_e_ = (const void *)(expected);                        \
        size_t pditest_n_ = (size_t)(len);                                        \
        if (0 != memcmp(pditest_a_, pditest_e_, pditest_n_))                      \
        {                                                                         \
            char pditest_abuf_[160];                                              \
            char pditest_ebuf_[160];                                              \
            char pditest_detail_[352];                                            \
            pditest::describeBytes(pditest_abuf_, sizeof(pditest_abuf_), pditest_a_, pditest_n_); \
            pditest::describeBytes(pditest_ebuf_, sizeof(pditest_ebuf_), pditest_e_, pditest_n_); \
            snprintf(pditest_detail_, sizeof(pditest_detail_), "expected %s, got %s",             \
                     pditest_ebuf_, pditest_abuf_);                               \
            PDITEST_REPORT_(#actual " == " #expected, pditest_detail_);           \
            return;                                                               \
        }                                                                         \
    } while (0)

#define ASSERT_NEAR(actual, expected, tolerance)                                  \
    do                                                                            \
    {                                                                             \
        double pditest_a_ = (double)(actual);                                     \
        double pditest_e_ = (double)(expected);                                   \
        double pditest_t_ = (double)(tolerance);                                  \
        double pditest_d_ = pditest_a_ - pditest_e_;                              \
        if (pditest_d_ < 0)                                                       \
        {                                                                         \
            pditest_d_ = -pditest_d_;                                             \
        }                                                                         \
        if (pditest_d_ > pditest_t_)                                              \
        {                                                                         \
            char pditest_detail_[288];                                            \
            snprintf(pditest_detail_, sizeof(pditest_detail_),                    \
                     "expected %f within %f, got %f", pditest_e_, pditest_t_, pditest_a_); \
            PDITEST_REPORT_(#actual " near " #expected, pditest_detail_);         \
            return;                                                               \
        }                                                                         \
    } while (0)

#define FAIL(reason)                    \
    do                                  \
    {                                   \
        PDITEST_REPORT_("FAIL", reason); \
        return;                         \
    } while (0)

#endif // _PDI_TEST_H_
