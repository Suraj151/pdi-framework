/*********** SMTP (Simple Mail Transfer Protocol) Client **********************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include <config/Config.h>

#if defined(ENABLE_EMAIL_SERVICE)

#include "SMTPClient.h"

/**
 * SMTPClient constructor.
 */
SMTPClient::SMTPClient() :
  m_client(nullptr),
  m_port(0),
  m_lastResponseCode(-1),
  m_responseBufferIndex(0),
  m_lineLen(0),
  m_lastReceive(0),
  m_timeOut(0),
  m_responseReaderStatus(SMTP_RESPONSE_MAX),
  m_responseBuffer(nullptr),
  m_host(nullptr),
  m_isSecure(false)
{
  this->m_crlfBuf[0] = 0;
  this->m_crlfBuf[1] = 0;
  memset( this->m_lineHead, 0, sizeof(this->m_lineHead) );
}

/**
 * SMTPClient destructor
 */
SMTPClient::~SMTPClient(){
  this->end();
}

/**
 * Connects to the server and starts reading its greeting. False when the
 * connection could not be made.
 */
bool SMTPClient::begin( iClientInterface *_client, char *_host, uint16_t _port, bool _isSecure ){

  this->m_client = _client;
  this->m_isSecure = _isSecure;
  this->m_lastResponseCode = SMTP_STATUS_SERVICE_UNAVAILABLE;

  if( this->m_isSecure && nullptr != this->m_client && !this->m_client->isSecure() ){
    SysLogE("SMTP: secure mode requested but client is not TLS-capable\n");
    return false;
  }

  size_t _host_len = strlen(_host);
  this->m_host = pdiutil::safe_new_array<char>( _host_len + 1 );
  if( nullptr != this->m_host ){

    memset( this->m_host, 0, _host_len + 1 );
    strcpy( this->m_host, _host );
  }
  this->m_port = _port;

  this->m_responseBuffer = pdiutil::safe_new_array<char>( SMTP_RESPONSE_BUFFER_SIZE );

  if( nullptr == this->m_client || nullptr == this->m_host || nullptr == this->m_responseBuffer ||
      !connectToServer( this->m_client, this->m_host, this->m_port, 3000 ) ){
    return false;
  }

  this->startReadResponse();
  return true;
}

/**
 * Reads what has arrived of the reply. A reply ends with the line whose code
 * is not followed by a dash; only its first SMTP_RESPONSE_BUFFER_SIZE bytes
 * are kept, which is where the code is. A connection the server closed ends
 * the wait at once.
 */
void SMTPClient::readResponse(){

  if( SMTP_RESPONSE_STARTING == this->m_responseReaderStatus ){
    this->m_responseReaderStatus = SMTP_RESPONSE_WAITING;
  }

  if( nullptr == this->m_client || nullptr == this->m_responseBuffer ){
    this->m_responseReaderStatus = SMTP_RESPONSE_ERROR;
    return;
  }

  while( SMTP_RESPONSE_WAITING == this->m_responseReaderStatus && this->m_client->available() > 0 ){

    char nextChar = (char)this->m_client->read();
    this->m_lastReceive = __i_dvc_ctrl.millis_now();

    if( this->m_responseBufferIndex < SMTP_RESPONSE_BUFFER_SIZE - 1 ){
      this->m_responseBuffer[this->m_responseBufferIndex++] = nextChar;
    }
    if( this->m_lineLen < sizeof(this->m_lineHead) ){
      this->m_lineHead[this->m_lineLen] = nextChar;
    }
    this->m_lineLen++;

    this->m_crlfBuf[0] = this->m_crlfBuf[1];
    this->m_crlfBuf[1] = nextChar;

    if( '\r' == this->m_crlfBuf[0] && '\n' == this->m_crlfBuf[1] ){

      bool last = this->m_lineLen <= sizeof(this->m_lineHead) || '-' != this->m_lineHead[3];
      this->m_lineLen = 0;
      this->m_crlfBuf[1] = 0;
      if( last ){
        this->m_responseReaderStatus = SMTP_RESPONSE_FINISHED;
      }
    }
  }

  if( SMTP_RESPONSE_WAITING == this->m_responseReaderStatus && !isConnected( this->m_client ) ){
    this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    this->m_lastResponseCode = 0;
  }else if( SMTP_RESPONSE_WAITING == this->m_responseReaderStatus &&
      __i_dvc_ctrl.millis_now() - this->m_lastReceive >= this->m_timeOut ){
    this->m_responseReaderStatus = SMTP_RESPONSE_TIMEOUT;
  }
}

