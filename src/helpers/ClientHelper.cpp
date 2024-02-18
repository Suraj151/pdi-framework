/******************************* Client helper ********************************
This file is part of the Ewings Esp Stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#include "ClientHelper.h"

/*   WiFi client support functions */

bool connectToServer( iClientInterface *client, char* host, uint16_t port, uint16_t timeout ){

  int result = 0;

  if ( nullptr != client && nullptr != host ) {
    LogFmtI("Client: Connecting to %s : %d\n", host, port);
    client->setTimeout(timeout);
    // client->keepAlive();
    result = client->connect( (const uint8_t*)host, port);
  }

  LogFmtI("Client Connect result : %d\n", result);

  return result != 0;
}

bool isConnected( iClientInterface *client ) {

  return ( nullptr != client ) ? client->connected() : false;
}

bool disconnect( iClientInterface *client ) {

  return isConnected(client) ? client->disconnect() : true;
}

bool sendPacket( iClientInterface *client, uint8_t *buffer, uint16_t len ) {

  bool status = false;

  if( nullptr != client && nullptr != buffer ){

    uint16_t sentBytes = 0;
    uint16_t _buf_len = strlen((char*)buffer);
    len =  _buf_len < len ? _buf_len : len;
    status = true;

    while (len > 0) {

      if ( isConnected(client) ) {
        // send 250 bytes at most at a time, can adjust this later based on Client

        uint16_t sendlen = len > 250 ? 250 : len;
        uint8_t  *_buff_pointer = buffer+sentBytes;
        uint16_t _sent = client->write( _buff_pointer, sendlen);
        sentBytes += _sent;
        len -= _sent;

        LogI("Client: sending packets : ");
        for (int i = 0; i < sendlen; i++) {
          LogFmtI("%c",(char)_buff_pointer[i]);
        }
        LogFmtI("\nClient: sent %d/%d\n", sentBytes, _buf_len);

        if (len == 0) {
  	      status = true;
          break;
        }
        if (_sent != sendlen) {
          LogE("Client: send packet - failed to send\n");
  	      status = false;
          break;
        }
      } else {
        LogE("Client: send packet - connection failed\n");
        status = false;
        break;
      }
      __i_dvc_ctrl.wait(0);
    }
  }

  return status;
}

uint16_t readPacket( iClientInterface *client, uint8_t *buffer, uint16_t maxlen, int16_t timeout ) {

  uint16_t len = 0;
  int16_t t = timeout;

  if( nullptr != client && nullptr != buffer ){

    while ( isConnected(client) && ( timeout >= 0 ) ) {

      while ( client->available() ) {

        char c = client->read();
        timeout = t;  // reset the timeout
        buffer[len] = c;
        len++;

        if( maxlen == 0 ){
          return 0;
        }

        if( len == maxlen ){
          return len;
        }
      }
      timeout -= 1;
      __i_dvc_ctrl.wait(1);
    }
  }

  return len;
}
