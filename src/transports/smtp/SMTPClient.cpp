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
  m_host(nullptr)
{

}

/**
 * SMTPClient destructor
 */
SMTPClient::~SMTPClient(){
  this->end();
}

bool SMTPClient::begin( iClientInterface *_client, char *_host, uint16_t _port ){

  this->m_client = _client;

  int _host_len = strlen(_host);
  this->m_host = new char[_host_len + 1];
  if( nullptr != this->m_host ){

    this->m_host[_host_len] = 0;
    memset( this->m_host, 0, _host_len + 1 );
    strcpy( this->m_host, _host );
  }
  this->m_port = _port;

  this->m_responseBuffer = new char[SMTP_RESPONSE_BUFFER_SIZE];
  if( nullptr != this->m_responseBuffer ){

    memset( this->m_responseBuffer, 0, SMTP_RESPONSE_BUFFER_SIZE );
  }

  int respcode = SMTP_STATUS_SERVICE_UNAVAILABLE;
  if( nullptr != this->m_client && nullptr != this->m_host ){

    if( connectToServer( this->m_client, this->m_host, this->m_port, 3000 ) ){
      respcode = this->sendCommandAndGetCode( (char*)"" );
    }
  }
  return ( SMTP_STATUS_SERVER_READY == respcode );
}

void SMTPClient::readResponse(){

	char _crlfBuf[2];
	char _crlfFound, crlfCount=1;
  bool _bufferFull = false;

  if( SMTP_RESPONSE_STARTING == this->m_responseReaderStatus ){
		_crlfFound = 0;
		memset(_crlfBuf, 0, 2);
		this->m_responseReaderStatus = SMTP_RESPONSE_WAITING;
	}

	if( __i_dvc_ctrl.millis_now() - this->m_lastReceive >= this->m_timeOut ){
		this->m_responseReaderStatus = SMTP_RESPONSE_TIMEOUT;
		return;
	}

  if( nullptr != this->m_client && nullptr != this->m_responseBuffer ){

    int availableBytes = this->m_client->available();
  	while(availableBytes) {

  		char nextChar = (char)this->m_client->read();
  		--availableBytes;
      if(this->m_responseBufferIndex < SMTP_RESPONSE_BUFFER_SIZE) {
  			this->m_responseBuffer[this->m_responseBufferIndex++] = nextChar;
  		}else{
        _bufferFull = true;
      }

      if( _bufferFull ) {
        this->m_responseReaderStatus = SMTP_RESPONSE_BUFFER_FULL;
        return;
      }

  		memmove(_crlfBuf, _crlfBuf + 1, 1);
  		_crlfBuf[1] = nextChar;

  		if(!strncmp(_crlfBuf, "\r\n", 2)) {

  			if(++_crlfFound == crlfCount) {

          while ( this->m_client->available() ) {
            if(this->m_responseBufferIndex < SMTP_RESPONSE_BUFFER_SIZE) {
        			this->m_responseBuffer[this->m_responseBufferIndex++] = (char)this->m_client->read();
            }
            if( (__i_dvc_ctrl.millis_now() - this->m_lastReceive) >= this->m_timeOut ){
          		this->m_responseReaderStatus = SMTP_RESPONSE_TIMEOUT;
          		return;
          	}
            __i_dvc_ctrl.wait(0);
          }
  				this->m_responseReaderStatus = SMTP_RESPONSE_FINISHED;
  				return;
  			}
  		}
      __i_dvc_ctrl.wait(0);
  	}
  }
  __i_dvc_ctrl.wait(0);
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

int SMTPClient::sendCommandAndGetCode( const char * command, uint16_t _timeOut ){

  int respcode = SMTP_STATUS_MAX;

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
        respcode = (int)StringToUint16( this->m_responseBuffer );
      }
    }else{

      this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    }
  }
  this->m_lastResponseCode = respcode;
	return respcode;
}

int SMTPClient::sendCommandAndGetCode( char *command, uint16_t _timeOut ){

  int respcode = SMTP_STATUS_MAX;

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
        respcode = (int)StringToUint16( this->m_responseBuffer );
      }
    }else{

      this->m_responseReaderStatus = SMTP_RESPONSE_CONN_ERR;
    }
  }
  this->m_lastResponseCode = respcode;
	return respcode;
}

bool SMTPClient::sendHello( char *domain ){

  int respcode = SMTP_STATUS_MAX;
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

  char *_username = (char *) malloc(400  *sizeof(char));
  char *_password = (char *) malloc(400  *sizeof(char));

  base64Encode( username, strlen(username), _username );
  base64Encode( password, strlen(password), _password );

  int respcode = this->sendCommandAndGetCode( SMTP_COMMAND_AUTH );

  if( nullptr != _username && respcode < SMTP_STATUS_SERVICE_UNAVAILABLE ){
    respcode = this->sendCommandAndGetCode( _username );
    free(_username);
  }

  if( nullptr != _password && respcode < SMTP_STATUS_SERVICE_UNAVAILABLE ){
    respcode = this->sendCommandAndGetCode( _password );
    free(_password);
  }

	return ( respcode < SMTP_STATUS_SERVICE_UNAVAILABLE );
}

bool SMTPClient::sendFrom( char *sender ){

  int respcode = SMTP_STATUS_MAX;
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

  int respcode = SMTP_STATUS_MAX;
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

  int respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending data: %s\n", body.c_str());

  if( nullptr != this->m_client ){
    sendPacket( this->m_client, (uint8_t*)body.c_str(), body.size() );
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA_TERMINATOR );
  }
  return respcode < SMTP_STATUS_SERVICE_UNAVAILABLE;
}

bool SMTPClient::sendDataBody( char *body ){

  int respcode = SMTP_STATUS_MAX;

	LogFmtI("SMTP sending data: %s\n", body);

  if( nullptr != this->m_client ){
    sendPacket( this->m_client, (uint8_t*)body, strlen(body) );
    this->m_client->write((const uint8_t*)SMTP_COMMAND_CRLF);
    respcode = this->sendCommandAndGetCode( SMTP_COMMAND_DATA_TERMINATOR );
  }
  return respcode < SMTP_STATUS_SERVICE_UNAVAILABLE;
}

bool SMTPClient::sendDataBody( const char * body ){

  int respcode = SMTP_STATUS_MAX;

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
    disconnect( this->m_client );
    this->m_client = nullptr;
  }
  if(  nullptr != this->m_host ){
    delete[] this->m_host;
  }
  if(  nullptr != this->m_responseBuffer ){
    delete[] this->m_responseBuffer;
  }
}

#endif
