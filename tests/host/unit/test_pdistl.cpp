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
#include <devices/posix/DeviceControlInterface.h>

#include <type_traits>
#include <utility>

namespace
{
    template <class T, class = void>
    struct HasEmplaceBack : std::false_type
    {
    };

    template <class T>
    struct HasEmplaceBack<T, decltype(void(std::declval<T &>().emplace_back('x')))> : std::true_type
    {
    };

    template <class T, class = void>
    struct HasShrinkToFit : std::false_type
    {
    };

    template <class T>
    struct HasShrinkToFit<T, decltype(void(std::declval<T &>().shrink_to_fit()))> : std::true_type
    {
    };

    /** True when c_str() hands back the whole text without asking the heap for anything. */
    bool terminatedWithoutHeap(const pdiutil::string &text)
    {
        uint32_t before = pdiutil::safe_alloc_refusals();
        __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);
        const char *raw = text.c_str();
        size_t seen = strlen(raw);
        __i_dvc_ctrl.capFreeHeap(0);
        return (pdiutil::safe_alloc_refusals() == before) && (seen == text.size()) &&
               (0 == memcmp(raw, text.data(), text.size()));
    }
}

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

TEST(pdistl, every_search_answers_without_taking_any_heap)
{
    pdiutil::string text("the quick brown fox jumps");
    pdiutil::string fox("fox");

    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);

    pdiutil::string::size_type byString = text.find(fox);
    pdiutil::string::size_type byPointer = text.find("jumps");
    pdiutil::string::size_type byRange = text.find("foxes", 0, 3);
    pdiutil::string::size_type byChar = text.find('q');
    pdiutil::string::size_type missing = text.find("cat");
    pdiutil::string::size_type backwards = text.rfind("o");
    pdiutil::string::size_type firstOf = text.find_first_of("xyz");
    pdiutil::string::size_type firstOfString = text.find_first_of(fox);
    pdiutil::string::size_type lastOf = text.find_last_of("aeiou");
    pdiutil::string::size_type firstNot = text.find_first_not_of("the ");
    pdiutil::string::size_type lastNot = text.find_last_not_of("spmuj");

    __i_dvc_ctrl.capFreeHeap(0);
    uint32_t refused = pdiutil::safe_alloc_refusals() - before;

    ASSERT_EQ(refused, (uint32_t)0);
    ASSERT_EQ(byString, (pdiutil::string::size_type)16);
    ASSERT_EQ(byPointer, (pdiutil::string::size_type)20);
    ASSERT_EQ(byRange, (pdiutil::string::size_type)16);
    ASSERT_EQ(byChar, (pdiutil::string::size_type)4);
    ASSERT_TRUE(missing == pdiutil::string::npos);
    ASSERT_EQ(backwards, (pdiutil::string::size_type)17);
    ASSERT_EQ(firstOf, (pdiutil::string::size_type)18);
    ASSERT_EQ(firstOfString, (pdiutil::string::size_type)12);
    ASSERT_EQ(lastOf, (pdiutil::string::size_type)21);
    ASSERT_EQ(firstNot, (pdiutil::string::size_type)4);
    ASSERT_EQ(lastNot, (pdiutil::string::size_type)19);
}

TEST(pdistl, moving_a_string_hands_over_its_buffer_without_taking_any_heap)
{
    pdiutil::string first("a line long enough to need a buffer of its own");
    const char *buffer = first.data();

    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);

    pdiutil::string second(pdistd::move(first));
    pdiutil::string third;
    third = pdistd::move(second);

    __i_dvc_ctrl.capFreeHeap(0);

    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)0);
    ASSERT_TRUE(third.data() == buffer);
    ASSERT_EQ(third.size(), (size_t)46);
    ASSERT_TRUE(first.empty());
    ASSERT_TRUE(second.empty());
    ASSERT_STREQ(third.c_str(), "a line long enough to need a buffer of its own");
}

TEST(pdistl, a_growing_vector_of_strings_moves_them_rather_than_copying)
{
    pdiutil::vector<pdiutil::string> lines;
    lines.push_back(pdiutil::string("the first line, kept in its own buffer"));
    const char *buffer = lines[0].data();

    for (int i = 0; i < 20; i++)
    {
        lines.push_back(pdiutil::string("another line"));
    }

    ASSERT_TRUE(lines[0].data() == buffer);
    ASSERT_STREQ(lines[0].c_str(), "the first line, kept in its own buffer");
}

TEST(pdistl, appending_to_a_temporary_reuses_its_buffer)
{
    pdiutil::string start("prefix");
    pdiutil::string more("-more");
    start.reserve(64);

    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);

    pdiutil::string joined = pdistd::move(start) + "-suffix";
    joined = pdistd::move(joined) + more;
    joined = pdistd::move(joined) + '!';

    __i_dvc_ctrl.capFreeHeap(0);

    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)0);
    ASSERT_STREQ(joined.c_str(), "prefix-suffix-more!");
}

TEST(pdistl, c_str_gives_the_whole_text_without_taking_any_heap)
{
    pdiutil::enable_heap_check();
    pdiutil::string exact("exactly sixteen!");
    pdiutil::string empty;

    ASSERT_TRUE(terminatedWithoutHeap(exact));
    ASSERT_TRUE(terminatedWithoutHeap(empty));
    ASSERT_STREQ(empty.c_str(), "");
}

