/********************** HTTP Server Interface Impl ****************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/

#ifndef _HTTP_SERVER_INTERFACE_IMPL_H_
#define _HTTP_SERVER_INTERFACE_IMPL_H_

#include <interface/pdi/middlewares/iServerInterface.h>

/**
 * HttpServerInterfaceImpl class
 */
class HttpServerInterfaceImpl : public iHttpServerInterface
{

public:
  /**
   * HttpServerInterfaceImpl constructor.
   */
  HttpServerInterfaceImpl();
  /**
   * HttpServerInterfaceImpl destructor.
   */
  virtual ~HttpServerInterfaceImpl();

  virtual void begin(uint16_t port=80) override;
  virtual void handleClient() override;
  virtual void close() override;

  virtual void on(const pdiutil::string &uri, CallBackVoidArgFn handler) override;
  virtual void onNotFound(CallBackVoidArgFn fn) override;   // called when handler is not assigned

  virtual pdiutil::string arg(const pdiutil::string &name) const override;                        // get request argument value by name
  virtual bool hasArg(const pdiutil::string &name) const override;                                // check if argument exists

  virtual void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override;   // set the request headers to collect
  virtual pdiutil::string header(const pdiutil::string &name) const override;                     // get request header value by name
  virtual bool hasHeader(const pdiutil::string &name) const override;                             // check if header exists
  virtual void addHeader(const pdiutil::string &name, const pdiutil::string &value) override;

  virtual void setStoragePath(const pdiutil::string &storagepath) override;                  // set storage path for static files
  virtual bool handleStaticFileRequest() override;

  virtual void send(int code, mimetype_t content_type = MIME_TYPE_MAX, const char *content = nullptr) override;
protected:

  iTcpServerInterface* m_server;
  iClientInterface* m_client;
  uint64_t m_currentclient_lastactivity_timestamp;

  struct UriToHandlerMap{
    pdiutil::string uri;
    CallBackVoidArgFn urihandler = nullptr;
    static CallBackVoidArgFn notFoundHandler;

    UriToHandlerMap(const pdiutil::string &uri, CallBackVoidArgFn handler)
      : uri(uri), urihandler(handler) {}
  };
  
  pdiutil::vector<UriToHandlerMap> m_uriHandlerMap;
  pdiutil::string m_responseHeaders;

  pdiutil::string m_storagePath; // Storage path for static files

  struct HttpRequestData{

    pdiutil::string method; // HTTP method
    pdiutil::string version; // HTTP version
    pdiutil::string uri;    // Request URI
    pdiutil::string body; // Request body
    pdiutil::vector<http_header_t> headers;
    pdiutil::vector<http_query_t> queries; // Query parameters
    pdiutil::vector<http_query_t> formdata; // Form data parameters
    pdiutil::vector<http_file_t> files; // Uploaded files
    bool isPending; // Indicates if the request is still being processed

    void clear() {
      method.clear();
      version.clear();
      uri.clear();
      body.clear();
      for (auto &header : headers) {
        header.setvalue((const char*)header.key, nullptr);
      }
      queries.clear();
      formdata.clear();
      files.clear();
      isPending = false;
    }

  } m_clientRequest;

  void parseRequest();
  void prepareResponseHeader(pdiutil::string& _header, int code, const char *content_type, uint32_t content_length);
  void sendResponse(int code, mimetype_t content_type, const char *content);
  void closeClient();
};

#endif
