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
  m_lastReceive(0),
  m_timeOut(0),
  m_responseReaderStatus(SMTP_RESPONSE_MAX),
  m_responseBuffer(nullptr),
  m_host(nullptr),
  m_crlfFound(0),
  m_isSecure(false)
{
  this->m_crlfBuf[0] = 0;
  this->m_crlfBuf[1] = 0;
}

/**
 * SMTPClient destructor
 */
SMTPClient::~SMTPClient(){
  this->end();
}

bool SMTPClient::begin( iClientInterface *_client, char *_host, uint16_t _port, bool _isSecure ){

  this->m_client = _client;
  this->m_isSecure = _isSecure;

  if( this->m_isSecure && nullptr != this->m_client && !this->m_client->isSecure() ){
    LogE("SMTP: secure mode requested but client is not TLS-capable\n");
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
  if( nullptr != this->m_responseBuffer ){

    memset( this->m_responseBuffer, 0, SMTP_RESPONSE_BUFFER_SIZE );
  }

  int16_t respcode = SMTP_STATUS_SERVICE_UNAVAILABLE;
  if( nullptr != this->m_client && nullptr != this->m_host ){

    if( connectToServer( this->m_client, this->m_host, this->m_port, 3000 ) ){
      respcode = this->sendCommandAndGetCode( (char*)"" );
    }
  }
  return ( SMTP_STATUS_SERVER_READY == respcode );
}

void SMTPClient::readResponse(){

  if( SMTP_RESPONSE_STARTING == this->m_responseReaderStatus ){
		this->m_responseReaderStatus = SMTP_RESPONSE_WAITING;
	}

	if( __i_dvc_ctrl.millis_now() - this->m_lastReceive >= this->m_timeOut ){
		this->m_responseReaderStatus = SMTP_RESPONSE_TIMEOUT;
		return;
	}

  if( nullptr != this->m_client && nullptr != this->m_responseBuffer ){

    int32_t availableBytes = this->m_client->available();
  	while(availableBytes) {

  		char nextChar = (char)this->m_client->read();
  		--availableBytes;
		this->m_lastReceive = __i_dvc_ctrl.millis_now();
      if(this->m_responseBufferIndex < SMTP_RESPONSE_BUFFER_SIZE) {
  			this->m_responseBuffer[this->m_responseBufferIndex++] = nextChar;
  		}else{
        this->m_responseReaderStatus = SMTP_RESPONSE_BUFFER_FULL;
        return;
      }

  		this->m_crlfBuf[0] = this->m_crlfBuf[1];
  		this->m_crlfBuf[1] = nextChar;

  		if( '\r' == this->m_crlfBuf[0] && '\n' == this->m_crlfBuf[1] ) {

  			if(++this->m_crlfFound == 1) {

          while ( this->m_client->available() ) {
            if(this->m_responseBufferIndex < SMTP_RESPONSE_BUFFER_SIZE) {
        			this->m_responseBuffer[this->m_responseBufferIndex++] = (char)this->m_client->read();
              this->m_lastReceive = __i_dvc_ctrl.millis_now();
            }else{
              (void)this->m_client->read();
            }
            if( (__i_dvc_ctrl.millis_now() - this->m_lastReceive) >= this->m_timeOut ){
          		this->m_responseReaderStatus = SMTP_RESPONSE_TIMEOUT;
          		return;
          	}
            __i_dvc_ctrl.wait(1);
          }
  				this->m_responseReaderStatus = SMTP_RESPONSE_FINISHED;
  				return;
  			}
  		}
      __i_dvc_ctrl.wait(1);
  	}
  }
  __i_dvc_ctrl.wait(1);
}

void SMTPClient::flushClient(){
  if( nullptr != this->m_client ){
    this->m_client->flush();
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
  this->m_crlfFound = 0;
  if( nullptr != this->m_responseBuffer ){
    memset(this->m_responseBuffer, 0, SMTP_RESPONSE_BUFFER_SIZE);
  }
}

void SMTPClient::waitForResponse( uint16_t _timeOut ) {

	this->startReadResponse( _timeOut );
	do {
 		this->readResponse();
	} while( SMTP_RESPONSE_WAITING == this->m_responseReaderStatus );

	LogFmtI("SMTP status: %d\n", this->m_responseReaderStatus);
	LogFmtI("SMTP response: %s\n", nullptr != this->m_responseBuffer ? this->m_responseBuffer : " ");
}

bool SMTPClient::waitForExpectedResponse(	char *expectedResponse, uint16_t _timeOut ){

  bool status = false;

	this->waitForResponse( _timeOut );

	LogFmtI("SMTP espected response: %s\n", expectedResponse);

  if( nullptr != this->m_responseBuffer ){
    status = ( __strstr( (char*)this->m_responseBuffer, expectedResponse ) > 0 );
  }

	return ( SMTP_RESPONSE_TIMEOUT != this->m_responseReaderStatus ) && status ;
}

bool SMTPClient::sendCommandAndExpect( char *command, char *expectedResponse, uint16_t _timeOut ){

  bool status = false;

	LogFmtI("SMTP sending command: %s\n", command);

  if( nullptr != this->m_client ){

    if( !isConnected( this->m_client ) ){

      this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    }else{

      this->flushClient();
      this->m_client->write((const uint8_t*)command);
      this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
      status = this->waitForExpectedResponse( expectedResponse, _timeOut );
    }
  }
	return status;
}

int16_t SMTPClient::sendCommandAndGetCode( const char * command, uint16_t _timeOut ){

  int16_t respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending command: %s\n", command);

  if( nullptr != this->m_client ){

    if( isConnected( this->m_client ) ){

      this->flushClient();
      if( strlen(command) > 0 ){
        this->m_client->write((const uint8_t*)command);
        this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
      }
      this->waitForResponse( _timeOut );

      if( nullptr != this->m_responseBuffer ){
        respcode = (int16_t)StringToUint16( this->m_responseBuffer );
      }
    }else{

      this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    }
  }
  this->m_lastResponseCode = respcode;
	return respcode;
}

int16_t SMTPClient::sendCommandAndGetCode( char *command, uint16_t _timeOut ){

  int16_t respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending command: %s\n", command);

  if( nullptr != this->m_client ){

    if( isConnected( this->m_client ) ){

      this->flushClient();
      if( strlen(command) > 0 ){
        this->m_client->write((const uint8_t*)command);
        this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
      }
      this->waitForResponse( _timeOut );

      if( nullptr != this->m_responseBuffer ){
        respcode = (int16_t)StringToUint16( this->m_responseBuffer );
      }
    }else{

      this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    }
  }
  this->m_lastResponseCode = respcode;
	return respcode;
}

bool SMTPClient::sendHello( char *domain ){

  int16_t respcode = SMTP_STATUS_MAX;
  char *cmd = new char[100];

  if( nullptr != cmd ){

    memset( cmd, 0, 100 );
    strcpy( cmd, SMTP_COMMAND_EHLO );
    strcat( cmd, " " );
    strcat( cmd, domain );
    respcode = this->sendCommandAndGetCode( cmd );

    delete[] cmd;
  }
	return ( SMTP_STATUS_ACTION_COMPLETED == respcode );
}

bool SMTPClient::sendAuthLogin( char *username, char *password ){

  char *_username = pdiutil::safe_new_array<char>(400);
  char *_password = pdiutil::safe_new_array<char>(400);
  int16_t respcode = SMTP_STATUS_MAX;

  if( nullptr != _username && nullptr != _password ){

    memset( _username, 0, 400 );
    memset( _password, 0, 400 );
    base64Encode( username, strlen(username), _username );
    base64Encode( password, strlen(password), _password );

    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_AUTH );

    if( respcode < SMTP_STATUS_SERVICE_UNAVAILABLE ){
      respcode = this->sendCommandAndGetCode( _username );
    }

    if( respcode < SMTP_STATUS_SERVICE_UNAVAILABLE ){
      respcode = this->sendCommandAndGetCode( _password );
    }
  }

  pdiutil::safe_delete_array( _username );
  pdiutil::safe_delete_array( _password );

	return ( respcode < SMTP_STATUS_SERVICE_UNAVAILABLE );
}

bool SMTPClient::sendFrom( char *sender ){

  int16_t respcode = SMTP_STATUS_MAX;
  char *cmd = new char[128];

  if( nullptr != cmd ){

    memset( cmd, 0, 128 );
    strcpy( cmd, SMTP_COMMAND_FROM );
    strcat( cmd, SMTP_COMMAND_SEPARATOR );
    strcat( cmd, SMTP_COMMAND_OPENING_ANG_BRACKET );
    strcat( cmd, sender );
    strcat( cmd, SMTP_COMMAND_CLOSING_ANG_BRACKET );
    respcode = this->sendCommandAndGetCode( cmd );

    delete[] cmd;
  }
	return ( SMTP_STATUS_ACTION_COMPLETED == respcode );
}

bool SMTPClient::sendTo( char *recipient ){

  int16_t respcode = SMTP_STATUS_MAX;
  char *cmd = new char[128];

  if( nullptr != cmd ){

    memset( cmd, 0, 128 );
    strcpy( cmd, SMTP_COMMAND_TO );
    strcat( cmd, SMTP_COMMAND_SEPARATOR );
    strcat( cmd, SMTP_COMMAND_OPENING_ANG_BRACKET );
    strcat( cmd, recipient );
    strcat( cmd, SMTP_COMMAND_CLOSING_ANG_BRACKET );
    respcode = this->sendCommandAndGetCode( cmd );

    delete[] cmd;
  }
	return ( SMTP_STATUS_ACTION_COMPLETED == respcode );
}

bool SMTPClient::sendDataCommand(){

  int respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA );
  return ( SMTP_STATUS_START_MAIL_INPUT == respcode );
}

