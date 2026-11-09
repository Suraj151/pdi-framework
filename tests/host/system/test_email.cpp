/******************************** Email Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The email service sends one message from its saved configuration. A message
goes out only when the configuration names a server, a domain, a sender and a
recipient, every step of the conversation uses the saved value, and a refusal
anywhere still ends the session rather than leaving the connection open. A mail
is taken at once and goes out one step per scheduler tick; its result arrives
through the done callback.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <ScriptedSmtp.h>
#include <StringTerminal.h>
#include <pditest.h>

#ifdef ENABLE_EMAIL_SERVICE

#include <service_provider/email/EmailServiceProvider.h>
#include <string>

using pditest::ScriptedSmtp;

namespace
{

    /**
     * Points the service at a scripted server with a known configuration, and
     * puts both the saved configuration and the service's client back after.
     */
    struct MailSetup
    {
        MailSetup(ScriptedSmtp &server)
        {
            pditest::mountedDb();
            pditest::readyScheduler();
            __database_service.get_email_config_table(&m_saved);
            __email_service.initService(&server);

            email_config_table table;
            setText(table.sending_domain, sizeof(table.sending_domain), "device.test");
            setText(table.mail_host, sizeof(table.mail_host), "mail.test");
            table.mail_port = 587;
            setText(table.mail_username, sizeof(table.mail_username), "user@device.test");
            setText(table.mail_password, sizeof(table.mail_password), "s3cret");
            setText(table.mail_from, sizeof(table.mail_from), "user@device.test");
            setText(table.mail_from_name, sizeof(table.mail_from_name), "Device");
            setText(table.mail_to, sizeof(table.mail_to), "ops@device.test");
            setText(table.mail_subject, sizeof(table.mail_subject), "Alarm");
            save(table);
        }

        ~MailSetup()
        {
            __database_service.set_email_config_table(&m_saved);
            __email_service.initService(nullptr);
        }

        static void setText(char *dest, size_t size, const char *text)
        {
            memset(dest, 0, size);
            strncpy(dest, text, size - 1);
        }

        void save(email_config_table &table)
        {
            ASSERT_TRUE(__database_service.set_email_config_table(&table));
        }

        email_config_table m_saved;
    };

    bool has(const std::string &text, const char *needle)
    {
        return std::string::npos != text.find(needle);
    }

    /**
     * Runs the scheduler until result is set by a done callback, or the time
     * given has passed.
     */
    void pumpUntilDone(const int &result, uint32_t limit_ms)
    {
        uint32_t began = __i_dvc_ctrl.millis_now();
        while (-1 == result && (__i_dvc_ctrl.millis_now() - began) < limit_ms)
        {
            __i_dvc_ctrl.wait(1);
            __task_scheduler.handle_tasks();
        }
    }

    /**
     * Starts a mail and runs the scheduler until it is done: 1 when the server
     * took it, 0 when it did not, -1 when the mail was refused at the start or
     * never finished.
     */
    template <typename Body>
    int sendAndWait(Body body, uint32_t limit_ms = 2000)
    {
        int result = -1;
        if (!__email_service.sendMail(body, [&result](bool sent) { result = sent ? 1 : 0; }))
        {
            return -1;
        }
        pumpUntilDone(result, limit_ms);
        return result;
    }

} // namespace

TEST(email, a_configured_service_sends_the_message_it_is_given)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    pdiutil::string body = "pin 4 went high";
    ASSERT_EQ(sendAndWait(body), 1);

    ASSERT_EQ(server.m_messages, 1);
    ASSERT_TRUE(has(server.m_body, "pin 4 went high"));
}

TEST(email, every_step_uses_the_saved_configuration)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    ASSERT_EQ(sendAndWait("hello"), 1);

    const std::string &said = server.m_conversation;
    ASSERT_TRUE(has(said, "EHLO device.test\r\n"));
    ASSERT_TRUE(has(said, "dXNlckBkZXZpY2UudGVzdA==\r\n"));
    ASSERT_TRUE(has(said, "MAIL FROM:<user@device.test>\r\n"));
    ASSERT_TRUE(has(said, "RCPT TO:<ops@device.test>\r\n"));
    ASSERT_TRUE(has(server.m_body, "From: Device"));
    ASSERT_TRUE(has(server.m_body, "Subject: Alarm"));
}

TEST(email, each_body_form_sends_the_same_message)
{
    ScriptedSmtp first, second, third;
    {
        MailSetup setup(first);
        pdiutil::string body = "same text";
        ASSERT_EQ(sendAndWait(body), 1);
    }
    {
        MailSetup setup(second);
        char body[] = "same text";
        ASSERT_EQ(sendAndWait((char *)body), 1);
    }
    {
        MailSetup setup(third);
        ASSERT_EQ(sendAndWait((const char *)"same text"), 1);
    }

    ASSERT_EQ(first.m_conversation, second.m_conversation);
    ASSERT_EQ(second.m_conversation, third.m_conversation);
}

TEST(email, a_configuration_missing_a_required_field_sends_nothing)
{
    const char *fields[] = {"host", "domain", "from", "to"};

    for (const char *field : fields)
    {
        ScriptedSmtp server;
        MailSetup setup(server);

        email_config_table table;
        __database_service.get_email_config_table(&table);
        if (0 == strcmp(field, "host")) table.mail_host[0] = 0;
        if (0 == strcmp(field, "domain")) table.sending_domain[0] = 0;
        if (0 == strcmp(field, "from")) table.mail_from[0] = 0;
        if (0 == strcmp(field, "to")) table.mail_to[0] = 0;
        setup.save(table);

        ASSERT_EQ(sendAndWait("hello"), -1);
        ASSERT_TRUE(server.m_conversation.empty());
    }
}

