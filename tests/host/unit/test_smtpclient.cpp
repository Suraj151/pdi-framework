/****************************** SMTP Client Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The smtp client against a scripted server that answers each line the way a mail
server does. What the client sent is checked word for word, and each way a
server can say no — a bad greeting, a refused login, a refused recipient, a
reply too long for the client's buffer — has to stop the conversation there.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <pditest.h>

#ifdef ENABLE_EMAIL_SERVICE

#include <ScriptedSmtp.h>
#include <transports/smtp/SMTPClient.h>
#include <string>

using pditest::ScriptedSmtp;

namespace {

/**
 * Walks the client through a whole message, stopping at the first refusal,
 * and reports how far it got.
 */
int sendWholeMessage(SMTPClient &smtp, ScriptedSmtp &server, const char *text = "pin 4 went high")
{
    char host[] = "mail.test";
    char domain[] = "device.test";
    char user[] = "user@device.test";
    char pass[] = "s3cret";
    char from[] = "user@device.test";
    char to[] = "ops@device.test";
    char name[] = "Device";
    char subject[] = "Alarm";

    int step = 0;
    if (!smtp.begin(&server, host, 587)) return step;
    step++;
    if (!smtp.sendHello(domain)) return step;
    step++;
    if (!smtp.sendAuthLogin(user, pass)) return step;
    step++;
    if (!smtp.sendFrom(from)) return step;
    step++;
    if (!smtp.sendTo(to)) return step;
    step++;
    if (!smtp.sendDataCommand()) return step;
    step++;
    smtp.sendDataHeader(name, to, subject);
    pdiutil::string body = text;
    if (!smtp.sendDataBody(body)) return step;
    step++;
    if (!smtp.sendQuit()) return step;
    step++;
    return step;
}

const int WHOLE = 8;

bool has(const std::string &text, const char *needle)
{
    return std::string::npos != text.find(needle);
}

/**
 * Whether every line feed the client sent was the end of a CRLF.
 */
bool onlyCrlf(const std::string &text)
{
    for (size_t i = 0; i < text.size(); i++)
    {
        if ('\n' == text[i] && (0 == i || '\r' != text[i - 1])) return false;
    }
    return true;
}

} // namespace

TEST(smtpclient, a_whole_message_goes_through_every_step)
{
    ScriptedSmtp server;
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), WHOLE);
    ASSERT_EQ(server.m_messages, 1);
    smtp.end();
}

TEST(smtpclient, the_client_says_exactly_what_the_protocol_asks)
{
    ScriptedSmtp server;
    SMTPClient smtp;
    sendWholeMessage(smtp, server);
    smtp.end();

    const std::string &said = server.m_conversation;
    ASSERT_TRUE(has(said, "EHLO device.test\r\n"));
    ASSERT_TRUE(has(said, "AUTH LOGIN\r\n"));
    ASSERT_TRUE(has(said, "dXNlckBkZXZpY2UudGVzdA==\r\n"));
    ASSERT_TRUE(has(said, "czNjcmV0\r\n"));
    ASSERT_TRUE(has(said, "MAIL FROM:<user@device.test>\r\n"));
    ASSERT_TRUE(has(said, "RCPT TO:<ops@device.test>\r\n"));
    ASSERT_TRUE(has(said, "DATA\r\n"));
    ASSERT_TRUE(has(said, "\r\n.\r\n"));
    ASSERT_TRUE(has(said, "QUIT\r\n"));
    ASSERT_TRUE(said.find("EHLO") < said.find("AUTH LOGIN"));
    ASSERT_TRUE(said.find("RCPT TO") < said.find("DATA"));
}

TEST(smtpclient, the_message_carries_its_headers_and_body)
{
    ScriptedSmtp server;
    SMTPClient smtp;
    sendWholeMessage(smtp, server);
    smtp.end();

    ASSERT_TRUE(has(server.m_body, "To: ops@device.test"));
    ASSERT_TRUE(has(server.m_body, "From: Device"));
    ASSERT_TRUE(has(server.m_body, "Subject: Alarm"));
    ASSERT_TRUE(has(server.m_body, "pin 4 went high"));
}

TEST(smtpclient, a_body_line_of_a_lone_dot_does_not_end_the_message)
{
    ScriptedSmtp server;
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server, "first\n.\nQUIT\nlast"), WHOLE);
    smtp.end();

    ASSERT_EQ(server.m_messages, 1);
    ASSERT_TRUE(has(server.m_body, "first\n.\nQUIT\nlast\n"));
    ASSERT_TRUE(has(server.m_conversation, "first\r\n..\r\nQUIT\r\n"));
}

TEST(smtpclient, a_body_line_starting_with_a_dot_keeps_its_dot)
{
    ScriptedSmtp server;
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server, ".hidden\n..two"), WHOLE);
    smtp.end();

    ASSERT_TRUE(has(server.m_body, "\n.hidden\n..two\n"));
}

