/***************************** String Terminal ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A terminal backed by two in memory buffers. Input is fed in by the test and
everything the unit writes is captured for inspection, so anything that talks
through iTerminalInterface can run without a device.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDI_TEST_STRING_TERMINAL_H_
#define _PDI_TEST_STRING_TERMINAL_H_

#include <string>
#include <utility/iIOInterface.h>

namespace pditest
{

    class StringTerminal : public iTerminalInterface
    {

    public:
        StringTerminal() : m_readpos(0), m_connected(true) {}
        ~StringTerminal() {}

        /**
         * @brief Queue text for the unit under test to read.
         */
        void feed(const char *text)
        {
            m_input.append(text);
        }

        /**
         * @brief Queue raw bytes, including any that are not printable.
         */
        void feed(const uint8_t *bytes, size_t len)
        {
            m_input.append((const char *)bytes, len);
        }

        /**
         * @brief Everything written to the terminal so far.
         */
        const std::string &captured() const { return m_output; }

        /**
         * @brief Whether the captured output contains the given text.
         */
        bool sawText(const char *needle) const
        {
            return m_output.find(needle) != std::string::npos;
        }

        /**
         * @brief Drop captured output and any unread input.
         */
        void forget()
        {
            m_output.clear();
            m_input.clear();
            m_readpos = 0;
        }

        void setConnected(bool connected) { m_connected = connected; }

        // overriding some write and read overloads would otherwise hide the
        // numeric and padded ones the base supplies
        using iTerminalInterface::read;
        using iTerminalInterface::write;

        int16_t disconnect() override
        {
            m_connected = false;
            return 0;
        }

        int32_t write(uint8_t c) override
        {
            m_output.push_back((char)c);
            return 1;
        }

        int32_t write(const uint8_t *c_str) override
        {
            if (nullptr == c_str)
            {
                return 0;
            }
            return write(c_str, (uint32_t)strlen((const char *)c_str));
        }

        int32_t write(const uint8_t *c_str, uint32_t size) override
        {
            if (nullptr == c_str || 0 == size)
            {
                return 0;
            }
            m_output.append((const char *)c_str, size);
            return (int32_t)size;
        }

        // the host keeps read only data in ordinary memory, so this is a plain write
        int32_t write_ro(const char *c_str) override
        {
            return write((const uint8_t *)c_str);
        }

        uint8_t read() override
        {
            if (m_readpos >= m_input.size())
            {
                return 0;
            }
            return (uint8_t)m_input[m_readpos++];
        }

        int32_t read(uint8_t *buf, uint32_t size) override
        {
            uint32_t count = 0;
            while (count < size && m_readpos < m_input.size())
            {
                buf[count++] = (uint8_t)m_input[m_readpos++];
            }
            return (int32_t)count;
        }

        int32_t available() override
        {
            return (int32_t)(m_input.size() - m_readpos);
        }

        int8_t connected() override { return m_connected ? 1 : 0; }

    private:
        std::string m_input;
        std::string m_output;
        size_t m_readpos;
        bool m_connected;
    };

} // namespace pditest

#endif // _PDI_TEST_STRING_TERMINAL_H_