TEST(pdistl, every_way_of_growing_or_shrinking_a_string_keeps_it_terminated)
{
    pdiutil::enable_heap_check();

    pdiutil::string s("12345678");
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.append("abcdefgh");
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s += 'x';
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.push_back('y');
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.append(9, 'z');
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.insert(0, "front-of-line ");
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.insert((pdiutil::string::size_type)3, 5, '-');
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.replace(0, 2, "a much longer replacement than two characters");
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.resize(100, 'r');
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.resize(10);
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.erase(2, 3);
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.erase(s.begin());
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.pop_back();
    ASSERT_TRUE(terminatedWithoutHeap(s));
    s.clear();
    ASSERT_TRUE(terminatedWithoutHeap(s));

    pdiutil::string filled(40, 'f');
    ASSERT_TRUE(terminatedWithoutHeap(filled));
    filled.assign(50, 'g');
    ASSERT_TRUE(terminatedWithoutHeap(filled));
    filled = 'c';
    ASSERT_TRUE(terminatedWithoutHeap(filled));
    filled = "assigned from a pointer, long enough";
    ASSERT_TRUE(terminatedWithoutHeap(filled));

    const char range[] = "built from a range of characters";
    pdiutil::string ranged(range, range + sizeof(range) - 1);
    ASSERT_TRUE(terminatedWithoutHeap(ranged));
    ranged.assign(range, range + 5);
    ASSERT_TRUE(terminatedWithoutHeap(ranged));
    ASSERT_STREQ(ranged.c_str(), "built");

    pdiutil::string reserved;
    reserved.reserve(30);
    ASSERT_TRUE(terminatedWithoutHeap(reserved));

    pdiutil::string copied(filled);
    ASSERT_TRUE(terminatedWithoutHeap(copied));
    pdiutil::string sub = filled.substr(3, 9);
    ASSERT_TRUE(terminatedWithoutHeap(sub));
    pdiutil::string joined = copied + sub;
    ASSERT_TRUE(terminatedWithoutHeap(joined));
}

TEST(pdistl, a_string_offers_none_of_the_vector_members_that_grow_it)
{
    ASSERT_FALSE(HasEmplaceBack<pdiutil::string>::value);
    ASSERT_FALSE(HasShrinkToFit<pdiutil::string>::value);
    ASSERT_FALSE((std::is_convertible<pdiutil::string &, pdiutil::vector<char> &>::value));
    ASSERT_FALSE((std::is_convertible<pdiutil::string *, pdiutil::vector<char> *>::value));

    ASSERT_TRUE(HasEmplaceBack<pdiutil::vector<char>>::value);
}

