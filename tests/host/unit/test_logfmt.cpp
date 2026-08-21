/**************************** Format Helper Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The stack formats through its own __snprintf rather than the C library, so the
specifier set it really supports is pinned here.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/StringOperations.h>

static const char *format(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    memset(buf, 0, size);
    __vsnprintf(buf, (int)size, fmt, args);
    va_end(args);
    return buf;
}

TEST(logfmt, plain_text_passes_through)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "no specifiers here"), "no specifiers here");
}

TEST(logfmt, signed_decimal)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%d", -1234), "-1234");
    ASSERT_STREQ(format(buf, sizeof(buf), "%i", 5678), "5678");
}

TEST(logfmt, unsigned_decimal)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%u", 4294967295u), "4294967295");
}

TEST(logfmt, hexadecimal_in_both_cases)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%x", 0xabcdefu), "abcdef");
    ASSERT_STREQ(format(buf, sizeof(buf), "%X", 0xabcdefu), "ABCDEF");
}

TEST(logfmt, character)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%c", 'Z'), "Z");
}

TEST(logfmt, string)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%s", "pdiStack"), "pdiStack");
}

TEST(logfmt, a_null_string_argument_is_skipped)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%s]", (const char *)nullptr), "[]");
}

TEST(logfmt, float_renders_a_fractional_part)
{
    char buf[64];
    format(buf, sizeof(buf), "%f", 1.5);
    ASSERT_TRUE(strchr(buf, '.') != nullptr);
    ASSERT_EQ(buf[0], '1');
}

TEST(logfmt, a_doubled_percent_is_a_literal)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "100%%"), "100%");
}

TEST(logfmt, several_specifiers_in_one_pass)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%s=%d/%u/%x/%c", "k", -1, 2u, 255u, 'q'),
                 "k=-1/2/ff/q");
}

TEST(logfmt, width_right_justifies_by_default)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%5d]", 42), "[   42]");
}

TEST(logfmt, the_minus_flag_left_justifies)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%-5d]", 42), "[42   ]");
}

TEST(logfmt, the_zero_flag_pads_with_zeros)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%05d]", 42), "[00042]");
}

TEST(logfmt, zero_padding_keeps_a_negative_sign_leading)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%05d]", -42), "[-0042]");
}

TEST(logfmt, width_applies_to_strings)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%6s]", "ab"), "[    ab]");
    ASSERT_STREQ(format(buf, sizeof(buf), "[%-6s]", "ab"), "[ab    ]");
}

TEST(logfmt, a_width_narrower_than_the_value_does_not_truncate)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "[%2d]", 123456), "[123456]");
}

TEST(logfmt, the_long_modifier_is_accepted)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%ld", (long)-70000), "-70000");
    ASSERT_STREQ(format(buf, sizeof(buf), "%lu", (unsigned long)70000u), "70000");
}

/**
 * Precision is not implemented, so the run is echoed rather than applied. Pinned
 * so a caller reaching for %.2f sees why the output looks the way it does.
 */
TEST(logfmt, precision_is_not_supported_and_is_echoed)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%.2f", 1.5), "%.2f");
}

TEST(logfmt, an_unknown_specifier_is_echoed)
{
    char buf[64];
    ASSERT_STREQ(format(buf, sizeof(buf), "%q"), "%q");
}

TEST(logfmt, output_is_truncated_to_the_buffer)
{
    char buf[8];
    format(buf, sizeof(buf), "%s", "abcdefghijklmno");
    ASSERT_EQ(strlen(buf), (size_t)7);
}

TEST(logfmt, truncation_still_terminates_the_buffer)
{
    char buf[8];
    memset(buf, 0xAA, sizeof(buf));
    __snprintf(buf, (int)sizeof(buf), "%s", "abcdefghijklmno");
    ASSERT_EQ(buf[7], '\0');
}

TEST(logfmt, rejects_a_null_buffer_or_format)
{
    char buf[16];
    ASSERT_EQ(__snprintf(nullptr, 16, "%d", 1), 0);
    ASSERT_EQ(__snprintf(buf, 16, nullptr), 0);
}

TEST(logfmt, rejects_a_non_positive_size)
{
    char buf[16];
    ASSERT_EQ(__snprintf(buf, 0, "%d", 1), 0);
    ASSERT_EQ(__snprintf(buf, -1, "%d", 1), 0);
}

TEST(logfmt, sprintf_matches_snprintf_for_a_fitting_value)
{
    char viaSprintf[64];
    char viaSnprintf[64];

    memset(viaSprintf, 0, sizeof(viaSprintf));
    memset(viaSnprintf, 0, sizeof(viaSnprintf));

    __sprintf(viaSprintf, "%s-%d", "id", 9);
    __snprintf(viaSnprintf, (int)sizeof(viaSnprintf), "%s-%d", "id", 9);

    ASSERT_STREQ(viaSprintf, viaSnprintf);
}

TEST(logfmt, an_empty_format_yields_an_empty_string)
{
    char buf[16];
    ASSERT_STREQ(format(buf, sizeof(buf), ""), "");
}

TEST(logfmt, a_trailing_percent_does_not_run_off_the_format)
{
    char buf[16];
    format(buf, sizeof(buf), "done%");
    ASSERT_EQ(strncmp(buf, "done", 4), 0);
}