TEST(email, a_refusal_part_way_still_closes_the_connection)
{
    ScriptedSmtp server;
    server.m_reply["RCPT"] = "550 5.1.1 no such user\r\n";
    MailSetup setup(server);

    ASSERT_EQ(sendAndWait("hello"), 0);

    ASSERT_EQ(server.m_messages, 0);
    ASSERT_EQ(server.m_connected, 0);
}

TEST(email, a_mail_is_taken_at_once_and_goes_out_a_step_per_tick)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    int result = -1;
    ASSERT_TRUE(__email_service.sendMail("hello", [&result](bool sent) { result = sent ? 1 : 0; }));

    ASSERT_EQ(result, -1);
    ASSERT_FALSE(has(server.m_conversation, "MAIL FROM"));
    ASSERT_EQ(server.m_messages, 0);

    pumpUntilDone(result, 2000);
    ASSERT_EQ(result, 1);
    ASSERT_EQ(server.m_messages, 1);
    ASSERT_TRUE(has(server.m_conversation, "QUIT\r\n"));
    ASSERT_EQ(server.m_connected, 0);
}

TEST(email, a_second_mail_while_one_goes_out_is_refused_and_the_first_arrives)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    int first = -1;
    ASSERT_TRUE(__email_service.sendMail("first", [&first](bool sent) { first = sent ? 1 : 0; }));
    int second = -1;
    ASSERT_FALSE(__email_service.sendMail("second", [&second](bool sent) { second = sent ? 1 : 0; }));

    pumpUntilDone(first, 2000);
    ASSERT_EQ(first, 1);
    ASSERT_EQ(second, -1);
    ASSERT_EQ(server.m_messages, 1);
    ASSERT_TRUE(has(server.m_body, "first"));
    ASSERT_FALSE(has(server.m_body, "second"));

    ASSERT_EQ(sendAndWait("third"), 1);
    ASSERT_EQ(server.m_messages, 2);
}

TEST(email, each_refused_step_ends_the_mail_and_closes_the_connection)
{
    const char *steps[][2] = {
        {"greeting", "554 no service\r\n"},
        {"EHLO", "502 not here\r\n"},
        {"AUTH", "504 no such mechanism\r\n"},
        {"USER", "535 bad user\r\n"},
        {"PASS", "535 bad password\r\n"},
        {"MAIL", "553 bad sender\r\n"},
        {"DATA", "554 no data\r\n"},
        {"END", "552 too big\r\n"},
        {"QUIT", "500 what\r\n"},
    };

    for (auto &step : steps)
    {
        ScriptedSmtp server;
        server.m_reply[step[0]] = step[1];
        MailSetup setup(server);

        ASSERT_EQ(sendAndWait("hello"), 0);
        ASSERT_EQ(server.m_connected, 0);
    }
}

TEST(email, a_server_that_stops_answering_ends_the_mail_after_the_step_timeout)
{
    ScriptedSmtp server;
    server.m_reply["MAIL"] = "";
    MailSetup setup(server);

    uint32_t began = __i_dvc_ctrl.millis_now();
    ASSERT_EQ(sendAndWait("hello", SMTP_DEFAULT_TIMEOUT + 2000), 0);
    uint32_t took = __i_dvc_ctrl.millis_now() - began;

    ASSERT_TRUE(took >= SMTP_DEFAULT_TIMEOUT);
    ASSERT_EQ(server.m_messages, 0);
    ASSERT_EQ(server.m_connected, 0);
}

TEST(email, a_reply_of_many_lines_longer_than_the_buffer_is_read_to_its_last_line)
{
    ScriptedSmtp server;
    std::string ehlo;
    for (int i = 0; i < 30; i++)
    {
        ehlo += "250-extension that the server advertises at some length\r\n";
    }
    ehlo += "250 AUTH LOGIN\r\n";
    server.m_reply["EHLO"] = ehlo;
    MailSetup setup(server);

    ASSERT_TRUE(ehlo.size() > SMTP_RESPONSE_BUFFER_SIZE);
    ASSERT_EQ(sendAndWait("hello"), 1);
    ASSERT_EQ(server.m_messages, 1);
}

TEST(email, stopping_the_service_ends_a_mail_going_out)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    int result = -1;
    ASSERT_TRUE(__email_service.sendMail("hello", [&result](bool sent) { result = sent ? 1 : 0; }));
    __email_service.stopService();

    ASSERT_EQ(result, 0);
    ASSERT_EQ(server.m_connected, 0);
    ASSERT_EQ(server.m_messages, 0);

    __email_service.initService(&server);
    ASSERT_EQ(sendAndWait("again"), 1);
}

TEST(email, the_configuration_lists_every_saved_field)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    pditest::StringTerminal terminal;
    __email_service.printConfigToTerminal(&terminal);

    ASSERT_TRUE(terminal.sawText("Email Configs :"));
    ASSERT_TRUE(terminal.sawText("device.test"));
    ASSERT_TRUE(terminal.sawText("mail.test"));
    ASSERT_TRUE(terminal.sawText("587"));
    ASSERT_TRUE(terminal.sawText("ops@device.test"));
    ASSERT_TRUE(terminal.sawText("Alarm"));
    ASSERT_FALSE(terminal.sawText("s3cret"));
    ASSERT_TRUE(terminal.sawText("********"));
}

TEST(email, no_terminal_prints_nothing_and_does_not_fail)
{
    __email_service.printConfigToTerminal(nullptr);
}

#endif