TEST(pdistl, find_last_of_looks_at_the_position_it_is_given)
{
    pdiutil::string text("abc");
    ASSERT_EQ(text.find_last_of('b', 1), (pdiutil::string::size_type)1);
    ASSERT_EQ(text.find_last_of("abc", 1), (pdiutil::string::size_type)1);
    ASSERT_EQ(text.find_last_of(pdiutil::string("c"), 2), (pdiutil::string::size_type)2);
    ASSERT_EQ(text.find_last_of('a', 0), (pdiutil::string::size_type)0);
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

TEST(pdistl, a_vector_and_a_string_hold_only_their_pointer_and_two_counts)
{
    ASSERT_EQ(sizeof(pdiutil::vector<int>), sizeof(int *) + 2 * sizeof(size_t));
    ASSERT_EQ(sizeof(pdiutil::vector<char>), sizeof(char *) + 2 * sizeof(size_t));
    ASSERT_EQ(sizeof(pdiutil::string), sizeof(char *) + 2 * sizeof(size_t));
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

namespace
{
    struct Tally
    {
        static int copies;
        static int moves;
        int value;

        Tally(int v = 0) : value(v) {}
        Tally(const Tally &other) : value(other.value) { copies++; }
        Tally(Tally &&other) noexcept : value(other.value) { moves++; }
        Tally &operator=(const Tally &other) { value = other.value; copies++; return *this; }
        Tally &operator=(Tally &&other) noexcept { value = other.value; moves++; return *this; }
    };

    int Tally::copies = 0;
    int Tally::moves = 0;

    void fillToCapacity(pdiutil::vector<pdiutil::string> &v)
    {
        v.push_back("a string long enough to need its own storage");
        while (v.size() < v.capacity())
        {
            v.push_back("another string long enough to need storage");
        }
    }
}

TEST(pdistl, strings_differing_after_an_embedded_nul_are_not_equal)
{
    pdiutil::string a("a\0b", 3);
    pdiutil::string b("a\0c", 3);

    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a.compare(b) < 0);
    ASSERT_TRUE(b.compare(a) > 0);
}

TEST(pdistl, an_empty_vector_holds_no_storage)
{
    pdiutil::vector<pdiutil::string> values;

    ASSERT_EQ(values.capacity(), (size_t)0);
    ASSERT_NULL(values.data());
    ASSERT_TRUE(values.begin() == values.end());
}

TEST(pdistl, a_copy_of_a_vector_holds_exactly_its_elements)
{
    pdiutil::vector<int> original;
    for (int i = 0; i < 5; i++)
    {
        original.push_back(i);
    }

    pdiutil::vector<int> copy(original);
    ASSERT_EQ(copy.size(), (size_t)5);
    ASSERT_EQ(copy.capacity(), (size_t)5);

    pdiutil::vector<int> none;
    pdiutil::vector<int> copyofnone(none);
    ASSERT_EQ(copyofnone.capacity(), (size_t)0);
    ASSERT_NULL(copyofnone.data());
}

TEST(pdistl, vector_growth_reallocates_a_bounded_number_of_times)
{
    pdiutil::vector<int> values;
    size_t last = values.capacity();
    int reallocations = 0;

    for (int i = 0; i < 1000; i++)
    {
        values.push_back(i);
        if (values.capacity() != last)
        {
            reallocations++;
            last = values.capacity();
        }
    }

    ASSERT_EQ(values.size(), (size_t)1000);
    ASSERT_EQ(values[999], 999);
    ASSERT_TRUE(reallocations < 25);
}

TEST(pdistl, growing_a_vector_moves_its_elements_rather_than_copying_them)
{
    pdiutil::vector<Tally> values;
    Tally::copies = 0;
    Tally::moves = 0;

    for (int i = 0; i < 50; i++)
    {
        values.push_back(Tally(i));
    }

    ASSERT_EQ(Tally::copies, 0);
    ASSERT_EQ(values.size(), (size_t)50);
    ASSERT_EQ(values[0].value, 0);
    ASSERT_EQ(values[49].value, 49);
}

TEST(pdistl, emplace_back_builds_the_element_in_place)
{
    pdiutil::vector<Tally> values;
    values.reserve(4);
    Tally::copies = 0;
    Tally::moves = 0;

    values.emplace_back(7);

    ASSERT_EQ(Tally::copies, 0);
    ASSERT_EQ(Tally::moves, 0);
    ASSERT_EQ(values[0].value, 7);
}

TEST(pdistl, pushing_a_vectors_own_element_survives_the_growth_it_causes)
{
    pdiutil::vector<pdiutil::string> values;
    fillToCapacity(values);
    size_t before = values.size();

    values.push_back(values[0]);

    ASSERT_EQ(values.size(), before + 1);
    ASSERT_STREQ(values.back().c_str(), values[0].c_str());
}

TEST(pdistl, inserting_a_vectors_own_element_survives_the_growth_it_causes)
{
    pdiutil::vector<pdiutil::string> values;
    fillToCapacity(values);
    pdiutil::string first = values[0];

    values.insert(values.begin(), values[0]);

    ASSERT_STREQ(values[0].c_str(), first.c_str());
    ASSERT_STREQ(values[1].c_str(), first.c_str());
}

TEST(pdistl, resizing_from_a_vectors_own_element_survives_the_growth_it_causes)
{
    pdiutil::vector<pdiutil::string> values;
    fillToCapacity(values);
    size_t before = values.size();

    values.resize(before + 3, values[0]);

    ASSERT_EQ(values.size(), before + 3);
    ASSERT_STREQ(values[before + 2].c_str(), values[0].c_str());
}

TEST(pdistl, insert_places_elements_where_asked)
{
    pdiutil::vector<int> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(4);

    pdiutil::vector<int>::iterator at = values.insert(values.begin() + 2, 3);
    ASSERT_EQ(*at, 3);

    values.insert(values.begin() + 1, (size_t)2, 9);

    pdiutil::vector<int> tail;
    tail.push_back(5);
    tail.push_back(6);
    values.insert(values.end(), tail.begin(), tail.end());

    int expected[] = {1, 9, 9, 2, 3, 4, 5, 6};
    ASSERT_EQ(values.size(), sizeof(expected) / sizeof(expected[0]));
    for (size_t i = 0; i < values.size(); i++)
    {
        ASSERT_EQ(values[i], expected[i]);
    }
}

TEST(pdistl, erasing_an_empty_range_changes_nothing)
{
    pdiutil::vector<pdiutil::string> values;
    values.push_back("alpha");
    values.push_back("beta");
    values.push_back("gamma");

    values.erase(values.begin() + 1, values.begin() + 1);

    ASSERT_EQ(values.size(), (size_t)3);
    ASSERT_STREQ(values[0].c_str(), "alpha");
    ASSERT_STREQ(values[1].c_str(), "beta");
    ASSERT_STREQ(values[2].c_str(), "gamma");
}

TEST(pdistl, erase_closes_the_gap_it_leaves)
{
    pdiutil::vector<pdiutil::string> values;
    values.push_back("alpha");
    values.push_back("beta");
    values.push_back("gamma");
    values.push_back("delta");

    values.erase(values.begin() + 1);
    values.erase(values.begin() + 1, values.begin() + 2);

    ASSERT_EQ(values.size(), (size_t)2);
    ASSERT_STREQ(values[0].c_str(), "alpha");
    ASSERT_STREQ(values[1].c_str(), "delta");
}

TEST(pdistl, clear_keeps_the_storage_and_shrink_to_fit_gives_it_back)
{
    pdiutil::vector<int> values;
    for (int i = 0; i < 10; i++)
    {
        values.push_back(i);
    }
    size_t held = values.capacity();

    values.clear();
    ASSERT_EQ(values.size(), (size_t)0);
    ASSERT_EQ(values.capacity(), held);

    values.shrink_to_fit();
    ASSERT_EQ(values.capacity(), (size_t)0);
    ASSERT_NULL(values.data());

    values.push_back(1);
    values.push_back(2);
    values.reserve(32);
    values.shrink_to_fit();
    ASSERT_EQ(values.capacity(), (size_t)2);
    ASSERT_EQ(values[1], 2);
}

TEST(pdistl, a_moved_vector_hands_over_its_storage)
{
    pdiutil::vector<pdiutil::string> source;
    source.push_back("kept");
    const pdiutil::string *held = source.data();

    pdiutil::vector<pdiutil::string> taken(pdistd::move(source));
    ASSERT_TRUE(taken.data() == held);
    ASSERT_EQ(source.size(), (size_t)0);
    ASSERT_NULL(source.data());

    pdiutil::vector<pdiutil::string> assigned;
    assigned.push_back("replaced");
    assigned = pdistd::move(taken);
    ASSERT_STREQ(assigned[0].c_str(), "kept");
    ASSERT_EQ(taken.size(), (size_t)0);
}

TEST(pdistl, assigning_a_vector_replaces_its_contents)
{
    pdiutil::vector<pdiutil::string> target;
    target.push_back("one");
    target.push_back("two");
    target.push_back("three");

    pdiutil::vector<pdiutil::string> shorter;
    shorter.push_back("only");
    target = shorter;
    ASSERT_EQ(target.size(), (size_t)1);
    ASSERT_STREQ(target[0].c_str(), "only");

    pdiutil::vector<pdiutil::string> longer;
    for (int i = 0; i < 20; i++)
    {
        longer.push_back("many");
    }
    target = longer;
    ASSERT_EQ(target.size(), (size_t)20);
    ASSERT_STREQ(target[19].c_str(), "many");
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

namespace
{
    const pdiutil::string::size_type NPOS = pdiutil::string::npos;
}

TEST(pdistring, every_constructor_builds_what_it_names)
{
    pdiutil::string fromcstr("hello");
    pdiutil::string bounded("hello world", 5);
    pdiutil::string repeated((size_t)3, 'x');
    pdiutil::string copied(fromcstr);
    pdiutil::string tail(pdiutil::string("hello world"), 6);
    pdiutil::string middle(pdiutil::string("hello world"), 2, 3);
    const char raw[] = "range";
    pdiutil::string ranged(raw, raw + 5);

    ASSERT_STREQ(fromcstr.c_str(), "hello");
    ASSERT_STREQ(bounded.c_str(), "hello");
    ASSERT_STREQ(repeated.c_str(), "xxx");
    ASSERT_STREQ(copied.c_str(), "hello");
    ASSERT_STREQ(tail.c_str(), "world");
    ASSERT_STREQ(middle.c_str(), "llo");
    ASSERT_STREQ(ranged.c_str(), "range");
}

TEST(pdistring, an_empty_string_reads_as_empty)
{
    pdiutil::string empty;

    ASSERT_TRUE(empty.empty());
    ASSERT_EQ(empty.size(), (size_t)0);
    ASSERT_EQ(empty.length(), (size_t)0);
    ASSERT_STREQ(empty.c_str(), "");
    ASSERT_TRUE(empty.begin() == empty.end());
}

TEST(pdistring, assignment_replaces_the_contents)
{
    pdiutil::string s("start");

    s = "other";
    ASSERT_STREQ(s.c_str(), "other");
    s = 'c';
    ASSERT_STREQ(s.c_str(), "c");
    s = pdiutil::string("copied");
    ASSERT_STREQ(s.c_str(), "copied");

    s.assign("abcdef", 3);
    ASSERT_STREQ(s.c_str(), "abc");
    s.assign((size_t)2, 'z');
    ASSERT_STREQ(s.c_str(), "zz");
    s.assign(pdiutil::string("0123456789"), 2, 4);
    ASSERT_STREQ(s.c_str(), "2345");
    s.assign("plain");
    ASSERT_STREQ(s.c_str(), "plain");
}

TEST(pdistring, append_adds_at_the_end_in_every_form)
{
    pdiutil::string s("a");

    s.append(pdiutil::string("b"));
    s.append(pdiutil::string("xcdx"), 1, 2);
    s.append("efg", 2);
    s.append("gh");
    s.append((size_t)2, 'i');
    s += pdiutil::string("j");
    s += "k";
    s += 'l';
    s.push_back('m');

    ASSERT_STREQ(s.c_str(), "abcdefghiijklm");

    s.pop_back();
    ASSERT_STREQ(s.c_str(), "abcdefghiijkl");
}

TEST(pdistring, insert_places_text_where_asked)
{
    pdiutil::string s("ad");

    s.insert(1, pdiutil::string("bc"));
    ASSERT_STREQ(s.c_str(), "abcd");

    s.insert(0, "<<");
    ASSERT_STREQ(s.c_str(), "<<abcd");

    s.insert(s.size(), (size_t)2, '>');
    ASSERT_STREQ(s.c_str(), "<<abcd>>");

    s.insert(2, "xyz", 1);
    ASSERT_STREQ(s.c_str(), "<<xabcd>>");
}

TEST(pdistring, erase_removes_what_it_names_and_no_more)
{
    pdiutil::string s("0123456789");

    s.erase(8);
    ASSERT_STREQ(s.c_str(), "01234567");

    s.erase(2, 3);
    ASSERT_STREQ(s.c_str(), "01567");

    s.erase(3, 100);
    ASSERT_STREQ(s.c_str(), "015");

    s.erase(s.begin());
    ASSERT_STREQ(s.c_str(), "15");

    s.erase(s.begin(), s.end());
    ASSERT_TRUE(s.empty());

    pdiutil::string all("gone");
    all.erase();
    ASSERT_TRUE(all.empty());
}

TEST(pdistring, replace_swaps_a_range_for_new_text)
{
    pdiutil::string s("the quick fox");

    s.replace(4, 5, pdiutil::string("slow"));
    ASSERT_STREQ(s.c_str(), "the slow fox");

    s.replace(0, 3, "a");
    ASSERT_STREQ(s.c_str(), "a slow fox");

    s.replace(2, 4, "very slow", 4);
    ASSERT_STREQ(s.c_str(), "a very fox");

    s.replace(s.size() - 3, 3, (size_t)2, '!');
    ASSERT_STREQ(s.c_str(), "a very !!");

    s.replace(2, 4, pdiutil::string("xxbigxx"), 2, 3);
    ASSERT_STREQ(s.c_str(), "a big !!");
}

TEST(pdistring, replace_can_grow_and_shrink_the_string)
{
    pdiutil::string grow("ab");
    grow.replace(1, 0, "-------------------------------");
    ASSERT_STREQ(grow.c_str(), "a-------------------------------b");

    pdiutil::string shrink("a-------------------------------b");
    shrink.replace(1, 31, "");
    ASSERT_STREQ(shrink.c_str(), "ab");
}

TEST(pdistring, replacing_every_occurrence_rewrites_them_all)
{
    pdiutil::string path("/storage//dir//file");
    path.replace("//", "/");
    ASSERT_STREQ(path.c_str(), "/storage/dir/file");

    pdiutil::string quoted("\"a\" \"b\"");
    quoted.replace("\"", "");
    ASSERT_STREQ(quoted.c_str(), "a b");

    pdiutil::string longer("x.x.x");
    longer.replace(".", "..");
    ASSERT_STREQ(longer.c_str(), "x..x..x");

    pdiutil::string none("unchanged");
    none.replace("zz", "yy");
    ASSERT_STREQ(none.c_str(), "unchanged");
}

TEST(pdistring, find_locates_text_from_a_position)
{
    pdiutil::string s("abcabcabc");

    ASSERT_EQ(s.find(pdiutil::string("bc")), (size_t)1);
    ASSERT_EQ(s.find("bc", 2), (size_t)4);
    ASSERT_EQ(s.find("bcz", 0, 2), (size_t)1);
    ASSERT_EQ(s.find('c'), (size_t)2);
    ASSERT_EQ(s.find('c', 3), (size_t)5);
    ASSERT_EQ(s.find("abc", 7), NPOS);
    ASSERT_EQ(s.find('z'), NPOS);
    ASSERT_EQ(s.find("", 3), (size_t)3);
    ASSERT_EQ(s.find("", 9), (size_t)9);
    ASSERT_EQ(s.find("", 10), NPOS);
}

TEST(pdistring, rfind_locates_the_last_occurrence)
{
    pdiutil::string s("abcabcabc");

    ASSERT_EQ(s.rfind(pdiutil::string("bc")), (size_t)7);
    ASSERT_EQ(s.rfind("bc", 6), (size_t)4);
    ASSERT_EQ(s.rfind("bcz", NPOS, 2), (size_t)7);
    ASSERT_EQ(s.rfind('a'), (size_t)6);
    ASSERT_EQ(s.rfind('a', 5), (size_t)3);
    ASSERT_EQ(s.rfind('z'), NPOS);
    ASSERT_EQ(s.rfind("abc", 0), (size_t)0);
    ASSERT_EQ(s.rfind("bc", 7), (size_t)7);
    ASSERT_EQ(s.rfind(""), (size_t)9);
    ASSERT_EQ(s.rfind("", 4), (size_t)4);
    ASSERT_EQ(s.rfind("abcabcabcabc"), NPOS);
    ASSERT_EQ(pdiutil::string().rfind(""), (size_t)0);
}

TEST(pdistring, find_first_and_last_of_a_set)
{
    pdiutil::string s("path/to/file.txt");

    ASSERT_EQ(s.find_first_of("/."), (size_t)4);
    ASSERT_EQ(s.find_first_of(pdiutil::string("/."), 5), (size_t)7);
    ASSERT_EQ(s.find_first_of('.'), (size_t)12);
    ASSERT_EQ(s.find_first_of("xyz", 0, 2), (size_t)14);
    ASSERT_EQ(s.find_first_of("qzx", 0, 2), NPOS);
    ASSERT_EQ(s.find_last_of("/"), (size_t)7);
    ASSERT_EQ(s.find_last_of(pdiutil::string("/"), 6), (size_t)4);
    ASSERT_EQ(s.find_last_of('t'), (size_t)15);
    ASSERT_EQ(s.find_last_of("#"), NPOS);
}

TEST(pdistring, find_first_and_last_not_of_a_set)
{
    pdiutil::string s("  value  ");

    ASSERT_EQ(s.find_first_not_of(' '), (size_t)2);
    ASSERT_EQ(s.find_first_not_of(" "), (size_t)2);
    ASSERT_EQ(s.find_first_not_of(pdiutil::string(" v")), (size_t)3);
    ASSERT_EQ(s.find_last_not_of(' '), (size_t)6);
    ASSERT_EQ(s.find_last_not_of(" "), (size_t)6);
    ASSERT_EQ(s.find_last_not_of(pdiutil::string(" e")), (size_t)5);

    pdiutil::string blank("   ");
    ASSERT_EQ(blank.find_first_not_of(' '), NPOS);
    ASSERT_EQ(blank.find_last_not_of(' '), NPOS);
}

TEST(pdistring, searches_on_an_empty_string_find_nothing)
{
    pdiutil::string empty;

    ASSERT_EQ(empty.find('a'), NPOS);
    ASSERT_EQ(empty.rfind('a'), NPOS);
    ASSERT_EQ(empty.find_first_of("abc"), NPOS);
    ASSERT_EQ(empty.find_last_of("abc"), NPOS);
    ASSERT_EQ(empty.find_first_not_of("abc"), NPOS);
    ASSERT_EQ(empty.find_last_not_of("abc"), NPOS);
    ASSERT_EQ(empty.find_last_not_of('a'), NPOS);
}

TEST(pdistring, substr_takes_a_range_that_may_run_to_the_end)
{
    pdiutil::string s("0123456789");

    ASSERT_STREQ(s.substr(3, 2).c_str(), "34");
    ASSERT_STREQ(s.substr(7).c_str(), "789");
    ASSERT_STREQ(s.substr(8, 100).c_str(), "89");
    ASSERT_STREQ(s.substr(10).c_str(), "");
    ASSERT_STREQ(s.substr().c_str(), "0123456789");
}

TEST(pdistring, compare_orders_by_content_then_length)
{
    pdiutil::string abc("abc");

    ASSERT_EQ(abc.compare(pdiutil::string("abc")), 0);
    ASSERT_LT(abc.compare(pdiutil::string("abd")), 0);
    ASSERT_GT(abc.compare(pdiutil::string("abb")), 0);
    ASSERT_LT(abc.compare(pdiutil::string("abcd")), 0);
    ASSERT_GT(abc.compare(pdiutil::string("ab")), 0);
    ASSERT_EQ(abc.compare("abc"), 0);
    ASSERT_LT(abc.compare("b"), 0);

    pdiutil::string longer("xxabcxx");
    ASSERT_EQ(longer.compare(2, 3, abc), 0);
    ASSERT_EQ(longer.compare(2, 3, pdiutil::string("zabcz"), 1, 3), 0);
    ASSERT_EQ(longer.compare(2, 3, "abc"), 0);
    ASSERT_NE(longer.compare(2, 3, "abd"), 0);

    pdiutil::string high("\xff");
    ASSERT_GT(high.compare("a"), 0);
}

TEST(pdistring, relational_operators_mix_strings_and_literals)
{
    pdiutil::string a("apple");
    pdiutil::string b("banana");

    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b > a);
    ASSERT_TRUE(a <= a);
    ASSERT_TRUE(b >= a);
    ASSERT_TRUE(a == "apple");
    ASSERT_TRUE("apple" == a);
    ASSERT_TRUE(a != "pear");
    ASSERT_TRUE("pear" != a);
    ASSERT_TRUE(a < "b");
    ASSERT_TRUE("b" > a);
}

TEST(pdistring, operator_plus_joins_every_combination)
{
    pdiutil::string s("mid");

    ASSERT_STREQ((s + pdiutil::string("!")).c_str(), "mid!");
    ASSERT_STREQ((s + "dle").c_str(), "middle");
    ASSERT_STREQ(("a" + s).c_str(), "amid");
    ASSERT_STREQ((s + '?').c_str(), "mid?");
    ASSERT_STREQ(('<' + s).c_str(), "<mid");
}

TEST(pdistring, copy_writes_characters_without_a_terminator)
{
    pdiutil::string s("0123456789");
    char out[8];
    memset(out, '#', sizeof(out));

    size_t n = s.copy(out, 4, 3);

    ASSERT_EQ(n, (size_t)4);
    ASSERT_MEMEQ(out, "3456#", 5);
    ASSERT_EQ(s.copy(out, 100, 8), (size_t)2);
}

TEST(pdistring, resize_grows_with_a_fill_and_shrinks_by_cutting)
{
    pdiutil::string s("abc");

    s.resize(6, '.');
    ASSERT_STREQ(s.c_str(), "abc...");

    s.resize(2);
    ASSERT_STREQ(s.c_str(), "ab");
    ASSERT_EQ(s.size(), (size_t)2);
}

TEST(pdistring, element_access_reads_and_writes_in_place)
{
    pdiutil::string s("abc");

    ASSERT_EQ(s[0], 'a');
    ASSERT_EQ(s.at(2), 'c');
    s[1] = 'X';
    ASSERT_STREQ(s.c_str(), "aXc");
    ASSERT_MEMEQ(s.data(), "aXc", 3);
}

TEST(pdistring, iteration_walks_forwards_and_backwards)
{
    pdiutil::string s("abc");
    pdiutil::string forward;
    pdiutil::string backward;

    for (pdiutil::string::iterator it = s.begin(); it != s.end(); ++it)
    {
        forward += *it;
    }
    for (pdiutil::string::reverse_iterator it = s.rbegin(); it != s.rend(); ++it)
    {
        backward += *it;
    }

    ASSERT_STREQ(forward.c_str(), "abc");
    ASSERT_STREQ(backward.c_str(), "cba");
}

TEST(pdistring, swap_exchanges_contents)
{
    pdiutil::string a("first");
    pdiutil::string b("second, and longer");

    a.swap(b);

    ASSERT_STREQ(a.c_str(), "second, and longer");
    ASSERT_STREQ(b.c_str(), "first");
}

TEST(pdistring, a_string_built_one_character_at_a_time_holds_them_all)
{
    pdiutil::string s;
    for (int i = 0; i < 500; i++)
    {
        s += (char)('a' + (i % 26));
    }

    ASSERT_EQ(s.size(), (size_t)500);
    ASSERT_EQ(s[0], 'a');
    ASSERT_EQ(s[499], (char)('a' + (499 % 26)));
    ASSERT_EQ(strlen(s.c_str()), (size_t)500);
}

TEST(pdistring, to_string_renders_every_integer_type)
{
    ASSERT_STREQ(pdiutil::to_string(0).c_str(), "0");
    ASSERT_STREQ(pdiutil::to_string(2147483647).c_str(), "2147483647");
    ASSERT_STREQ(pdiutil::to_string((int)(-2147483647 - 1)).c_str(), "-2147483648");
    ASSERT_STREQ(pdiutil::to_string(4294967295u).c_str(), "4294967295");
    ASSERT_STREQ(pdiutil::to_string((long)-7).c_str(), "-7");
    ASSERT_STREQ(pdiutil::to_string((unsigned long)42).c_str(), "42");
}

TEST(pdistl, min_max_and_swap_follow_the_standard)
{
    ASSERT_EQ(pdistd::min(3, 7), 3);
    ASSERT_EQ(pdistd::max(3, 7), 7);
    ASSERT_EQ(pdistd::min(-1, -1), -1);

    pdiutil::string a("left");
    pdiutil::string b("right");
    pdistd::swap(a, b);
    ASSERT_STREQ(a.c_str(), "right");
    ASSERT_STREQ(b.c_str(), "left");
}

TEST(pdistl, a_vector_built_from_a_range_copies_it)
{
    int values[] = {4, 5, 6};
    pdiutil::vector<int> v(values, values + 3);

    ASSERT_EQ(v.size(), (size_t)3);
    ASSERT_EQ(v.at(1), 5);
    const pdiutil::vector<int> &view = v;
    ASSERT_EQ(view.data()[2], 6);
    ASSERT_EQ(view.front(), 4);
    ASSERT_EQ(view.back(), 6);
}

TEST(pdistl, vectors_compare_element_by_element)
{
    pdiutil::vector<int> a;
    pdiutil::vector<int> b;
    a.push_back(1);
    a.push_back(2);
    b.push_back(1);
    b.push_back(3);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b > a);
    ASSERT_TRUE(a <= a);
    ASSERT_TRUE(b >= a);

    b[1] = 2;
    ASSERT_TRUE(a == b);
}

