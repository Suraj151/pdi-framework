/*************************** Http Protocol service ****************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "HttpServiceProvider.h"

/**
 * HttpServiceProvider constructor.
 */
HttpServiceProvider::HttpServiceProvider():
  m_http_client(&__i_http_client),
  m_port(80),
  m_retry(HTTP_REQUEST_RETRY)
{
  memset(m_host, 0, HTTP_HOST_ADDR_MAX_SIZE);
}

/**
 * HttpServiceProvider destructor
 */
HttpServiceProvider::~HttpServiceProvider(){
  this->m_http_client = nullptr;
}

/**
 * check http response and give it retry if it not ok
 *
 * @param   int _httpCode
 * @return  bool
 */
bool HttpServiceProvider::followHttpRequest( int _httpCode ){

  LogI("Following Http Request\n");
  LogFmtI("Http Request Status Code : %d\n", _httpCode);
  if ( _httpCode == HTTP_CODE_OK || _httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    LogFmtI("Http Response : %s\n", this->m_http_client->getString().c_str() );
  }
  this->m_http_client->end();

  if( _httpCode < 0 && this->m_retry > 0 ){
    this->m_retry--;
    LogI("Http Request retrying...\n");
    return true;
  }
  this->m_retry = HTTP_REQUEST_RETRY;

  return false;
}

HttpServiceProvider __http_service;
