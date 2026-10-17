/*********************** Data Type Conversion Tests ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeConversions.h>

TEST(dataconv, bcd_round_trips)
{
    for (uint8_t v = 0; v < 100; v++)
    {
        ASSERT_EQ(BcdToUint8(Uint8ToBcd(v)), v);
    }
}

TEST(dataconv, string_to_uint32_parses_decimal)
{
    ASSERT_EQ(StringToUint32("12345"), 12345u);
}

TEST(dataconv, string_to_uint32_skips_leading_zeros_spaces_and_quotes)
{
    ASSERT_EQ(StringToUint32("\" 007\""), 7u);
}

TEST(dataconv, string_to_uint32_stops_at_first_non_digit)
{
    ASSERT_EQ(StringToUint32("42abc"), 42u);
}

TEST(dataconv, string_to_uint32_handles_null_and_empty)
{
    ASSERT_EQ(StringToUint32(nullptr), 0u);
    ASSERT_EQ(StringToUint32(""), 0u);
}

TEST(dataconv, string_to_uint32_honours_length_limit)
{
    ASSERT_EQ(StringToUint32("123456", 3), 123u);
}

TEST(dataconv, string_to_uint32_reads_full_range)
{
    ASSERT_EQ(StringToUint32("4294967295"), 4294967295u);
}

TEST(dataconv, string_to_uint64_reads_beyond_32_bits)
{
    ASSERT_EQ(StringToUint64("4294967296"), (uint64_t)4294967296ULL);
}

TEST(dataconv, string_to_uint16_and_uint8_truncate_to_width)
{
    ASSERT_EQ(StringToUint16("65535"), (uint16_t)65535);
    ASSERT_EQ(StringToUint8("255"), (uint8_t)255);
}

TEST(dataconv, string_to_octal_parses_permission_form)
{
    ASSERT_EQ(StringToOctalUint16("644", 3), (uint16_t)0644);
    ASSERT_EQ(StringToOctalUint16("755", 3), (uint16_t)0755);
}

TEST(dataconv, string_to_hex16_parses_both_cases)
{
    ASSERT_EQ(StringToHex16("00ff", 4), (uint16_t)0x00ff);
    ASSERT_EQ(StringToHex16("00FF", 4), (uint16_t)0x00ff);
}

TEST(dataconv, uint32_to_string_formats_value)
{
    char buf[16];
    Uint32ToString(1234567u, buf, sizeof(buf));
    ASSERT_STREQ(buf, "1234567");
}

TEST(dataconv, uint32_to_string_right_pads_to_column_width)
{
    char buf[16];
    Uint32ToString(42u, buf, sizeof(buf), 5);
    ASSERT_STREQ(buf, "42   ");
}

TEST(dataconv, uint32_to_string_ignores_padding_below_value_width)
{
    char buf[16];
    Uint32ToString(123456u, buf, sizeof(buf), 3);
    ASSERT_STREQ(buf, "123456");
}

TEST(dataconv, uint32_to_string_round_trips)
{
    const uint32_t values[] = {0u, 1u, 9u, 10u, 99u, 100u, 65535u, 4294967295u};
    char buf[16];

    for (uint8_t i = 0; i < (sizeof(values) / sizeof(values[0])); i++)
    {
        Uint32ToString(values[i], buf, sizeof(buf));
        ASSERT_EQ(StringToUint32(buf), values[i]);
    }
}

TEST(dataconv, int32_to_string_keeps_sign)
{
    char buf[16];
    Int32ToString(-4096, buf, sizeof(buf));
    ASSERT_STREQ(buf, "-4096");
}

TEST(dataconv, int64_to_string_covers_wide_values)
{
    char buf[24];
    Int64ToString(-1234567890123LL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "-1234567890123");
}

TEST(dataconv, uint32_to_hex_string_formats_both_cases)
{
    char lower[16];
    char upper[16];
    Uint32ToHexString(0xdeadbeefu, lower, sizeof(lower), false);
    Uint32ToHexString(0xdeadbeefu, upper, sizeof(upper), true);
    ASSERT_STREQ(lower, "deadbeef");
    ASSERT_STREQ(upper, "DEADBEEF");
}

TEST(dataconv, bytes_to_hex_string_formats_each_byte)
{
    const uint8_t bytes[] = {0x00, 0x0f, 0xa5, 0xff};
    char buf[16];
    BytesToHexString(bytes, sizeof(bytes), buf);
    ASSERT_STREQ(buf, "000fa5ff");
}

TEST(dataconv, hex_string_to_bytes_parses_pairs)
{
    uint8_t out[4] = {0, 0, 0, 0};
    const uint8_t expected[] = {0x00, 0x0f, 0xa5, 0xff};
    ASSERT_TRUE(HexStringToBytes("000fa5ff", 4, out));
    ASSERT_MEMEQ(out, expected, 4);
}

TEST(dataconv, hex_bytes_round_trip)
{
    const uint8_t original[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    char text[32];
    uint8_t parsed[8] = {0};

    BytesToHexString(original, sizeof(original), text);
    ASSERT_TRUE(HexStringToBytes(text, sizeof(original), parsed));
    ASSERT_MEMEQ(parsed, original, sizeof(original));
}

TEST(dataconv, hex_string_to_bytes_rejects_non_hex)
{
    uint8_t out[2] = {0, 0};
    ASSERT_FALSE(HexStringToBytes("zz00", 2, out));
}

TEST(dataconv, file_perms_to_string_renders_regular_file)
{
    char buf[12];
    FilePermsToString(0644, false, buf);
    ASSERT_STREQ(buf, "-rw-r--r--");
}

TEST(dataconv, file_perms_to_string_renders_directory)
{
    char buf[12];
    FilePermsToString(0755, true, buf);
    ASSERT_STREQ(buf, "drwxr-xr-x");
}

TEST(dataconv, file_perms_to_string_renders_no_permissions)
{
    char buf[12];
    FilePermsToString(0, false, buf);
    ASSERT_STREQ(buf, "----------");
}

TEST(dataconv, float_to_string_formats_fraction)
{
    char buf[24];
    FloatToString(3.25, buf, sizeof(buf));
    ASSERT_EQ(StringToUint32(buf), 3u);
    ASSERT_TRUE(strchr(buf, '.') != nullptr);
}

TEST(dataconv, int32_digit_count_counts_digits)
{
    ASSERT_EQ(Int32DigitCount(0), (uint8_t)1);
    ASSERT_EQ(Int32DigitCount(9), (uint8_t)1);
    ASSERT_EQ(Int32DigitCount(10), (uint8_t)2);
    ASSERT_EQ(Int32DigitCount(999), (uint8_t)3);
}

TEST(dataconv, int64_digit_count_counts_wide_digits)
{
    ASSERT_EQ(Int64DigitCount(1000000000000LL), (uint8_t)13);
}

TEST(dataconv, epoch_breaks_down_into_calendar_fields)
{
    datetime_t dt;

    // 2024-02-29T13:45:07Z, a leap day so the month arithmetic is exercised
    EpochToDateTime(1709214307u, dt);
    ASSERT_EQ((int)dt.m_year, 2024);
    ASSERT_EQ((int)dt.m_month, 2);
    ASSERT_EQ((int)dt.m_day, 29);
    ASSERT_EQ((int)dt.m_hour, 13);
    ASSERT_EQ((int)dt.m_minute, 45);
    ASSERT_EQ((int)dt.m_second, 7);
}

TEST(dataconv, epoch_zero_is_the_start_of_1970)
{
    datetime_t dt;
    EpochToDateTime(0u, dt);
    ASSERT_EQ((int)dt.m_year, 1970);
    ASSERT_EQ((int)dt.m_month, 1);
    ASSERT_EQ((int)dt.m_day, 1);
    ASSERT_EQ((int)dt.m_hour, 0);
    ASSERT_EQ((int)dt.m_minute, 0);
    ASSERT_EQ((int)dt.m_second, 0);

    // the epoch fell on a Thursday, which is what anchors every weekday below
    ASSERT_EQ((int)dt.m_weekday, 4);
}

TEST(dataconv, weekday_is_sunday_zero_across_a_whole_week)
{
    // 2024-03-03 was a Sunday; walk the seven days after it
    const int expected[7] = {0, 1, 2, 3, 4, 5, 6};

    for (int i = 0; i < 7; i++)
    {
        datetime_t dt;
        EpochToDateTime(1709424000u + (uint32_t)i * 86400u, dt);
        ASSERT_EQ((int)dt.m_weekday, expected[i]);
        ASSERT_EQ((int)dt.m_day, 3 + i);
    }
}

TEST(dataconv, the_string_formatter_agrees_with_the_breakdown)
{
    // the formatter is built on the same breakdown now, so a disagreement
    // would mean the refactor changed what a timestamp reads as
    const uint32_t samples[4] = {1u, 951782400u, 1709214307u, 2147483647u};

    for (int i = 0; i < 4; i++)
    {
        datetime_t dt;
        EpochToDateTime(samples[i], dt);

        char text[24];
        EpochToDateTimeString(samples[i], text, sizeof(text), "%Y-%m-%d %H:%M:%S");

        char built[24];
        snprintf(built, sizeof(built), "%04u-%02u-%02u %02u:%02u:%02u",
                 (unsigned)dt.m_year, (unsigned)dt.m_month, (unsigned)dt.m_day,
                 (unsigned)dt.m_hour, (unsigned)dt.m_minute, (unsigned)dt.m_second);

        ASSERT_STREQ(text, built);
    }
}

TEST(dataconv, a_space_padded_day_pads_a_single_digit_with_a_space)
{
    char single[24];
    char twodigit[24];
    char unknown[24];
    EpochToDateTimeString(1759968000u, single, sizeof(single), "%b %e %H:%M:%S");
    EpochToDateTimeString(1760400000u, twodigit, sizeof(twodigit), "%b %e %H:%M:%S");
    EpochToDateTimeString(0u, unknown, sizeof(unknown), "%b %e %H:%M:%S");

    ASSERT_STREQ(single, "Oct  9 00:00:00");
    ASSERT_STREQ(twodigit, "Oct 14 00:00:00");
    ASSERT_EQ(strlen(unknown), (size_t)15);
}