TEST(pdistl, function_reports_whether_it_holds_a_target)
{
    pdiutil::function<int()> empty;
    ASSERT_FALSE((bool)empty);

    pdiutil::function<int()> full = []() { return 1; };
    ASSERT_TRUE((bool)full);

    full = nullptr;
    ASSERT_FALSE((bool)full);
}

TEST(pdistl, a_copied_function_calls_its_own_target)
{
    int counter = 0;
    pdiutil::function<void()> original = [&counter]() { counter++; };
    pdiutil::function<void()> copy = original;

    original();
    copy();
    ASSERT_EQ(counter, 2);

    copy = []() {};
    original();
    ASSERT_EQ(counter, 3);
}

namespace {

// past the host's whole heap budget, so the framework's heap check refuses it
const size_t BEYOND_HEAP = 64u * 1024u * 1024u;

} // namespace

TEST(pdistl, a_vector_the_heap_cannot_grow_is_left_as_it_was)
{
    pdiutil::enable_heap_check();
    pdiutil::vector<char> v;
    v.push_back('a');
    size_t capacity = v.capacity();

    v.reserve(BEYOND_HEAP);
    v.resize(BEYOND_HEAP, 'z');

    ASSERT_EQ(v.size(), (size_t)1);
    ASSERT_EQ(v.capacity(), capacity);
    ASSERT_EQ(v[0], 'a');
}

