/************************* Http Server Interface ******************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2025
******************************************************************************/

#ifndef _ESP8266_HTTP_SERVER_INTERFACE_H_
#define _ESP8266_HTTP_SERVER_INTERFACE_H_

#include "esp8266.h"
#include <interface/pdi/impl/middlewares/HttpServerInterfaceImpl.h>

/**
 * HttpServerInterface class
 */
class HttpServerInterface : public HttpServerInterfaceImpl
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

  // void printreq() override{

  //     Serial.println("\n\nHTTP Request:");
  //     Serial.print("Method: ");
  //     Serial.println(m_clientRequest.method.c_str());
  //     Serial.print("URI: ");
  //     Serial.println(m_clientRequest.uri.c_str());
  //     Serial.print("Version: ");
  //     Serial.println(m_clientRequest.version.c_str());
  //     Serial.print("Body: ");
  //     Serial.println(m_clientRequest.body.c_str());
  //     Serial.println("Headers:");
  //     for (const auto &header : m_clientRequest.headers) {
  //         Serial.print("  ");
  //         Serial.print(header.key ? header.key : "null");
  //         Serial.print(": ");
  //         Serial.println(header.value ? header.value : "null");
  //     }
  //     Serial.println("Queries:");
  //     for (const auto &query : m_clientRequest.queries) {
  //         Serial.print("  ");
  //         Serial.print(query.key ? query.key : "null");
  //         Serial.print(": ");
  //         Serial.println(query.value ? query.value : "null");
  //     }
  //     Serial.println("Form Data:");
  //     for (const auto &form : m_clientRequest.formdata) {
  //         Serial.print("  ");
  //         Serial.print(form.key ? form.key : "null");
  //         Serial.print(": ");
  //         Serial.println(form.value ? form.value : "null");
  //     }
  //     Serial.println("Files:");
  //     for (const auto &file : m_clientRequest.files) {
  //         Serial.print("  ");
  //         Serial.print(file.key ? file.key : "null");
  //         Serial.print(": ");
  //         Serial.println(file.value ? file.value : "null");
  //     }
  // }
};

#endif
