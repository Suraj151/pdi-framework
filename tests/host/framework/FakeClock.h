/******************************* Fake Clock ***********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A utility interface whose time only moves when a test moves it, so anything
driven off millis_now can be exercised without waiting in real seconds.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDI_TEST_FAKE_CLOCK_H_
#define _PDI_TEST_FAKE_CLOCK_H_

#include <utility/iUtilityInterface.h>

namespace pditest
{

    class FakeClock : public iUtilityInterface
    {

    public:
        FakeClock() : m_micros(0), m_yield_count(0), m_random_state(0x2545F491u) {}
        ~FakeClock() {}

        /**
         * @brief Move time forward by whole milliseconds.
         */
        void advance(uint64_t milliseconds) { m_micros += milliseconds * 1000ULL; }

        /**
         * @brief Move time forward by microseconds.
         */
        void advanceMicros(uint64_t microseconds) { m_micros += microseconds; }

        /**
         * @brief Put the clock back to zero.
         */
        void reset()
        {
            m_micros = 0;
            m_yield_count = 0;
        }

        /**
         * @brief How many times the unit under test yielded.
         */
        uint32_t yieldCount() const { return m_yield_count; }

        // a wait has to move the clock, or a caller that waits would hang forever
        void wait(double timeoutms) override { m_micros += (uint64_t)(timeoutms * 1000.0); }

        uint32_t millis_now() override { return (uint32_t)(m_micros / 1000ULL); }
        uint64_t micros_now() override { return m_micros; }

        void log(logger_type_t log_type, const char *content) override {}
        void yield() override { m_yield_count++; }

        uint32_t random_now() override
        {
            m_random_state ^= m_random_state << 13;
            m_random_state ^= m_random_state >> 17;
            m_random_state ^= m_random_state << 5;
            return m_random_state;
        }

        uint32_t get_free_heap() override { return 65536; }

    private:
        uint64_t m_micros;
        uint32_t m_yield_count;
        uint32_t m_random_state;
    };

} // namespace pditest

#endif // _PDI_TEST_FAKE_CLOCK_H_