/**
 * Reads what the server has sent of the reply being waited for, without
 * waiting for more. The reply code is kept once the reply is complete.
 */
smtp_command_status SMTPClient::pollResponse(){

  if( SMTP_RESPONSE_STARTING == this->m_responseReaderStatus ||
      SMTP_RESPONSE_WAITING == this->m_responseReaderStatus ){

    this->readResponse();

    if( SMTP_RESPONSE_FINISHED == this->m_responseReaderStatus ||
        SMTP_RESPONSE_TIMEOUT == this->m_responseReaderStatus ){
      this->m_lastResponseCode = (int16_t)StringToUint16( this->m_responseBuffer );
    }
  }
  return this->m_responseReaderStatus;
}

void SMTPClient::flushClient(){
  if( nullptr != this->m_client ){
    this->m_client->flush(FLUSH_ALL);
    uint32_t _now = __i_dvc_ctrl.millis_now();
  	while(this->m_client->available() && (__i_dvc_ctrl.millis_now()-_now) < SMTP_DEFAULT_TIMEOUT ){
      this->m_client->read();
    }
  }
}

void SMTPClient::startReadResponse( uint16_t _timeOut ) {

  this->m_responseBufferIndex = 0;
	this->m_lastReceive = __i_dvc_ctrl.millis_now();
	this->m_responseReaderStatus = SMTP_RESPONSE_STARTING;
	this->m_timeOut = _timeOut;
  this->m_crlfBuf[0] = 0;
  this->m_crlfBuf[1] = 0;
  this->m_lineLen = 0;
  memset( this->m_lineHead, 0, sizeof(this->m_lineHead) );
  if( nullptr != this->m_responseBuffer ){
    memset(this->m_responseBuffer, 0, SMTP_RESPONSE_BUFFER_SIZE);
  }
}

/**
 * Sends one command line, dropping what is left of an earlier reply, and
 * starts reading its reply.
 */
void SMTPClient::startCommand( const char *command ){

  if( nullptr == this->m_client || !isConnected( this->m_client ) ){
    this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    this->m_lastResponseCode = SMTP_STATUS_MAX;
    return;
  }

  this->flushClient();
  this->m_client->write((const uint8_t*)command);
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
  this->startReadResponse();
}

/**
 * Writes a command made of a verb and an address in angle brackets.
 */
void SMTPClient::startAddressCommand( const char *verb, char *address ){

  pdiutil::string cmd = verb;
  cmd += SMTP_COMMAND_SEPARATOR;
  cmd += SMTP_COMMAND_OPENING_ANG_BRACKET;
  cmd += address;
  cmd += SMTP_COMMAND_CLOSING_ANG_BRACKET;
  this->startCommand( cmd.c_str() );
}

/**
 * Writes a credential encoded in base64 as the next line of a login.
 */
void SMTPClient::startEncoded( char *text ){

  uint32_t len = strlen( text );
  uint32_t size = 4 * ((len + 2) / 3) + 1;
  char *encoded = pdiutil::safe_new_array<char>( size );

  if( nullptr == encoded ){
    this->m_responseReaderStatus = SMTP_RESPONSE_ERROR;
    this->m_lastResponseCode = SMTP_STATUS_MAX;
    return;
  }

  memset( encoded, 0, size );
  base64Encode( text, len, encoded );
  this->startCommand( encoded );
  memset( encoded, 0, size );
  pdiutil::safe_delete_array( encoded );
}

/**
 * Sends EHLO with the domain the board greets as, and starts reading its reply.
 */
void SMTPClient::startHello( char *domain ){

  pdiutil::string cmd = CHARPTR_WRAP(SMTP_COMMAND_EHLO);
  cmd += " ";
  cmd += domain;
  this->startCommand( cmd.c_str() );
}

