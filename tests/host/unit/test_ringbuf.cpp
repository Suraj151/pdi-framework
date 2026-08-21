/***************************** Ring Buffer Tests ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <pditest.h>
#include <utility/queue/ringbuf.h>

TEST(ringbuf, init_accepts_a_valid_buffer)
{
    RINGBUF ring;
    uint8_t storage[8];
    ASSERT_EQ(RINGBUF_Init(&ring, storage, sizeof(storage)), 0);
}

TEST(ringbuf, init_rejects_null_and_undersized_buffers)
{
    RINGBUF ring;
    uint8_t storage[8];
    ASSERT_EQ(RINGBUF_Init(nullptr, storage, sizeof(storage)), -1);
    ASSERT_EQ(RINGBUF_Init(&ring, nullptr, sizeof(storage)), -1);
    ASSERT_EQ(RINGBUF_Init(&ring, storage, 1), -1);
}

TEST(ringbuf, get_on_an_empty_buffer_fails)
{
    RINGBUF ring;
    uint8_t storage[8];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));
    ASSERT_EQ(RINGBUF_Get(&ring, &out), -1);
}

TEST(ringbuf, put_then_get_returns_the_same_byte)
{
    RINGBUF ring;
    uint8_t storage[8];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));
    ASSERT_EQ(RINGBUF_Put(&ring, 0xA5), 0);
    ASSERT_EQ(RINGBUF_Get(&ring, &out), 0);
    ASSERT_EQ(out, (uint8_t)0xA5);
}

TEST(ringbuf, preserves_order)
{
    RINGBUF ring;
    uint8_t storage[8];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));
    for (uint8_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(RINGBUF_Put(&ring, (uint8_t)(i + 1)), 0);
    }

    for (uint8_t i = 0; i < 5; i++)
    {
        ASSERT_EQ(RINGBUF_Get(&ring, &out), 0);
        ASSERT_EQ(out, (uint8_t)(i + 1));
    }
}

TEST(ringbuf, refuses_a_put_when_full)
{
    RINGBUF ring;
    uint8_t storage[4];

    RINGBUF_Init(&ring, storage, sizeof(storage));
    for (uint8_t i = 0; i < 4; i++)
    {
        ASSERT_EQ(RINGBUF_Put(&ring, i), 0);
    }

    ASSERT_EQ(RINGBUF_Put(&ring, 99), -1);
}

TEST(ringbuf, full_buffer_keeps_its_contents_after_a_refused_put)
{
    RINGBUF ring;
    uint8_t storage[4];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));
    for (uint8_t i = 0; i < 4; i++)
    {
        RINGBUF_Put(&ring, (uint8_t)(i + 10));
    }
    RINGBUF_Put(&ring, 99);

    for (uint8_t i = 0; i < 4; i++)
    {
        ASSERT_EQ(RINGBUF_Get(&ring, &out), 0);
        ASSERT_EQ(out, (uint8_t)(i + 10));
    }
}

TEST(ringbuf, wraps_around_the_physical_end)
{
    RINGBUF ring;
    uint8_t storage[4];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));

    // walk the write pointer past the end several times over
    for (uint8_t round = 0; round < 5; round++)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            ASSERT_EQ(RINGBUF_Put(&ring, (uint8_t)(round * 10 + i)), 0);
        }
        for (uint8_t i = 0; i < 3; i++)
        {
            ASSERT_EQ(RINGBUF_Get(&ring, &out), 0);
            ASSERT_EQ(out, (uint8_t)(round * 10 + i));
        }
    }
}

TEST(ringbuf, interleaved_put_and_get_stay_in_order)
{
    RINGBUF ring;
    uint8_t storage[4];
    uint8_t out = 0;
    uint8_t next_written = 0;
    uint8_t next_read = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));

    for (uint8_t step = 0; step < 40; step++)
    {
        if (0 == RINGBUF_Put(&ring, next_written))
        {
            next_written++;
        }
        if (step % 2 == 1)
        {
            if (0 == RINGBUF_Get(&ring, &out))
            {
                ASSERT_EQ(out, next_read);
                next_read++;
            }
        }
    }

    ASSERT_GT(next_read, (uint8_t)0);
}

TEST(ringbuf, reports_fill_count)
{
    RINGBUF ring;
    uint8_t storage[8];
    uint8_t out = 0;

    RINGBUF_Init(&ring, storage, sizeof(storage));
    ASSERT_EQ(ring.fill_cnt, 0);

    RINGBUF_Put(&ring, 1);
    RINGBUF_Put(&ring, 2);
    ASSERT_EQ(ring.fill_cnt, 2);

    RINGBUF_Get(&ring, &out);
    ASSERT_EQ(ring.fill_cnt, 1);
}
