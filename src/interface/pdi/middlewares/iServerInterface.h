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


#ifdef ENABLE_TLS_SERVICE

/**
 * iTlsServerInterface class
 *
 * A TLS-secured TCP server. accept() returns iClientInterface* whose dynamic
 * type is a TLS-capable client, so the existing HTTP/SSH/MQTT server code
 * paths require no transport-specific changes once the cert material is set.
 *
 * Configure certificate material BEFORE calling begin(port).
 */
class iTlsServerInterface : public iTcpServerInterface
{

public:
  /**
   * iTlsServerInterface constructor.
   */
  iTlsServerInterface() {}
  /**
   * iTlsServerInterface destructor.
   */
  virtual ~iTlsServerInterface() {}

  /**
   * @brief Set the path to the server certificate presented to connecting clients.
   * @param path Filesystem path to a PEM/DER-encoded server certificate.
   * @return True on success, false otherwise.
   */
  virtual bool setServerCertificatePath(const char* path) = 0;

  /**
   * @brief Set the path to the server private key matching the configured certificate.
   * @param path Filesystem path to a PEM/DER-encoded private key.
   * @return True on success, false otherwise.
   */
  virtual bool setServerPrivateKeyPath(const char* path) = 0;

  /**
   * @brief Set the path to the CA used to verify client certificates (mutual TLS, optional).
   * @param path Filesystem path to a PEM/DER-encoded CA certificate.
   * @return True on success, false otherwise.
   */
  virtual bool setClientCertificateAuthorityPath(const char* path) { return false; }
};

#endif


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

  virtual void begin(uint16_t port=80, bool secure=false) = 0;
  virtual void handleClient() = 0;
  virtual void close() = 0;

  #ifdef ENABLE_TLS_SERVICE
  /**
   * @brief Set the path to the server certificate used for HTTPS. Must be called before begin(port, true).
   * @param path Filesystem path to a PEM/DER-encoded server certificate.
   */
  virtual void setServerCertificatePath(const char* path) {}
  /**
   * @brief Set the path to the server private key used for HTTPS. Must be called before begin(port, true).
   * @param path Filesystem path to a PEM/DER-encoded private key.
   */
  virtual void setServerPrivateKeyPath(const char* path) {}
  /**
   * @brief Set the path to the CA used to verify client certificates (mutual TLS).
   *        When configured, the HTTPS server will send a CertificateRequest and
   *        reject clients that fail validation. Must be called before begin(port, true).
   * @param path Filesystem path to a PEM/DER-encoded CA bundle.
   */
  virtual void setClientCertificateAuthorityPath(const char* path) {}
  #endif

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
  
  virtual void send(int code, mimetype_t content_type = MIME_TYPE_MAX, const char *content = nullptr, bool send_in_chunks = false) = 0;
  virtual void sendChunk(const char *chunk = nullptr) {}
};

/// derived class must define this
extern HttpServerInterface __i_http_server;

#endif
