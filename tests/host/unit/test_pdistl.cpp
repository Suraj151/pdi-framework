/******************************* PdiSTL Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The stack builds against its own embedded STL, so the containers everything
above it relies on are pinned here rather than assumed to match the host one.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeDef.h>
#include <utility/Utility.h>
#include <utility/pdistl/map>

TEST(pdistl, string_starts_empty)
{
    pdiutil::string text;
    ASSERT_EQ(text.size(), (size_t)0);
    ASSERT_TRUE(text.empty());
}

TEST(pdistl, string_constructs_from_a_c_string)
{
    pdiutil::string text("pdiStack");
    ASSERT_EQ(text.size(), (size_t)8);
    ASSERT_STREQ(text.c_str(), "pdiStack");
}

TEST(pdistl, string_constructs_from_a_bounded_range)
{
    pdiutil::string text("pdiStack", 3);
    ASSERT_STREQ(text.c_str(), "pdi");
}

TEST(pdistl, string_appends_and_grows_past_the_small_buffer)
{
    pdiutil::string text;
    for (int i = 0; i < 200; i++)
    {
        text += 'a';
    }

    ASSERT_EQ(text.size(), (size_t)200);
    ASSERT_EQ(text[0], 'a');
    ASSERT_EQ(text[199], 'a');
}

TEST(pdistl, string_concatenates)
{
    pdiutil::string text("pdi");
    text += "Stack";
    ASSERT_STREQ(text.c_str(), "pdiStack");
}

TEST(pdistl, string_compares_by_value)
{
    pdiutil::string a("same");
    pdiutil::string b("same");
    pdiutil::string c("other");

    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a == c);
}

TEST(pdistl, string_find_locates_a_substring)
{
    pdiutil::string text("hello world");
    ASSERT_EQ(text.find("world"), (size_t)6);
    ASSERT_EQ(text.find("hello"), (size_t)0);
}

TEST(pdistl, string_find_reports_npos_when_absent)
{
    pdiutil::string text("hello world");
    ASSERT_TRUE(text.find("absent") == pdiutil::string::npos);
}

TEST(pdistl, string_substr_extracts_a_range)
{
    pdiutil::string text("hello world");
    ASSERT_STREQ(text.substr(6).c_str(), "world");
    ASSERT_STREQ(text.substr(0, 5).c_str(), "hello");
}

TEST(pdistl, string_clear_empties_it)
{
    pdiutil::string text("something");
    text.clear();
    ASSERT_EQ(text.size(), (size_t)0);
    ASSERT_TRUE(text.empty());
}

TEST(pdistl, string_copies_independently)
{
    pdiutil::string original("original");
    pdiutil::string copy = original;

    copy += " changed";
    ASSERT_STREQ(original.c_str(), "original");
    ASSERT_STREQ(copy.c_str(), "original changed");
}

TEST(pdistl, string_holds_an_embedded_nul)
{
    pdiutil::string text;
    text += 'a';
    text += '\0';
    text += 'b';

    ASSERT_EQ(text.size(), (size_t)3);
    ASSERT_EQ(text[1], '\0');
}

/**
 * A bounded range says how long it is, so the first byte can be anything at
 * all. A nul there is what an uploaded file's first block looks like, and a
 * byte above 0x7f is negative wherever char is signed.
 */
TEST(pdistl, string_constructs_from_a_range_beginning_with_nul)
{
    const char payload[] = {'\0', 'a', 'b'};
    pdiutil::string text(payload, sizeof(payload));

    ASSERT_EQ(text.size(), sizeof(payload));
    ASSERT_EQ(text[0], '\0');
    ASSERT_EQ(text[2], 'b');
}

TEST(pdistl, string_constructs_from_a_range_beginning_with_a_high_byte)
{
    const char payload[] = {(char)0xFF, 'a', 'b'};
    pdiutil::string text(payload, sizeof(payload));

    ASSERT_EQ(text.size(), sizeof(payload));
    ASSERT_EQ((uint8_t)text[0], (uint8_t)0xFF);
    ASSERT_EQ(text[2], 'b');
}

TEST(pdistl, substr_of_binary_content_keeps_every_byte)
{
    pdiutil::string text;
    text += (char)0x00;
    text += (char)0xFF;
    text += 'x';
    text += 'y';

    pdiutil::string head = text.substr(0, 3);
    ASSERT_EQ(head.size(), (size_t)3);
    ASSERT_EQ(head[0], '\0');
    ASSERT_EQ((uint8_t)head[1], (uint8_t)0xFF);
    ASSERT_EQ(head[2], 'x');
}

TEST(pdistl, to_string_renders_integers)
{
    ASSERT_STREQ(pdiutil::to_string(0).c_str(), "0");
    ASSERT_STREQ(pdiutil::to_string(-42).c_str(), "-42");
    ASSERT_STREQ(pdiutil::to_string(123456).c_str(), "123456");
}

