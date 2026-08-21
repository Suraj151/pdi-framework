/************************ Session Lifecycle Tests *****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

What happens to an unfinished prompt when the client behind it goes away. The
command service is driven the way the telnet and ssh providers drive it, so the
attach and teardown under test are the real ones.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <StringTerminal.h>
#include <pditest.h>

using pditest::saw;

/**
 * Feed keystrokes and let the reader drain them, one line ending at a time.
 */
static void feed(pditest::StringTerminal *term, const char *keys)
{
    term->feed(keys);
    while (term->available() > 0)
    {
        __cmd_service.processTerminalInput(term);
    }
}

/**
 * A client connecting: attach a terminal, authenticate, and land on a prompt.
 * Whatever the attach printed is left in the buffer for the caller to inspect;
 * callers that only care about later output clear it themselves.
 */
static pditest::StringTerminal *connectClient(bool authenticate = true)
{
    pditest::mountedVfs();
    pditest::seedRootAccount();

    pditest::StringTerminal *client = pdiutil::safe_new<pditest::StringTerminal>();
    __cmd_service.useTerminal(client);

    if (authenticate)
    {
        __auth_service.setVerifiedUsername("pdiStack");
        __auth_service.setAuthorized(true);
        CommandLineServiceProvider::startInteraction();
    }

    return client;
}

/**
 * A client going away, exactly as TelnetServiceProvider::closeClient and
 * SSHServer::closeSession do it: detach the session, then free the terminal.
 */
static void dropClient(pditest::StringTerminal *client)
{
    SessionManager::detach(client);
    pdiutil::safe_delete(client);
}

/**
 * Holds a connected client so it is dropped even when an assertion ends the
 * test early. Without this a failed test leaves a session slot occupied and
 * the next test lands on a different one, which changes what it measures.
 */
struct Client
{
    pditest::StringTerminal *m_term;

    explicit Client(bool authenticate = true) : m_term(connectClient(authenticate)) {}
    ~Client() { if (nullptr != m_term) dropClient(m_term); }

    pditest::StringTerminal *operator->() { return m_term; }
    operator pditest::StringTerminal *() { return m_term; }

    /**
     * @brief Drop early, to model a disconnect in the middle of a test.
     */
    void drop()
    {
        if (nullptr != m_term) dropClient(m_term);
        m_term = nullptr;
    }
};

/* --------------------------------------------- the prompt belongs to a session */

TEST(sessionlife, a_prompt_left_open_does_not_follow_the_slot_to_the_next_client)
{
    Client first;
    first->forget();
    feed(first, "su\n");
    ASSERT_TRUE(saw(first->captured(), "user"));
    first->forget();
    feed(first, "someuser\n");
    ASSERT_TRUE(saw(first->captured(), "Pass"));

    session_t *slot = SessionManager::current();
    first.drop();

    Client second;
    ASSERT_EQ(SessionManager::current(), slot);

    // the second client's own command must run, not be swallowed by the first
    // client's unfinished su, and no password prompt may appear
    second->forget();
    feed(second, "pwd\n");

    ASSERT_TRUE(saw(second->captured(), "/"));
    ASSERT_FALSE(saw(second->captured(), "Pass"));
}

TEST(sessionlife, the_next_client_is_asked_to_log_in_rather_than_for_a_password)
{
    Client first;
    feed(first, "su\n");
    feed(first, "someuser\n");
    first.drop();

    // arriving unauthenticated is what a real second connection does
    Client second(false);
    ASSERT_TRUE(saw(second->captured(), "login"));
    ASSERT_FALSE(saw(second->captured(), "Pass"));
}

TEST(sessionlife, a_dropped_client_leaves_nothing_holding_what_it_typed)
{
    // each round abandons a command holding the username that was typed. they
    // must not pile up against the slot, or a later client inherits the oldest
    for (uint8_t round = 0; round < 3; round++)
    {
        Client client;
        feed(client, "su\n");
        feed(client, "abandoned\n");
    }

    Client fresh;
    fresh->forget();
    feed(fresh, "pwd\n");
    ASSERT_TRUE(saw(fresh->captured(), "/"));
}

TEST(sessionlife, a_client_that_finishes_cleanly_leaves_nothing_behind_either)
{
    {
        Client first;
        feed(first, "pwd\n");
    }

    Client second;
    second->forget();
    feed(second, "whoami\n");
    ASSERT_TRUE(saw(second->captured(), "pdiStack"));
}

/* ------------------------------------ the waiting behaviour itself is unchanged */

TEST(sessionlife, a_waiting_command_still_owns_the_next_line_within_its_session)
{
    Client client;
    client->forget();

    feed(client, "su\n");
    ASSERT_TRUE(saw(client->captured(), "user"));

    // this is the prompt answering, not a new command running
    client->forget();
    feed(client, "pwd\n");
    ASSERT_TRUE(saw(client->captured(), "Pass"));
    ASSERT_FALSE(saw(client->captured(), "\n/"));

    client->forget();
    feed(client, "\x03");
    ASSERT_TRUE(saw(client->captured(), "CmdErr"));
}

TEST(sessionlife, an_interrupted_prompt_releases_the_session_for_ordinary_commands)
{
    Client client;
    client->forget();

    feed(client, "su\n");
    feed(client, "\x03");

    client->forget();
    feed(client, "pwd\n");
    ASSERT_TRUE(saw(client->captured(), "/"));
    ASSERT_FALSE(saw(client->captured(), "Pass"));
}
