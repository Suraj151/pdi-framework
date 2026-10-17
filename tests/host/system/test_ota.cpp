/********************************* OTA Tests **********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

The ota service checks for a newer firmware on a fixed period. With no host to
ask, every check finds the same thing, so the syslog carries that once and again
only after a host was set and taken away.

Author          : Suraj I.
created Date    : 12th Oct 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#if defined(ENABLE_OTA_SERVICE) && defined(ENABLE_SYSLOG_SERVICE)

#include <service_provider/device/OtaServiceProvider.h>

namespace
{

    uint16_t s_unconfigured_lines = 0;

    /** Counts the syslog lines that report the missing ota host. */
    void countUnconfigured(logger_type_t, const char *line, uint16_t len)
    {
        pdiutil::string text(line, len);
        if (pdiutil::string::npos != text.find("Not Configured")) s_unconfigured_lines++;
    }

    /** Sets the ota host the next check reads. */
    void setHost(const char *host)
    {
        ota_config_table table;
        __database_service.get_ota_config_table(&table);
        memset(table.ota_host, 0, sizeof(table.ota_host));
        memcpy(table.ota_host, host, strlen(host));
        __database_service.set_ota_config_table(&table);
    }

    /** Runs the scheduler until the shared http client has handed its response over. */
    bool clientIdle()
    {
        uint32_t began = __i_dvc_ctrl.millis_now();
        while (HTTP_ASYNC_IDLE != Http_Client::GetStaticInstance()->GetAsyncState() &&
               (__i_dvc_ctrl.millis_now() - began) < 200)
        {
            __i_dvc_ctrl.wait(1);
            __task_scheduler.handle_tasks();
        }
        return HTTP_ASYNC_IDLE == Http_Client::GetStaticInstance()->GetAsyncState();
    }
}

TEST(ota, a_missing_host_is_logged_once_until_a_host_is_set)
{
    pditest::mountedDb();
    pditest::readyScheduler();

    ota_config_table saved;
    __database_service.get_ota_config_table(&saved);

    s_unconfigured_lines = 0;
    __ota_service.m_unconfigured_logged = false;
    __log_manager.setSyslogSink(&countUnconfigured);

    setHost("");
    __ota_service.handleOtaVersionRequest();
    __ota_service.handleOtaVersionRequest();
    __ota_service.handleOtaVersionRequest();
    uint16_t whileMissing = s_unconfigured_lines;

    setHost("http://127.0.0.1:1");
    __ota_service.handleOtaVersionRequest();
    bool idle = clientIdle();
    uint16_t whileSet = s_unconfigured_lines;

    setHost("");
    __ota_service.handleOtaVersionRequest();
    __ota_service.handleOtaVersionRequest();
    uint16_t afterCleared = s_unconfigured_lines;

    __log_manager.setSyslogSink(nullptr);
    __database_service.set_ota_config_table(&saved);

    ASSERT_EQ(whileMissing, (uint16_t)1);
    ASSERT_TRUE(idle);
    ASSERT_EQ(whileSet, (uint16_t)1);
    ASSERT_EQ(afterCleared, (uint16_t)2);
}

#endif