TEST(smtpclient, every_body_line_ends_in_crlf_whatever_the_text_used)
{
    ScriptedSmtp server;
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server, "unix\nline\r\nwindows\r\n\nRegards\n"), WHOLE);
    smtp.end();

    ASSERT_TRUE(onlyCrlf(server.m_conversation));
    ASSERT_TRUE(has(server.m_body, "unix\nline\nwindows\n\nRegards\n"));
}

TEST(smtpclient, a_server_that_does_not_greet_ready_is_not_a_connection)
{
    ScriptedSmtp server;
    server.m_reply["greeting"] = "554 no service here\r\n";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 0);
    smtp.end();
}

TEST(smtpclient, a_refused_login_stops_before_the_message)
{
    ScriptedSmtp server;
    server.m_reply["PASS"] = "535 5.7.8 credentials invalid\r\n";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 2);
    ASSERT_FALSE(has(server.m_conversation, "MAIL FROM"));
    smtp.end();
}

TEST(smtpclient, a_refused_recipient_stops_before_the_data)
{
    ScriptedSmtp server;
    server.m_reply["RCPT"] = "550 5.1.1 no such user\r\n";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 4);
    ASSERT_FALSE(has(server.m_conversation, "DATA"));
    smtp.end();
}

TEST(smtpclient, a_message_the_server_will_not_queue_is_a_failure)
{
    ScriptedSmtp server;
    server.m_reply["END"] = "552 5.3.4 message too big\r\n";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 6);
    smtp.end();
}

TEST(smtpclient, a_message_the_server_never_acknowledges_is_a_failure)
{
    ScriptedSmtp server;
    server.m_reply["END"] = "";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 6);
    ASSERT_FALSE(has(server.m_conversation, "QUIT"));
    smtp.end();
}

TEST(smtpclient, a_server_silent_at_the_login_stops_there)
{
    ScriptedSmtp server;
    server.m_reply["AUTH"] = "";
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), 2);
    ASSERT_FALSE(has(server.m_conversation, "dXNlckBkZXZpY2UudGVzdA=="));
    ASSERT_FALSE(has(server.m_conversation, "MAIL FROM"));
    smtp.end();
}

TEST(smtpclient, a_reply_longer_than_the_buffer_is_read_safely)
{
    ScriptedSmtp server;
    std::string longreply;
    while (longreply.size() < SMTP_RESPONSE_BUFFER_SIZE + 200)
    {
        longreply += "250-an extension the server advertises at some length\r\n";
    }
    longreply += "250 AUTH LOGIN\r\n";
    server.m_reply["EHLO"] = longreply;

    SMTPClient smtp;
    char host[] = "mail.test";
    char domain[] = "device.test";

    ASSERT_TRUE(smtp.begin(&server, host, 587));
    ASSERT_TRUE(smtp.sendHello(domain));
    ASSERT_TRUE(smtp.sendFrom((char *)"user@device.test"));
    smtp.end();
}

TEST(smtpclient, a_secure_session_is_refused_over_a_plain_connection)
{
    ScriptedSmtp server;
    SMTPClient smtp;
    char host[] = "mail.test";

    ASSERT_FALSE(smtp.begin(&server, host, 465, true));
    ASSERT_FALSE(has(server.m_conversation, "EHLO"));
    smtp.end();
}

TEST(smtpclient, a_server_that_never_answers_times_out)
{
    ScriptedSmtp server;
    server.m_reply["greeting"] = "";
    SMTPClient smtp;
    char host[] = "mail.test";

    uint32_t began = __i_dvc_ctrl.millis_now();
    ASSERT_FALSE(smtp.begin(&server, host, 587));
    uint32_t took = __i_dvc_ctrl.millis_now() - began;

    ASSERT_GE(took, (uint32_t)(SMTP_DEFAULT_TIMEOUT - 100));
    ASSERT_LT(took, (uint32_t)(SMTP_DEFAULT_TIMEOUT * 3));
    smtp.end();
}

TEST(smtpclient, ending_a_session_closes_the_connection)
{
    ScriptedSmtp server;
    SMTPClient smtp;

    ASSERT_EQ(sendWholeMessage(smtp, server), WHOLE);
    smtp.end();
    ASSERT_EQ(server.m_connected, 0);
}

TEST(smtpclient, a_client_can_send_a_second_message_after_ending_the_first)
{
    SMTPClient smtp;
    {
        ScriptedSmtp first;
        ASSERT_EQ(sendWholeMessage(smtp, first), WHOLE);
        smtp.end();
    }
    {
        ScriptedSmtp second;
        ASSERT_EQ(sendWholeMessage(smtp, second), WHOLE);
        smtp.end();
    }
}

#endif
