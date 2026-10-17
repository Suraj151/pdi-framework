/**************************** HTTP Client Tests *******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Covers how a response is read back: which line the status is taken from, and
that a header carrying a version string of its own cannot stand in for it.

Author          : Suraj I.
created Date    : 10th Sep 2026
******************************************************************************/

#include <MountedStack.h>
#include <ShellHarness.h>
#include <pditest.h>

#include <transports/http/HTTPClient.h>

#ifdef ENABLE_HTTP_CLIENT

namespace {

/**
 * A client that answers from a scripted response instead of a socket, so a
 * reply can be shaped byte for byte and no listener is needed.
 */
class ReplayClient : public iClientInterface
{

public:

    ReplayClient(const char *response, bool closes_when_drained = false)
        : m_at(0), m_connected(1), m_closes_when_drained(closes_when_drained)
    {
        m_response = response;
        m_size = (uint32_t)strlen(response);
    }

    int16_t connect(const uint8_t *host, uint16_t port) override { m_connected = 1; return 1; }
    int16_t disconnect() override { m_connected = 0; return 1; }
    int8_t connected() override { return (m_closes_when_drained && m_at >= m_size) ? 0 : m_connected; }
    void setTimeout(uint32_t timeout) override {}

    int32_t write(uint8_t c) override { return 1; }
    int32_t write(const uint8_t *c_str) override { return (int32_t)strlen((const char *)c_str); }
    int32_t write(const uint8_t *c_str, uint32_t size) override { return (int32_t)size; }

    int32_t available() override { return (int32_t)(m_size - m_at); }

    uint8_t read() override
    {
        if (m_at >= m_size) return 0;
        return (uint8_t)m_response[m_at++];
    }

    int32_t read(uint8_t *buf, uint32_t size) override
    {
        uint32_t taken = 0;
        while (taken < size && m_at < m_size) {
            buf[taken++] = (uint8_t)m_response[m_at++];
        }
        return (int32_t)taken;
    }

private:

    const char *m_response;
    uint32_t m_size;
    uint32_t m_at;
    int8_t m_connected;
    bool m_closes_when_drained;
};

/**
 * Sends one GET through a scripted reply and reports how long the client held
 * on to it and what body it kept.
 */
static uint32_t getThrough(ReplayClient &client, int16_t &status, pdiutil::string &body)
{
    Http_Client http;
    http.Begin();
    http.SetClient(&client);

    uint32_t began = __i_dvc_ctrl.millis_now();
    status = http.Get("http://127.0.0.1:8080/v");
    uint32_t took = __i_dvc_ctrl.millis_now() - began;

    char *resp = nullptr;
    int16_t len = 0;
    http.GetResponse(resp, len);
    body.clear();
    if (nullptr != resp && len > 0) body.assign(resp, (pdiutil::string::size_type)len);

    http.End();
    return took;
}

/**
 * Runs one scripted response through a download and reports what the body
 * collector received, so a status misread shows up as a refused transfer.
 */
static int64_t downloadThrough(const char *response, pdiutil::string &body,
                               const char *from = nullptr)
{
    ReplayClient client(response);
    Http_Client http;

    http.Begin();
    http.SetClient(&client);

    body.clear();

    pdiutil::string url = CHARPTR_WRAP("http://127.0.0.1:8080/f.txt");
    if (nullptr != from)
    {
        url = from;
    }
    int64_t got = http.DownloadStream(url.c_str(), [&body](const uint8_t *buf, uint32_t sz) -> bool {
        if (nullptr != buf) {
            body.append((const char *)buf, (pdiutil::string::size_type)sz);
        }
        return true;
    });

    http.End();

    return got;
}

}

TEST(httpclient, a_plain_response_streams_its_body)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_server_header_naming_a_version_does_not_become_the_status)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\r\n"
        "Server: SimpleHTTP/0.6 Python/3.12.3\r\n"
        "Content-Length: 5\r\n\r\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_header_carrying_a_whole_status_line_does_not_replace_it)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\r\n"
        "X-Note: HTTP/9.9 404 nope\r\n"
        "Content-Length: 5\r\n\r\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_status_that_is_not_ok_refuses_the_transfer)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 404 Not Found\r\nContent-Length: 3\r\n\r\nno!", body);

    ASSERT_LT((int)got, 0);
    ASSERT_TRUE(body.empty());
}

TEST(httpclient, a_status_on_the_first_line_is_read_whatever_the_version)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_reply_whose_lines_end_in_a_bare_feed_is_read)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\nContent-Length: 5\n\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_reply_whose_header_block_is_empty_is_read)
{
    pdiutil::string body;
    int64_t got = downloadThrough("HTTP/1.1 200 OK\n\nhello", body);

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_url_carrying_a_colon_without_a_host_is_refused)
{
    pdiutil::string body;
    const char *reply = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";

    ASSERT_TRUE(downloadThrough(reply, body, "http:") < 0);
    ASSERT_TRUE(downloadThrough(reply, body, "https:") < 0);
    ASSERT_TRUE(downloadThrough(reply, body, "http:/") < 0);
    ASSERT_TRUE(downloadThrough(reply, body, "nohost:x") < 0);
}

TEST(httpclient, a_url_padded_with_leading_space_is_still_read)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello", body,
        "   http://127.0.0.1:8080/f.txt");

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_path_naming_https_does_not_make_a_plain_url_secure)
{
    pdiutil::string body;
    int64_t got = downloadThrough(
        "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello", body,
        "http://127.0.0.1:8080/redirect?to=https");

    ASSERT_EQ((int)got, 5);
    ASSERT_STREQ(body.c_str(), "hello");
}