TEST(pdistl, a_string_the_heap_cannot_grow_keeps_its_text)
{
    pdiutil::enable_heap_check();
    pdiutil::string s = "keep";

    s.append(BEYOND_HEAP, 'x');
    s.insert(1, BEYOND_HEAP, 'y');
    s.resize(BEYOND_HEAP);

    ASSERT_STREQ(s.c_str(), "keep");
    ASSERT_EQ(s.size(), (size_t)4);
}

TEST(pdistl, the_allocator_refuses_a_count_its_size_would_overflow)
{
    pdistd::allocator<uint32_t> a;
    ASSERT_NULL(a.allocate(a.max_size() + 1));
    ASSERT_NULL(a.allocate(0));

    uint32_t *p = a.allocate(4);
    ASSERT_NOT_NULL(p);
    a.deallocate(p, 4);
}

TEST(pdistl, raw_storage_comes_from_the_one_allocation_api)
{
    pdiutil::enable_heap_check();
    ASSERT_NULL(pdiutil::safe_alloc_bytes(BEYOND_HEAP));
    ASSERT_NULL(pdiutil::safe_alloc_bytes(0));

    void *p = pdiutil::safe_alloc_bytes(32);
    ASSERT_NOT_NULL(p);
    pdiutil::safe_free_bytes(p);
    pdiutil::safe_free_bytes(nullptr);
}

