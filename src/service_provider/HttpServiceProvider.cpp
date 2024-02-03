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

  #ifdef EW_SERIAL_LOG
  Logln( F("Following Http Request") );
  Log( F("Http Request Status Code : ") ); Logln( _httpCode );
  if ( _httpCode == HTTP_CODE_OK || _httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
    Log( F("Http Response : ") );
    Logln( this->m_http_client->getString() );
  }
  #endif
  this->m_http_client->end();

  if( _httpCode < 0 && this->m_retry > 0 ){
    this->m_retry--;
    #ifdef EW_SERIAL_LOG
    Logln( F("Http Request retrying...") );
    #endif
    return true;
  }
  this->m_retry = HTTP_REQUEST_RETRY;

  return false;
}

HttpServiceProvider __http_service;
