/****************************** Scripted SMTP *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

A mail server behind the client interface. It answers each command line the
way a server does, records the whole conversation, and keeps the message body
as the server would deliver it, with the transparency dot removed.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#ifndef _PDITEST_SCRIPTED_SMTP_H_
#define _PDITEST_SCRIPTED_SMTP_H_

#include <interface/pdi/middlewares/iClientInterface.h>
#include <cstring>
#include <string>
#include <map>

namespace pditest
{

/**
 * @brief A mail server that answers every command line it is sent, and lets a
 * test change any one of its answers.
 */
class ScriptedSmtp : public iClientInterface
{

public:

    ScriptedSmtp() : m_connected(0), m_messages(0), m_greetingDue(false), m_inData(false), m_authStep(0)
    {
        m_reply["greeting"] = "220 mail.test ESMTP ready\r\n";
        m_reply["EHLO"] = "250-mail.test greets you\r\n250-SIZE 35882577\r\n250 AUTH LOGIN PLAIN\r\n";
        m_reply["AUTH"] = "334 VXNlcm5hbWU6\r\n";
        m_reply["USER"] = "334 UGFzc3dvcmQ6\r\n";
        m_reply["PASS"] = "235 2.7.0 Accepted\r\n";
        m_reply["MAIL"] = "250 2.1.0 OK\r\n";
        m_reply["RCPT"] = "250 2.1.5 OK\r\n";
        m_reply["DATA"] = "354 Go ahead\r\n";
        m_reply["END"] = "250 2.0.0 queued\r\n";
        m_reply["QUIT"] = "221 2.0.0 closing\r\n";
    }

    int16_t connect(const uint8_t *host, uint16_t port) override
    {
        m_connected = 1;
        m_greetingDue = true;
        return 1;
    }

    int16_t disconnect() override { m_connected = 0; return 1; }
    int8_t connected() override { return m_connected; }
    void setTimeout(uint32_t timeout) override {}

    int32_t write(uint8_t c) override
    {
        take((const char *)&c, 1);
        return 1;
    }

    int32_t write(const uint8_t *c_str) override
    {
        uint32_t len = (uint32_t)strlen((const char *)c_str);
        take((const char *)c_str, len);
        return (int32_t)len;
    }

    int32_t write(const uint8_t *c_str, uint32_t size) override
    {
        take((const char *)c_str, size);
        return (int32_t)size;
    }

    int32_t available() override
    {
        // a greeting arrives a moment after the connect, so a drain made
        // straight after connecting sees nothing yet
        if (m_greetingDue)
        {
            m_greetingDue = false;
            m_pending += m_reply["greeting"];
            return 0;
        }
        return (int32_t)m_pending.size();
    }

    uint8_t read() override
    {
        if (m_pending.empty()) return 0;
        uint8_t c = (uint8_t)m_pending[0];
        m_pending.erase(0, 1);
        return c;
    }

    int32_t read(uint8_t *buf, uint32_t size) override
    {
        uint32_t taken = 0;
        while (taken < size && !m_pending.empty())
        {
            buf[taken++] = (uint8_t)m_pending[0];
            m_pending.erase(0, 1);
        }
        return (int32_t)taken;
    }

    std::map<std::string, std::string> m_reply;
    std::string m_conversation;
    std::string m_body;
    int8_t m_connected;
    int m_messages;

private:

    void take(const char *data, uint32_t size)
    {
        m_conversation.append(data, size);
        m_line.append(data, size);

        size_t end;
        while (std::string::npos != (end = m_line.find("\r\n")))
        {
            std::string line = m_line.substr(0, end);
            m_line.erase(0, end + 2);
            answer(line);
        }
    }

    void answer(const std::string &line)
    {
        if (m_inData)
        {
            if ("." == line)
            {
                m_inData = false;
                m_messages++;
                m_pending += m_reply["END"];
            }
            else
            {
                m_body += (!line.empty() && '.' == line[0]) ? line.substr(1) : line;
                m_body += "\n";
            }
            return;
        }

        if (1 == m_authStep) { m_authStep = 2; m_pending += m_reply["USER"]; return; }
        if (2 == m_authStep) { m_authStep = 0; m_pending += m_reply["PASS"]; return; }

        std::string verb = line.substr(0, 4);
        if ("AUTH" == verb) m_authStep = 1;
        if ("DATA" == verb) m_inData = true;
        if (m_reply.count(verb)) m_pending += m_reply[verb];
    }

    bool m_greetingDue;
    bool m_inData;
    int m_authStep;
    std::string m_line;
    std::string m_pending;
};

} // namespace pditest

#endif // _PDITEST_SCRIPTED_SMTP_H_