void SMTPClient::sendDataHeader( char *sender, char *recipient, char *subject ){

  if( nullptr != this->m_client ){
    this->m_client->write((const uint8_t*)SMTP_COMMAND_DATA_HEADER_TO);
    this->m_client->write((const uint8_t*)recipient);
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);

    this->m_client->write((const uint8_t*)SMTP_COMMAND_DATA_HEADER_FROM);
    this->m_client->write((const uint8_t*)sender);
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);

    this->m_client->write((const uint8_t*)SMTP_COMMAND_DATA_HEADER_SUBJECT);
    this->m_client->write((const uint8_t*)subject);
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
  }
}

bool SMTPClient::sendDataBody( pdiutil::string &body ){

  int16_t respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending data: %s\n", body.c_str());

  if( nullptr != this->m_client ){
    sendPacket( this->m_client, (uint8_t*)body.c_str(), body.size() );
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA_TERMINATOR );
  }
  return respcode < SMTP_STATUS_SERVICE_UNAVAILABLE;
}

bool SMTPClient::sendDataBody( char *body ){

  int16_t respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending data: %s\n", body);

  if( nullptr != this->m_client ){
    sendPacket( this->m_client, (uint8_t*)body, strlen(body) );
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA_TERMINATOR );
  }
  return respcode < SMTP_STATUS_SERVICE_UNAVAILABLE;
}

bool SMTPClient::sendDataBody( const char * body ){

  int16_t respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending data: %s\n", body);

  if( nullptr != this->m_client ){
    this->m_client->write((const uint8_t*)body);
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA_TERMINATOR );
  }
  return respcode < SMTP_STATUS_SERVICE_UNAVAILABLE;
}

bool SMTPClient::sendQuit(){

  int respcode = this->sendCommandAndGetCode( SMTP_COMMAND_QUIT );
  return ( SMTP_STATUS_SERVER_ENDING_CON == respcode );
}

void SMTPClient::end(){

  if( nullptr != this->m_client ){
    close( this->m_client );
    this->m_client = nullptr;
  }
  pdiutil::safe_delete_array( this->m_host );
  pdiutil::safe_delete_array( this->m_responseBuffer );
}

#endif