/**
 * Sends AUTH LOGIN, and starts reading the reply that asks for the username.
 */
void SMTPClient::startAuthLogin(){
  pdiutil::string cmd = CHARPTR_WRAP(SMTP_COMMAND_AUTH);
  this->startCommand( cmd.c_str() );
}

/**
 * Sends the username of an AUTH LOGIN, and starts reading its reply.
 */
void SMTPClient::startAuthUser( char *username ){
  this->startEncoded( username );
}

/**
 * Sends the password of an AUTH LOGIN, and starts reading its reply.
 */
void SMTPClient::startAuthPass( char *password ){
  this->startEncoded( password );
}

/**
 * Sends MAIL FROM with the sender address, and starts reading its reply.
 */
void SMTPClient::startFrom( char *sender ){
  pdiutil::string verb = CHARPTR_WRAP(SMTP_COMMAND_FROM);
  this->startAddressCommand( verb.c_str(), sender );
}

/**
 * Sends RCPT TO with the recipient address, and starts reading its reply.
 */
void SMTPClient::startTo( char *recipient ){
  pdiutil::string verb = CHARPTR_WRAP(SMTP_COMMAND_TO);
  this->startAddressCommand( verb.c_str(), recipient );
}

/**
 * Sends DATA, and starts reading the reply that asks for the message.
 */
void SMTPClient::startDataCommand(){
  pdiutil::string cmd = CHARPTR_WRAP(SMTP_COMMAND_DATA);
  this->startCommand( cmd.c_str() );
}

/**
 * Writes the message headers and body, then the line that ends the message.
 */
void SMTPClient::startDataBody( char *sender, char *recipient, char *subject, const char *body, uint32_t size ){

  if( nullptr == this->m_client || !isConnected( this->m_client ) ){
    this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    this->m_lastResponseCode = SMTP_STATUS_MAX;
    return;
  }

  pdiutil::string header_to = CHARPTR_WRAP(SMTP_COMMAND_DATA_HEADER_TO);
  pdiutil::string header_from = CHARPTR_WRAP(SMTP_COMMAND_DATA_HEADER_FROM);
  pdiutil::string header_subject = CHARPTR_WRAP(SMTP_COMMAND_DATA_HEADER_SUBJECT);
  this->m_client->write((const uint8_t*)header_to.c_str());
  this->m_client->write((const uint8_t*)recipient);
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);

  this->m_client->write((const uint8_t*)header_from.c_str());
  this->m_client->write((const uint8_t*)sender);
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);

  this->m_client->write((const uint8_t*)header_subject.c_str());
  this->m_client->write((const uint8_t*)subject);
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);

  this->writeBody( body, size );
  this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
  this->startCommand( SMTP_COMMAND_DATA_TERMINATOR );
}

/**
 * writes a message body with each line ended in CRLF and a leading dot doubled
 */
void SMTPClient::writeBody( const char *body, uint32_t size ){

  if( nullptr == this->m_client || nullptr == body ) return;

  uint32_t _start = 0;
  for( uint32_t i = 0; i <= size; i++ ){

    if( i < size && '\n' != body[i] ) continue;

    uint32_t _end = ( i > _start && '\r' == body[i - 1] ) ? i - 1 : i;
    if( _end > _start ){
      if( '.' == body[_start] ){
        this->m_client->write((uint8_t)'.');
      }
      sendPacket( this->m_client, (uint8_t*)body + _start, _end - _start );
    }
    if( i < size ){
      this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    }
    _start = i + 1;
  }
}

/**
 * Sends QUIT, and starts reading its reply.
 */
void SMTPClient::startQuit(){
  pdiutil::string cmd = CHARPTR_WRAP(SMTP_COMMAND_QUIT);
  this->startCommand( cmd.c_str() );
}

void SMTPClient::end(){

  if( nullptr != this->m_client ){
    close( this->m_client );
    this->m_client = nullptr;
  }
  pdiutil::safe_delete_array( this->m_host );
  pdiutil::safe_delete_array( this->m_responseBuffer );
  this->m_responseReaderStatus = SMTP_RESPONSE_MAX;
}

#endif
