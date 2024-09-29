/******************************** HTTP Client *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/

#include "HTTPClient.h"

/**
 * http request header structure
 */
// Constructor
httt_header_t::httt_header_t() : key(nullptr),
                                 value(nullptr)
{
}

// Copy Constructor
httt_header_t::httt_header_t(const httt_header_t &obj)
{
    if (nullptr != obj.key && nullptr != obj.value)
    {
        uint16_t _len = strlen(obj.key);
        key = new char[_len + 1];
        if (nullptr != key)
        {
            memset(key, 0, _len + 1);
            strcpy(key, obj.key);
        }

        _len = strlen(obj.value);
        value = new char[_len + 1];
        if (nullptr != value)
        {
            memset(value, 0, _len + 1);
            strcpy(value, obj.value);
        }
    }
}

// Destructor
httt_header_t::~httt_header_t()
{
    clear();
}

// clear request resources and set to defaults
void httt_header_t::clear()
{
    if (nullptr != key)
    {
        delete[] key;
        key = nullptr;
    }
    if (nullptr != value)
    {
        delete[] value;
        value = nullptr;
    }
}

/**
 * http request structure
 */
// Constructor
httt_req_t::httt_req_t() : host(nullptr),
                           port(HTTP_DEFAULT_PORT),
                           reuse(false),
                           timeout(HTTP_HOST_CONNECT_TIMEOUT),
                           uri(nullptr),
                           isHttps(false),
                           http_version(HTTP_VERSION_1_1)
{
    headers.clear();
}

// Destructor
httt_req_t::~httt_req_t()
{
    clear();
}

// clear request resources and set to defaults
void httt_req_t::clear(bool keep_headers)
{
    if (nullptr != host)
    {
        delete[] host;
        host = nullptr;
    }
    if (nullptr != uri)
    {
        delete[] uri;
        uri = nullptr;
    }
    port = HTTP_DEFAULT_PORT;
    reuse = false;
    timeout = HTTP_HOST_CONNECT_TIMEOUT;
    isHttps = false;
    http_version = HTTP_VERSION_1_1;
    if (!keep_headers)
    {
        headers.clear();
    }
}

// init the http request
bool httt_req_t::init(const char *url)
{
    bool bStatus = false;
    char *_url = (char *)url;

    if (nullptr != _url)
    {
        // check for : (http: or https:)
        int16_t index = __strstr(_url, ":");
        bStatus = (-1 != index);

        if (bStatus)
        {
            // check for correct protocol (http or https)
            if (-1 != __strstr(_url, "https"))
            {
                port = 443;
                isHttps = true;
            }
            else if (-1 != __strstr(_url, "http"))
            {
                port = 80;
            }
            else
            {
                bStatus = false;
            }

            int16_t _url_len = strlen(_url);
            int16_t host_index = index + 3;                       // remove "://" part from http://url....
            int16_t uri_index = __strstr(_url + host_index, "/"); // find uri start index i.e. /
            uri_index += (uri_index != -1) ? host_index : 0;

            if (bStatus && _url_len)
            {
                uint16_t _host_len = ((uri_index != -1) ? uri_index : _url_len) - host_index;
                host = new char[_host_len + 1];
                bStatus = (nullptr != host);
                if (bStatus)
                {
                    memset(host, 0, _host_len + 1);
                    strncpy(host, _url + host_index, _host_len);

                    // todo add host validation logic

                    index = __strstr(host, ":");
                    if (-1 != index && index < _host_len)
                    {
                        host[index++] = 0; // remove the ":" and increase index to port
                        port = StringToUint16(host + index, _host_len - index);
                        memset(host + index, 0, _host_len - index); // remove the port info from host string
                        if (port == 443)
                        {
                            isHttps = true;
                        }
                        // todo add port validation logic
                    }
                }
            }

            if (bStatus && host_index < uri_index)
            {
                int _uri_len = strlen(_url) - uri_index;
                uri = new char[_uri_len + 1];
                bStatus = (nullptr != uri);
                if (bStatus)
                {
                    memset(uri, 0, _uri_len + 1);
                    strncpy(uri, _url + uri_index, _uri_len);
                }
            }
        }
    }

    return bStatus;
}

