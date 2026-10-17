/******************************** Email Tests *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The email service sends one message from its saved configuration. A message
goes out only when the configuration names a server, a domain, a sender and a
recipient, every step of the conversation uses the saved value, and a refusal
anywhere still ends the session rather than leaving the connection open.

Author          : Suraj I.
created Date    : 9th Oct 2026
******************************************************************************/

#include <MountedStack.h>
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

} // namespace

TEST(email, a_configured_service_sends_the_message_it_is_given)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    pdiutil::string body = "pin 4 went high";
    ASSERT_TRUE(__email_service.sendMail(body));

    ASSERT_EQ(server.m_messages, 1);
    ASSERT_TRUE(has(server.m_body, "pin 4 went high"));
}

TEST(email, every_step_uses_the_saved_configuration)
{
    ScriptedSmtp server;
    MailSetup setup(server);

    ASSERT_TRUE(__email_service.sendMail("hello"));

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
        ASSERT_TRUE(__email_service.sendMail(body));
    }
    {
        MailSetup setup(second);
        char body[] = "same text";
        ASSERT_TRUE(__email_service.sendMail(body));
    }
    {
        MailSetup setup(third);
        ASSERT_TRUE(__email_service.sendMail((const char *)"same text"));
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

        ASSERT_FALSE(__email_service.sendMail("hello"));
        ASSERT_TRUE(server.m_conversation.empty());
    }
}

TEST(email, a_refusal_part_way_still_closes_the_connection)
{
    ScriptedSmtp server;
    server.m_reply["RCPT"] = "550 5.1.1 no such user\r\n";
    MailSetup setup(server);

    ASSERT_FALSE(__email_service.sendMail("hello"));

    ASSERT_EQ(server.m_messages, 0);
    ASSERT_EQ(server.m_connected, 0);
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
