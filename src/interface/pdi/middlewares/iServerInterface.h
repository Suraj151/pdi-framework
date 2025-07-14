/***************************** Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _I_SERVER_INTERFACE_H_
#define _I_SERVER_INTERFACE_H_

#include <interface/interface_includes.h>

// forward declaration
class iClientInterface;
class HttpServerInterface;

/**
 * iServerInterface class
 */
class iServerInterface
{

public:
  /**
   * iServerInterface constructor.
   */
  iServerInterface() {}
  /**
   * iServerInterface destructor.
   */
  virtual ~iServerInterface() {}

  virtual int32_t begin(uint16_t port) = 0;
  virtual bool hasClient() const = 0;
  virtual iClientInterface* accept() = 0;
  virtual void setTimeout(uint32_t timeout_ms) {}
  virtual void close() = 0;
  virtual void setOnAcceptClientEventCallback(CallBackVoidPointerArgFn callbk, void* arg = nullptr) {}
};

/**
 * iTcpServerInterface class
 */
class iTcpServerInterface : public iServerInterface
{

public:
  /**
   * iTcpServerInterface constructor.
   */
  iTcpServerInterface() {}
  /**
   * iTcpServerInterface destructor.
   */
  virtual ~iTcpServerInterface() {}
};


/**
 * iHttpServerInterface class
 */
class iHttpServerInterface
{

public:
  /**
   * iHttpServerInterface constructor.
   */
  iHttpServerInterface() {}
  /**
   * iHttpServerInterface destructor.
   */
  virtual ~iHttpServerInterface() {}

  virtual void begin(uint16_t port=80) = 0;
  virtual void handleClient() = 0;
  virtual void close() = 0;

  virtual void on(const pdiutil::string &uri, CallBackVoidArgFn handler) = 0;
  virtual void onNotFound(CallBackVoidArgFn fn) = 0;   // called when handler is not assigned

  virtual pdiutil::string arg(const pdiutil::string &name) const = 0;                       // get request argument value by name
  virtual bool hasArg(const pdiutil::string &name) const = 0;                               // check if argument exists

  virtual void collectHeaders(const char *headerKeys[], const size_t headerKeysCount) = 0;  // set the request headers to collect
  virtual pdiutil::string header(const pdiutil::string &name) const = 0;                    // get request header value by name
  virtual bool hasHeader(const pdiutil::string &name) const = 0;                            // check if header exists
  virtual void addHeader(const pdiutil::string &name, const pdiutil::string &value) = 0;

  virtual void setStoragePath(const pdiutil::string &storagepath) {}
  virtual bool handleStaticFileRequest() { return false; }
  
  virtual void send(int code, mimetype_t content_type = MIME_TYPE_MAX, const char *content = nullptr) = 0;
};

/// derived class must define this
extern HttpServerInterface __i_http_server;

#endif
