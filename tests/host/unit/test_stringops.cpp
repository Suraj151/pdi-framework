/************************ String Operations Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/StringOperations.h>

TEST(stringops, strstr_finds_substring_at_start)
{
    ASSERT_EQ(__strstr("hello world", "hello"), 0);
}

TEST(stringops, strstr_finds_substring_in_middle)
{
    ASSERT_EQ(__strstr("hello world", "wor"), 6);
}

TEST(stringops, strstr_reports_missing_substring)
{
    ASSERT_EQ(__strstr("hello world", "worlds"), -1);
}

TEST(stringops, strstr_rejects_null_and_empty)
{
    ASSERT_EQ(__strstr(nullptr, "a"), -1);
    ASSERT_EQ(__strstr("a", nullptr), -1);
    ASSERT_EQ(__strstr("", "a"), -1);
    ASSERT_EQ(__strstr("a", ""), -1);
}

TEST(stringops, strstr_honours_search_length)
{
    ASSERT_EQ(__strstr("abcdefghij", "hij", 3), -1);
}

TEST(stringops, strstr_sized_finds_from_offset)
{
    const char *hay = "abcabcabc";
    ASSERT_EQ(__strstr(hay, 9, "abc", 3, 0), 0);
    ASSERT_EQ(__strstr(hay, 9, "abc", 3, 1), 3);
    ASSERT_EQ(__strstr(hay, 9, "abc", 3, 4), 6);
}

TEST(stringops, strstr_sized_handles_embedded_nul)
{
    const char hay[] = {'a', '\0', 'b', 'c'};
    const char needle[] = {'\0', 'b'};
    ASSERT_EQ(__strstr(hay, 4, needle, 2, 0), 1);
}

TEST(stringops, strstr_sized_rejects_needle_longer_than_haystack)
{
    ASSERT_EQ(__strstr("ab", 2, "abc", 3, 0), -1);
}

TEST(stringops, strtrim_removes_surrounding_spaces)
{
    char buf[32];
    strcpy(buf, "   padded   ");
    ASSERT_STREQ(__strtrim(buf), "padded");
}

TEST(stringops, strtrim_leaves_inner_spaces)
{
    char buf[32];
    strcpy(buf, "  a b  ");
    ASSERT_STREQ(__strtrim(buf), "a b");
}

TEST(stringops, strtrim_on_all_spaces_yields_empty)
{
    char buf[32];
    strcpy(buf, "     ");
    ASSERT_STREQ(__strtrim(buf), "");
}

TEST(stringops, strtrim_rejects_null_and_empty)
{
    char empty[1] = {0};
    ASSERT_NULL(__strtrim(nullptr));
    ASSERT_NULL(__strtrim(empty));
}

TEST(stringops, strtrim_val_removes_given_character)
{
    char buf[32];
    strcpy(buf, "\"quoted\"");
    ASSERT_STREQ(__strtrim_val(buf, '"'), "quoted");
}

TEST(stringops, are_str_equals_matches_identical_strings)
{
    ASSERT_TRUE(__are_str_equals("pdiStack", "pdiStack"));
}

TEST(stringops, are_str_equals_rejects_different_length)
{
    ASSERT_FALSE(__are_str_equals("pdi", "pdiStack"));
}

TEST(stringops, are_str_equals_rejects_null)
{
    ASSERT_FALSE(__are_str_equals(nullptr, "a"));
    ASSERT_FALSE(__are_str_equals("a", nullptr));
}

TEST(stringops, are_arrays_equal_compares_fixed_length)
{
    const char a[] = {1, 2, 3, 4};
    const char b[] = {1, 2, 3, 9};
    ASSERT_TRUE(__are_arrays_equal(a, b, 3));
    ASSERT_FALSE(__are_arrays_equal(a, b, 4));
}

TEST(stringops, int_ip_to_str_formats_dotted_quad)
{
    uint8_t ip[4] = {192, 168, 0, 1};
    char buf[16];
    __int_ip_to_str(buf, ip, sizeof(buf));
    ASSERT_STREQ(buf, "192.168.0.1");
}

TEST(stringops, int_ip_to_str_formats_all_max_octets)
{
    uint8_t ip[4] = {255, 255, 255, 255};
    char buf[16];
    __int_ip_to_str(buf, ip, sizeof(buf));
    ASSERT_STREQ(buf, "255.255.255.255");
}

TEST(stringops, str_ip_to_int_parses_dotted_quad)
{
    char text[16];
    strcpy(text, "10.0.13.240");
    uint8_t ip[4] = {0, 0, 0, 0};
    __str_ip_to_int(text, ip, sizeof(text), false);
    ASSERT_EQ(ip[0], 10);
    ASSERT_EQ(ip[1], 0);
    ASSERT_EQ(ip[2], 13);
    ASSERT_EQ(ip[3], 240);
}

TEST(stringops, ip_conversion_round_trips)
{
    uint8_t original[4] = {172, 16, 254, 3};
    char buf[16];
    __int_ip_to_str(buf, original, sizeof(buf));

    uint8_t parsed[4] = {0, 0, 0, 0};
    __str_ip_to_int(buf, parsed, sizeof(buf), false);
    ASSERT_MEMEQ(parsed, original, 4);
}

TEST(stringops, tolowercase_converts_in_place)
{
    char buf[16];
    strcpy(buf, "PdiStack1");
    __tolowercase(buf, strlen(buf));
    ASSERT_STREQ(buf, "pdistack1");
}

TEST(stringops, touppercase_converts_in_place)
{
    char buf[16];
    strcpy(buf, "PdiStack1");
    __touppercase(buf, strlen(buf));
    ASSERT_STREQ(buf, "PDISTACK1");
}

TEST(stringops, find_and_replace_swaps_shorter_text)
{
    char buf[64];
    strcpy(buf, "hello cruel world");
    __find_and_replace(buf, "cruel ", "", 1);
    ASSERT_STREQ(buf, "hello world");
}

TEST(stringops, find_and_replace_honours_occurrence_count)
{
    char buf[64];
    strcpy(buf, "a-a-a-a");
    __find_and_replace(buf, "-a", "", 2);
    ASSERT_STREQ(buf, "a-a");
}

TEST(stringops, find_and_replace_swaps_longer_text)
{
    char buf[64];
    strcpy(buf, "client-[mac]");
    __find_and_replace(buf, "[mac]", "A0:B7:65:12:34:56", 1, sizeof(buf));
    ASSERT_STREQ(buf, "client-A0:B7:65:12:34:56");
}

TEST(stringops, find_and_replace_swaps_same_length_text)
{
    char buf[64];
    strcpy(buf, "device-[id]");
    __find_and_replace(buf, "[id]", "0042", 1, sizeof(buf));
    ASSERT_STREQ(buf, "device-0042");
}

TEST(stringops, find_and_replace_replaces_every_occurrence_asked_for)
{
    char buf[64];
    strcpy(buf, "[m]/x/[m]");
    __find_and_replace(buf, "[m]", "aabbcc", 2, sizeof(buf));
    ASSERT_STREQ(buf, "aabbcc/x/aabbcc");
}

TEST(stringops, find_and_replace_leaves_text_that_does_not_fit)
{
    char buf[12];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "a-[mac]");
    __find_and_replace(buf, "[mac]", "A0:B7:65:12:34:56", 1, sizeof(buf));
    ASSERT_STREQ(buf, "a-[mac]");
}

TEST(stringops, find_and_replace_fills_the_buffer_exactly)
{
    char buf[8];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "a[x]");
    __find_and_replace(buf, "[x]", "bcdefg", 1, sizeof(buf));
    ASSERT_STREQ(buf, "abcdefg");
}

TEST(stringops, find_and_replace_without_a_capacity_only_shrinks)
{
    char buf[64];
    strcpy(buf, "hello cruel world");
    __find_and_replace(buf, "cruel ", "", 1);
    ASSERT_STREQ(buf, "hello world");

    strcpy(buf, "client-[mac]");
    __find_and_replace(buf, "[mac]", "A0:B7:65:12:34:56", 1);
    ASSERT_STREQ(buf, "client-[mac]");
}

TEST(stringops, find_and_replace_rejects_null)
{
    char buf[8];
    strcpy(buf, "abc");
    __find_and_replace(buf, nullptr, "x", 1);
    ASSERT_STREQ(buf, "abc");
}

TEST(stringops, get_from_json_reads_string_value)
{
    char value[32];
    ASSERT_TRUE(__get_from_json("{\"ssid\":\"pdiStack\",\"ch\":6}", "ssid", value, sizeof(value)));
    ASSERT_STREQ(value, "pdiStack");
}

TEST(stringops, get_from_json_reads_numeric_value)
{
    char value[32];
    ASSERT_TRUE(__get_from_json("{\"ssid\":\"pdiStack\",\"ch\":6}", "ch", value, sizeof(value)));
    ASSERT_STREQ(value, "6");
}

TEST(stringops, get_from_json_reads_nested_object)
{
    char value[64];
    ASSERT_TRUE(__get_from_json("{\"a\":1,\"b\":{\"c\":2},\"d\":3}", "b", value, sizeof(value)));
    ASSERT_STREQ(value, "{\"c\":2}");
}

TEST(stringops, get_from_json_reports_missing_key)
{
    char value[32];
    ASSERT_FALSE(__get_from_json("{\"a\":1}", "b", value, sizeof(value)));
}

TEST(stringops, get_from_json_rejects_null)
{
    char value[8];
    ASSERT_FALSE(__get_from_json(nullptr, "a", value, sizeof(value)));
    ASSERT_FALSE(__get_from_json("{}", nullptr, value, sizeof(value)));
    ASSERT_FALSE(__get_from_json("{}", "a", nullptr, 8));
}

TEST(stringops, append_uint_to_buff_substitutes_specifier)
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
    __appendUintToBuff(buf, "/gpio%d", 13, sizeof(buf));
    ASSERT_STREQ(buf, "/gpio13");
}

TEST(stringops, snprintf_formats_supported_specifiers)
{
    char buf[64];
    memset(buf, 0, sizeof(buf));
    __snprintf(buf, sizeof(buf), "%s=%d,%u,%x,%c", "v", -12, 34u, 255u, 'z');
    ASSERT_STREQ(buf, "v=-12,34,ff,z");
}

TEST(stringops, snprintf_truncates_within_buffer)
{
    char buf[8];
    memset(buf, 0xAA, sizeof(buf));
    __snprintf(buf, sizeof(buf), "%s", "abcdefghijkl");
    ASSERT_EQ(strlen(buf), (size_t)7);
    ASSERT_EQ(buf[7], '\0');
}
