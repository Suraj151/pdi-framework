/************************ String Operations Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/StringOperations.h>
#include <utility/DataTypeConversions.h>

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

TEST(stringops, get_from_json_matches_the_whole_key_not_a_part_of_another)
{
    char value[32];
    ASSERT_TRUE(__get_from_json("{\"data\":\"z\",\"a\":1}", "a", value, sizeof(value)));
    ASSERT_STREQ(value, "1");

    ASSERT_TRUE(__get_from_json("{\"D10\":7,\"D1\":3}", "D1", value, sizeof(value)));
    ASSERT_STREQ(value, "3");

    ASSERT_FALSE(__get_from_json("{\"D10\":7}", "D1", value, sizeof(value)));
}

TEST(stringops, get_from_json_skips_the_key_written_inside_a_value)
{
    char value[32];
    ASSERT_TRUE(__get_from_json("{\"note\":\"latest: 9\",\"latest\":\"2\"}", "latest", value, sizeof(value)));
    ASSERT_STREQ(value, "2");
}

TEST(stringops, get_from_json_allows_space_between_the_key_and_its_colon)
{
    char value[32];
    ASSERT_TRUE(__get_from_json("{\"a\" : 5}", "a", value, sizeof(value)));
    ASSERT_STREQ(value, "5");
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

TEST(stringops, strstr_unquoted_skips_a_double_quoted_run)
{
    // the separator inside the quotes belongs to the value, not to the scan
    ASSERT_EQ(__strstr_unquoted("\"a,b\",i=3", ","), 5);
}

TEST(stringops, strstr_unquoted_skips_a_single_quoted_run)
{
    ASSERT_EQ(__strstr_unquoted("'a,b',i=3", ","), 5);
}

TEST(stringops, strstr_unquoted_finds_an_unquoted_separator)
{
    ASSERT_EQ(__strstr_unquoted("a,b", ","), 1);
}

TEST(stringops, strstr_unquoted_reports_a_fully_quoted_run_as_missing)
{
    ASSERT_EQ(__strstr_unquoted("\"a,b\"", ","), -1);
}

TEST(stringops, strstr_unquoted_does_not_end_a_run_on_the_other_quote)
{
    // a quote of the other kind is text inside the run
    ASSERT_EQ(__strstr_unquoted("\"a'b,c\",d", ","), 7);
}

TEST(stringops, append_padded_fills_out_to_the_column)
{
    pdiutil::string out;
    __append_padded(out, "ab", 5);
    ASSERT_TRUE(out == pdiutil::string("ab   "));
}

TEST(stringops, append_padded_can_right_align)
{
    pdiutil::string out;
    __append_padded(out, "ab", 5, true);
    ASSERT_TRUE(out == pdiutil::string("   ab"));
}

TEST(stringops, append_padded_truncates_an_overlong_field)
{
    // the property that keeps a table lined up, and the one that silently
    // clipped a proc key when its column was left too narrow
    pdiutil::string out;
    __append_padded(out, "MemMaxBlock:", 10);
    ASSERT_TRUE(out == pdiutil::string("MemMaxBloc"));
    ASSERT_EQ((int)out.length(), 10);
}

TEST(stringops, append_padded_takes_the_whole_field_at_exactly_the_column)
{
    pdiutil::string out;
    __append_padded(out, "abcde", 5);
    ASSERT_TRUE(out == pdiutil::string("abcde"));
}

TEST(stringops, append_padded_treats_null_as_an_empty_column)
{
    pdiutil::string out;
    __append_padded(out, nullptr, 3);
    ASSERT_TRUE(out == pdiutil::string("   "));
}

TEST(stringops, append_padded_uses_the_given_pad_character)
{
    pdiutil::string out;
    __append_padded(out, "7", 4, true, '0');
    ASSERT_TRUE(out == pdiutil::string("0007"));
}

TEST(stringops, append_padded_num_writes_a_number_in_the_column)
{
    pdiutil::string out;
    __append_padded_num(out, 42, 5);
    ASSERT_TRUE(out == pdiutil::string("42   "));
}

TEST(stringops, append_padded_num_carries_a_negative_value)
{
    pdiutil::string out;
    __append_padded_num(out, -7, 5);
    ASSERT_TRUE(out == pdiutil::string("-7   "));
}

TEST(stringops, append_padded_appends_rather_than_replacing)
{
    pdiutil::string out = "x";
    __append_padded(out, "y", 3);
    __append_padded(out, "z", 2);
    ASSERT_TRUE(out == pdiutil::string("xy  z "));
}

TEST(conversions, mac_string_defaults_to_uppercase_and_colons)
{
    const uint8_t mac[6] = {0x84, 0x0D, 0x8E, 0xBF, 0xF5, 0x49};
    ASSERT_TRUE(BytesToMacString(mac, 6) == pdiutil::string("84:0D:8E:BF:F5:49"));
}

TEST(conversions, mac_string_honours_lowercase)
{
    const uint8_t mac[6] = {0x84, 0x0D, 0x8E, 0xBF, 0xF5, 0x49};
    ASSERT_TRUE(BytesToMacString(mac, 6, ':', false) == pdiutil::string("84:0d:8e:bf:f5:49"));
}

TEST(conversions, mac_string_honours_another_separator)
{
    const uint8_t mac[6] = {0x84, 0x0D, 0x8E, 0xBF, 0xF5, 0x49};
    ASSERT_TRUE(BytesToMacString(mac, 6, '-') == pdiutil::string("84-0D-8E-BF-F5-49"));
}

TEST(conversions, mac_string_joins_nothing_when_separator_is_zero)
{
    const uint8_t mac[3] = {0xBF, 0xF5, 0x49};
    ASSERT_TRUE(BytesToMacString(mac, 3, 0) == pdiutil::string("BFF549"));
}

TEST(conversions, mac_string_keeps_leading_zero_of_a_byte)
{
    const uint8_t mac[2] = {0x00, 0x0A};
    ASSERT_TRUE(BytesToMacString(mac, 2) == pdiutil::string("00:0A"));
}

TEST(conversions, mac_string_is_empty_for_nothing_to_render)
{
    const uint8_t mac[1] = {0x01};
    ASSERT_TRUE(BytesToMacString(mac, 0).empty());
    ASSERT_TRUE(BytesToMacString(nullptr, 6).empty());
}

TEST(conversions, micros_render_as_seconds_and_six_digits)
{
    char buf[24];
    MicrosToTimeString(4201489ULL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "4.201489");
}

TEST(conversions, micros_below_a_second_keep_their_leading_zeros)
{
    char buf[24];
    MicrosToTimeString(33708ULL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "0.033708");
}

TEST(conversions, micros_fraction_length_is_selectable)
{
    char buf[24];
    MicrosToTimeString(4201489ULL, buf, sizeof(buf), 3);
    ASSERT_STREQ(buf, "4.201");
    MicrosToTimeString(4201489ULL, buf, sizeof(buf), 0);
    ASSERT_STREQ(buf, "4");
}

TEST(conversions, micros_render_beyond_a_32_bit_count)
{
    char buf[24];
    MicrosToTimeString(9000000000ULL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "9000.000000");
}

TEST(conversions, uint64_renders_past_the_32_bit_ceiling)
{
    char buf[24];
    Uint64ToString(4294967296ULL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "4294967296");
    Uint64ToString(0ULL, buf, sizeof(buf));
    ASSERT_STREQ(buf, "0");
}

TEST(stringops, copy_to_buffer_replaces_a_longer_value_whole)
{
    char buf[16];
    memset(buf, 0, sizeof(buf));
    ASSERT_TRUE(__copy_to_buffer(buf, sizeof(buf), "a-longer-value", 14));
    ASSERT_TRUE(__copy_to_buffer(buf, sizeof(buf), "ab", 2));

    ASSERT_STREQ(buf, "ab");
    for (size_t i = 2; i < sizeof(buf); i++)
    {
        ASSERT_EQ(buf[i], '\0');
    }
}

TEST(stringops, copy_to_buffer_refuses_text_that_leaves_no_room_for_the_terminator)
{
    char buf[8];
    memcpy(buf, "keepme!", 8);

    ASSERT_FALSE(__copy_to_buffer(buf, sizeof(buf), "12345678", 8));
    ASSERT_FALSE(__copy_to_buffer(buf, sizeof(buf), "123456789", 9));
    ASSERT_STREQ(buf, "keepme!");

    ASSERT_TRUE(__copy_to_buffer(buf, sizeof(buf), "1234567", 7));
    ASSERT_STREQ(buf, "1234567");
}

TEST(stringops, copy_to_buffer_clears_the_field_on_empty_text)
{
    char buf[8];
    memcpy(buf, "old", 4);

    ASSERT_TRUE(__copy_to_buffer(buf, sizeof(buf), "", 0));
    ASSERT_STREQ(buf, "");
    ASSERT_TRUE(__copy_to_buffer(buf, sizeof(buf), nullptr, 0));
}

TEST(stringops, copy_to_buffer_refuses_what_it_cannot_copy)
{
    char buf[8];
    memcpy(buf, "same", 5);

    ASSERT_FALSE(__copy_to_buffer(nullptr, 8, "x", 1));
    ASSERT_FALSE(__copy_to_buffer(buf, sizeof(buf), nullptr, 3));
    ASSERT_FALSE(__copy_to_buffer(buf, 0, "", 0));
    ASSERT_STREQ(buf, "same");
}

TEST(stringops, json_escaped_leaves_plain_text_alone)
{
    pdiutil::string out = "[";
    __append_json_escaped(out, "home-net 5G");
    ASSERT_STREQ(out.c_str(), "[home-net 5G");
}

TEST(stringops, json_escaped_escapes_quotes_and_backslashes)
{
    pdiutil::string out;
    __append_json_escaped(out, "my \"home\" \\ net");
    ASSERT_STREQ(out.c_str(), "my \\\"home\\\" \\\\ net");
}

TEST(stringops, json_escaped_writes_control_characters_as_unicode_escapes)
{
    pdiutil::string out;
    __append_json_escaped(out, "a\nb\tc\x1f");
    ASSERT_STREQ(out.c_str(), "a\\u000ab\\u0009c\\u001f");
}

TEST(stringops, json_escaped_passes_utf8_through_whole)
{
    pdiutil::string out;
    __append_json_escaped(out, "caf\xc3\xa9");
    ASSERT_STREQ(out.c_str(), "caf\xc3\xa9");
}

TEST(stringops, json_escaped_of_nothing_appends_nothing)
{
    pdiutil::string out = "x";
    __append_json_escaped(out, nullptr);
    __append_json_escaped(out, "");
    ASSERT_STREQ(out.c_str(), "x");
}

TEST(stringops, split_keeps_inner_empty_pieces_and_drops_an_empty_last_one)
{
    pdiutil::vector<pdiutil::string> parts;
    __split(pdiutil::string("aes128-ctr,,hmac-sha2-256,"), ',', parts);
    ASSERT_EQ((int)parts.size(), 3);
    ASSERT_STREQ(parts[0].c_str(), "aes128-ctr");
    ASSERT_STREQ(parts[1].c_str(), "");
    ASSERT_STREQ(parts[2].c_str(), "hmac-sha2-256");

    pdiutil::vector<pdiutil::string> none;
    __split(pdiutil::string(""), ',', none);
    ASSERT_EQ((int)none.size(), 0);
}

TEST(stringops, a_list_contains_only_whole_pieces)
{
    pdiutil::string options = "defaults,nofail,rw";
    ASSERT_TRUE(__list_contains(options, ',', pdiutil::string("nofail")));
    ASSERT_TRUE(__list_contains(options, ',', pdiutil::string("rw")));
    ASSERT_TRUE(__list_contains(options, ',', pdiutil::string("defaults")));
    ASSERT_FALSE(__list_contains(options, ',', pdiutil::string("fail")));
    ASSERT_FALSE(__list_contains(options, ',', pdiutil::string("noauto")));
    ASSERT_FALSE(__list_contains(pdiutil::string(""), ',', pdiutil::string("rw")));
}

TEST(stringops, strip_trailing_keeps_a_lone_root)
{
    pdiutil::string dir = "/mnt///";
    __strip_trailing(dir, '/');
    ASSERT_STREQ(dir.c_str(), "/mnt");
    pdiutil::string root = "/";
    __strip_trailing(root, '/');
    ASSERT_STREQ(root.c_str(), "/");
    pdiutil::string plain = "/etc";
    __strip_trailing(plain, '/');
    ASSERT_STREQ(plain.c_str(), "/etc");
}