// set http version
void httt_req_t::setHttpVersion(http_version_t ver)
{
    http_version = ver;
}

/**
 * http response structure
 */
// Constructor
httt_resp_t::httt_resp_t() : response(nullptr),
                             status_code(HTTP_RESP_MAX),
                             resp_length(0),
                             max_resp_length(HTTP_CLIENT_BUF_SIZE),
                             follow_redirects(false),
                             redirect_limit(10)
{
    headers.clear();
}

// Destructor
httt_resp_t::~httt_resp_t()
{
    clear();
}

// clear request resources and set to defaults
void httt_resp_t::clear()
{
    if (nullptr != response)
    {
        delete[] response;
        response = nullptr;
    }
    status_code = HTTP_RESP_MAX;
    resp_length = 0;
    max_resp_length = HTTP_CLIENT_BUF_SIZE;
    follow_redirects = false;
    redirect_limit = 10;
    headers.clear();
}

/**
 * Constructor
 */
Http_Client::Http_Client() : m_client(nullptr)
{
}

/**
 * Destructor
 */
Http_Client::~Http_Client()
{
    End();
    if (nullptr != m_client)
    {
        m_client = nullptr;
    }
}

/**
 * Begin Http client
 */
void Http_Client::Begin()
{
    End(true);
}

/**
 * End everything
 */
void Http_Client::End(bool preserve_client)
{
    if (Connected())
    {
        while (m_client->available() > 0)
        {
            m_client->read();
        }

        m_client->disconnect();
        if (!preserve_client)
        {
            m_client = nullptr;
        }
    }
    else
    {
        if (!preserve_client && m_client) // Also destroy m_client if not connected()
        {
            m_client = nullptr;
        }
    }

    ClearAll();
}

/**
 * return the connection status
 */
void Http_Client::ClearAll()
{
    m_request.clear();
    m_response.clear();
}

/**
 * return the connection status
 */
bool Http_Client::Connected()
{
    bool bStatus = false;

    if (nullptr != m_client)
    {
        bStatus = (m_client->connected() || (m_client->available() > 0));
    }

    return bStatus;
}

/**
 * set client interface to use
 */
void Http_Client::SetClient(iClientInterface *client)
{
    m_client = client;
}

/**
 * set whether to keep the connection alive ?
 */
void Http_Client::SetKeepAlive(bool keep_alive)
{
    m_request.reuse = keep_alive;
    AddReqHeader(HTTP_HEADER_KEY_CONNECTION, keep_alive ? RODT_ATTR("keep-alive") : RODT_ATTR("close"));
}

/**
 * set timeout
 */
void Http_Client::SetTimeout(uint32_t timeout)
{
    if (nullptr != m_client)
    {
        m_client->setTimeout(timeout);
    }
}

/**
 * set whether to follow redirects
 */
void Http_Client::SetFollowRedirects(bool follow)
{
    m_response.follow_redirects = follow;
}

/**
 * set count of redirects can be followed
 */
void Http_Client::SetRedirectLimit(uint8_t limit)
{
    m_response.redirect_limit = limit;
}

/**
 * set the url
 */
bool Http_Client::SetUrl(const char *url)
{
    return m_request.init(url);
}

/**
 * set the http version
 */
void Http_Client::SetHttpVersion(http_version_t ver)
{
    m_request.setHttpVersion(ver);
}

/**
 * set the max response buffer size
 */
void Http_Client::SetMaxRespBufferSize(uint32_t size)
{
    m_response.max_resp_length = size;
}

/**
 * set to use default headers
 */
void Http_Client::SetDefaultHeaders(bool set_default)
{
    if( set_default )
    {
        AddReqHeader(HTTP_HEADER_KEY_USER_AGENT, RODT_ATTR("ew_client"));
        AddReqHeader(HTTP_HEADER_KEY_ACCEPT_ENCODING, RODT_ATTR("identity;q=1,chunked;q=0.1,*;q=0"));
    }
    else
    {
        m_request.headers.clear();
    }
}

/**
 * add the header to the request. return false if already exist
 */
