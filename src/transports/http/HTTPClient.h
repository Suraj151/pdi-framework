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

/**
 * http request structure
 */
struct http_req_t
{
	char *host;
	uint16_t port;
	bool reuse;
	uint16_t timeout;
	char *uri;
	bool isHttps;
	http_version_t http_version;
	pdiutil::vector<http_header_t> headers;

	// Constructor
	http_req_t();
	// Destructor
	~http_req_t();

	// clear request resources and set to defaults
	void clear(bool keep_headers = false);
	// init the http request
	bool init(const char *url);
	// set http version
	void setHttpVersion(http_version_t ver);

	void print()
	{
		LogFmtI("http_req_t host : %s\r\n", host);
		LogFmtI("http_req_t port : %d\r\n", port);
		LogFmtI("http_req_t timeout : %d\r\n", timeout);
		LogFmtI("http_req_t uri : %s\r\n", uri);
		LogFmtI("http_req_t isHttps : %d\r\n", isHttps);
		for (size_t i = 0; i < headers.size(); i++)
		{
			if (nullptr != headers[i].key && nullptr != headers[i].value)
			{
				LogFmtI("http_req_t header[%s] : %s\r\n", headers[i].key, headers[i].value);
			}
		}
	}
};

/**
 * http response structure
 */
struct http_resp_t
{
	char *response;
	int16_t status_code;
	uint16_t resp_length;
	uint16_t max_resp_length;
	pdiutil::vector<http_header_t> headers;
	bool follow_redirects;
	uint8_t redirect_limit;

	// Constructor
	http_resp_t();
	// Destructor
	~http_resp_t();

	// clear request resources and set to defaults
	void clear();

	void print()
	{
		LogFmtI("http_resp_t response : %s\r\n", response);
		LogFmtI("http_resp_t status_code : %d\r\n", status_code);
		LogFmtI("http_resp_t resp_length : %d\r\n", resp_length);
		LogFmtI("http_resp_t max_resp_length : %d\r\n", max_resp_length);
		for (size_t i = 0; i < headers.size(); i++)
		{
			if (nullptr != headers[i].key && nullptr != headers[i].value)
			{
				LogFmtI("http_resp_t header[%s] : %s\r\n", headers[i].key, headers[i].value);
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
	http_req_t m_request;
	http_resp_t m_response;
};

#endif