TEST(pdistl, vector_starts_empty)
{
    pdiutil::vector<int> values;
    ASSERT_EQ(values.size(), (size_t)0);
}

TEST(pdistl, vector_push_back_grows_it)
{
    pdiutil::vector<int> values;
    for (int i = 0; i < 100; i++)
    {
        values.push_back(i);
    }

    ASSERT_EQ(values.size(), (size_t)100);
    ASSERT_EQ(values[0], 0);
    ASSERT_EQ(values[99], 99);
}

TEST(pdistl, vector_preserves_order)
{
    pdiutil::vector<int> values;
    values.push_back(3);
    values.push_back(1);
    values.push_back(2);

    ASSERT_EQ(values[0], 3);
    ASSERT_EQ(values[1], 1);
    ASSERT_EQ(values[2], 2);
}

TEST(pdistl, vector_clear_empties_it)
{
    pdiutil::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.clear();

    ASSERT_EQ(values.size(), (size_t)0);
}

TEST(pdistl, vector_reserve_does_not_change_the_size)
{
    pdiutil::vector<int> values;
    values.reserve(64);
    ASSERT_EQ(values.size(), (size_t)0);

    values.push_back(7);
    ASSERT_EQ(values.size(), (size_t)1);
    ASSERT_EQ(values[0], 7);
}

TEST(pdistl, vector_holds_strings)
{
    pdiutil::vector<pdiutil::string> names;
    names.push_back("first");
    names.push_back("second");

    ASSERT_EQ(names.size(), (size_t)2);
    ASSERT_STREQ(names[0].c_str(), "first");
    ASSERT_STREQ(names[1].c_str(), "second");
}

TEST(pdistl, vector_copies_independently)
{
    pdiutil::vector<int> original;
    original.push_back(1);

    pdiutil::vector<int> copy = original;
    copy.push_back(2);

    ASSERT_EQ(original.size(), (size_t)1);
    ASSERT_EQ(copy.size(), (size_t)2);
}

TEST(pdistl, function_wraps_a_lambda)
{
    pdiutil::function<int(int)> doubler = [](int v) { return v * 2; };
    ASSERT_EQ(doubler(21), 42);
}

TEST(pdistl, function_captures_by_value)
{
    int base = 10;
    pdiutil::function<int(int)> adder = [base](int v) { return v + base; };
    ASSERT_EQ(adder(5), 15);
}

TEST(pdistl, function_can_be_reassigned)
{
    pdiutil::function<int(int)> fn = [](int v) { return v + 1; };
    ASSERT_EQ(fn(1), 2);

    fn = [](int v) { return v * 10; };
    ASSERT_EQ(fn(1), 10);
}

struct SampleConfig
{
    char name[8];
    uint32_t counter;
    uint8_t flag;
};

TEST(utility, clear_object_zeroes_every_byte)
{
    SampleConfig config;
    memset(&config, 0xAB, sizeof(config));

    _ClearObject(&config);

    const uint8_t *raw = (const uint8_t *)&config;
    for (size_t i = 0; i < sizeof(config); i++)
    {
        ASSERT_EQ(raw[i], (uint8_t)0);
    }
}

TEST(utility, clear_object_leaves_a_neighbour_untouched)
{
    SampleConfig first;
    SampleConfig second;
    memset(&first, 0xAB, sizeof(first));
    memset(&second, 0xCD, sizeof(second));

    _ClearObject(&first);

    ASSERT_EQ(second.flag, (uint8_t)0xCD);
}

TEST(pdistl, vector_of_strings_keeps_every_push_across_growth)
{
    pdiutil::vector<pdiutil::string> routes;

    for (int i = 0; i < 40; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "/route-number-%d", i);
        routes.push_back(pdiutil::string(name));
        ASSERT_EQ((int)routes.size(), i + 1);
    }

    for (int i = 0; i < 40; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "/route-number-%d", i);
        ASSERT_STREQ(routes[i].c_str(), name);
    }
}

TEST(pdistl, vector_of_uri_handler_pairs_survives_growth)
{
    struct UriToHandler {
        pdiutil::string uri;
        pdistd::function<void()> handler;
        UriToHandler(const pdiutil::string &u, pdistd::function<void()> h)
            : uri(u), handler(h) {}
    };

    static int called;
    pdiutil::vector<UriToHandler> routes;

    for (int i = 0; i < 30; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "/route-%d", i);
        int mine = i;
        routes.push_back(UriToHandler(pdiutil::string(name),
                                      [mine]() { called = mine; }));
        ASSERT_EQ((int)routes.size(), i + 1);
    }

    for (int i = 0; i < 30; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "/route-%d", i);
        ASSERT_STREQ(routes[i].uri.c_str(), name);

        called = -1;
        routes[i].handler();
        ASSERT_EQ(called, i);
    }
}