bool Http_Client::SetUserAgent(const char *agent)
{
    return AddReqHeader(HTTP_HEADER_KEY_USER_AGENT, agent);
}

bool Http_Client::SetBasicAuthorization(const char *user, const char *pass)
{
    bool bStatus = false;

    if (nullptr != user && nullptr != pass)
    {
        uint16_t _len = strlen(user) + strlen(pass) + 3;
        char *auth = new char[_len];
        if (nullptr != auth)
        {
            memset(auth, 0, _len);
            strcpy(auth, user);
            strcat(auth, ":");
            strcat(auth, pass);

            char *base64_encoded_auth = new char[300];
            if(nullptr != base64_encoded_auth)
            {
                memset(base64_encoded_auth, 0, 300);
                strcpy(base64_encoded_auth, RODT_ATTR("Basic "));
                base64Encode( auth, strlen(auth), base64_encoded_auth+6 );

                bStatus = AddReqHeader(HTTP_HEADER_KEY_AUTHERIZATION, base64_encoded_auth);
                delete []base64_encoded_auth;
            }

            delete []auth;
        }
    }

    return bStatus;
}

/**
 * add the header to the request. return false if already exist
 */
bool Http_Client::AddReqHeader(const char *name, const char *value, bool overwrite_if_exist)
{
    bool bStatus = true;
    uint16_t headerIndex = 0;

    for (; headerIndex < m_request.headers.size(); headerIndex++)
    {
        if (nullptr != m_request.headers[headerIndex].key && __are_str_equals(m_request.headers[headerIndex].key, name))
        {
            // avoid adding similar header if already exist in list
            bStatus = false;
            break;
        }
    }

    // check whether to overwrite exist header
    if( overwrite_if_exist && !bStatus )
    {
        m_request.headers.erase(m_request.headers.begin() + headerIndex);
        bStatus = true;
    }

    if( bStatus )
    {
        AddHeader(name, value);
    }

    return bStatus;
}

/**
 * check whether response header present and return result
 */
bool Http_Client::GetRespHeader(const char *name, char *&value)
{
    return GetHeader( name, value, false );
}

/**
 * Get method
 */
int16_t Http_Client::Get(const char *url)
{
    return SendRequest("GET", url);
}

/**
 * Post method
 */
int16_t Http_Client::Post(const char *url, const char *payload)
{
    char *value;
    if( !GetHeader(HTTP_HEADER_KEY_CONTENT_TYPE, value) )
    {
        AddHeader(HTTP_HEADER_KEY_CONTENT_TYPE, RODT_ATTR("text/plain"));
    }
    AddReqHeader(HTTP_HEADER_KEY_CONTENT_LENGTH, std::to_string(strlen(payload)).c_str());
    
    return SendRequest("POST", url, payload, strlen(payload));
}

/**
 * return the response status and get response body
 */
int16_t Http_Client::GetResponse(char *&resp_body, int16_t &resp_len)
{
    resp_body = m_response.response;
    resp_len = m_response.resp_length;
    
    return m_response.status_code;
}

/**
 * get the static instance of http client
 */
Http_Client *Http_Client::GetStaticInstance()
{
    static Http_Client _client;
    return &_client;
}

/**
 * send the request to server
 */