TEST(pdistl, every_refusal_is_counted)
{
    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();

    ASSERT_NULL(pdiutil::safe_alloc_bytes(BEYOND_HEAP));
    pdiutil::vector<char> v;
    v.reserve(BEYOND_HEAP);

    ASSERT_EQ(pdiutil::safe_alloc_refusals(), before + 2);

    void *small = pdiutil::safe_alloc_bytes(PDI_SAFE_ALLOC_SMALL_REQUEST);
    ASSERT_NOT_NULL(small);
    pdiutil::safe_free_bytes(small);
    ASSERT_EQ(pdiutil::safe_alloc_refusals(), before + 2);
}

TEST(pdistl, a_number_the_heap_cannot_hold_comes_back_empty)
{
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);

    pdiutil::string a = pdiutil::to_string((int)-12345);
    pdiutil::string b = pdiutil::to_string((unsigned int)12345u);
    pdiutil::string c = pdiutil::to_string((long)-12345);
    pdiutil::string d = pdiutil::to_string((unsigned long)12345ul);
    pdiutil::string e = pdiutil::to_string((long long)-12345);
    pdiutil::string f = pdiutil::to_string((unsigned long long)12345ull);

    __i_dvc_ctrl.capFreeHeap(0);

    ASSERT_TRUE(a.empty());
    ASSERT_TRUE(b.empty());
    ASSERT_TRUE(c.empty());
    ASSERT_TRUE(d.empty());
    ASSERT_TRUE(e.empty());
    ASSERT_TRUE(f.empty());

    ASSERT_STREQ(pdiutil::to_string((int)-12345).c_str(), "-12345");
    ASSERT_STREQ(pdiutil::to_string((unsigned long long)12345ull).c_str(), "12345");
}