TEST(httpclient, a_reply_whose_length_has_arrived_is_not_waited_on)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok");
    int16_t status = 0;
    pdiutil::string body;

    uint32_t took = getThrough(client, status, body);

    ASSERT_EQ((int)status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "ok");
    ASSERT_LT(took, (uint32_t)(HTTP_CLIENT_MAX_READ_MS / 3));
}

TEST(httpclient, an_empty_reply_with_a_length_of_zero_is_not_waited_on)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    int16_t status = 0;
    pdiutil::string body;

    uint32_t took = getThrough(client, status, body);

    ASSERT_EQ((int)status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "");
    ASSERT_LT(took, (uint32_t)(HTTP_CLIENT_MAX_READ_MS / 3));
}

TEST(httpclient, a_reply_the_server_closes_after_is_not_waited_on)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nok", true);
    int16_t status = 0;
    pdiutil::string body;

    uint32_t took = getThrough(client, status, body);

    ASSERT_EQ((int)status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "ok");
    ASSERT_LT(took, (uint32_t)(HTTP_CLIENT_MAX_READ_MS / 3));
}

TEST(httpclient, a_reply_longer_than_its_length_keeps_only_the_length)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nokEXTRA");
    int16_t status = 0;
    pdiutil::string body;

    getThrough(client, status, body);

    ASSERT_EQ((int)status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "ok");
}

TEST(httpclient, a_reply_with_no_length_on_an_open_connection_still_gets_its_wait)
{
    ReplayClient client("HTTP/1.1 200 OK\r\n\r\nok");
    int16_t status = 0;
    pdiutil::string body;

    uint32_t took = getThrough(client, status, body);

    ASSERT_EQ((int)status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "ok");
    ASSERT_GE(took, (uint32_t)(HTTP_CLIENT_MAX_READ_MS - 100));
}

namespace {

/**
 * Runs the scheduler until the flag is raised or the time is up, the way the
 * loop would carry an async response to its callback.
 */
static bool tickUntil(const bool &raised, uint32_t milliseconds)
{
    pditest::readyScheduler();
    uint32_t began = __i_dvc_ctrl.millis_now();
    while (!raised && (__i_dvc_ctrl.millis_now() - began) < milliseconds)
    {
        __i_dvc_ctrl.wait(1);
        __task_scheduler.handle_tasks();
    }
    return raised;
}

/**
 * Hands a response still waiting over to its callback before the client
 * leaves scope, so a test that stops early leaves no watcher behind.
 */
struct Drained
{
    Http_Client &m_http;

    ~Drained()
    {
        uint32_t began = __i_dvc_ctrl.millis_now();
        while (HTTP_ASYNC_IDLE != m_http.GetAsyncState() && (__i_dvc_ctrl.millis_now() - began) < 200)
        {
            __i_dvc_ctrl.wait(1);
            __task_scheduler.handle_tasks();
        }
    }
};

}

TEST(httpclient, a_response_waiting_for_its_callback_refuses_every_other_request)
{
    pditest::readyScheduler();
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok");
    Http_Client http;

    ASSERT_TRUE(http.Begin());
    http.SetClient(&client);

    bool delivered = false;
    Drained drained{http};
    http.GetAsync("http://127.0.0.1:8080/v", [&delivered](void *) { delivered = true; });

    ASSERT_FALSE(delivered);
    ASSERT_TRUE(HTTP_ASYNC_IDLE != http.GetAsyncState());
    ASSERT_FALSE(http.Begin());
    ASSERT_EQ((int)http.DownloadStream("http://127.0.0.1:8080/f", [](const uint8_t *, uint32_t) -> bool { return true; }),
              (int)PDI_ERR_BUSY);

    ASSERT_TRUE(tickUntil(delivered, 200));
    http.End();
}

TEST(httpclient, a_callback_reads_its_response_whole_and_finds_the_client_free)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok");
    Http_Client http;

    ASSERT_TRUE(http.Begin());
    http.SetClient(&client);

    bool delivered = false;
    int status = 0;
    pdiutil::string body;
    bool idle = false;
    Drained drained{http};
    http.GetAsync("http://127.0.0.1:8080/v", [&](void *arg) {
        Http_Client *c = reinterpret_cast<Http_Client *>(arg);
        char *resp = nullptr;
        int16_t len = 0;
        status = c->GetResponse(resp, len);
        if (nullptr != resp) body.assign(resp, (pdiutil::string::size_type)len);
        idle = (HTTP_ASYNC_IDLE == c->GetAsyncState());
        delivered = true;
    });

    ASSERT_TRUE(tickUntil(delivered, 200));
    ASSERT_EQ(status, (int)HTTP_RESP_OK);
    ASSERT_STREQ(body.c_str(), "ok");
    ASSERT_TRUE(idle);
    ASSERT_TRUE(http.Begin());
    http.End();
}

TEST(httpclient, another_user_beginning_cannot_wipe_a_waiting_response)
{
    ReplayClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok");
    Http_Client http;

    ASSERT_TRUE(http.Begin());
    http.SetClient(&client);

    bool delivered = false;
    pdiutil::string body;
    Drained drained{http};
    http.GetAsync("http://127.0.0.1:8080/v", [&](void *arg) {
        char *resp = nullptr;
        int16_t len = 0;
        reinterpret_cast<Http_Client *>(arg)->GetResponse(resp, len);
        if (nullptr != resp) body.assign(resp, (pdiutil::string::size_type)len);
        delivered = true;
    });

    http.Begin();
    http.SetUserAgent("someone-else");

    ASSERT_TRUE(tickUntil(delivered, 200));
    ASSERT_STREQ(body.c_str(), "ok");
    http.End();
}

#endif