int16_t Http_Client::SendRequest(const char *type, const char *url, const char *payload, uint16_t size)
{
    bool bStatus = false;
    int16_t respStatus = HTTP_RESP_MAX;
    char *_url = (char *)url;
    uint8_t redirect_count = 0;

    do
    {
        bStatus = (nullptr != m_client) && SetUrl(_url);

        m_request.print();

        if (bStatus)
        {
            if (m_request.reuse && m_client->connected())
            {
            }
            else
            {
                m_client->flush();
                m_client->disconnect();
                __i_dvc_ctrl.wait(100);
                m_client->setTimeout(m_request.timeout);
                bStatus = m_client->connect((const uint8_t *)m_request.host, m_request.port);
            }
        }

        // Send headers
        if (bStatus)
        {
            bStatus = SendHeaders(type);
        }

        // Send payload
        if (bStatus && nullptr != payload && size)
        {
            bStatus = sendPacket(m_client, (uint8_t *)payload, size);
        }

        // handle and parse the response
        if (bStatus)
        {
            respStatus = handleResponse();
            m_response.print();

            if (HTTP_RESP_OK == respStatus)
            {
                bStatus = false;
            }
            else if (HTTP_RESP_TEMPORARY_REDIRECT == respStatus || HTTP_RESP_MOVED_PERMANENTLY == respStatus)
            {
                if (m_response.follow_redirects && redirect_count < m_response.redirect_limit)
                {
                    char *redirect_location = nullptr;
                    bStatus = GetRespHeader(HTTP_HEADER_KEY_LOCATION, redirect_location);
                    uint16_t req_was_https = m_request.isHttps;
                    bool can_keep_alive = m_request.reuse;

                    if (nullptr != redirect_location)
                    {
                        _url = redirect_location; // set the redirection url to start with
                        // m_client->disconnect();
                        m_request.clear(true); // clear request object but keep the last headers
                        redirect_count++;
                        // if port are different in prev and redirect location then cant keep alive same connection
                        can_keep_alive = (req_was_https == (-1 != __strstr(redirect_location, "https")));
                    }
                    else
                    {
                        can_keep_alive = false;
                    }

                    // check whether we can reuse the connection
                    char *connection = nullptr;
                    GetRespHeader(HTTP_HEADER_KEY_CONNECTION, connection);
                    if (nullptr != connection && can_keep_alive)
                    {
                        m_request.reuse = (-1 != __strstr(connection, "aliv"));
                    }
                }
                else
                {
                    bStatus = false;
                }
            }
            else
            {
                bStatus = false;
            }
        }

    } while (bStatus);

    return respStatus;
}

/**
 * send the request headers
 */
bool Http_Client::SendHeaders(const char *type)
{
    bool bStatus = false;

    if (Connected() && nullptr != m_request.host)
    {
        uint8_t space = ' ';
        uint8_t slash = '/';
        uint8_t colon = ':';
        uint8_t host_key[] = "Host";
        uint8_t http_txt[] = "HTTP";
        uint8_t http_v_1_0[] = "1.0";
        uint8_t http_v_1_1[] = "1.1";
        uint8_t http_v_2 = '2';
        uint8_t http_v_3 = '3';
        uint8_t crlf[] = "\r\n";

        // Send the http request method type
        m_client->write((const uint8_t *)type);
        m_client->write(space);

        // Send the uri
        if (nullptr != m_request.uri && strlen(m_request.uri))
        {
            m_client->write((const uint8_t *)m_request.uri);
        }
        else
        {
            m_client->write(slash);
        }

        // Send the http version
        m_client->write(space);
        m_client->write(http_txt);
        m_client->write(slash);
        switch (m_request.http_version)
        {
        case HTTP_VERSION_1_0:
            m_client->write(http_v_1_0);
            break;
        case HTTP_VERSION_2:
            m_client->write(http_v_2);
            break;
        case HTTP_VERSION_3:
            m_client->write(http_v_3);
            break;
        case HTTP_VERSION_1_1:
        default:
            m_client->write(http_v_1_1);
            break;
        }
        m_client->write(crlf);

        // Send the host
        m_client->write(host_key);
        m_client->write(colon);
        m_client->write(space);
        m_client->write((const uint8_t *)m_request.host);
        if (m_request.port != 80 && m_request.port != 443)
        {
            m_client->write(colon);
            m_client->write((const uint8_t *)std::to_string(m_request.port).c_str());
        }
        m_client->write(crlf);

        // Send the rest headers
        for (size_t i = 0; i < m_request.headers.size(); i++)
        {
            if (nullptr != m_request.headers[i].key && nullptr != m_request.headers[i].value)
            {
                m_client->write((const uint8_t *)m_request.headers[i].key);
                m_client->write(colon);
                m_client->write(space);
                m_client->write((const uint8_t *)m_request.headers[i].value);
                m_client->write(crlf);
            }
        }
        m_client->write(crlf);

        bStatus = true;
    }

    return bStatus;
}