TEST(pdistl, calling_an_empty_function_returns_a_default_value)
{
    pdiutil::function<int()> empty;
    ASSERT_EQ(empty(), 0);

    pdiutil::function<void()> nothing;
    nothing();
}

TEST(pdistl, a_function_assigned_to_itself_keeps_its_target)
{
    int counter = 0;
    pdiutil::function<void()> f = [&counter]() { counter++; };
    pdiutil::function<void()> &same = f;

    f = same;
    f();
    ASSERT_EQ(counter, 1);
}

namespace
{
    /** A callable and an argument type that count how often they are copied, moved and destroyed. */
    struct Tracker
    {
        static int s_copies;
        static int s_moves;
        static int s_alive;

        int m_value;

        Tracker(int value = 0) : m_value(value) { s_alive++; }
        Tracker(const Tracker &other) : m_value(other.m_value)
        {
            s_copies++;
            s_alive++;
        }
        Tracker(Tracker &&other) : m_value(other.m_value)
        {
            s_moves++;
            s_alive++;
        }
        ~Tracker() { s_alive--; }

        int operator()(int add) const { return m_value + add; }

        static void reset()
        {
            s_copies = 0;
            s_moves = 0;
        }
    };

    int Tracker::s_copies = 0;
    int Tracker::s_moves = 0;
    int Tracker::s_alive = 0;

    int answer() { return 42; }
}

TEST(pdistl, a_copy_of_an_empty_function_is_empty)
{
    pdiutil::function<void()> empty;
    pdiutil::function<void()> copied(empty);
    const pdiutil::function<void()> constEmpty;
    pdiutil::function<void()> copiedConst(constEmpty);

    ASSERT_FALSE((bool)copied);
    ASSERT_FALSE((bool)copiedConst);
    ASSERT_TRUE(copied == nullptr);
}

TEST(pdistl, a_function_built_from_a_null_function_pointer_is_empty)
{
    int (*none)() = nullptr;
    pdiutil::function<int()> fromNull(none);
    pdiutil::function<int()> fromAnswer(&answer);

    ASSERT_FALSE((bool)fromNull);
    ASSERT_TRUE((bool)fromAnswer);
    ASSERT_EQ(fromAnswer(), 42);
}

TEST(pdistl, a_function_copies_its_target_only_as_often_as_the_standard_says)
{
    int fromTemporary, fromNamed, copyingFunction, copyingNonConst, movingFunction, assigningTemporary;
    {
        Tracker::reset();
        pdiutil::function<int(int)> a(Tracker(1));
        fromTemporary = Tracker::s_copies;

        Tracker named(2);
        Tracker::reset();
        pdiutil::function<int(int)> b(named);
        fromNamed = Tracker::s_copies;

        Tracker::reset();
        const pdiutil::function<int(int)> &constB = b;
        pdiutil::function<int(int)> c(constB);
        copyingFunction = Tracker::s_copies;

        Tracker::reset();
        pdiutil::function<int(int)> d(b);
        copyingNonConst = Tracker::s_copies;

        Tracker::reset();
        pdiutil::function<int(int)> e(pdistd::move(c));
        movingFunction = Tracker::s_copies;

        Tracker::reset();
        a = Tracker(5);
        assigningTemporary = Tracker::s_copies;

        ASSERT_EQ(a(1), 6);
        ASSERT_EQ(b(1), 3);
        ASSERT_EQ(d(1), 3);
        ASSERT_EQ(e(1), 3);
    }

    ASSERT_EQ(fromTemporary, 0);
    ASSERT_EQ(fromNamed, 1);
    ASSERT_EQ(copyingFunction, 1);
    ASSERT_EQ(copyingNonConst, 1);
    ASSERT_EQ(movingFunction, 0);
    ASSERT_EQ(assigningTemporary, 0);
    ASSERT_EQ(Tracker::s_alive, 0);
}

TEST(pdistl, a_function_passes_an_argument_on_without_copying_it_again)
{
    pdiutil::function<int(Tracker)> f = [](Tracker t) { return t.m_value; };
    Tracker arg(9);

    Tracker::reset();
    int got = f(arg);

    ASSERT_EQ(got, 9);
    ASSERT_EQ(Tracker::s_copies, 1);
}

