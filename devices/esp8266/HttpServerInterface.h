/************************* HTTP Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _HTTP_SERVER_INTERFACE_H_
#define _HTTP_SERVER_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/middlewares/iServerInterface.h>

/**
 * HttpServerInterface class
 */
class HttpServerInterface : public iHttpServerInterface
{

public:
  /**
   * HttpServerInterface constructor.
   */
  HttpServerInterface();
  /**
   * HttpServerInterface destructor.
   */
  ~HttpServerInterface();

  void begin(uint16_t port=80) override;
  void handleClient() override;
  void close() override;

  void on(const pdiutil::string &uri, CallBackVoidArgFn handler) override;
  void onNotFound(CallBackVoidArgFn fn) override;   // called when handler is not assigned

  pdiutil::string arg(const pdiutil::string &name) const override;                        // get request argument value by name
  bool hasArg(const pdiutil::string &name) const override;                                // check if argument exists

  void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) override;   // set the request headers to collect
  pdiutil::string header(const pdiutil::string &name) const override;                     // get request header value by name
  bool hasHeader(const pdiutil::string &name) const override;                             // check if header exists
  void addHeader(const pdiutil::string &name, const pdiutil::string &value) override;

  void setStoragePath(const pdiutil::string &storagepath) override;                  // set storage path for static files
  bool handleStaticFileRequest() override;

  void send(int code, const char *content_type = nullptr, const char *content = nullptr) override;
private:

  iTcpServerInterface* m_server;
  iClientInterface* m_client;

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

    // void printreq(){

    //   Serial.println("\n\nHTTP Request:");
    //   Serial.print("Method: ");
    //   Serial.println(method.c_str());
    //   Serial.print("URI: ");
    //   Serial.println(uri.c_str());
    //   Serial.print("Version: ");
    //   Serial.println(version.c_str());
    //   Serial.print("Body: ");
    //   Serial.println(body.c_str());
    //   Serial.println("Headers:");
    //   for (const auto &header : headers) {
    //       Serial.print("  ");
    //       Serial.print(header.key ? header.key : "null");
    //       Serial.print(": ");
    //       Serial.println(header.value ? header.value : "null");
    //   }
    //   Serial.println("Queries:");
    //   for (const auto &query : queries) {
    //       Serial.print("  ");
    //       Serial.print(query.key ? query.key : "null");
    //       Serial.print(": ");
    //       Serial.println(query.value ? query.value : "null");
    //   }
    //   Serial.println("Form Data:");
    //   for (const auto &form : formdata) {
    //       Serial.print("  ");
    //       Serial.print(form.key ? form.key : "null");
    //       Serial.print(": ");
    //       Serial.println(form.value ? form.value : "null");
    //   }
    //   Serial.println("Files:");
    //   for (const auto &file : files) {
    //       Serial.print("  ");
    //       Serial.print(file.key ? file.key : "null");
    //       Serial.print(": ");
    //       Serial.println(file.value ? file.value : "null");
    //   }
    // }
  } m_clientRequest;

  void parseRequest();
  void prepareResponseHeader(pdiutil::string& _header, int code, const char *content_type, uint32_t content_length);
  void sendResponse(int code, const char *content_type, const char *content);
  bool handleIfStaticFileRequest();
  void closeClient();
};

#endif