/**
 * Add header in request or reaponse bag
 */
void Http_Client::AddHeader(const char *name, const char *value, bool inReqHeader)
{
    if (nullptr != name && nullptr != value)
    {
        httt_header_t header;

        uint16_t _len = strlen(name);
        header.key = new char[_len + 1];
        if (nullptr != header.key)
        {
            memset(header.key, 0, _len + 1);
            strcpy(header.key, name);
        }

        _len = strlen(value);
        header.value = new char[_len + 1];
        if (nullptr != header.value)
        {
            memset(header.value, 0, _len + 1);
            strcpy(header.value, value);
        }

        if (inReqHeader)
        {
            m_request.headers.push_back(header);
        }
        else
        {
            m_response.headers.push_back(header);
        }
    }
}

/**
 * return the result of header key value presence
 */
bool Http_Client::GetHeader(const char *name, char *&value, bool fromReqHeader)
{
    bool bStatus = false;
    uint16_t headers_size = fromReqHeader ? m_request.headers.size() : m_response.headers.size();

    for (uint16_t i = 0; i < headers_size; i++)
    {
        char *_key = fromReqHeader ? m_request.headers[i].key : m_response.headers[i].key;
        char *_value = fromReqHeader ? m_request.headers[i].value : m_response.headers[i].value;

        if (nullptr != _key && __are_str_equals(_key, name))
        {
            bStatus = true;
            value = _value;
            break;
        }
    }

    return bStatus;
}

/**
 * handle the response
 */
int16_t Http_Client::handleResponse()
{
    uint8_t space = ' ';

    int32_t max_timeout = HTTP_CLIENT_MAX_READ_RESPONSE_MS;
    uint32_t start = __i_dvc_ctrl.millis_now();
    uint32_t now = start;
    bool header_ends = false;

    // Have a response buffer if not already
    if (nullptr == m_response.response)
    {
        m_response.response = new char[m_response.max_resp_length + 1];
    }

    // clear the previous response headers and status code
    m_response.headers.clear();
    m_response.status_code = HTTP_RESP_MAX;

    while (m_client && Connected() && nullptr != m_response.response && max_timeout > 0)
    {
        // yield & record now time
        __i_dvc_ctrl.yield();
        now = __i_dvc_ctrl.millis_now();

        // clear the buffer before reading
        memset(m_response.response, 0, m_response.max_resp_length + 1);
        // read until the next line (lf) char
        m_response.resp_length = readPacket(m_client,
                                            (uint8_t *)m_response.response,
                                            m_response.max_resp_length,
                                            max_timeout,
                                            header_ends ? 0 : '\n');

        // Process the response
        if (m_response.resp_length)
        {
            // LogFmtI("ReadResponse (%d) : %s\r\n", m_response.resp_length, m_response.response);
            // trim response
            m_response.response = __strtrim(m_response.response);
            m_response.response = __strtrim_val(m_response.response, '\n');
            m_response.response = __strtrim_val(m_response.response, '\r');
            m_response.resp_length = strlen(m_response.response);

            // break once header end and response collected
            if (header_ends)
            {
                break;
            }

            // check for status code in initial resp
            int index = __strstr(m_response.response, "HTTP/");
            if (-1 != index)
            {
                index += 5; // ignore version for now - HTTP/
                while (m_response.response[index++] != (char)space && index < m_response.resp_length);
                m_response.status_code = StringToUint16(&m_response.response[index]);
            }

            // check for header
            int headerSeperatorIndex = __strstr(m_response.response, ":");
            if (-1 != headerSeperatorIndex)
            {
                m_response.response[headerSeperatorIndex] = 0;
                char *header_name = __strtrim(m_response.response);
                char *header_value = __strtrim(m_response.response + headerSeperatorIndex + 1);
                AddHeader(header_name, header_value, false);
            }

            // check for all headers end
            if (-1 == headerSeperatorIndex && strlen(m_response.response) == 0)
            {
                header_ends = true;
            }
        }

        // Update the remaining time
        max_timeout -= (int32_t)(__i_dvc_ctrl.millis_now() - now);
    }

    return m_response.status_code;
}