TEST(pdistl, captured_state_behaves_as_it_does_in_the_standard)
{
    int counter = 0;
    pdiutil::function<int()> bump = [counter]() mutable { return ++counter; };
    ASSERT_EQ(bump(), 1);
    ASSERT_EQ(bump(), 2);

    pdiutil::function<int()> branch = bump;
    ASSERT_EQ(branch(), 3);
    ASSERT_EQ(branch(), 4);
    ASSERT_EQ(bump(), 3);
    ASSERT_EQ(counter, 0);

    int shared = 0;
    pdiutil::function<void()> add = [&shared]() { shared += 10; };
    pdiutil::function<void()> addToo = add;
    add();
    addToo();
    ASSERT_EQ(shared, 20);

    pdiutil::string text("captured by value");
    pdiutil::function<size_t()> length = [text]() { return text.size(); };
    text.clear();
    ASSERT_EQ(length(), (size_t)17);
}

TEST(pdistl, every_target_is_destroyed_exactly_once)
{
    {
        pdiutil::function<int(int)> f = Tracker(1);
        pdiutil::function<int(int)> g = f;
        pdiutil::function<int(int)> h = pdistd::move(g);
        f = Tracker(2);
        h = f;
        g = h;
        f = nullptr;
        h = pdistd::move(g);
        ASSERT_EQ(h(0), 2);
    }
    ASSERT_EQ(Tracker::s_alive, 0);
}

#if __PDISTD_FUNCTION_INLINE_STORAGE__
TEST(pdistl, a_small_callable_is_held_without_taking_any_heap)
{
    int local = 5;
    int *where = &local;

    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);

    pdiutil::function<int()> none = []() { return 7; };
    pdiutil::function<int()> onePointer = [where]() { return *where; };
    pdiutil::function<int()> pointer = &answer;
    pdiutil::function<int()> copied = onePointer;
    pdiutil::function<int()> moved = pdistd::move(copied);
    int fromNone = none();
    int fromOne = onePointer();
    int fromPointer = pointer();
    int fromMoved = moved();

    __i_dvc_ctrl.capFreeHeap(0);

    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)0);
    ASSERT_EQ(fromNone, 7);
    ASSERT_EQ(fromOne, 5);
    ASSERT_EQ(fromPointer, 42);
    ASSERT_EQ(fromMoved, 5);
}
#endif

TEST(pdistl, a_large_callable_still_works_and_a_refused_one_leaves_the_function_empty)
{
    int a = 1, b = 2, c = 3, d = 4;
    pdiutil::function<int()> large = [a, b, c, d]() { return a + b + c + d; };
    pdiutil::function<int()> copy = large;
    pdiutil::function<int()> moved = pdistd::move(large);
    ASSERT_EQ(copy(), 10);
    ASSERT_EQ(moved(), 10);

    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN);
    pdiutil::function<int()> refused = [a, b, c, d]() { return a + b + c + d; };
    pdiutil::function<int()> refusedCopy = copy;
    __i_dvc_ctrl.capFreeHeap(0);

    ASSERT_FALSE((bool)refused);
    ASSERT_FALSE((bool)refusedCopy);
    ASSERT_EQ(refused(), 0);
}

namespace
{
    /** An argument that can be moved but never copied. */
    struct MoveOnly
    {
        int m_value;
        explicit MoveOnly(int value) : m_value(value) {}
        MoveOnly(MoveOnly &&other) : m_value(other.m_value) { other.m_value = 0; }
        MoveOnly(const MoveOnly &) = delete;
    };
}

TEST(pdistl, a_function_takes_an_argument_that_can_only_be_moved)
{
    pdiutil::function<int(MoveOnly)> f = [](MoveOnly m) { return m.m_value; };
    ASSERT_EQ(f(MoveOnly(11)), 11);
}

TEST(pdistl, functions_swap_their_callables)
{
    pdiutil::function<int(int)> a = Tracker(1);
    pdiutil::function<int(int)> b = [](int x) { return x * 100; };
    pdiutil::function<int(int)> none;

    a.swap(b);
    ASSERT_EQ(a(2), 200);
    ASSERT_EQ(b(2), 3);

    pdistd::swap(b, none);
    ASSERT_FALSE((bool)b);
    ASSERT_EQ(none(2), 3);
}

TEST(pdistl, a_function_compares_with_nullptr_either_way_round)
{
    pdiutil::function<void()> empty;
    pdiutil::function<void()> full = []() {};

    ASSERT_TRUE(empty == nullptr);
    ASSERT_TRUE(nullptr == empty);
    ASSERT_TRUE(full != nullptr);
    ASSERT_TRUE(nullptr != full);
    ASSERT_FALSE(nullptr == full);
}

#if __PDISTD_FUNCTION_INLINE_STORAGE__
TEST(pdistl, a_function_hands_back_its_target_only_as_the_type_it_is)
{
    pdiutil::function<int(int)> f = Tracker(7);
    ASSERT_NOT_NULL(f.target<Tracker>());
    ASSERT_EQ(f.target<Tracker>()->m_value, 7);
    ASSERT_NULL(f.target<int (*)(int)>());

    pdiutil::function<int()> pointer = &answer;
    ASSERT_NOT_NULL(pointer.target<int (*)()>());
    ASSERT_TRUE(*pointer.target<int (*)()>() == &answer);
}
#endif

TEST(pdistl, a_function_of_another_signature_is_wrapped_and_an_empty_one_stays_empty)
{
    pdiutil::function<int()> source = []() { return 3; };
    pdiutil::function<void()> wrapped(source);
    pdiutil::function<long()> widened(source);
    pdiutil::function<int()> emptySource;
    pdiutil::function<void()> fromEmpty(emptySource);

    wrapped();
    ASSERT_TRUE((bool)wrapped);
    ASSERT_EQ(widened(), 3L);
    ASSERT_FALSE((bool)fromEmpty);
}

TEST(pdistl, a_function_is_as_big_as_its_storage_and_tests_as_bool_only_explicitly)
{
    const size_t pointers = __PDISTD_FUNCTION_INLINE_STORAGE__ ? 2 : 1;
    ASSERT_EQ(sizeof(pdiutil::function<void()>), pointers * sizeof(void *));
    ASSERT_EQ(sizeof(pdiutil::function<int(int, int)>), pointers * sizeof(void *));
    ASSERT_FALSE((std::is_convertible<pdiutil::function<void()>, bool>::value));
    ASSERT_TRUE((std::is_constructible<bool, pdiutil::function<void()>>::value));
}
