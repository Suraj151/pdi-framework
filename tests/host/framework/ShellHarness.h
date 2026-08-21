/***************************** Shell Harness **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Runs real commands against a string terminal, in process. The command line
service is the same one the serial, telnet and ssh terminals drive, so what is
under test is the command, not a re-implementation of the shell around it.

Author          : Suraj I.
created Date    : 16th Aug 2026
******************************************************************************/

#ifndef _PDITEST_SHELL_HARNESS_H_
#define _PDITEST_SHELL_HARNESS_H_

#include <MountedStack.h>
#include <StringTerminal.h>
#include <service_provider/auth/AuthServiceProvider.h>
#include <service_provider/cmd/CommandLineServiceProvider.h>
#include <service_provider/session/SessionManager.h>
#include <service_provider/user/UserStoreService.h>
#include <string>

namespace pditest
{

    static const char *SHELL_USER = "pdiStack";
    static const char *SHELL_PASSWORD = "pdiStack@123";

    /**
     * @brief Put the logged-in account in /etc/passwd, the way boot does.
     *
     * Several commands — id, groups, useradd, userdel — decide who the session
     * is by looking its username up in the user store rather than by reading
     * the session's own uid. On a device UserStoreService::bootstrapFromLoginTable
     * writes that record from the login table at startup; without the database
     * running, nothing does, and root would look like an unknown user.
     */
    inline void seedRootAccount()
    {
        static bool done = false;
        if (done)
        {
            return;
        }
        done = true;

        mountedVfs();
        if (!__i_fs.isDirectory(USER_STORE_ETC_DIR))
        {
            __i_fs.createDirectory(USER_STORE_ETC_DIR);
        }

        user_record_t existing;
        if (__user_store_service.findUserByName(SHELL_USER, existing))
        {
            return;
        }

        user_record_t root;
        root.m_username = SHELL_USER;
        root.m_uid = USER_STORE_ROOT_UID;
        root.m_gid = USER_STORE_ROOT_GID;
        root.m_home = FILE_SEPARATOR;
        root.m_shell = USER_STORE_DEFAULT_SHELL;
        __user_store_service.addUser(root, SHELL_PASSWORD);
    }

    /**
     * @brief Give the scheduler its clock and its task limit.
     *
     * PdiStack::initialize does this at boot. Without it the limit is zero and
     * nothing can be scheduled, so ps has nothing to list and the signal
     * commands have nothing to signal.
     */
    inline void readyScheduler()
    {
        static bool done = false;
        if (done)
        {
            return;
        }
        done = true;

        __task_scheduler.setUtilityInterface(&__i_dvc_ctrl);
        __task_scheduler.setMaxTasksLimit(MAX_SCHEDULABLE_TASKS);
    }

    class Shell
    {

    public:
        /**
         * Attaching creates the session, so authorisation can only be granted
         * afterwards — it is recorded on the session, not on the service. The
         * attach also leaves a login command waiting for a username, which
         * would swallow the first line every test sends, so interaction is
         * restarted once authorised to clear it. Login itself is covered end to
         * end in the system tier.
         */
        Shell(uint16_t uid = 0, uint16_t gid = 0)
        {
            mountedVfs();
            seedRootAccount();
            readyScheduler();

            __cmd_service.useTerminal(&m_terminal);
            m_session = SessionManager::current();

            __auth_service.setVerifiedUsername(SHELL_USER);
            __auth_service.setAuthorized(true);
            CommandLineServiceProvider::startInteraction();

            if (nullptr != m_session)
            {
                m_session->m_uid = uid;
                m_session->m_gid = gid;
            }

            SessionManager::setPWD(FILE_SEPARATOR);
            m_terminal.forget();
        }

        /**
         * Interrupt anything still waiting for input before letting the session
         * go, the way a caller leaving a prompt would. Detaching on its own
         * frees the session slot but leaves the command bound to it, and the
         * next session handed that slot would inherit the prompt.
         */
        ~Shell()
        {
            SessionManager::setCurrent(m_session);
            ServiceProvider::setTerminal(&m_terminal);

            pdiutil::string empty;
            __cmd_service.executeCommand(&empty, CMD_TERM_INSEQ_CTRL_C);

            __cmd_service.useTerminal(nullptr);
        }

        /**
         * @brief Run one command line and hand back everything it printed.
         */
        std::string run(const char *line)
        {
            SessionManager::setCurrent(m_session);
            ServiceProvider::setTerminal(&m_terminal);
            m_terminal.forget();

            pdiutil::string command(line);
            m_result = __cmd_service.executeCommand(&command, CMD_TERM_INSEQ_ENTER);

            return m_terminal.captured();
        }

        /**
         * @brief Type characters at the terminal and let the shell read them.
         *
         * This goes through processTerminalInput rather than executeCommand, so
         * the line editor, the escape sequences and the echo are all exercised.
         * The reader stops at each line ending, so it is pumped until the input
         * is drained.
         */
        std::string type(const char *keys)
        {
            SessionManager::setCurrent(m_session);
            ServiceProvider::setTerminal(&m_terminal);
            m_terminal.forget();
            m_terminal.feed(keys);

            while (m_terminal.available() > 0)
            {
                m_result = __cmd_service.processTerminalInput(&m_terminal);
            }

            return m_terminal.captured();
        }

        /**
         * @brief What the line buffer holds right now.
         */
        std::string lineBuffer() const
        {
            return (nullptr == m_session) ? std::string() : std::string(m_session->m_linebuf.c_str());
        }

        /**
         * @brief Result the last run reported.
         */
        cmd_result_t result() const { return m_result; }

        /**
         * @brief The terminal underneath, for feeding a command that prompts.
         */
        StringTerminal &terminal() { return m_terminal; }

        session_t *session() { return m_session; }

    private:
        StringTerminal m_terminal;
        session_t *m_session = nullptr;
        cmd_result_t m_result = CMD_RESULT_MAX;
    };

    /**
     * @brief Whether output contains the text, for a readable assertion.
     */
    inline bool saw(const std::string &output, const char *needle)
    {
        return output.find(needle) != std::string::npos;
    }

} // namespace pditest

#endif // _PDITEST_SHELL_HARNESS_H_
