/******************************** HTTP Client *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Jan 2024
******************************************************************************/
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <interface/pdi.h>
#include <helpers/ClientHelper.h>

/* Http defs */
typedef enum {
    HTTP_RESP_OK = 200,
    HTTP_RESP_MULTIPLE_CHOICES = 300,
    HTTP_RESP_MOVED_PERMANENTLY = 301,
    HTTP_RESP_FOUND = 302,
    HTTP_RESP_SEE_OTHER = 303,
    HTTP_RESP_NOT_MODIFIED = 304,
    HTTP_RESP_USE_PROXY = 305,
    HTTP_RESP_TEMPORARY_REDIRECT = 307,
    HTTP_RESP_PERMANENT_REDIRECT = 308,
    HTTP_RESP_BAD_REQUEST = 400,
    HTTP_RESP_UNAUTHORIZED = 401,
    HTTP_RESP_PAYMENT_REQUIRED = 402,
    HTTP_RESP_FORBIDDEN = 403,
    HTTP_RESP_NOT_FOUND = 404,
    HTTP_RESP_METHOD_NOT_ALLOWED = 405,
    HTTP_RESP_REQUEST_TIMEOUT = 408,
    HTTP_RESP_INTERNAL_SERVER_ERROR = 500,
    HTTP_RESP_NOT_IMPLEMENTED = 501,
    HTTP_RESP_BAD_GATEWAY = 502,
    HTTP_RESP_SERVICE_UNAVAILABLE = 503,
    HTTP_RESP_GATEWAY_TIMEOUT = 504,
    HTTP_RESP_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_RESP_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_RESP_MAX = 999
} http_resp_code_t;

typedef enum {
    HTTP_ERROR_CONNECTION_FAILED = -1,
    HTTP_ERROR_MAX = -999
} http_err_code_t;

typedef enum {
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_2,
    HTTP_VERSION_3,
    HTTP_VERSION_MAX
} http_version_t;


#ifndef HTTP_HOST_CONNECT_TIMEOUT
#define HTTP_HOST_CONNECT_TIMEOUT 2500
#endif

#ifndef HTTP_DEFAULT_PORT
#define HTTP_DEFAULT_PORT 80
#endif

#ifndef HTTP_DEFAULT_VERSION
#define HTTP_DEFAULT_VERSION HTTP_VERSION_1_1
#endif

#define HTTP_CLIENT_BUF_SIZE 640
#define HTTP_CLIENT_READINTERVAL_MS 10
#define HTTP_CLIENT_MAX_READ_RESPONSE_MS 1500

#define HTTP_HEADER_KEY_USER_AGENT "User-Agent"
#define HTTP_HEADER_KEY_ACCEPT_ENCODING "Accept-Encoding"
#define HTTP_HEADER_KEY_CONNECTION "Connection"
#define HTTP_HEADER_KEY_LOCATION "Location"
#define HTTP_HEADER_KEY_AUTHERIZATION "Authorization"
#define HTTP_HEADER_KEY_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_KEY_CONTENT_LENGTH "Content-Length"

/**
 * http request header structure
 */
struct httt_header_t
{
	char *key;
	char *value;
	// Constructor
	httt_header_t();
	// Copy Constructor
	httt_header_t(const httt_header_t &obj);
	// Destructor
	~httt_header_t();

	// clear request resources and set to defaults
	void clear();
};

/**
 * http request structure
 */
struct httt_req_t
{
	char *host;
	uint16_t port;
	bool reuse;
	uint16_t timeout;
	char *uri;
	bool isHttps;
	http_version_t http_version;
	pdiutil::vector<httt_header_t> headers;

	// Constructor
	httt_req_t();
	// Destructor
	~httt_req_t();

	// clear request resources and set to defaults
	void clear(bool keep_headers = false);
	// init the http request
	bool init(const char *url);
	// set http version
	void setHttpVersion(http_version_t ver);

	void print()
	{
		LogFmtI("httt_req_t host : %s\r\n", host);
		LogFmtI("httt_req_t port : %d\r\n", port);
		LogFmtI("httt_req_t timeout : %d\r\n", timeout);
		LogFmtI("httt_req_t uri : %s\r\n", uri);
		LogFmtI("httt_req_t isHttps : %d\r\n", isHttps);
		for (size_t i = 0; i < headers.size(); i++)
		{
			if (nullptr != headers[i].key && nullptr != headers[i].value)
			{
				LogFmtI("httt_req_t header[%s] : %s\r\n", headers[i].key, headers[i].value);
			}
		}
	}
};

/**
 * http response structure
 */
struct httt_resp_t
{
	char *response;
	int16_t status_code;
	uint16_t resp_length;
	uint16_t max_resp_length;
	pdiutil::vector<httt_header_t> headers;
	bool follow_redirects;
	uint8_t redirect_limit;

	// Constructor
	httt_resp_t();
	// Destructor
	~httt_resp_t();

	// clear request resources and set to defaults
	void clear();

	void print()
	{
		LogFmtI("httt_resp_t response : %s\r\n", response);
		LogFmtI("httt_resp_t status_code : %d\r\n", status_code);
		LogFmtI("httt_resp_t resp_length : %d\r\n", resp_length);
		LogFmtI("httt_resp_t max_resp_length : %d\r\n", max_resp_length);
		for (size_t i = 0; i < headers.size(); i++)
		{
			if (nullptr != headers[i].key && nullptr != headers[i].value)
			{
				LogFmtI("httt_resp_t header[%s] : %s\r\n", headers[i].key, headers[i].value);
			}
		}
	}
};

class Http_Client
{

public:
	// Constructor & Destructor
	Http_Client();
	~Http_Client();

	// initialization api
	void Begin();

	// deinit api
	void End(bool preserve_client = false);
	void ClearAll();

	// useful api
	bool Connected();
	void SetClient(iClientInterface *client);
	void SetKeepAlive(bool keep_alive = true); /// keep-alive
	void SetTimeout(uint32_t timeout);
	void SetFollowRedirects(bool follow = true);
	void SetRedirectLimit(uint8_t limit); // max redirects to follow for a single request
	bool SetUrl(const char *url);
	void SetHttpVersion(http_version_t ver);
	void SetMaxRespBufferSize(uint32_t size);

	// header api
	void SetDefaultHeaders(bool set_default = true);
	bool SetUserAgent(const char *agent);
	bool SetBasicAuthorization(const char *user, const char *pass);
	bool AddReqHeader(const char *name, const char *value, bool overwrite_if_exist = true);
	bool GetRespHeader(const char *name, char *&value);

	// request api's
	int16_t Get(const char *url);
	int16_t Post(const char *url, const char *payload);

	// response api
	int16_t GetResponse(char *&resp_body, int16_t &resp_len);

	// static instance
	static Http_Client *GetStaticInstance();

protected:
	int16_t SendRequest(const char *type, const char *url, const char *payload = nullptr, uint16_t size = 0);
	bool SendHeaders(const char *type);
	void AddHeader(const char *name, const char *value, bool inReqHeader = true);
	bool GetHeader(const char *name, char *&value, bool fromReqHeader = true);
	int16_t handleResponse();

	iClientInterface *m_client;
	httt_req_t m_request;
	httt_resp_t m_response;
};

#endif
