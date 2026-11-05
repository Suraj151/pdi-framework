/***************************** Safe Alloc Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Leak and overflow detection comes from the sanitizers the host build enables,
so these tests assert the contracts and let ASan judge the memory itself.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/DataTypeDef.h>
#include <utility/SafeAlloc.h>
#include <utility/Utility.h>
#include <devices/posix/DeviceControlInterface.h>

struct Tracked
{
    static int s_alive;
    int value;

    Tracked() : value(7) { s_alive++; }
    ~Tracked() { s_alive--; }
};

int Tracked::s_alive = 0;

static thread_local bool s_refuse_next = false;

void *operator new(size_t size, const std::nothrow_t &) noexcept
{
    if (s_refuse_next)
    {
        s_refuse_next = false;
        return nullptr;
    }
    try { return ::operator new(size); } catch (...) { return nullptr; }
}

void *operator new[](size_t size, const std::nothrow_t &) noexcept
{
    if (s_refuse_next)
    {
        s_refuse_next = false;
        return nullptr;
    }
    try { return ::operator new[](size); } catch (...) { return nullptr; }
}

TEST(safealloc, new_array_returns_usable_memory)
{
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(32);
    ASSERT_NOT_NULL(block);

    block[0] = 1;
    block[31] = 2;
    ASSERT_EQ(block[0], (uint8_t)1);
    ASSERT_EQ(block[31], (uint8_t)2);

    pdiutil::safe_delete_array(block);
    ASSERT_NULL(block);
}

TEST(safealloc, new_array_value_initializes)
{
    const size_t count = 64;
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(count);
    ASSERT_NOT_NULL(block);

    for (size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(block[i], (uint8_t)0);
    }

    pdiutil::safe_delete_array(block);
}

TEST(safealloc, new_array_value_initializes_wider_types)
{
    const size_t count = 16;
    uint32_t *block = pdiutil::safe_new_array<uint32_t>(count);
    ASSERT_NOT_NULL(block);

    for (size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(block[i], 0u);
    }

    pdiutil::safe_delete_array(block);
}

TEST(safealloc, new_array_of_zero_returns_null)
{
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(0);
    ASSERT_NULL(block);
}

TEST(safealloc, delete_array_tolerates_null)
{
    uint8_t *block = nullptr;
    pdiutil::safe_delete_array(block);
    ASSERT_NULL(block);
}

TEST(safealloc, delete_array_clears_the_caller_pointer)
{
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(8);
    ASSERT_NOT_NULL(block);
    pdiutil::safe_delete_array(block);
    ASSERT_NULL(block);
}

TEST(safealloc, new_constructs_the_object)
{
    Tracked::s_alive = 0;
    Tracked *object = pdiutil::safe_new<Tracked>();
    ASSERT_NOT_NULL(object);
    ASSERT_EQ(object->value, 7);
    ASSERT_EQ(Tracked::s_alive, 1);

    pdiutil::safe_delete(object);
    ASSERT_NULL(object);
    ASSERT_EQ(Tracked::s_alive, 0);
}

TEST(safealloc, delete_tolerates_null)
{
    Tracked *object = nullptr;
    pdiutil::safe_delete(object);
    ASSERT_NULL(object);
}

TEST(safealloc, a_string_buffer_keeps_room_for_its_terminator)
{
    const char *source = "pdiStack";
    size_t len = strlen(source);

    char *copy = pdiutil::safe_new_array<char>(len + 1);
    ASSERT_NOT_NULL(copy);

    memcpy(copy, source, len);
    ASSERT_EQ(copy[len], '\0');
    ASSERT_STREQ(copy, source);

    pdiutil::safe_delete_array(copy);
}

TEST(safealloc, has_heap_for_answers_before_the_check_is_armed)
{
    ASSERT_TRUE(pdiutil::has_heap_for(16));
}

TEST(safealloc, upto_takes_the_expected_size_when_the_heap_holds_it)
{
    pdiutil::enable_heap_check();
    size_t granted = 0;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(4096, 512, granted);
    ASSERT_NOT_NULL(block);
    ASSERT_EQ(granted, (size_t)4096);
    block[4095] = 1;
    pdiutil::safe_delete_array(block);
}

TEST(safealloc, upto_takes_the_largest_block_when_the_expected_size_does_not_fit)
{
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(1500);
    size_t granted = 0;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(4096, 512, granted);
    __i_dvc_ctrl.capMaxFreeBlock(0);
    ASSERT_NOT_NULL(block);
    ASSERT_EQ(granted, (size_t)1500);
    block[1499] = 1;
    pdiutil::safe_delete_array(block);
}

TEST(safealloc, upto_keeps_the_heap_margin_on_the_total)
{
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capFreeHeap(PDI_SAFE_ALLOC_HEAP_MARGIN + 1000);
    size_t granted = 0;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(4096, 512, granted);
    __i_dvc_ctrl.capFreeHeap(0);
    ASSERT_NOT_NULL(block);
    ASSERT_EQ(granted, (size_t)1000);
    pdiutil::safe_delete_array(block);
}

TEST(safealloc, upto_counts_elements_not_bytes)
{
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(1000);
    size_t granted = 0;
    uint32_t *block = pdiutil::safe_new_array_upto<uint32_t>(1024, 64, granted);
    __i_dvc_ctrl.capMaxFreeBlock(0);
    ASSERT_NOT_NULL(block);
    ASSERT_EQ(granted, (size_t)250);
    ASSERT_EQ(block[249], 0u);
    pdiutil::safe_delete_array(block);
}

TEST(safealloc, upto_refuses_below_the_minimum_and_counts_it)
{
    pdiutil::enable_heap_check();
    uint32_t before = pdiutil::safe_alloc_refusals();
    __i_dvc_ctrl.capMaxFreeBlock(300);
    size_t granted = 99;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(4096, 512, granted);
    __i_dvc_ctrl.capMaxFreeBlock(0);
    ASSERT_NULL(block);
    ASSERT_EQ(granted, (size_t)0);
    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)1);
}

TEST(safealloc, upto_grows_back_once_memory_frees)
{
    pdiutil::enable_heap_check();
    size_t granted = 0;
    __i_dvc_ctrl.capMaxFreeBlock(700);
    uint8_t *small = pdiutil::safe_new_array_upto<uint8_t>(2048, 256, granted);
    __i_dvc_ctrl.capMaxFreeBlock(0);
    ASSERT_EQ(granted, (size_t)700);
    pdiutil::safe_delete_array(small);

    uint8_t *full = pdiutil::safe_new_array_upto<uint8_t>(2048, 256, granted);
    ASSERT_EQ(granted, (size_t)2048);
    pdiutil::safe_delete_array(full);
}

TEST(safealloc, upto_with_no_minimum_needs_the_whole_expected_size)
{
    pdiutil::enable_heap_check();
    __i_dvc_ctrl.capMaxFreeBlock(100);
    size_t granted = 0;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(200, 0, granted);
    __i_dvc_ctrl.capMaxFreeBlock(0);
    ASSERT_NULL(block);
    ASSERT_EQ(granted, (size_t)0);
}

TEST(safealloc, raw_bytes_the_allocator_refuses_come_back_null_and_counted)
{
    uint32_t before = pdiutil::safe_alloc_refusals();
    s_refuse_next = true;
    void *storage = pdiutil::safe_alloc_bytes(16384);
    s_refuse_next = false;
    ASSERT_NULL(storage);
    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)1);

    storage = pdiutil::safe_alloc_bytes(16384);
    ASSERT_NOT_NULL(storage);
    pdiutil::safe_free_bytes(storage);
}

TEST(safealloc, an_array_the_allocator_refuses_comes_back_null_and_counted)
{
    uint32_t before = pdiutil::safe_alloc_refusals();
    s_refuse_next = true;
    uint8_t *block = pdiutil::safe_new_array<uint8_t>(4096);
    s_refuse_next = false;
    ASSERT_NULL(block);
    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)1);
}

TEST(safealloc, an_object_the_allocator_refuses_is_never_constructed)
{
    Tracked::s_alive = 0;
    uint32_t before = pdiutil::safe_alloc_refusals();
    s_refuse_next = true;
    Tracked *object = pdiutil::safe_new<Tracked>();
    s_refuse_next = false;
    ASSERT_NULL(object);
    ASSERT_EQ(Tracked::s_alive, 0);
    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)1);
}

TEST(safealloc, upto_the_allocator_refuses_grants_nothing_and_counts_it)
{
    uint32_t before = pdiutil::safe_alloc_refusals();
    size_t granted = 99;
    s_refuse_next = true;
    uint8_t *block = pdiutil::safe_new_array_upto<uint8_t>(2048, 256, granted);
    s_refuse_next = false;
    ASSERT_NULL(block);
    ASSERT_EQ(granted, (size_t)0);
    ASSERT_EQ(pdiutil::safe_alloc_refusals() - before, (uint32_t)1);
}

TEST(safealloc, a_vector_the_allocator_refuses_keeps_its_old_storage)
{
    pdiutil::vector<uint8_t> data;
    data.push_back(7);
    size_t had = data.capacity();
    s_refuse_next = true;
    data.reserve(16384);
    s_refuse_next = false;
    ASSERT_EQ(data.capacity(), had);
    ASSERT_EQ(data.size(), (size_t)1);
    ASSERT_EQ(data[0], (uint8_t)7);
}

TEST(roptr, wraps_a_read_only_string)
{
    rofn::ROPTR wrapped = ROPTR_WRAP("read only text");
    ASSERT_STREQ((char *)wrapped, "read only text");
}

TEST(roptr, charptr_wrap_yields_a_usable_copy)
{
    rofn::ROPTR wrapped = ROPTR_WRAP("pdi");
    char *text = (char *)wrapped;
    ASSERT_NOT_NULL(text);
    ASSERT_EQ(strlen(text), (size_t)3);
}

TEST(roptr, copy_transfers_ownership_to_the_copy)
{
    rofn::ROPTR original = ROPTR_WRAP("owned once");
    rofn::ROPTR copy = original;

    ASSERT_STREQ((char *)copy, "owned once");
}

TEST(roptr, move_transfers_ownership_to_the_target)
{
    rofn::ROPTR original = ROPTR_WRAP("moved once");
    rofn::ROPTR moved = static_cast<rofn::ROPTR &&>(original);

    ASSERT_STREQ((char *)moved, "moved once");
}

TEST(roptr, assignment_releases_the_previous_buffer)
{
    rofn::ROPTR target = ROPTR_WRAP("first");
    rofn::ROPTR replacement = ROPTR_WRAP("second");

    target = replacement;
    ASSERT_STREQ((char *)target, "second");
}

TEST(roptr, wrapping_an_empty_string_is_safe)
{
    rofn::ROPTR wrapped = ROPTR_WRAP("");
    ASSERT_STREQ((char *)wrapped, "");
}

TEST(roptr, to_charptr_rejects_null)
{
    ASSERT_NULL(rofn::to_charptr(nullptr));
}
