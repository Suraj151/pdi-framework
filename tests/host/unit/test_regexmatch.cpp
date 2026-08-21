/**************************** Regex Match Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/RegexMatch.h>

/**
 * regex_match answers with the index the match starts at, or -1 when there is
 * none, so a hit at the very start is zero rather than a truthy value
 */
static bool matches(const char *pattern, const char *text)
{
    return regex_match(pattern, text, (int)strlen(text)) >= 0;
}

TEST(regex, literal_matches_substring)
{
    ASSERT_TRUE(matches("world", "hello world"));
}

TEST(regex, literal_rejects_absent_text)
{
    ASSERT_FALSE(matches("worlds", "hello world"));
}

TEST(regex, dot_matches_any_character)
{
    ASSERT_TRUE(matches("h.llo", "hello"));
    ASSERT_TRUE(matches("h.llo", "hallo"));
}

TEST(regex, star_matches_zero_or_more)
{
    ASSERT_TRUE(matches("ab*c", "ac"));
    ASSERT_TRUE(matches("ab*c", "abc"));
    ASSERT_TRUE(matches("ab*c", "abbbbc"));
}

TEST(regex, plus_requires_at_least_one)
{
    ASSERT_FALSE(matches("^ab+c$", "ac"));
    ASSERT_TRUE(matches("^ab+c$", "abc"));
    ASSERT_TRUE(matches("^ab+c$", "abbc"));
}

TEST(regex, question_matches_zero_or_one)
{
    ASSERT_TRUE(matches("^ab?c$", "ac"));
    ASSERT_TRUE(matches("^ab?c$", "abc"));
    ASSERT_FALSE(matches("^ab?c$", "abbc"));
}

TEST(regex, caret_anchors_at_start)
{
    ASSERT_TRUE(matches("^hello", "hello world"));
    ASSERT_FALSE(matches("^world", "hello world"));
}

TEST(regex, dollar_anchors_at_end)
{
    ASSERT_TRUE(matches("world$", "hello world"));
    ASSERT_FALSE(matches("hello$", "hello world"));
}

TEST(regex, both_anchors_require_exact_match)
{
    ASSERT_TRUE(matches("^hello$", "hello"));
    ASSERT_FALSE(matches("^hello$", "hello world"));
}

TEST(regex, character_class_matches_any_member)
{
    ASSERT_TRUE(matches("^[abc]$", "b"));
    ASSERT_FALSE(matches("^[abc]$", "d"));
}

TEST(regex, character_range_matches_span)
{
    ASSERT_TRUE(matches("^[a-z]+$", "lowercase"));
    ASSERT_FALSE(matches("^[a-z]+$", "hasUpper"));
    ASSERT_TRUE(matches("^[0-9]+$", "12345"));
}

TEST(regex, negated_class_excludes_members)
{
    ASSERT_TRUE(matches("^[^0-9]+$", "letters"));
    ASSERT_FALSE(matches("^[^0-9]+$", "letters1"));
}

TEST(regex, escape_takes_next_character_literally)
{
    ASSERT_TRUE(matches("^a\\.c$", "a.c"));
    ASSERT_FALSE(matches("^a\\.c$", "abc"));
}

TEST(regex, empty_pattern_matches_anything)
{
    ASSERT_TRUE(matches("", "anything"));
}

TEST(regex, pattern_longer_than_text_fails)
{
    ASSERT_FALSE(matches("^abcdef$", "abc"));
}

TEST(regex, honours_supplied_text_length)
{
    ASSERT_EQ(regex_match("world$", "hello world", 5), -1);
    ASSERT_EQ(regex_match("hello$", "hello world", 5), 0);
}

TEST(regex, reports_index_of_the_match)
{
    ASSERT_EQ(regex_match("world", "hello world", 11), 6);
    ASSERT_EQ(regex_match("hello", "hello world", 11), 0);
}

TEST(regex, rejects_null_arguments)
{
    ASSERT_EQ(regex_match(nullptr, "text", 4), -1);
    ASSERT_EQ(regex_match("pattern", nullptr, 4), -1);
}

TEST(regex, matches_binary_text_with_embedded_nul)
{
    const char text[] = {'a', '\0', 'b', 'c'};
    ASSERT_EQ(regex_match("bc$", text, 4), 2);
}
